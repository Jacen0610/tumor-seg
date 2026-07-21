#include "TensorRTInferenceEngine.h"
#include <cuda_runtime_api.h>
#include <fstream>
#include <stdexcept>
#include <cmath>
#include <algorithm>
#include <iostream>

void TrtLogger::log(Severity severity, const char* msg) noexcept {
    if (severity <= Severity::kWARNING) {
        std::cout << "[TensorRT] " << msg << std::endl;
    }
}

TensorRTInferenceEngine::TensorRTInferenceEngine(const std::string& enginePath,
                                                   int patchSize,
                                                   int numClasses)
    : m_patchSize(patchSize), m_numClasses(numClasses) {

    // 1. 读取engine文件
    std::ifstream file(enginePath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("无法打开engine文件: " + enginePath);
    }
    file.seekg(0, std::ios::end);
    size_t fileSize = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    std::vector<char> engineData(fileSize);
    file.read(engineData.data(), static_cast<std::streamsize>(fileSize));
    file.close();

    // 2. 反序列化engine
    m_runtime = nvinfer1::createInferRuntime(m_logger);
    if (!m_runtime) throw std::runtime_error("创建TensorRT Runtime失败");

    m_engine = m_runtime->deserializeCudaEngine(engineData.data(), fileSize);
    if (!m_engine) throw std::runtime_error("反序列化engine失败");

    m_context = m_engine->createExecutionContext();
    if (!m_context) throw std::runtime_error("创建ExecutionContext失败");

    // 3. 固定batch=1的输入形状 (1, 4, patchSize, patchSize, patchSize)
    nvinfer1::Dims inputDims;
    inputDims.nbDims = 5;
    inputDims.d[0] = 1;
    inputDims.d[1] = 4;
    inputDims.d[2] = m_patchSize;
    inputDims.d[3] = m_patchSize;
    inputDims.d[4] = m_patchSize;

    if (!m_context->setInputShape("input", inputDims)) {
        throw std::runtime_error("设置输入形状失败，请确认导出onnx时的输入名是否为'input'");
    }

    // 4. 分配GPU显存
    m_inputBytes = static_cast<size_t>(1) * 4 * m_patchSize * m_patchSize * m_patchSize * sizeof(float);
    m_outputBytes = static_cast<size_t>(1) * m_numClasses * m_patchSize * m_patchSize * m_patchSize * sizeof(float);

    cudaMalloc(&m_inputDevice, m_inputBytes);
    cudaMalloc(&m_outputDevice, m_outputBytes);

    m_context->setTensorAddress("input", m_inputDevice);
    m_context->setTensorAddress("output", m_outputDevice);

    cudaStream_t stream;
    cudaStreamCreate(&stream);
    m_cudaStream = stream;

    std::cout << "TensorRT推理引擎加载成功" << std::endl;
}

TensorRTInferenceEngine::~TensorRTInferenceEngine() {
    if (m_inputDevice) cudaFree(m_inputDevice);
    if (m_outputDevice) cudaFree(m_outputDevice);
    if (m_cudaStream) cudaStreamDestroy(static_cast<cudaStream_t>(m_cudaStream));

    delete m_context;
    delete m_engine;
    delete m_runtime;
}

void TensorRTInferenceEngine::runSinglePatch(const std::vector<float>& inputPatch,
                                              std::vector<float>& outputPatch) {
    cudaStream_t stream = static_cast<cudaStream_t>(m_cudaStream);

    cudaMemcpyAsync(m_inputDevice, inputPatch.data(), m_inputBytes,
                     cudaMemcpyHostToDevice, stream);

    if (!m_context->enqueueV3(stream)) {
        throw std::runtime_error("TensorRT推理执行失败(enqueueV3)");
    }

    cudaMemcpyAsync(outputPatch.data(), m_outputDevice, m_outputBytes,
                     cudaMemcpyDeviceToHost, stream);

    cudaStreamSynchronize(stream);
}

std::vector<int> TensorRTInferenceEngine::computeWindowStarts(int dimSize, int patchSize, float overlap) {
    std::vector<int> starts;

    if (dimSize <= patchSize) {
        starts.push_back(0);
        return starts;
    }

    int stride = std::max(1, static_cast<int>(patchSize * (1.0f - overlap)));
    int pos = 0;
    while (pos + patchSize < dimSize) {
        starts.push_back(pos);
        pos += stride;
    }
    // 最后一个窗口对齐到末尾，确保完整覆盖到边界，不需要padding
    starts.push_back(dimSize - patchSize);

    return starts;
}

