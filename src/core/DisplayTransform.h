#pragma once
#include "Volume.h"
#include <string>

// 描述"数据存储顺序" -> "解剖学标准显示顺序"的转换规则
// permuteOrder[i]: 输出(显示)的第i个轴，对应原始数据的第几个轴
// flip[i]: 输出的第i个轴是否需要翻转(反向)
struct DisplayTransform {
    unsigned int permuteOrder[3] = {0, 1, 2};
    bool flip[3] = {false, false, false};

    // 是否是恒等变换(不需要做任何重排/翻转)，用于跳过不必要的拷贝
    bool isIdentity() const {
        return permuteOrder[0] == 0 && permuteOrder[1] == 1 && permuteOrder[2] == 2 &&
               !flip[0] && !flip[1] && !flip[2];
    }
};

// 从参考模态的nii文件(通常用flair)计算显示校正规则
// 内部用itk::OrientImageFilter推算，不需要真正重新处理整个图像，只借用它的计算结果
DisplayTransform computeDisplayTransform(const std::string& referenceNiiPath);

// 按DisplayTransform重新排列Volume的索引顺序，生成显示专用副本
Volume remapVolumeForDisplay(const Volume& src, const DisplayTransform& transform);

// 同上，针对LabelVolume(分割结果)
LabelVolume remapLabelVolumeForDisplay(const LabelVolume& src, const DisplayTransform& transform);