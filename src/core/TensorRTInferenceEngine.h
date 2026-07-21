#pragma once
#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <NvInfer.h>
#include "Volume.h"

class TrtLogger : public nvinfer1::ILogger {
    void log(Severity severity, const char* msg) noexcept override;
};

class TensorRTInferenceEngine {
public:
    explicit TensorRTInferenceEngine(const std::string& enginePath,
                                      int patchSize = 96,
                                      int numClasses = 4);
    ~TensorRTInferenceEngine();

    // 禁止拷贝(持有GPU资源，拷贝语义容易出错，需要就自己实现移动语义)
    TensorRTInferenceEngine(const TensorRTInferenceEngine&) = delete;
    TensorRTInferenceEngine& operator=(const TensorRTInferenceEngine&) = delete;

    // 输入: 4个模态的Volume(裁剪+归一化后)，输出: 预测的类别标签体(值域0-3)
    LabelVolume runSlidingWindowInference(const std::array<Volume, 4>& modalityVolumes,
                                          float overlap = 0.5f);

private:
    void runSinglePatch(const std::vector<float>& inputPatch, std::vector<float>& outputPatch);

    static std::vector<int> computeWindowStarts(int dimSize, int patchSize, float overlap);
    std::vector<float> buildGaussianWeightMap() const;

    TrtLogger m_logger;
    nvinfer1::IRuntime* m_runtime = nullptr;
    nvinfer1::ICudaEngine* m_engine = nullptr;
    nvinfer1::IExecutionContext* m_context = nullptr;

    void* m_inputDevice = nullptr;
    void* m_outputDevice = nullptr;
    size_t m_inputBytes = 0;
    size_t m_outputBytes = 0;

    void* m_cudaStream = nullptr; // 实际类型是cudaStream_t，这里用void*避免头文件里引入cuda_runtime.h

    int m_patchSize;
    int m_numClasses;
};