#pragma once
#include "Volume.h"
#include <vector>

struct Slice2D {
    std::vector<float> data;
    size_t rows = 0, cols = 0;
};

struct LabelSlice2D {
    std::vector<uint8_t> data;
    size_t rows = 0, cols = 0;
};

class SliceExtractor {
public:
    // axis: 0对应D轴, 1对应H轴, 2对应W轴
    static Slice2D extract(const Volume& volume, int axis, size_t index);
    static LabelSlice2D extract(const LabelVolume& volume, int axis, size_t index);

    // 对提取出的2D切片做90度逆时针旋转，用于修正显示方向和解剖直觉不一致的问题
    static Slice2D rotate90CCW(const Slice2D& src);
    static LabelSlice2D rotate90CCW(const LabelSlice2D& src);
};