# MultiVectorDEG - 多向量离散权重DEG算法

## 概述

MultiVectorDEG 是 DEG (Dynamic Edge Graph) 算法的扩展实现，支持多向量场景下的离散化权重计算。该实现将连续的权重空间离散化为0.1步长，使用bitmap高效存储不同权重组合下的剪枝状态。

## 核心特性

### 1. 离散化权重支持
- **权重范围**: 0.0 ~ 1.0，步长 0.1
- **约束条件**: 所有权重之和必须等于 1
- **组合生成**: 自动计算所有满足约束的权重组合

### 2. 多向量支持
- **最大维度**: 支持最多 8 个向量维度（可配置）
- **灵活扩展**: 可轻松调整 `MAX_VECTOR_DIM` 常量支持更多维度

### 3. Bitmap 剪枝状态存储
- **高效存储**: 使用 `std::vector<uint64_t>` 作为 bitmap
- **位操作**: 每个 bit 代表一种权重组合下的剪枝状态
- **空间优化**: 相比存储 active_range（每个范围需要两个 int8_t），bitmap 更加紧凑

## 文件结构

```
/home/ruanb/code/DEG/
├── include/
│   └── multi_vector_deg.h          # 核心头文件，包含所有实现
├── tests/
│   └── test_multi_vector_deg.cpp   # 单元测试文件
└── README_MultiVectorDEG.md         # 本文档
```

## 核心类与结构

### 1. `MultiVectorDEGNeighbor`

存储多向量DEG算法中的邻居节点信息，包含：
- `id_`: 节点ID
- `distances_`: 各个维度的距离值
- `pruning_bitmap_`: 剪枝bitmap
- `num_weight_combinations_`: 权重组合数量

**主要方法**:
- `setPruned(weight_idx, pruned)`: 设置指定权重组合的剪枝状态
- `isPruned(weight_idx)`: 获取指定权重组合的剪枝状态

### 2. `MultiVectorDEG`

核心算法类，实现：
- 权重组合生成
- Pareto前沿计算（Skyline算法）
- 邻居初始化和更新
- 剪枝状态管理

**主要方法**:
- `generateWeightCombinations(num_dimensions)`: 生成所有有效权重组合
- `findSkyline(points, skyline, remain_points)`: 计算Pareto前沿
- `initNeighbor(insert_points)`: 初始化邻居集合
- `updateNeighbor()`: 更新邻居集合

### 3. `MultiVectorDEGSkylineQueue`

管理多向量DEG Pareto前沿的队列结构，用于构建分层索引。

## 权重组合生成算法

使用回溯算法生成所有满足约束的权重组合：

```cpp
// 伪代码
function backtrack(dim, current_sum, remaining):
    if dim == num_dimensions - 1:
        // 最后一个维度，填充剩余权重
        last_weight = 1.0 - current_sum
        if isValid(last_weight):
            addCombination(current + [last_weight])
        return
    
    for w from 0.0 to 1.0 step 0.1:
        if current_sum + w <= 1.0:
            if hasValidRemaining(dim, current_sum + w):
                backtrack(dim + 1, current_sum + w, remaining - w)
```

## 使用示例

### 基本用法

```cpp
#include "multi_vector_deg.h"

using namespace stkq;

// 创建3维DEG算法实例
MultiVectorDEG deg(10, 5, 2, 3);

// 查看权重组合数量
std::cout << "Weight combinations: " << deg.getNumWeightCombinations() << std::endl;

// 创建测试点集
std::vector<MultiVectorDEGNeighbor> points = {
    MultiVectorDEGNeighbor(0, {10.0f, 20.0f, 30.0f}, 
                          deg.getNumWeightCombinations(), true, 0),
    MultiVectorDEGNeighbor(1, {5.0f, 30.0f, 25.0f}, 
                          deg.getNumWeightCombinations(), true, 0)
};

// 初始化邻居集合
deg.initNeighbor(points);
```

### 剪枝状态管理

```cpp
// 获取邻居节点
MultiVectorDEGNeighbor& neighbor = ...;

// 设置第3种权重组合下的剪枝状态
neighbor.setPruned(3, true);

// 查询第3种权重组合下的剪枝状态
bool is_pruned = neighbor.isPruned(3);
```

## 性能特点

### 空间复杂度

- **权重组合存储**: O(C)，其中 C 是权重组合数量
  - 2D: 11 种组合
  - 3D: 66 种组合
  - 4D: 286 种组合
  - 5D: 1,001 种组合
  
- **Bitmap 存储**: O(C/64) uint64_t 每边
  - 相比存储 float 范围的 active_range，bitmap 方式更加紧凑

### 时间复杂度

- **权重组合生成**: O(C × D)，回溯算法
- **Skyline 计算**: O(N² × D)，其中 N 是点数，D 是维度数
- **剪枝状态查询**: O(1)，通过位运算

## 与原始 DEG 算法的对比

| 特性 | 原始 DEG | MultiVectorDEG |
|------|----------|----------------|
| 向量维度 | 2 (emb + geo) | 任意 N ≥ 1 |
| 权重表示 | active_range (int8_t 对) | Bitmap |
| 权重类型 | 连续 | 离散 (0.1 步长) |
| 权重约束 | 无 | 之和 = 1 |
| 存储效率 | 每边多个范围 | 每边一个 bitmap |

## 扩展建议

1. **更多维度**: 增加 `MAX_VECTOR_DIM` 常量
2. **更细粒度**: 调整 `WEIGHT_STEP` 常量（如 0.05）
3. **并行优化**: 对 Skyline 计算使用 OpenMP 并行化
4. **索引优化**: 实现 R-tree 或 KD-tree 加速最近邻搜索

## 许可证

本实现遵循与 DEG 项目相同的许可证条款。

---

**作者**: AI Assistant
**创建日期**: 2026-02-16
**版本**: 1.0
