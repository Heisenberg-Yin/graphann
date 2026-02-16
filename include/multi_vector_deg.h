#ifndef STKQ_MULTI_VECTOR_DEG_H
#define STKQ_MULTI_VECTOR_DEG_H

#include <vector>
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <iostream>
#include <mutex>
#include <bitset>
#include <cmath>

namespace stkq
{
    // 最大支持的向量维度数（可根据需要调整）
    constexpr size_t MAX_VECTOR_DIM = 8;
    // 权重步长（0.1）
    constexpr float WEIGHT_STEP = 0.1f;
    // 权重离散化的步数（0到1，步长0.1，共11个值）
    constexpr size_t WEIGHT_STEPS = 11;

    /**
     * @brief 生成所有满足约束的权重组合
     * 
     * 约束条件：
     * 1. 每个权重在0~1之间，步长0.1
     * 2. 所有权重之和为1
     * 
     * @param num_dimensions 向量维度数
     * @return 所有有效的权重组合
     */
    inline std::vector<std::vector<float>> generateWeightCombinations(size_t num_dimensions)
    {
        std::vector<std::vector<float>> result;
        
        if (num_dimensions == 0)
        {
            return result;
        }
        
        if (num_dimensions == 1)
        {
            // 只有一个维度时，权重必须为1
            result.push_back({1.0f});
            return result;
        }
        
        // 使用递归生成所有组合
        std::vector<float> current;
        
        std::function<void(size_t, float, float)> backtrack = [&](size_t dim, float current_sum, float remaining)
        {
            if (dim == num_dimensions - 1)
            {
                // 最后一个维度，必须填满剩余权重
                float last_weight = 1.0f - current_sum;
                // 检查是否是0.1的倍数且在[0,1]范围内
                if (last_weight >= -0.001f && last_weight <= 1.001f)
                {
                    float rounded = std::round(last_weight * 10.0f) / 10.0f;
                    if (rounded >= -0.001f && rounded <= 1.001f)
                    {
                        current.push_back(rounded);
                        result.push_back(current);
                        current.pop_back();
                    }
                }
                return;
            }
            
            // 尝试所有可能的权重值
            for (float w = 0.0f; w <= 1.001f && current_sum + w <= 1.001f; w += WEIGHT_STEP)
            {
                float rounded_w = std::round(w * 10.0f) / 10.0f;
                // 确保剩余维度有足够的权重分配（每个至少0，最多1）
                float min_needed = 0.0f;
                float max_possible = (num_dimensions - dim - 1) * 1.0f;
                float remaining_needed = 1.0f - (current_sum + rounded_w);
                
                if (remaining_needed >= min_needed - 0.001f && remaining_needed <= max_possible + 0.001f)
                {
                    current.push_back(rounded_w);
                    backtrack(dim + 1, current_sum + rounded_w, remaining_needed);
                    current.pop_back();
                }
            }
        };
        
        backtrack(0, 0.0f, 1.0f);
        
        // 去重（由于浮点数精度问题可能有重复）
        std::sort(result.begin(), result.end());
        result.erase(std::unique(result.begin(), result.end()), result.end());
        
        return result;
    }

    /**
     * @brief 计算权重组合的索引
     * 
     * 每个权重组合对应一个唯一的索引，用于bitmap中的位位置
     * 
     * @param weights 权重向量
     * @return 权重组合的索引
     */
    inline size_t getWeightCombinationIndex(const std::vector<float> &weights)
    {
        // 使用简单的哈希方式：将每个权重乘以10后作为数字的一部分
        size_t index = 0;
        for (float w : weights)
        {
            index = index * WEIGHT_STEPS + static_cast<size_t>(std::round(w * 10.0f));
        }
        return index;
    }

