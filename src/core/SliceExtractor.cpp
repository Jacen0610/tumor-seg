#include "SliceExtractor.h"

Slice2D SliceExtractor::extract(const Volume& volume, int axis, size_t index) {
    Slice2D slice;

    if (axis == 0) {
        slice.rows = volume.H;
        slice.cols = volume.W;
        slice.data.resize(slice.rows * slice.cols);
        for (size_t h = 0; h < volume.H; ++h)
            for (size_t w = 0; w < volume.W; ++w)
                slice.data[h * slice.cols + w] = volume.at(index, h, w);
    } else if (axis == 1) {
        slice.rows = volume.D;
        slice.cols = volume.W;
        slice.data.resize(slice.rows * slice.cols);
        for (size_t d = 0; d < volume.D; ++d)
            for (size_t w = 0; w < volume.W; ++w)
                slice.data[d * slice.cols + w] = volume.at(d, index, w);
    } else {
        slice.rows = volume.D;
        slice.cols = volume.H;
        slice.data.resize(slice.rows * slice.cols);
        for (size_t d = 0; d < volume.D; ++d)
            for (size_t h = 0; h < volume.H; ++h)
                slice.data[d * slice.cols + h] = volume.at(d, h, index);
    }

    return slice;
}

Slice2D SliceExtractor::rotate90CCW(const Slice2D& src) {
    Slice2D dst;
    dst.rows = src.cols;
    dst.cols = src.rows;
    dst.data.resize(src.rows * src.cols);

    // 逆时针90度: dst(cols-1-c, r) = src(r, c)
    for (size_t r = 0; r < src.rows; ++r) {
        for (size_t c = 0; c < src.cols; ++c) {
            size_t dstRow = src.cols - 1 - c;
            size_t dstCol = r;
            dst.data[dstRow * dst.cols + dstCol] = src.data[r * src.cols + c];
        }
    }
    return dst;
}

LabelSlice2D SliceExtractor::rotate90CCW(const LabelSlice2D& src) {
    LabelSlice2D dst;
    dst.rows = src.cols;
    dst.cols = src.rows;
    dst.data.resize(src.rows * src.cols);

    for (size_t r = 0; r < src.rows; ++r) {
        for (size_t c = 0; c < src.cols; ++c) {
            size_t dstRow = src.cols - 1 - c;
            size_t dstCol = r;
            dst.data[dstRow * dst.cols + dstCol] = src.data[r * src.cols + c];
        }
    }
    return dst;
}

LabelSlice2D SliceExtractor::extract(const LabelVolume& volume, int axis, size_t index) {
    LabelSlice2D slice;

    if (axis == 0) {
        slice.rows = volume.H;
        slice.cols = volume.W;
        slice.data.resize(slice.rows * slice.cols);
        for (size_t h = 0; h < volume.H; ++h)
            for (size_t w = 0; w < volume.W; ++w)
                slice.data[h * slice.cols + w] = volume.at(index, h, w);
    } else if (axis == 1) {
        slice.rows = volume.D;
        slice.cols = volume.W;
        slice.data.resize(slice.rows * slice.cols);
        for (size_t d = 0; d < volume.D; ++d)
            for (size_t w = 0; w < volume.W; ++w)
                slice.data[d * slice.cols + w] = volume.at(d, index, w);
    } else {
        slice.rows = volume.D;
        slice.cols = volume.H;
        slice.data.resize(slice.rows * slice.cols);
        for (size_t d = 0; d < volume.D; ++d)
            for (size_t h = 0; h < volume.H; ++h)
                slice.data[d * slice.cols + h] = volume.at(d, h, index);
    }

    return slice;
}