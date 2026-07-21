#pragma once
#include "Volume.h"

// 裁剪范围，[d0,d1) [h0,h1) [w0,w1) 均为左闭右开区间，对应python的切片语义
struct BBox {
    size_t d0, d1, h0, h1, w0, w1;
};

class BraTSPreprocessor {
public:
    // 计算脑组织bounding box，对应python的get_brain_bbox()
    static BBox computeBrainBBox(const Volume& volume, int margin = 0);

    // 按bbox裁剪volume，对应python的切片裁剪
    static Volume cropVolume(const Volume& volume, const BBox& bbox);

    // z-score归一化，只在非零(脑组织)区域内计算均值方差，对应python的zscore_normalize()
    static void zscoreNormalize(Volume& volume);

    // 统计信息，用于和Python结果做数值对比验证
    struct Stats {
        double mean = 0.0;
        double stddev = 0.0;
        float minVal = 0.0f;
        float maxVal = 0.0f;
    };
    static Stats computeStats(const Volume& volume, bool maskNonZeroOnly = true);
};