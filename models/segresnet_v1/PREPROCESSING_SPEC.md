# models/segresnet_v1/PREPROCESSING_SPEC.md

## 模型信息
- 架构: MONAI SegResNet, init_filters=16
- 训练数据: BraTS2020, 320 cases (49 held out for validation)
- 验证集Dice: WT 0.9198 / TC 0.8379 / ET 0.7336

## 预处理规范（C++端必须严格遵循）
- 模态通道顺序: [flair, t1, t1ce, t2]
- 裁剪: 基于flair模态非零区域计算bounding box, margin=0
- 归一化: z-score, 仅在非零(脑组织)区域内计算均值方差, std保护阈值1e-8
- 网络输入: (batch, 4, 96, 96, 96)

## 标签映射
- 网络输出类别: 0=背景, 1=NCR/NET, 2=ED, 3=ET(对应BraTS原始标签4)
- 还原时: 网络输出3 → 原始标签4，其余类别值不变