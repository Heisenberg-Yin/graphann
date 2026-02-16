#ifndef STKQ_MULTI_VECTOR_GPS_H
#define STKQ_MULTI_VECTOR_GPS_H

#include <vector>
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <iostream>
#include <mutex>

namespace stkq
{
    /**
     * @brief 多向量邻居节点，支持任意数量的距离向量
     * 
     * 该结构体用于存储多向量Pareto前沿计算中的邻居节点信息
     */
    struct MultiVectorNeighbor
    {
        unsigned id_;                    // 节点ID
        std::vector<float> distances_;   // 各个维度的距离值
        bool flag;                       // 标记位
        int layer_;                      // 层级信息

        MultiVectorNeighbor() = default;

        /**
         * @brief 构造函数
         * @param id 节点ID
         * @param distances 距离向量
         * @param f 标记位
         * @param layer 层级
         */
        MultiVectorNeighbor(unsigned id, const std::vector<float> &distances, bool f, int layer)
            : id_(id), distances_(distances), flag(f), layer_(layer)
        {
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
         * @brief 小于运算符重载（用于排序）
         * @param other 另一个MultiVectorNeighbor对象
         * @return 如果当前对象小于other，返回true
         */
        bool operator<(const MultiVectorNeighbor &other) const
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
         * @param other 另一个MultiVectorNeighbor对象
         * @return 如果当前对象大于other，返回true
         */
        bool operator>(const MultiVectorNeighbor &other) const
        {
            return other < *this;
        }

        /**
         * @brief 等于运算符重载
         * @param other 另一个MultiVectorNeighbor对象
         * @return 如果当前对象等于other，返回true
         */
        bool operator==(const MultiVectorNeighbor &other) const
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
         * @param other 另一个MultiVectorNeighbor对象
         * @return 如果当前对象不等于other，返回true
         */
        bool operator!=(const MultiVectorNeighbor &other) const
        {
            return !(*this == other);
        }
    };

    /**
     * @brief 多向量GPS算法（Generalized Pareto Skyline）
     * 
     * 该算法支持任意数量的输入向量，用于计算近似Pareto前沿
     * 默认向量数量为2，保持与原算法的兼容性
     */
    class MultiVectorGPS
    {
    private:
        std::mutex lock_;                          // 互斥锁，用于线程安全
        std::vector<MultiVectorNeighbor> pool_;   // 候选池
        std::vector<MultiVectorNeighbor> outlier_; // 被剔除的点
        unsigned M_;                               // 池的最大容量
        unsigned Q_;                               // 质量参数
        unsigned num_layer_;                       // 层数
        unsigned num_dimensions_;                 // 向量维度数（默认为2）
        std::vector<unsigned> nn_old_;             // 旧邻居
        std::vector<unsigned> nn_new_;             // 新邻居
        std::vector<unsigned> rnn_old_;            // 旧反向邻居
        std::vector<unsigned> rnn_new_;            // 新反向邻居

    public:
        /**
         * @brief 默认构造函数，使用2个向量维度
         */
        MultiVectorGPS() : M_(0), Q_(0), num_layer_(0), num_dimensions_(2)
        {
        }

        /**
         * @brief 构造函数
         * @param m 池的最大容量
         * @param s 邻居数量
         * @param q 质量参数
         * @param num_dimensions 向量维度数（默认为2）
         */
        MultiVectorGPS(unsigned m, unsigned s, unsigned q, unsigned num_dimensions = 2)
            : M_(m), Q_(q), num_layer_(0), num_dimensions_(num_dimensions)
        {
            // 参数验证
            if (num_dimensions < 1)
            {
                throw std::invalid_argument("Number of dimensions must be at least 1");
            }
            nn_new_.reserve(s * 2);
            pool_.reserve(4 * m * m);
        }