    /**
     * @brief 多向量DEG邻居节点
     * 
     * 该结构体用于存储多向量DEG算法中的邻居节点信息
     * 使用bitmap表示在不同权重组合下的剪枝状态
     */
    struct MultiVectorDEGNeighbor
    {
        unsigned id_;                           // 节点ID
        std::vector<float> distances_;          // 各个维度的距离值
        std::vector<uint64_t> pruning_bitmap_;  // 剪枝bitmap，每个位表示一种权重组合下是否需要剪枝
        bool flag;                              // 标记位
        int layer_;                             // 层级信息
        size_t num_weight_combinations_;        // 权重组合数量

        MultiVectorDEGNeighbor() = default;

        /**
         * @brief 构造函数
         * @param id 节点ID
         * @param distances 距离向量
         * @param num_weight_combinations 权重组合数量
         * @param f 标记位
         * @param layer 层级
         */
        MultiVectorDEGNeighbor(unsigned id, const std::vector<float> &distances, 
                               size_t num_weight_combinations, bool f, int layer)
            : id_(id), distances_(distances), flag(f), layer_(layer), 
              num_weight_combinations_(num_weight_combinations)
        {
            // 初始化bitmap，每个位表示一种权重组合
            size_t num_uint64 = (num_weight_combinations + 63) / 64;
            pruning_bitmap_.resize(num_uint64, 0);
        }

        /**
         * @brief 获取距离向量的维度数
         * @return 维度数
         */
        size_t getDimension() const
        {
            return distances_.size();
        }

        /**
         * @brief 获取指定维度的距离值
         * @param dim 维度索引
         * @return 距离值
         */
        float getDistance(size_t dim) const
        {
            if (dim >= distances_.size())
            {
                throw std::out_of_range("Dimension index out of range");
            }
            return distances_[dim];
        }

        /**
         * @brief 设置指定权重组合的剪枝状态
         * @param weight_idx 权重组合索引
         * @param pruned 是否剪枝
         */
        void setPruned(size_t weight_idx, bool pruned)
        {
            if (weight_idx >= num_weight_combinations_)
            {
                throw std::out_of_range("Weight index out of range");
            }
            size_t uint64_idx = weight_idx / 64;
            size_t bit_idx = weight_idx % 64;
            if (pruned)
            {
                pruning_bitmap_[uint64_idx] |= (1ULL << bit_idx);
            }
            else
            {
                pruning_bitmap_[uint64_idx] &= ~(1ULL << bit_idx);
            }
        }

        /**
         * @brief 获取指定权重组合的剪枝状态
         * @param weight_idx 权重组合索引
         * @return 是否剪枝
         */
        bool isPruned(size_t weight_idx) const
        {
            if (weight_idx >= num_weight_combinations_)
            {
                throw std::out_of_range("Weight index out of range");
            }
            size_t uint64_idx = weight_idx / 64;
            size_t bit_idx = weight_idx % 64;
            return (pruning_bitmap_[uint64_idx] & (1ULL << bit_idx)) != 0;
        }

        /**
         * @brief 小于运算符重载（用于排序）
         * @param other 另一个MultiVectorDEGNeighbor对象
         * @return 如果当前对象小于other，返回true
         */
        bool operator<(const MultiVectorDEGNeighbor &other) const
        {
            // 按照距离向量的字典序比较
            for (size_t i = 0; i < distances_.size() && i < other.distances_.size(); ++i)
            {
                if (distances_[i] != other.distances_[i])
                {
                    return distances_[i] < other.distances_[i];
                }
            }
            return distances_.size() < other.distances_.size();
        }

        /**
         * @brief 大于运算符重载（用于排序）
         * @param other 另一个MultiVectorDEGNeighbor对象
         * @return 如果当前对象大于other，返回true
         */
        bool operator>(const MultiVectorDEGNeighbor &other) const
        {
            return other < *this;
        }

        /**
         * @brief 等于运算符重载
         * @param other 另一个MultiVectorDEGNeighbor对象
         * @return 如果当前对象等于other，返回true
         */
        bool operator==(const MultiVectorDEGNeighbor &other) const
        {
            if (id_ != other.id_)
                return false;
            if (distances_.size() != other.distances_.size())
                return false;
            for (size_t i = 0; i < distances_.size(); ++i)
            {
                if (distances_[i] != other.distances_[i])
                    return false;
            }
            return true;
        }

