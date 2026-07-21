#include "NiiVolumeLoader.h"
#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkNiftiImageIOFactory.h>
#include <itkImageRegionConstIterator.h>
#include <stdexcept>

void NiiVolumeLoader::RegisterFactories() {
    itk::NiftiImageIOFactory::RegisterOneFactory();
}

Volume NiiVolumeLoader::Load(const std::string& filepath) {
    using PixelType = float;
    using ImageType = itk::Image<PixelType, 3>;
    using ReaderType = itk::ImageFileReader<ImageType>;

    auto reader = ReaderType::New();
    reader->SetFileName(filepath);

    try {
        reader->Update();
    } catch (const itk::ExceptionObject& err) {
        throw std::runtime_error(
            "读取nii文件失败: " + filepath + " - " + std::string(err.GetDescription()));
    }

    ImageType::Pointer image = reader->GetOutput();
    ImageType::SizeType size = image->GetLargestPossibleRegion().GetSize();

    Volume vol;
    vol.D = size[0];
    vol.H = size[1];
    vol.W = size[2];
    vol.data.resize(vol.voxelCount());

    itk::ImageRegionConstIterator<ImageType> it(image, image->GetLargestPossibleRegion());
    for (it.GoToBegin(); !it.IsAtEnd(); ++it) {
        ImageType::IndexType idx = it.GetIndex();
        vol.at(static_cast<size_t>(idx[0]),
               static_cast<size_t>(idx[1]),
               static_cast<size_t>(idx[2])) = it.Get();
    }

    return vol;
}