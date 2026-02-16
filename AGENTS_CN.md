# DEG: 动态边导航图

## 项目概述

这是 **DEG (Dynamic Edge Navigation Graph，动态边导航图)** 的 C++ 实现，这是一个用于高效混合向量搜索的研究项目。该项目实现了一种新颖的基于图的索引结构，支持使用动态权重参数（alpha）结合两个向量空间（嵌入向量和空间/位置向量）进行查询。

### 主要特性

- **混合向量查询支持**：使用嵌入距离和空间距离的加权和计算相似度
- **动态 Alpha 支持**：高效处理具有不同 alpha 参数的查询（嵌入和空间相似度之间的权重）
- **多种索引结构**：实现了 HNSW、NSG、NSW、R-Tree 以及提出的 DEG 算法
- **多线程**：使用 OpenMP 进行并行处理
- **SIMD 优化**：使用 AVX2 指令集进行快速距离计算

## 技术栈

| 组件 | 技术 |
|-----------|------------|
| 语言 | C++14 |
| 构建系统 | CMake 2.8+ |
| 并行处理 | OpenMP |
| 外部库 | Boost 1.55+ |
| 编译器 | GCC 4.9+ |

## 项目结构

```
DEG/
├── CMakeLists.txt          # 主 CMake 配置文件
├── README.md               # 项目文档
├── AGENTS.md              # 英文版本文档
├── AGENTS_CN.md           # 中文版本文档
├── include/               # 头文件
│   ├── index.h            # 核心索引数据结构（NNDescent、HNSW、DEG 等）
│   ├── builder.h          # 索引构建器接口
│   ├── component.h        # 模块化操作的组件类层次结构
│   ├── distance.h         # 使用 AVX2 SIMD 的距离计算
│   ├── parameters.h       # 参数管理类
│   ├── policy.h           # 算法类型枚举
│   ├── rtree.h            # R-Tree 空间索引实现
│   ├── set_para.h         # 数据集特定参数配置
│   ├── util.h             # 工具函数（随机数生成）
│   └── CommonDataStructure.h  # 数组包装模板类
├── src/                   # 源文件
│   ├── builder.cpp        # IndexBuilder 实现
│   ├── component_*.cpp    # 组件实现（初始化、搜索、剪枝等）
│   ├── nndescent.cpp      # NN-Descent 算法
│   └── rtree.cpp          # R-Tree 实现
├── test/                  # 测试/可执行代码
│   └── main.cpp           # 实验的主入口点
└── build/                 # 构建输出目录
```

## 架构

### 核心组件

1. **Index 类** (`index.h`)：包含以下内容的中心数据结构：
   - 基础数据（嵌入向量、位置向量）
   - 查询数据和真值
   - 图结构（HNSW 节点、DEG 节点等）
   - 距离计算对象
   - 参数

2. **IndexBuilder 类** (`builder.h`)：构建器模式，用于：
   - 加载数据集
   - 初始化索引结构
   - 构建和保存图
   - 执行搜索
   - 性能评估

3. **Component 层次结构** (`component.h`)：模块化设计，包含以下组件：
   - `ComponentLoad`：数据加载
   - `ComponentInit*`：索引初始化（HNSW、DEG、RTree 等）
   - `ComponentSearchRoute*`：搜索路由策略
   - `ComponentPrune*`：图剪枝算法
   - `ComponentRefine*`：图精炼（NN-Descent、NSG）

### 支持的算法

| 算法 | 类型 | 描述 |
|-----------|------|-------------|
| HNSW | 基准 | 分层导航小世界图 |
| baseline1 | 基准 | 固定 alpha=0.5 的 HNSW |
| baseline2 | 基准 | 双 HNSW 索引（alpha=0 和 alpha=1） |
| baseline3 | 基准 | R-Tree + HNSW 混合 |
| baseline4 | 基准 | 多层 HNSW 变体 |
| DEG | 提出的方法 | 动态边导航图 |

### 数据格式

项目使用标准向量文件格式：
- **`.fvecs`**：浮点向量（基础嵌入、查询嵌入、位置）
- **`.ivecs`**：整型向量（真值标签）

格式规范：http://yael.gforge.inria.fr/file_format.html

### 支持的数据集

代码在 `include/set_para.h` 中包含了这些数据集的硬编码路径：
- `openimage` - Open Images 数据集
- `sg-ins` - 新加坡 Instagram 数据集
- `howto100m` - HowTo100M 数据集
- `cc3m` - Conceptual Captions 3M
- `Twitter10M` - Twitter 数据集

**注意**：数据集路径在 `test/main.cpp` 中硬编码为 `/mnt/hdd/yinziqi/yinziqi/graphann-tkq/dataset/`。

## 构建说明

### 前置要求

```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake libboost-all-dev

# 确保 GCC 版本 >= 4.9
gcc --version
```

### 构建命令

```bash
# 创建构建目录
mkdir -p build && cd build/

# 配置
cmake ..

# 使用并行作业构建
make -j

# 主可执行文件将位于：build/test/main
```

### CMake 选项

`CMakeLists.txt` 设置了这些重要标志：
- `-std=c++14`：C++14 标准
- `-O2`：优化级别 2
- `-march=native`：针对当前 CPU 优化
- `-Wall`：启用所有警告
- `-DINFO`：启用信息日志

## 使用方法

### 命令行界面

