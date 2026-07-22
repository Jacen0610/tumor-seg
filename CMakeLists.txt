cmake_minimum_required(VERSION 4.2)
project(tumor_seg)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ==================== 第三方库路径配置 ====================
set(ITK_DIR "/home/jacen/software/ITK-build")
set(CMAKE_PREFIX_PATH "/home/jacen/software/Qt/6.11.1/gcc_64")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent Quick QuickControls2)
find_package(ITK REQUIRED)
find_package(CUDAToolkit REQUIRED)



# ==================== target 1: tumor_seg (Qt Widgets版界面) ====================
add_executable(tumor_seg
        src/main.cpp
        src/core/Volume.h
        src/core/OverlaySettings.h
        src/core/NiiVolumeLoader.h
        src/core/NiiVolumeLoader.cpp
        src/core/BraTSPreprocessor.h
        src/core/BraTSPreprocessor.cpp
        src/core/SliceExtractor.h
        src/core/SliceExtractor.cpp
        src/core/CaseLoader.h
        src/core/CaseLoader.cpp
        src/core/TensorRTInferenceEngine.h
        src/core/TensorRTInferenceEngine.cpp
        src/core/DisplayTransform.h
        src/core/DisplayTransform.cpp
        src/ui_widgets/SliceView.h
        src/ui_widgets/SliceView.cpp
        src/ui_widgets/MainWindow.h
        src/ui_widgets/MainWindow.cpp
)

target_include_directories(tumor_seg PRIVATE /usr/local/cuda-12.9/include)

target_link_libraries(tumor_seg PRIVATE
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::Concurrent
        ${ITK_LIBRARIES}
        nvinfer
        CUDA::cudart
)


# ==================== target 2: tumor_seg_qml (QML版界面) ====================
# 注意: 只依赖 core/，完全不包含 ui_widgets/ 下的任何文件
qt_add_executable(tumor_seg_qml
        src/ui_qml/main_qml.cpp
        src/ui_qml/Backend.h
        src/ui_qml/Backend.cpp
        src/ui_qml/SliceImageProvider.h
        src/ui_qml/SliceImageProvider.cpp
        src/core/Volume.h
        src/core/OverlaySettings.h
        src/core/NiiVolumeLoader.h
        src/core/NiiVolumeLoader.cpp
        src/core/BraTSPreprocessor.h
        src/core/BraTSPreprocessor.cpp
        src/core/SliceExtractor.h
        src/core/SliceExtractor.cpp
        src/core/CaseLoader.h
        src/core/CaseLoader.cpp
        src/core/TensorRTInferenceEngine.h
        src/core/TensorRTInferenceEngine.cpp
        src/core/DisplayTransform.h
        src/core/DisplayTransform.cpp
)

qt_add_qml_module(tumor_seg_qml
        URI TumorSegUi
        VERSION 1.0
        QML_FILES src/ui_qml/qml/Main.qml
)

target_include_directories(tumor_seg_qml PRIVATE /usr/local/cuda-12.9/include)

target_link_libraries(tumor_seg_qml PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Concurrent
        ${ITK_LIBRARIES}
        nvinfer
        CUDA::cudart
)


# ==================== target 3: test_inference (命令行验证工具，不依赖任何UI) ====================
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