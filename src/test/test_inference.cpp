#include <iostream>
#include <fstream>
#include <chrono>
#include <array>
#include "../core/NiiVolumeLoader.h"
#include "../core/CaseLoader.h"
#include "../core/TensorRTInferenceEngine.h"

int main(int argc, char** argv) {
    if (argc != 3 && argc != 4) {
        std::cerr << "用法: " << argv[0] << " <case文件夹路径> <engine文件路径> [导出的二进制文件路径(可选)]" << std::endl;
        return 1;
    }
    std::string caseFolder = argv[1];
    std::string enginePath = argv[2];
    std::string dumpPath = (argc == 4) ? argv[3] : "";

    NiiVolumeLoader::RegisterFactories();

    std::cout << "加载case: " << caseFolder << std::endl;
    CaseLoader::LoadedCase loadedCase;
    try {
        loadedCase = CaseLoader::loadCase(caseFolder);
    } catch (const std::exception& e) {
        std::cerr << "加载case失败: " << e.what() << std::endl;
        return 1;
    }

    const Volume& ref = loadedCase.modalityVolumes[0];
    std::cout << "预处理后尺寸: (" << ref.D << ", " << ref.H << ", " << ref.W << ")" << std::endl;

    std::cout << "加载TensorRT推理引擎: " << enginePath << std::endl;
    TensorRTInferenceEngine engine(enginePath);

    std::cout << "开始滑窗推理..." << std::endl;
    auto startTime = std::chrono::high_resolution_clock::now();

    LabelVolume result = engine.runSlidingWindowInference(loadedCase.modalityVolumes, 0.5f);

    auto endTime = std::chrono::high_resolution_clock::now();
    double elapsedSec = std::chrono::duration<double>(endTime - startTime).count();

    std::cout << "推理完成，耗时: " << elapsedSec << " 秒" << std::endl;

    // 统计各类别体素数量
    std::array<size_t, 4> classCounts = {0, 0, 0, 0};
    for (uint8_t v : result.data) {
        classCounts[v]++;
    }

    std::cout << "\n分割结果类别统计:" << std::endl;
    std::cout << "  背景        : " << classCounts[0] << std::endl;
    std::cout << "  NCR/NET(坏死): " << classCounts[1] << std::endl;
    std::cout << "  ED(水肿)     : " << classCounts[2] << std::endl;
    std::cout << "  ET(增强肿瘤) : " << classCounts[3] << " (对应原始标签值4)" << std::endl;

    if (!dumpPath.empty()) {
        std::ofstream out(dumpPath, std::ios::binary);
        // 文件格式: 3个size_t(D,H,W) + D*H*W个uint8
        out.write(reinterpret_cast<const char*>(&result.D), sizeof(size_t));
        out.write(reinterpret_cast<const char*>(&result.H), sizeof(size_t));
        out.write(reinterpret_cast<const char*>(&result.W), sizeof(size_t));
        out.write(reinterpret_cast<const char*>(result.data.data()),
                   static_cast<std::streamsize>(result.data.size()));
        out.close();
        std::cout << "\n预测结果已导出到: " << dumpPath << std::endl;
    }

    return 0;
}