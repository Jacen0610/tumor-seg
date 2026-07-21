#pragma once
#include <vector>
#include <cstddef>
#include <cstdint>

// 简单的三维体数据容器，对应Python里的numpy数组(D,H,W)
struct Volume {
    std::vector<float> data;
    size_t D = 0, H = 0, W = 0;

    float& at(size_t d, size_t h, size_t w) {
        return data[d * H * W + h * W + w];
    }
    float at(size_t d, size_t h, size_t w) const {
        return data[d * H * W + h * W + w];
    }

    size_t voxelCount() const { return D * H * W; }
};

// 分割结果标签体，值域0-3(对应网络输出类别: 0=背景,1=NCR/NET,2=ED,3=ET)
struct LabelVolume {
    std::vector<uint8_t> data;
    size_t D = 0, H = 0, W = 0;

    uint8_t& at(size_t d, size_t h, size_t w) {
        return data[d * H * W + h * W + w];
    }
    uint8_t at(size_t d, size_t h, size_t w) const {
        return data[d * H * W + h * W + w];
    }
};