        /**
         * @brief 不等于运算符重载
         * @param other 另一个MultiVectorDEGNeighbor对象
         * @return 如果当前对象不等于other，返回true
         */
        bool operator!=(const MultiVectorDEGNeighbor &other) const
        {
            return !(*this == other);
        }
    };

    /**
     * @brief 多向量DEG算法（Discrete-weight Enhanced Graph）
     * 
     * 该算法支持任意数量的输入向量，用于计算近似Pareto前沿
     * 使用离散化权重（0~1，步长0.1），权重之和为1
     * 使用bitmap表示边在不同权重组合下的剪枝状态
     */
    class MultiVectorDEG
    {
    private:
        std::mutex lock_;                                       // 互斥锁，用于线程安全
        std::vector<MultiVectorDEGNeighbor> pool_;             // 候选池
        std::vector<MultiVectorDEGNeighbor> outlier_;        // 被剔除的点
        unsigned M_;                                            // 池的最大容量
        unsigned Q_;                                            // 质量参数
        unsigned num_layer_;                                    // 层数
        unsigned num_dimensions_;                              // 向量维度数
        std::vector<std::vector<float>> weight_combinations_; // 所有有效的权重组合
        size_t num_weight_combinations_;                      // 权重组合数量

    public:
        /**
         * @brief 默认构造函数，使用2个向量维度
         */
        MultiVectorDEG() : M_(0), Q_(0), num_layer_(0), num_dimensions_(2), num_weight_combinations_(0)
        {
            initializeWeightCombinations();
        }

        /**
         * @brief 构造函数
         * @param m 池的最大容量
         * @param s 邻居数量
         * @param q 质量参数
         * @param num_dimensions 向量维度数（默认为2）
         */
        MultiVectorDEG(unsigned m, unsigned s, unsigned q, unsigned num_dimensions = 2)
            : M_(m), Q_(q), num_layer_(0), num_dimensions_(num_dimensions)
        {
            // 参数验证
            if (num_dimensions < 1)
            {
                throw std::invalid_argument("Number of dimensions must be at least 1");
            }
            if (num_dimensions > MAX_VECTOR_DIM)
            {
                throw std::invalid_argument("Number of dimensions exceeds maximum supported");
            }
            
            initializeWeightCombinations();
        }

        /**
         * @brief 初始化权重组合
         */
        void initializeWeightCombinations()
        {
            weight_combinations_ = generateWeightCombinations(num_dimensions_);
            num_weight_combinations_ = weight_combinations_.size();
        }

        /**
         * @brief 获取权重组合数量
         * @return 权重组合数量
         */
        size_t getNumWeightCombinations() const
        {
            return num_weight_combinations_;
        }

        /**
         * @brief 获取所有权重组合
         * @return 权重组合列表
         */
        const std::vector<std::vector<float>> &getWeightCombinations() const
        {
            return weight_combinations_;
        }

        /**
         * @brief 计算给定权重的组合索引
         * @param weights 权重向量
         * @return 权重组合索引，如果找不到则返回SIZE_MAX
         */
        size_t getWeightIndex(const std::vector<float> &weights) const
        {
            if (weights.size() != num_dimensions_)
            {
                return SIZE_MAX;
            }
            
            // 四舍五入到最近的0.1
            std::vector<float> rounded_weights;
            rounded_weights.reserve(weights.size());
            for (float w : weights)
            {
                rounded_weights.push_back(std::round(w * 10.0f) / 10.0f);
            }
            
            // 查找匹配的权重组合
            for (size_t i = 0; i < weight_combinations_.size(); ++i)
            {
                bool match = true;
                for (size_t j = 0; j < num_dimensions_; ++j)
                {
                    if (std::abs(weight_combinations_[i][j] - rounded_weights[j]) > 0.001f)
                    {
                        match = false;
                        break;
                    }
                }
                if (match)
                {
                    return i;
                }
            }
            
            return SIZE_MAX;
        }