        /**
         * @brief 拷贝构造函数
         * @param other 另一个MultiVectorGPS对象
         */
        MultiVectorGPS(const MultiVectorGPS &other)
            : M_(other.M_), Q_(other.Q_), num_layer_(other.num_layer_), num_dimensions_(other.num_dimensions_)
        {
            std::copy(other.nn_new_.begin(), other.nn_new_.end(), std::back_inserter(nn_new_));
            nn_new_.reserve(other.nn_new_.capacity());
            pool_.reserve(other.pool_.capacity());
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
            num_dimensions_ = num_dimensions;
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
        void findSkyline(std::vector<MultiVectorNeighbor> &points,
                         std::vector<MultiVectorNeighbor> &skyline,
                         std::vector<MultiVectorNeighbor> &remain_points)
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
        void initNeighbor(std::vector<MultiVectorNeighbor> &insert_points)
        {
            std::lock_guard<std::mutex> guard(lock_);
            std::vector<MultiVectorNeighbor> skyline_result;
            std::vector<MultiVectorNeighbor> remain_points;
            int l = 0;

            while (!insert_points.empty())
            {
                findSkyline(insert_points, skyline_result, remain_points);
                insert_points.swap(remain_points);

                for (auto &point : skyline_result)
                {
                    pool_.emplace_back(point.id_, point.distances_, true, l);
                }

                outlier_.swap(skyline_result);
                std::vector<MultiVectorNeighbor>().swap(skyline_result);
                std::vector<MultiVectorNeighbor>().swap(remain_points);
                l++;
            }

            num_layer_ = l;
            sort(pool_.begin(), pool_.end());
            std::vector<unsigned>().swap(nn_new_);
            std::vector<unsigned>().swap(nn_old_);
            std::vector<unsigned>().swap(rnn_new_);
            std::vector<unsigned>().swap(rnn_old_);
        }

        /**
         * @brief 更新邻居集合
         * 
         * 在已有邻居的基础上，使用GPS算法更新邻居集合
         */
        void updateNeighbor()
        {
            std::lock_guard<std::mutex> guard(lock_);
            std::vector<MultiVectorNeighbor> skyline_result;
            std::vector<MultiVectorNeighbor> remain_points;
            std::vector<MultiVectorNeighbor> candidate;
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
                    pool_.emplace_back(point.id_, point.distances_, point.flag, l);
                }

                outlier_.swap(skyline_result);
                std::vector<MultiVectorNeighbor>().swap(skyline_result);
                std::vector<MultiVectorNeighbor>().swap(remain_points);
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
            outlier_.emplace_back(id, distances, true, 0);
        }

        /**
         * @brief 获取池中的点
         * @return 池中的点集
         */
        const std::vector<MultiVectorNeighbor> &getPool() const
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
    };

    /**
     * @brief 多向量Skyline队列
     * 
     * 用于管理多向量Pareto前沿的队列结构
     */
    class MultiVectorSkylineQueue
    {
    private:
        std::vector<MultiVectorNeighbor> pool_;
        unsigned M_;              // 池的最大容量
        unsigned num_layer_;       // 层数
        unsigned num_dimensions_;  // 向量维度数

    public:
        /**
         * @brief 默认构造函数
         */
        MultiVectorSkylineQueue() : M_(0), num_layer_(0), num_dimensions_(2)
        {
        }

        /**
         * @brief 构造函数
         * @param m 池的最大容量
         * @param num_dimensions 向量维度数（默认为2）
         */
        MultiVectorSkylineQueue(unsigned m, unsigned num_dimensions = 2)
            : M_(m), num_layer_(0), num_dimensions_(num_dimensions)
        {
            if (num_dimensions < 1)
            {
                throw std::invalid_argument("Number of dimensions must be at least 1");
            }
            pool_.reserve(M_);
        }

