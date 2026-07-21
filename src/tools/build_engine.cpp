#include <NvInfer.h>
#include <NvOnnxParser.h>
#include <fstream>
#include <iostream>
#include <memory>

class Logger : public nvinfer1::ILogger {
    void log(Severity severity, const char* msg) noexcept override {
        if (severity <= Severity::kWARNING) {
            std::cout << "[TensorRT] " << msg << std::endl;
        }
    }
};

nvinfer1::Dims makeDims5(int d0, int d1, int d2, int d3, int d4) {
    nvinfer1::Dims dims;
    dims.nbDims = 5;
    dims.d[0] = d0;
    dims.d[1] = d1;
    dims.d[2] = d2;
    dims.d[3] = d3;
    dims.d[4] = d4;
    return dims;
};

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "用法: " << argv[0] << " <输入onnx路径> <输出engine路径>" << std::endl;
        return 1;
    }
    std::string onnxPath = argv[1];
    std::string enginePath = argv[2];

    Logger logger;

    // 1. 创建Builder
    auto builder = std::unique_ptr<nvinfer1::IBuilder>(nvinfer1::createInferBuilder(logger));
    if (!builder) {
        std::cerr << "创建Builder失败" << std::endl;
        return 1;
    }

    auto network = std::unique_ptr<nvinfer1::INetworkDefinition>(
    builder->createNetworkV2(0U));

    // 3. 创建ONNX解析器，解析model.onnx
    auto parser = std::unique_ptr<nvonnxparser::IParser>(
        nvonnxparser::createParser(*network, logger));

    if (!parser->parseFromFile(onnxPath.c_str(),
            static_cast<int>(nvinfer1::ILogger::Severity::kWARNING))) {
        std::cerr << "解析ONNX失败:" << std::endl;
        for (int i = 0; i < parser->getNbErrors(); ++i) {
            std::cerr << parser->getError(i)->desc() << std::endl;
        }
        return 1;
    }
    std::cout << "ONNX模型解析成功" << std::endl;

    // 4. 配置Builder参数
    auto config = std::unique_ptr<nvinfer1::IBuilderConfig>(builder->createBuilderConfig());
    config->setMemoryPoolLimit(nvinfer1::MemoryPoolType::kWORKSPACE, 1ULL << 30); // 1GB workspace

    auto profile = builder->createOptimizationProfile();
    profile->setDimensions("input", nvinfer1::OptProfileSelector::kMIN,
                            makeDims5(1, 4, 96, 96, 96));
    profile->setDimensions("input", nvinfer1::OptProfileSelector::kOPT,
                            makeDims5(1, 4, 96, 96, 96));
    profile->setDimensions("input", nvinfer1::OptProfileSelector::kMAX,
                            makeDims5(4, 4, 96, 96, 96));
    config->addOptimizationProfile(profile);

    // 5. 构建engine（这一步比较耗时，会做算子融合、kernel选择等优化）
    std::cout << "开始构建engine，请稍候（可能需要几分钟）..." << std::endl;
    auto serializedEngine = std::unique_ptr<nvinfer1::IHostMemory>(
        builder->buildSerializedNetwork(*network, *config));

    if (!serializedEngine) {
        std::cerr << "构建engine失败" << std::endl;
        return 1;
    }

    // 6. 保存到文件
    std::ofstream outFile(enginePath, std::ios::binary);
    outFile.write(static_cast<const char*>(serializedEngine->data()), serializedEngine->size());
    outFile.close();

    std::cout << "Engine已保存到: " << enginePath << std::endl;
    return 0;
}