```bash
./main <algorithm> <dataset> <alpha> <max_spatial_distance> <max_emb_distance> <exc_type>
```

参数：
- `algorithm`：`hnsw`、`baseline1`、`baseline2`、`baseline3`、`baseline4`、`deg` 之一
- `dataset`：`openimage`、`sg-ins`、`howto100m`、`cc3m`、`Twitter10M` 之一
- `alpha`：权重参数（0.0 到 1.0）
- `max_spatial_distance`：最大空间距离归一化因子
- `max_emb_distance`：最大嵌入距离归一化因子
- `exc_type`：`build` 或 `search`

### 示例命令

```bash
# 构建 DEG 索引
cd build/test/
./main deg openimage 0.5 1 1 build

# 使用 DEG 索引搜索
./main deg openimage 0.5 1 1 search
```

### 输出文件

索引文件保存到 `/mnt/hdd/yinziqi/yinziqi/graphann-tkq/saved_index/`（硬编码）：
- HNSW：`{alg}_{dataset}.index`
- DEG：`{alg}_{dataset}.index`
- BS4：`{alg}_{dataset}.index_subindex_0` 到 `_4`
- R-Tree：自定义二进制格式

## 代码风格指南

### 命名约定

- **类**：PascalCase（例如：`IndexBuilder`、`ComponentInitHNSW`）
- **方法**：公共方法使用 PascalCase（例如：`InitInner()`、`LoadInner()`）
- **私有成员**：snake_case 带下划线后缀（例如：`final_index_`、`max_level_`）
- **常量**：UPPER_CASE（例如：`MAXNODES`、`RTREEMAXNODES`）
- **命名空间**：`stkq`（所有代码都在此命名空间内）

### 注释

- 现有代码库中的注释主要是**中文**
- 使用 `//` 进行单行注释
- 关键算法有解释逻辑的内联注释

### 代码模式

1. **组件模式**：操作封装在继承自基类 `Component` 的组件类中
2. **构建器模式**：`IndexBuilder` 提供流畅接口（方法链式调用）
3. **类型安全**：使用 `TYPE` 枚举进行算法选择
4. **内存管理**：使用原始指针，在析构函数中显式删除

## 测试策略

项目通过主可执行文件使用**手动测试方法**：

1. **构建测试**：验证索引构建完成且无错误
2. **搜索测试**：将搜索结果与真值进行比较
3. **召回率测量**：自动计算 recall@K
'4. **性能指标**：
   - 每个查询的搜索时间
   - 距离计算次数
   - 跳数（图遍历步骤）
   - 内存占用

### 评估指标

代码自动输出：
```
search time: <seconds_per_query>
DistCount: <number_of_distance_computations>
HopCount: <number_of_hops>
<K> NN accuracy: <recall_value>
```

## 重要实现细节

### 线程安全性

- OpenMP 用于并行图构建
- 互斥锁保护共享数据结构（节点类中的 `std::mutex`）
- 线程局部随机数生成器

### 距离计算

- **嵌入距离**：AVX2 优化的平方欧几里得距离，按最大距离归一化
- **空间距离**：2D 欧几里得距离，按最大距离归一化
- **组合距离**：`alpha * e_dist + (1 - alpha) * s_dist`

### 图结构

- **HNSW**：带有入口点的多层图
- **DEG**：单层图，具有：
  - 基于 Pareto 前沿的边选择
  - 每条边的 alpha 范围元数据
  - 多个入口点

### 内存管理

- 大数组使用 `mm_malloc` 进行对齐内存分配
- 自定义 `Array<T>` 模板类用于安全数组处理
- 图数据尽可能存储在连续内存中

## 安全注意事项

1. **文件 I/O**：二进制文件读取没有边界检查 - 确保输入文件有效
2. **内存**：数据集的大内存分配 - 检查可用 RAM
3. **路径处理**：main.cpp 中的硬编码绝对路径 - 根据您的环境修改
4. **无输入清理**：命令行参数直接转换为数字

## 常见修改

### 添加新数据集

编辑 `include/set_para.h`：
1. 在 `set_data_path()` 中添加数据集名称检查
2. 定义 `.fvecs` 和 `.ivecs` 文件的文件路径
3. 在算法特定函数中添加参数配置

### 更改数据集根路径

编辑 `test/main.cpp` 第 ~265 行：
```cpp
std::string dataset_root = R"(/your/path/to/dataset/)";
```

### 调整线程数

编辑 `test/main.cpp` 第 ~269` 行：
```cpp
parameters.set<unsigned>("n_threads", <num_threads>);
```

## 故障排除

### 构建问题

- **未找到 OpenMP**：确保已安装 `libgomp`
- **未找到 Boost**：在 CMakeLists.txt 中设置 `BOOST_ROOT`
- **AVX2 错误**：删除 `-march=native` 或添加 `-mavx2` 标志

### 运行时问题

- **文件未找到**：检查 main.cpp 和 set_para.h 中的硬编码路径
- **段错误**：验证数据集文件与预期维度匹配
- **内存不足**：减少批处理大小或使用较小的数据集

## 参考文献

- 项目基于动态边导航图的研究论文
- 实现了 HNSW (Malkov & Yashunin)、NSG (Fu et al.) 和 NN-Descent (Dong et al.) 的概念
- R-Tree 实现基于 Greg Douglas 的 RTree 模板