        /**
         * @brief 拷贝构造函数
         * @param other 另一个MultiVectorSkylineQueue对象
         */
        MultiVectorSkylineQueue(const MultiVectorSkylineQueue &other)
            : M_(other.M_), num_layer_(other.num_layer_), num_dimensions_(other.num_dimensions_)
        {
            pool_.reserve(other.pool_.capacity());
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
            num_dimensions_ = num_dimensions;
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
         * @brief 计算叉积（用于凸包计算）
         * 
         * @param O 原点
         * @param A 点A
         * @param B 点B
         * @return 叉积值
         */
        float cross(const MultiVectorNeighbor &O, const MultiVectorNeighbor &A, const MultiVectorNeighbor &B)
        {
            // 仅支持2D情况下的叉积计算
            if (num_dimensions_ != 2)
            {
                throw std::invalid_argument("Cross product only supported for 2D case");
            }
            return (A.distances_[1] - O.distances_[1]) * (B.distances_[0] - O.distances_[0]) -
                   (A.distances_[0] - O.distances_[0]) * (B.distances_[1] - O.distances_[1]);
        }

        /**
         * @brief 计算凸包（仅2D情况）
         * 
         * @param points 输入点集
         * @param convex_hull 输出的凸包点集
         * @param remain_points 剩余的非凸包点集
         */
        void findConvexHull(std::vector<MultiVectorNeighbor> &points,
                            std::vector<MultiVectorNeighbor> &convex_hull,
                            std::vector<MultiVectorNeighbor> &remain_points)
        {
            if (num_dimensions_ != 2)
            {
                throw std::invalid_argument("Convex hull only supported for 2D case");
            }

            // 构建下凸包
            for (const auto &point : points)
            {
                while (convex_hull.size() >= 2 &&
                       cross(convex_hull[convex_hull.size() - 2], convex_hull.back(), point) <= 0)
                {
                    remain_points.push_back(convex_hull.back());
                    convex_hull.pop_back();
                }
                convex_hull.push_back(point);
            }
        }

        /**
         * @brief 初始化队列
         * 
         * 使用GPS算法初始化队列，构建多层Pareto前沿
         * 
         * @param insert_points 要插入的点集
         */
        void initQueue(std::vector<MultiVectorNeighbor> &insert_points)
        {
            std::vector<MultiVectorNeighbor> skyline_result;
            std::vector<MultiVectorNeighbor> remain_points;
            int l = 0;

            while (!insert_points.empty())
            {
                findSkyline(insert_points, skyline_result, remain_points);
                insert_points.swap(remain_points);

                for (auto &point : skyline_result)
                {
                    pool_.emplace_back(point.id_, point.distances_, true, l);
                }

                std::vector<MultiVectorNeighbor>().swap(skyline_result);
                std::vector<MultiVectorNeighbor>().swap(remain_points);
                l++;
            }

            num_layer_ = l;
        }

        /**
         * @brief 核心算法：计算Pareto前沿（Skyline）
         * 
         * @param points 输入点集
         * @param skyline 输出的Pareto前沿点集
         * @param remain_points 剩余的非前沿点集
         */
        void findSkyline(std::vector<MultiVectorNeighbor> &points,
                          std::vector<MultiVectorNeighbor> &skyline,
                          std::vector<MultiVectorNeighbor> &remain_points)
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
                // 检查当前点是否在Pareto前沿上
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
         * @brief 更新邻居集合
         * 
         * @param nk 更新的位置
         */
        void updateNeighbor(int &nk)
        {
            std::vector<MultiVectorNeighbor> skyline_result;
            std::vector<MultiVectorNeighbor> remain_points;
            std::vector<MultiVectorNeighbor> candidate;
            candidate.swap(pool_);
            int l = 0;
            int k = 0;

            sort(candidate.begin(), candidate.end());
            bool updated = true;

            while (pool_.size() < M_ && candidate.size() > 0)
            {
                findSkyline(candidate, skyline_result, remain_points);
                candidate.swap(remain_points);

                for (auto &point : skyline_result)
                {
                    pool_.emplace_back(point.id_, point.distances_, point.flag, l);
                    if (updated)
                    {
                        if (point.flag)
                        {
                            nk = k;
                            updated = false;
                        }
                        else
                        {
                            k++;
                        }
                    }
                    k++;
                }

                std::vector<MultiVectorNeighbor>().swap(skyline_result);
                std::vector<MultiVectorNeighbor>().swap(remain_points);
                l++;
            }

            num_layer_ = l;
        }

        /**
         * @brief 获取池中的点
         * @return 池中的点集
         */
        const std::vector<MultiVectorNeighbor> &getPool() const
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
            pool_.clear();
            num_layer_ = 0;
        }
    };

} // namespace stkq

#endif // STKQ_MULTI_VECTOR_GPS_H