        /**
         * @brief 设置向量维度数
         * @param num_dimensions 向量维度数
         */
        void setNumDimensions(unsigned num_dimensions)
        {
            if (num_dimensions < 1)
            {
                throw std::invalid_argument("Number of dimensions must be at least 1");
            }
            if (num_dimensions > MAX_VECTOR_DIM)
            {
                throw std::invalid_argument("Number of dimensions exceeds maximum supported");
            }
            num_dimensions_ = num_dimensions;
            initializeWeightCombinations();
        }

        /**
         * @brief 获取向量维度数
         * @return 向量维度数
         */
        unsigned getNumDimensions() const
        {
            return num_dimensions_;
        }

        /**
         * @brief 核心算法：计算Pareto前沿（Skyline）
         * 
         * 该函数使用GPS算法从给定的点集中找出Pareto前沿
         * 算法复杂度为O(n^2*d)，其中n是点数，d是维度数
         * 
         * @param points 输入点集
         * @param skyline 输出的Pareto前沿点集
         * @param remain_points 剩余的非前沿点集
         */
        void findSkyline(std::vector<MultiVectorDEGNeighbor> &points,
                         std::vector<MultiVectorDEGNeighbor> &skyline,
                         std::vector<MultiVectorDEGNeighbor> &remain_points)
        {
            // 参数验证
            if (points.empty())
            {
                return;
            }

            // 验证所有点的维度一致
            size_t expected_dim = num_dimensions_;
            for (const auto &point : points)
            {
                if (point.distances_.size() != expected_dim)
                {
                    throw std::invalid_argument("All points must have the same number of dimensions");
                }
            }

            // 遍历所有点，找出Pareto前沿
            for (const auto &point : points)
            {
                // 检查当前点是否在Pareto前沿上
                // 一个点p在Pareto前沿上，如果不存在另一个点q，使得q在所有维度上都小于等于p，且至少在一个维度上严格小于
                bool is_skyline = true;
                for (const auto &other : points)
                {
                    if (point.id_ == other.id_) continue;

                    // 检查other是否支配point
                    bool dominates = true;
                    bool strictly_less = false;
                    for (size_t dim = 0; dim < expected_dim; ++dim)
                    {
                        if (other.distances_[dim] > point.distances_[dim])
                        {
                            dominates = false;
                            break;
                        }
                        if (other.distances_[dim] < point.distances_[dim])
                        {
                            strictly_less = true;
                        }
                    }

                    // 如果other支配point，则point不在Pareto前沿上
                    if (dominates && strictly_less)
                    {
                        is_skyline = false;
                        break;
                    }
                }

                if (is_skyline)
                {
                    skyline.push_back(point);
                }
                else
                {
                    remain_points.emplace_back(point);
                }
            }
            // O(n^2*d)
        }

        /**
         * @brief 初始化邻居集合
         * 
         * 使用GPS算法初始化邻居集合，构建多层Pareto前沿
         * 
         * @param insert_points 要插入的点集
         */
        void initNeighbor(std::vector<MultiVectorDEGNeighbor> &insert_points)
        {
            std::lock_guard<std::mutex> guard(lock_);
            std::vector<MultiVectorDEGNeighbor> skyline_result;
            std::vector<MultiVectorDEGNeighbor> remain_points;
            int l = 0;

            while (!insert_points.empty())
            {
                findSkyline(insert_points, skyline_result, remain_points);
                insert_points.swap(remain_points);

                for (auto &point : skyline_result)
                {
                    pool_.emplace_back(point.id_, point.distances_, num_weight_combinations_, true, l);
                }

                outlier_.swap(skyline_result);
                std::vector<MultiVectorDEGNeighbor>().swap(skyline_result);
                std::vector<MultiVectorDEGNeighbor>().swap(remain_points);
                l++;
            }

            num_layer_ = l;
            sort(pool_.begin(), pool_.end());
        }

