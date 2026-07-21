#include "CaseLoader.h"
#include "NiiVolumeLoader.h"
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

static const std::vector<std::string> MODALITIES = {"flair", "t1", "t1ce", "t2"};

std::string CaseLoader::findModalityFile(const std::string& folderPath, const std::string& suffix) {
    std::string target1 = "_" + suffix + ".nii";
    std::string target2 = "_" + suffix + ".nii.gz";

    for (const auto& entry : fs::directory_iterator(folderPath)) {
        if (!entry.is_regular_file()) continue;
        std::string filename = entry.path().filename().string();

        auto endsWith = [&](const std::string& s, const std::string& suf) {
            return s.size() >= suf.size() &&
                   s.compare(s.size() - suf.size(), suf.size(), suf) == 0;
        };

        if (endsWith(filename, target1) || endsWith(filename, target2)) {
            return entry.path().string();
        }
    }
    return "";
}

bool CaseLoader::validateCaseFolder(const std::string& folderPath, std::string& errorMessage) {
    if (!fs::exists(folderPath) || !fs::is_directory(folderPath)) {
        errorMessage = "文件夹不存在: " + folderPath;
        return false;
    }
    for (const auto& m : MODALITIES) {
        if (findModalityFile(folderPath, m).empty()) {
            errorMessage = "缺少模态文件: " + m;
            return false;
        }
    }
    return true;
}

CaseLoader::LoadedCase CaseLoader::loadCase(const std::string& folderPath) {
    std::string errorMessage;
    if (!validateCaseFolder(folderPath, errorMessage)) {
        throw std::runtime_error(errorMessage);
    }

    LoadedCase result;
    result.caseName = fs::path(folderPath).filename().string();
    if (result.caseName.empty()) {
        result.caseName = fs::path(folderPath).parent_path().filename().string();
    }

    std::array<Volume, 4> rawVolumes;
    for (size_t i = 0; i < MODALITIES.size(); ++i) {
        std::string path = findModalityFile(folderPath, MODALITIES[i]);
        rawVolumes[i] = NiiVolumeLoader::Load(path);
    }

    BBox bbox = BraTSPreprocessor::computeBrainBBox(rawVolumes[0], 0);
    result.cropBBox = bbox;

    for (size_t i = 0; i < MODALITIES.size(); ++i) {
        Volume cropped = BraTSPreprocessor::cropVolume(rawVolumes[i], bbox);
        BraTSPreprocessor::zscoreNormalize(cropped);
        result.modalityVolumes[i] = std::move(cropped);
    }

    return result;
}