cmake_minimum_required(VERSION 4.2)
project(tumor_seg)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 自编译ITK
set(ITK_DIR "/home/jacen/software/ITK-build")

set(CMAKE_PREFIX_PATH "/home/jacen/software/Qt/6.11.1/gcc_64")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent)

# 自编译ITK
find_package(ITK REQUIRED)

# CUDA
find_package(CUDAToolkit REQUIRED)


add_executable(tumor_seg
        src/main.cpp
        src/core/Volume.h
        src/core/NiiVolumeLoader.h
        src/core/NiiVolumeLoader.cpp
        src/core/BraTSPreprocessor.cpp
        src/core/BraTSPreprocessor.h
        src/core/SliceExtractor.h
        src/core/SliceExtractor.cpp
        src/core/CaseLoader.h
        src/core/CaseLoader.cpp
        src/ui_widgets/SliceView.h
        src/ui_widgets/SliceView.cpp
        src/ui_widgets/MainWindow.h
        src/ui_widgets/MainWindow.cpp
        src/core/TensorRTInferenceEngine.h
        src/core/TensorRTInferenceEngine.cpp
        src/core/DisplayTransform.h
        src/core/DisplayTransform.cpp
)


target_link_libraries(tumor_seg PRIVATE
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::Concurrent
        ${ITK_LIBRARIES}
        nvinfer
        CUDA::cudart
)

add_executable(build_engine src/tools/build_engine.cpp)
target_include_directories(build_engine PRIVATE /usr/local/cuda-12.9/include)
target_link_libraries(build_engine PRIVATE
        nvinfer
        nvonnxparser
        CUDA::cudart
)

add_executable(test_inference
        src/test/test_inference.cpp
        src/core/NiiVolumeLoader.cpp
        src/core/BraTSPreprocessor.cpp
        src/core/CaseLoader.cpp
        src/core/TensorRTInferenceEngine.cpp
        src/core/DisplayTransform.h
        src/core/DisplayTransform.cpp
)

target_include_directories(test_inference PRIVATE /usr/local/cuda-12.9/include)

target_link_libraries(test_inference PRIVATE
        ${ITK_LIBRARIES}
        nvinfer
        CUDA::cudart
)