        /**
         * @brief 更新邻居集合
         * 
         * 在已有邻居的基础上，使用GPS算法更新邻居集合
         */
        void updateNeighbor()
        {
            std::lock_guard<std::mutex> guard(lock_);
            std::vector<MultiVectorDEGNeighbor> skyline_result;
            std::vector<MultiVectorDEGNeighbor> remain_points;
            std::vector<MultiVectorDEGNeighbor> candidate;
            candidate.clear();
            candidate.swap(pool_);
            int l = 0;

            sort(candidate.begin(), candidate.end());

            while (pool_.size() < M_ && candidate.size() > 0)
            {
                findSkyline(candidate, skyline_result, remain_points);
                candidate.swap(remain_points);

                for (auto &point : skyline_result)
                {
                    pool_.emplace_back(point.id_, point.distances_, num_weight_combinations_, point.flag, l);
                }

                outlier_.swap(skyline_result);
                std::vector<MultiVectorDEGNeighbor>().swap(skyline_result);
                std::vector<MultiVectorDEGNeighbor>().swap(remain_points);
                l++;
            }

            num_layer_ = l;
        }

        /**
         * @brief 插入新的邻居点
         * 
         * @param id 节点ID
         * @param distances 距离向量
         */
        void insert(unsigned id, const std::vector<float> &distances)
        {
            std::lock_guard<std::mutex> guard(lock_);

            // 参数验证
            if (distances.size() != num_dimensions_)
            {
                throw std::invalid_argument("Distance vector size must match the number of dimensions");
            }

            // 简单的插入策略：检查是否已存在
            for (const auto &point : pool_)
            {
                if (point.id_ == id)
                {
                    return; // 已存在，不重复插入
                }
            }

            // 检查是否在outlier中
            for (const auto &point : outlier_)
            {
                if (point.id_ == id)
                {
                    return; // 已存在，不重复插入
                }
            }

            // 插入到outlier中，等待下次更新
            outlier_.emplace_back(id, distances, num_weight_combinations_, true, 0);
        }

        /**
         * @brief 获取池中的点
         * @return 池中的点集
         */
        const std::vector<MultiVectorDEGNeighbor> &getPool() const
        {
            return pool_;
        }

        /**
         * @brief 获取层数
         * @return 层数
         */
        unsigned getNumLayer() const
        {
            return num_layer_;
        }

        /**
         * @brief 获取池的大小
         * @return 池的大小
         */
        size_t getPoolSize() const
        {
            return pool_.size();
        }

        /**
         * @brief 清空池
         */
        void clear()
        {
            std::lock_guard<std::mutex> guard(lock_);
            pool_.clear();
            outlier_.clear();
            num_layer_ = 0;
        }

        /**
         * @brief 根据权重计算剪枝状态
         * 
         * 对于给定的权重组合，计算该边是否应该被剪枝
         * 
         * @param neighbor 邻居节点
         * @param weight_idx 权重组合索引
         * @return 是否应该剪枝
         */
        bool calculatePruningStatus(const MultiVectorDEGNeighbor &neighbor, size_t weight_idx) const
        {
            if (weight_idx >= num_weight_combinations_ || weight_idx >= weight_combinations_.size())
            {
                return false;
            }
            
            // 使用bitmap中存储的剪枝状态
            return neighbor.isPruned(weight_idx);
        }

        /**
         * @brief 设置剪枝状态
         * 
         * @param neighbor 邻居节点（会被修改）
         * @param weight_idx 权重组合索引
         * @param pruned 是否剪枝
         */
        void setPruningStatus(MultiVectorDEGNeighbor &neighbor, size_t weight_idx, bool pruned)
        {
            if (weight_idx >= num_weight_combinations_)
            {
                throw std::out_of_range("Weight index out of range");
            }
            
            neighbor.setPruned(weight_idx, pruned);
        }
    };

