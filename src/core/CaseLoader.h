#pragma once
#include <string>
#include <array>
#include "Volume.h"
#include "BraTSPreprocessor.h"

class CaseLoader {
public:
    struct LoadedCase {
        std::string caseName;
        std::array<Volume, 4> modalityVolumes; // 顺序固定: flair, t1, t1ce, t2
        BBox cropBBox; // 相对原始nii坐标系的裁剪范围，后续坐标还原要用
    };

    static bool validateCaseFolder(const std::string& folderPath, std::string& errorMessage);
    static LoadedCase loadCase(const std::string& folderPath);

private:
    static std::string findModalityFile(const std::string& folderPath, const std::string& suffix);
};