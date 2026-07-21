#include "BraTSPreprocessor.h"
#include <algorithm>
#include <limits>
#include <cmath>

BBox BraTSPreprocessor::computeBrainBBox(const Volume& volume, int margin) {
    size_t dMin = volume.D, dMax = 0;
    size_t hMin = volume.H, hMax = 0;
    size_t wMin = volume.W, wMax = 0;
    bool found = false;

    for (size_t d = 0; d < volume.D; ++d) {
        for (size_t h = 0; h < volume.H; ++h) {
            for (size_t w = 0; w < volume.W; ++w) {
                if (volume.at(d, h, w) > 0.0f) {
                    found = true;
                    dMin = std::min(dMin, d); dMax = std::max(dMax, d);
                    hMin = std::min(hMin, h); hMax = std::max(hMax, h);
                    wMin = std::min(wMin, w); wMax = std::max(wMax, w);
                }
            }
        }
    }

    if (!found) {
        // 全零，对应python的极端情况：返回整个范围
        return {0, volume.D, 0, volume.H, 0, volume.W};
    }

    // 上界+1，对应python的 nonzero.max() + 1
    dMax += 1; hMax += 1; wMax += 1;

    long long d0 = static_cast<long long>(dMin) - margin;
    long long h0 = static_cast<long long>(hMin) - margin;
    long long w0 = static_cast<long long>(wMin) - margin;
    long long d1 = static_cast<long long>(dMax) + margin;
    long long h1 = static_cast<long long>(hMax) + margin;
    long long w1 = static_cast<long long>(wMax) + margin;

    BBox box;
    box.d0 = static_cast<size_t>(std::max<long long>(d0, 0));
    box.h0 = static_cast<size_t>(std::max<long long>(h0, 0));
    box.w0 = static_cast<size_t>(std::max<long long>(w0, 0));
    box.d1 = static_cast<size_t>(std::min<long long>(d1, static_cast<long long>(volume.D)));
    box.h1 = static_cast<size_t>(std::min<long long>(h1, static_cast<long long>(volume.H)));
    box.w1 = static_cast<size_t>(std::min<long long>(w1, static_cast<long long>(volume.W)));

    return box;
}

Volume BraTSPreprocessor::cropVolume(const Volume& volume, const BBox& bbox) {
    Volume cropped;
    cropped.D = bbox.d1 - bbox.d0;
    cropped.H = bbox.h1 - bbox.h0;
    cropped.W = bbox.w1 - bbox.w0;
    cropped.data.resize(cropped.voxelCount());

    for (size_t d = 0; d < cropped.D; ++d) {
        for (size_t h = 0; h < cropped.H; ++h) {
            for (size_t w = 0; w < cropped.W; ++w) {
                cropped.at(d, h, w) = volume.at(d + bbox.d0, h + bbox.h0, w + bbox.w0);
            }
        }
    }
    return cropped;
}

void BraTSPreprocessor::zscoreNormalize(Volume& volume) {
    double sum = 0.0;
    size_t count = 0;

    for (float v : volume.data) {
        if (v > 0.0f) {
            sum += v;
            ++count;
        }
    }

    if (count == 0) {
        return; // 全零，对应python的mask.sum()==0情况
    }

    double mean = sum / static_cast<double>(count);

    double sqSum = 0.0;
    for (float v : volume.data) {
        if (v > 0.0f) {
            double diff = v - mean;
            sqSum += diff * diff;
        }
    }
    double stddev = std::sqrt(sqSum / static_cast<double>(count));
    if (stddev <= 1e-8) {
        stddev = 1e-8; // 对应python的 std if std > 1e-8 else 1e-8
    }

    for (float& v : volume.data) {
        if (v > 0.0f) {
            v = static_cast<float>((static_cast<double>(v) - mean) / stddev);
        } else {
            v = 0.0f;
        }
    }
}

BraTSPreprocessor::Stats BraTSPreprocessor::computeStats(const Volume& volume, bool maskNonZeroOnly) {
    Stats stats;
    double sum = 0.0;
    size_t count = 0;
    float minVal = std::numeric_limits<float>::max();
    float maxVal = std::numeric_limits<float>::lowest();

    for (float v : volume.data) {
        minVal = std::min(minVal, v);
        maxVal = std::max(maxVal, v);
        if (!maskNonZeroOnly || v != 0.0f) {
            sum += v;
            ++count;
        }
    }

    if (count > 0) {
        double mean = sum / static_cast<double>(count);
        double sqSum = 0.0;
        for (float v : volume.data) {
            if (!maskNonZeroOnly || v != 0.0f) {
                double diff = v - mean;
                sqSum += diff * diff;
            }
        }
        stats.mean = mean;
        stats.stddev = std::sqrt(sqSum / static_cast<double>(count));
    }

    stats.minVal = minVal;
    stats.maxVal = maxVal;
    return stats;
}