    /**
     * @brief 多向量DEG Skyline队列
     * 
     * 用于管理多向量DEG Pareto前沿的队列结构
     */
    class MultiVectorDEGSkylineQueue
    {
    private:
        std::vector<MultiVectorDEGNeighbor> pool_;
        unsigned M_;                         // 池的最大容量
        unsigned num_layer_;                  // 层数
        unsigned num_dimensions_;            // 向量维度数
        size_t num_weight_combinations_;     // 权重组合数量

    public:
        /**
         * @brief 默认构造函数
         */
        MultiVectorDEGSkylineQueue() : M_(0), num_layer_(0), num_dimensions_(2), num_weight_combinations_(0)
        {
        }

        /**
         * @brief 构造函数
         * @param m 池的最大容量
         * @param num_dimensions 向量维度数（默认为2）
         */
        MultiVectorDEGSkylineQueue(unsigned m, unsigned num_dimensions = 2)
            : M_(m), num_layer_(0), num_dimensions_(num_dimensions)
        {
            if (num_dimensions < 1)
            {
                throw std::invalid_argument("Number of dimensions must be at least 1");
            }
            if (num_dimensions > MAX_VECTOR_DIM)
            {
                throw std::invalid_argument("Number of dimensions exceeds maximum supported");
            }
            
            auto weight_combinations = generateWeightCombinations(num_dimensions);
            num_weight_combinations_ = weight_combinations.size();
            
            pool_.reserve(M_);
        }

        /**
         * @brief 核心算法：计算Pareto前沿（Skyline）
         * 
         * @param points 输入点集
         * @param skyline 输出的Pareto前沿点集
         * @param remain_points 剩余的非前沿点集
         */
        void findSkyline(std::vector<MultiVectorDEGNeighbor> &points,
                          std::vector<MultiVectorDEGNeighbor> &skyline,
                          std::vector<MultiVectorDEGNeighbor> &remain_points)
        {
            if (points.empty())
            {
                return;
            }

            // 验证所有点的维度一致
            size_t expected_dim = num_dimensions_;
            for (const auto &point : points)
            {
                if (point.distances_.size() != expected_dim)
                {
                    throw std::invalid_argument("All points must have the same number of dimensions");
                }
            }

            // 遍历所有点，找出Pareto前沿
            for (const auto &point : points)
            {
                bool is_skyline = true;
                for (const auto &other : points)
                {
                    if (point.id_ == other.id_) continue;

                    // 检查other是否支配point
                    bool dominates = true;
                    bool strictly_less = false;
                    for (size_t dim = 0; dim < expected_dim; ++dim)
                    {
                        if (other.distances_[dim] > point.distances_[dim])
                        {
                            dominates = false;
                            break;
                        }
                        if (other.distances_[dim] < point.distances_[dim])
                        {
                            strictly_less = true;
                        }
                    }

                    if (dominates && strictly_less)
                    {
                        is_skyline = false;
                        break;
                    }
                }

                if (is_skyline)
                {
                    skyline.push_back(point);
                }
                else
                {
                    remain_points.emplace_back(point);
                }
            }
        }

        /**
         * @brief 初始化队列
         * 
         * 使用GPS算法初始化队列，构建多层Pareto前沿
         * 
         * @param insert_points 要插入的点集
         */
        void initQueue(std::vector<MultiVectorDEGNeighbor> &insert_points)
        {
            std::vector<MultiVectorDEGNeighbor> skyline_result;
            std::vector<MultiVectorDEGNeighbor> remain_points;
            int l = 0;

            while (!insert_points.empty())
            {
                findSkyline(insert_points, skyline_result, remain_points);
                insert_points.swap(remain_points);

                for (auto &point : skyline_result)
                {
                    pool_.emplace_back(point.id_, point.distances_, num_weight_combinations_, true, l);
                }

                std::vector<MultiVectorDEGNeighbor>().swap(skyline_result);
                std::vector<MultiVectorDEGNeighbor>().swap(remain_points);
                l++;
            }

            num_layer_ = l;
        }
    };

} // namespace stkq

#endif // STKQ_MULTI_VECTOR_DEG_H
