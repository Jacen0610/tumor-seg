#pragma once
#include <string>
#include "Volume.h"

class NiiVolumeLoader {
public:
    // 程序启动时调用一次，注册NIfTI IO工厂（对应之前踩的NoFactoryException坑）
    static void RegisterFactories();

    // 读取单个nii文件，返回Volume
    static Volume Load(const std::string& filepath);
};