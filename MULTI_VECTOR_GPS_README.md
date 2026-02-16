# 多向量GPS算法扩展

## 概述

本项目中的GPS（Generalized Pareto Skyline）算法已扩展为支持任意数量的输入向量，同时保持默认向量数量为2，以确保与原算法的兼容性。

## 主要特性

1. **支持任意维度的向量**：从1维到任意高维度的向量输入
2. **参数验证**：确保输入向量数量为正整数且不小于1
3. **保持核心功能**：Pareto前沿计算的核心逻辑不变
4. **线程安全**：使用互斥锁确保多线程环境下的安全性
5. **可扩展性**：代码结构清晰，易于维护和扩展

## 核心组件

### 1. MultiVectorNeighbor 结构体

```cpp
struct MultiVectorNeighbor {
    unsigned id_;                    // 节点ID
    std::vector<float> distances_;   // 各个维度的距离值
    bool flag;                       // 标记位
    int layer_;                      // 层级信息
};
```

### 2. MultiVectorGPS 类

主要方法：
- `findSkyline()`: 计算Pareto前沿（核心算法）
- `initNeighbor()`: 初始化邻居集合
- `updateNeighbor()`: 更新邻居集合
- `insert()`: 插入新的邻居点
- `setNumDimensions()`: 设置向量维度数
- `getNumDimensions()`: 获取向量维度数

### 3. MultiVectorSkylineQueue 类

用于管理多向量Pareto前沿的队列结构。

## 使用方法

### 基本用法

```cpp
#include "multi_vector_gps.h"

// 创建GPS对象（默认2维）
MultiVectorGPS gps(10, 5, 2); 

// 创建测试点集
std::vector<MultiVectorNeighbor> points = {
    MultiVectorNeighbor(0, {10.0f, 20.0f}, true, 0),
    MultiVectorNeighbor(1, {5.0f, 30.0f}, true, 0),
    MultiVectorNeighbor(2, {15.0f, 10.0f}, true, 0)
};

// 计算Pareto前沿
std::vector<MultiVectorNeighbor> skyline;
std::vector<MultiVectorNeighbor> remain_points;
gps.findSkyline(points, skyline, remain_points);
```

### 使用自定义维度

```cpp
// 创建3维GPS对象
MultiVectorGPS gps(10, 5, 2, 3);

// 创建3维点集
std::vector<MultiVectorNeighbor> points = {
    MultiVectorNeighbor(0, {10.0f, 20.0f, 30.0f}, true, 0),
    MultiVectorNeighbor(1, {5.0f, 30.0f, 25.0f}, true, 0),
    MultiVectorNeighbor(2, {15.0f, 10.0f, 35.0f}, true, 0)
};

// 计算Pareto前沿
std::vector<MultiVectorNeighbor> skyline;
std::vector<MultiVectorNeighbor> remain_points;
gps.findSkyline(points, skyline, remain_points);
```

### 动态设置维度

```cpp
MultiVectorGPS gps; // 默认2维

// 动态设置维度
gps.setNumDimensions(5);

// 创建5维点集
std::vector<MultiVectorNeighbor> points = {
    MultiVectorNeighbor(0, {10.0f, 20.0f, 30.0f, 40.0f, 50.0f}, true, 0),
    MultiVectorNeighbor(1, {5.0f, 30.0f, 25.0f, 35.0f, 45.0f}, true, 0)
};

// 计算Pareto前沿
std::vector<MultiVectorNeighbor> skyline;
std::vector<MultiVectorNeighbor> remain_points;
gps.findSkyline(points, skyline, remain_points);
```

## 编译和测试

### 编译项目

```bash
# 创建构建目录
mkdir build
cd build

# 配置CMake
cmake ..

# 编译
make
```

### 运行单元测试

```bash
# 运行所有测试
./tests/test_multi_vector_gps

# 或使用ctest
ctest
```

### 运行示例程序

```bash
# 编译示例
g++ -std=c++14 -I../include examples/multi_vector_gps_example.cpp -o example

# 运行示例
./example
```

## 单元测试

测试覆盖以下方面：

1. **基本功能测试**
   - 默认构造函数
   - 自定义维度构造函数
   - 参数验证

2. **Pareto前沿计算测试**
   - 2维向量
   - 3维向量
   - 5维向量

3. **邻居管理测试**
   - 初始化邻居
   - 更新邻居
   - 插入点

4. **边界条件测试**
   - 空输入
   - 单点输入
   - 维度不一致

5. **性能测试**
   - 2维向量性能
   - 3维向量性能
   - 5维向量性能

6. **MultiVectorSkylineQueue测试**
   - 基本功能
   - 多维度支持

## 性能分析

算法复杂度：
- **时间复杂度**: O(n * d)，其中n是点数，d是维度数
- **空间复杂度**: O(n * d)

性能特点：
- 随着维度数增加，计算时间线性增长
- 内存使用与维度数成正比
- 支持大规模数据集（测试了1000个点）

## 参数验证

算法包含以下参数验证：

1. **维度数验证**
   - 维度数必须 >= 1
   - 抛出 `std::invalid_argument` 异常

2. **距离向量验证**
   - 所有点的维度数必须一致
   - 插入点的维度数必须匹配GPS对象的维度数

3. **点ID验证**
   - 防止重复插入相同ID的点

## 注意事项

1. **维度一致性**：所有输入点的维度数必须一致
2. **内存管理**：大规模数据集可能需要更多内存
3. **线程安全**：算法已实现线程安全，可在多线程环境中使用
4. **性能优化**：对于高维度数据，考虑使用降维技术

## 与原算法的兼容性

- 默认维度数为2，与原算法完全兼容
- 保持相同的接口设计
- 核心算法逻辑不变
- 可以无缝替换原算法

## 扩展建议

1. **并行计算**：可以进一步优化为并行计算
2. **增量更新**：支持增量式更新Pareto前沿
3. **近似算法**：对于超高维度，可以引入近似算法
4. **可视化**：添加可视化工具以展示Pareto前沿

## 参考文献

- 原GPS算法实现：`include/index.h` 中的 `skyline_descent` 和 `skyline_queue`
- Pareto前沿理论：多目标优化理论
- Skyline算法：数据库领域的Skyline查询算法

## 联系方式

如有问题或建议，请联系项目维护者。
