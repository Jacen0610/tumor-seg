#include "DisplayTransform.h"
#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkOrientImageFilter.h>
#include <stdexcept>
#include <iostream>

DisplayTransform computeDisplayTransform(const std::string& referenceNiiPath) {
    using ImageType = itk::Image<float, 3>;
    using ReaderType = itk::ImageFileReader<ImageType>;

    auto reader = ReaderType::New();
    reader->SetFileName(referenceNiiPath);
    reader->Update();

    auto orienter = itk::OrientImageFilter<ImageType, ImageType>::New();
    orienter->UseImageDirectionOn();
    // RAI: Right-Anterior-Inferior，放射科常用的标准显示朝向之一
    orienter->SetDesiredCoordinateOrientation(
        itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_RAI);
    orienter->SetInput(reader->GetOutput());
    orienter->Update(); // 必须真正执行(而非UpdateOutputInformation)，否则PermuteOrder/FlipAxes不会被正确计算

    auto permuteOrderItk = orienter->GetPermuteOrder();
    auto flipAxesItk = orienter->GetFlipAxes();

    DisplayTransform transform;
    for (int i = 0; i < 3; ++i) {
        transform.permuteOrder[i] = permuteOrderItk[i];
        transform.flip[i] = flipAxesItk[i];
    }

    // 调试输出：确认实际算出的转换规则
    std::cout << "[DisplayTransform] permuteOrder = ("
              << transform.permuteOrder[0] << ", "
              << transform.permuteOrder[1] << ", "
              << transform.permuteOrder[2] << "), flip = ("
              << transform.flip[0] << ", "
              << transform.flip[1] << ", "
              << transform.flip[2] << ")" << std::endl;

    return transform;
}

Volume remapVolumeForDisplay(const Volume& src, const DisplayTransform& transform) {
    // 恒等变换直接返回拷贝，跳过逐体素重排的开销(编译器会优化成一次memcpy，比三重循环快得多)
    if (transform.isIdentity()) {
        return src;
    }

    size_t srcDims[3] = {src.D, src.H, src.W};
    size_t dstDims[3] = {
        srcDims[transform.permuteOrder[0]],
        srcDims[transform.permuteOrder[1]],
        srcDims[transform.permuteOrder[2]]
    };

    Volume dst;
    dst.D = dstDims[0];
    dst.H = dstDims[1];
    dst.W = dstDims[2];
    dst.data.resize(dst.voxelCount());

    for (size_t o0 = 0; o0 < dstDims[0]; ++o0) {
        for (size_t o1 = 0; o1 < dstDims[1]; ++o1) {
            for (size_t o2 = 0; o2 < dstDims[2]; ++o2) {
                size_t outIdx[3] = {o0, o1, o2};
                size_t srcIdx[3];
                for (int i = 0; i < 3; ++i) {
                    size_t srcAxis = transform.permuteOrder[i];
                    size_t val = outIdx[i];
                    if (transform.flip[i]) {
                        val = srcDims[srcAxis] - 1 - val;
                    }
                    srcIdx[srcAxis] = val;
                }
                dst.at(o0, o1, o2) = src.at(srcIdx[0], srcIdx[1], srcIdx[2]);
            }
        }
    }
    return dst;
}

LabelVolume remapLabelVolumeForDisplay(const LabelVolume& src, const DisplayTransform& transform) {
    // 恒等变换直接返回拷贝，跳过逐体素重排的开销
    if (transform.isIdentity()) {
        return src;
    }

    size_t srcDims[3] = {src.D, src.H, src.W};
    size_t dstDims[3] = {
        srcDims[transform.permuteOrder[0]],
        srcDims[transform.permuteOrder[1]],
        srcDims[transform.permuteOrder[2]]
    };

    LabelVolume dst;
    dst.D = dstDims[0];
    dst.H = dstDims[1];
    dst.W = dstDims[2];
    dst.data.resize(dst.D * dst.H * dst.W);

    for (size_t o0 = 0; o0 < dstDims[0]; ++o0) {
        for (size_t o1 = 0; o1 < dstDims[1]; ++o1) {
            for (size_t o2 = 0; o2 < dstDims[2]; ++o2) {
                size_t outIdx[3] = {o0, o1, o2};
                size_t srcIdx[3];
                for (int i = 0; i < 3; ++i) {
                    size_t srcAxis = transform.permuteOrder[i];
                    size_t val = outIdx[i];
                    if (transform.flip[i]) {
                        val = srcDims[srcAxis] - 1 - val;
                    }
                    srcIdx[srcAxis] = val;
                }
                dst.at(o0, o1, o2) = src.at(srcIdx[0], srcIdx[1], srcIdx[2]);
            }
        }
    }
    return dst;
}