std::vector<float> TensorRTInferenceEngine::buildGaussianWeightMap() const {
    int p = m_patchSize;
    std::vector<float> gauss1d(p);

    float sigma = p * 0.125f; // 对应MONAI默认的sigma_scale=0.125
    float center = (p - 1) / 2.0f;

    for (int i = 0; i < p; ++i) {
        float diff = static_cast<float>(i) - center;
        gauss1d[i] = std::exp(-(diff * diff) / (2.0f * sigma * sigma));
    }

    std::vector<float> weights(static_cast<size_t>(p) * p * p);
    for (int d = 0; d < p; ++d) {
        for (int h = 0; h < p; ++h) {
            for (int w = 0; w < p; ++w) {
                weights[static_cast<size_t>(d) * p * p + h * p + w] =
                    gauss1d[d] * gauss1d[h] * gauss1d[w];
            }
        }
    }

    // 归一化到最大值1.0，避免数值过小
    float maxVal = *std::max_element(weights.begin(), weights.end());
    for (auto& v : weights) v /= maxVal;

    return weights;
}

LabelVolume TensorRTInferenceEngine::runSlidingWindowInference(
        const std::array<Volume, 4>& modalityVolumes, float overlap) {

    const Volume& ref = modalityVolumes[0];
    const size_t D = ref.D, H = ref.H, W = ref.W;
    const int p = m_patchSize;

    std::vector<float> accumLogits(static_cast<size_t>(m_numClasses) * D * H * W, 0.0f);
    std::vector<float> weightSum(D * H * W, 0.0f);

    auto gaussWeights = buildGaussianWeightMap();

    auto dStarts = computeWindowStarts(static_cast<int>(D), p, overlap);
    auto hStarts = computeWindowStarts(static_cast<int>(H), p, overlap);
    auto wStarts = computeWindowStarts(static_cast<int>(W), p, overlap);

    std::vector<float> inputPatch(static_cast<size_t>(4) * p * p * p);
    std::vector<float> outputPatch(static_cast<size_t>(m_numClasses) * p * p * p);

    int totalWindows = static_cast<int>(dStarts.size() * hStarts.size() * wStarts.size());
    int windowCount = 0;

    for (int ds : dStarts) {
        for (int hs : hStarts) {
            for (int ws : wStarts) {
                ++windowCount;
                std::cout << "\r滑窗推理进度: " << windowCount << "/" << totalWindows << std::flush;

                // 提取4模态patch
                for (int c = 0; c < 4; ++c) {
                    const Volume& vol = modalityVolumes[c];
                    for (int pd = 0; pd < p; ++pd) {
                        for (int ph = 0; ph < p; ++ph) {
                            for (int pw = 0; pw < p; ++pw) {
                                size_t srcIdx = static_cast<size_t>(c) * p * p * p
                                              + static_cast<size_t>(pd) * p * p
                                              + static_cast<size_t>(ph) * p + pw;
                                inputPatch[srcIdx] = vol.at(ds + pd, hs + ph, ws + pw);
                            }
                        }
                    }
                }

                runSinglePatch(inputPatch, outputPatch);

                // 加权累加到全局缓冲区
                for (int cls = 0; cls < m_numClasses; ++cls) {
                    for (int pd = 0; pd < p; ++pd) {
                        for (int ph = 0; ph < p; ++ph) {
                            for (int pw = 0; pw < p; ++pw) {
                                float w = gaussWeights[static_cast<size_t>(pd) * p * p
                                                      + static_cast<size_t>(ph) * p + pw];
                                size_t globalIdx = static_cast<size_t>(ds + pd) * H * W
                                                  + static_cast<size_t>(hs + ph) * W
                                                  + (ws + pw);
                                size_t patchIdx = static_cast<size_t>(cls) * p * p * p
                                                 + static_cast<size_t>(pd) * p * p
                                                 + static_cast<size_t>(ph) * p + pw;

                                accumLogits[static_cast<size_t>(cls) * D * H * W + globalIdx] +=
                                    outputPatch[patchIdx] * w;

                                if (cls == 0) weightSum[globalIdx] += w;
                            }
                        }
                    }
                }
            }
        }
    }
    std::cout << std::endl;

    // 归一化 + argmax，得到最终分割标签
    LabelVolume result;
    result.D = D; result.H = H; result.W = W;
    result.data.resize(D * H * W);

    for (size_t idx = 0; idx < D * H * W; ++idx) {
        float bestVal = -1e30f;
        int bestCls = 0;
        for (int cls = 0; cls < m_numClasses; ++cls) {
            float val = accumLogits[static_cast<size_t>(cls) * D * H * W + idx] / weightSum[idx];
            if (val > bestVal) {
                bestVal = val;
                bestCls = cls;
            }
        }
        result.data[idx] = static_cast<uint8_t>(bestCls);
    }

    return result;
}