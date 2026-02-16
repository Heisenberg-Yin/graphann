#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include "../include/multi_vector_gps.h"

using namespace stkq;

// 测试辅助函数：创建测试点集
std::vector<MultiVectorNeighbor> createTestPoints(size_t num_points, size_t num_dimensions)
{
    std::vector<MultiVectorNeighbor> points;
    points.reserve(num_points);

    std::random_device rd;
    std::mt19937 gen(42); // 固定种子以确保可重复性
    std::uniform_real_distribution<float> dis(0.0f, 100.0f);

    for (size_t i = 0; i < num_points; ++i)
    {
        std::vector<float> distances(num_dimensions);
        for (size_t j = 0; j < num_dimensions; ++j)
        {
            distances[j] = dis(gen);
        }
        points.emplace_back(i, distances, true, 0);
    }

    return points;
}

// 测试辅助函数：验证Pareto前沿的正确性
bool verifyParetoFrontier(const std::vector<MultiVectorNeighbor> &skyline,
                           const std::vector<MultiVectorNeighbor> &remain_points,
                           size_t num_dimensions)
{
    // 验证skyline中的每个点都是Pareto最优的
    for (const auto &p1 : skyline)
    {
        for (const auto &p2 : skyline)
        {
            if (p1.id_ == p2.id_) continue;

            // 检查p1是否支配p2（p1在所有维度上都小于等于p2，且至少在一个维度上严格小于）
            bool dominates = true;
            bool strictly_less = false;
            for (size_t dim = 0; dim < num_dimensions; ++dim)
            {
                if (p1.distances_[dim] > p2.distances_[dim])
                {
                    dominates = false;
                    break;
                }
                if (p1.distances_[dim] < p2.distances_[dim])
                {
                    strictly_less = true;
                }
            }

            // 如果p1支配p2，则skyline不正确（Pareto前沿中的点不应该互相支配）
            if (dominates && strictly_less)
            {
                return false;
            }
        }
    }

    // 验证remain_points中的每个点都被skyline中的某个点支配
    for (const auto &remain : remain_points)
    {
        bool is_dominated = false;
        for (const auto &sky : skyline)
        {
            // 检查sky是否支配remain
            bool dominates = true;
            bool strictly_less = false;
            for (size_t dim = 0; dim < num_dimensions; ++dim)
            {
                if (sky.distances_[dim] > remain.distances_[dim])
                {
                    dominates = false;
                    break;
                }
                if (sky.distances_[dim] < remain.distances_[dim])
                {
                    strictly_less = true;
                }
            }

            if (dominates && strictly_less)
            {
                is_dominated = true;
                break;
            }
        }

        // 如果remain点没有被任何skyline点支配，则验证失败
        if (!is_dominated)
        {
            return false;
        }
    }

    return true;
}

// 测试1：默认构造函数和基本功能测试
TEST(MultiVectorGPSTest, DefaultConstructor)
{
    MultiVectorGPS gps;
    EXPECT_EQ(gps.getNumDimensions(), 2); // 默认维度为2
    EXPECT_EQ(gps.getPoolSize(), 0);
    EXPECT_EQ(gps.getNumLayer(), 0);
}

// 测试2：自定义维度构造函数
TEST(MultiVectorGPSTest, CustomDimensionConstructor)
{
    MultiVectorGPS gps(10, 5, 2, 3); // 3个维度
    EXPECT_EQ(gps.getNumDimensions(), 3);
    EXPECT_EQ(gps.getPoolSize(), 0);
}

// 测试3：参数验证测试 - 无效维度
TEST(MultiVectorGPSTest, InvalidDimension)
{
    EXPECT_THROW(MultiVectorGPS(10, 5, 2, 0), std::invalid_argument);
    // 注意：unsigned类型不能接受负数，所以不测试-1
}

// 测试4：设置维度测试
TEST(MultiVectorGPSTest, SetDimension)
{
    MultiVectorGPS gps;
    gps.setNumDimensions(5);
    EXPECT_EQ(gps.getNumDimensions(), 5);

    EXPECT_THROW(gps.setNumDimensions(0), std::invalid_argument);
}

// 测试5：2维向量Pareto前沿计算测试
TEST(MultiVectorGPSTest, FindSkyline2D)
{
    MultiVectorGPS gps(10, 5, 2, 2);

    // 创建测试点集
    std::vector<MultiVectorNeighbor> points = {
        MultiVectorNeighbor(0, {10.0f, 20.0f}, true, 0),
        MultiVectorNeighbor(1, {5.0f, 30.0f}, true, 0),
        MultiVectorNeighbor(2, {15.0f, 10.0f}, true, 0),
        MultiVectorNeighbor(3, {8.0f, 15.0f}, true, 0),
        MultiVectorNeighbor(4, {20.0f, 25.0f}, true, 0)
    };

    std::vector<MultiVectorNeighbor> skyline;
    std::vector<MultiVectorNeighbor> remain_points;

    gps.findSkyline(points, skyline, remain_points);

    // 验证结果
    EXPECT_GT(skyline.size(), 0);
    EXPECT_TRUE(verifyParetoFrontier(skyline, remain_points, 2));

    // 打印结果
    std::cout << "2D Skyline points:" << std::endl;
    for (const auto &p : skyline)
    {
        std::cout << "  ID: " << p.id_ << ", distances: [" 
                  << p.distances_[0] << ", " << p.distances_[1] << "]" << std::endl;
    }
}

// 测试6：3维向量Pareto前沿计算测试
TEST(MultiVectorGPSTest, FindSkyline3D)
{
    MultiVectorGPS gps(10, 5, 2, 3);

    // 创建测试点集
    std::vector<MultiVectorNeighbor> points = {
        MultiVectorNeighbor(0, {10.0f, 20.0f, 30.0f}, true, 0),
        MultiVectorNeighbor(1, {5.0f, 30.0f, 25.0f}, true, 0),
        MultiVectorNeighbor(2, {15.0f, 10.0f, 35.0f}, true, 0),
        MultiVectorNeighbor(3, {8.0f, 15.0f, 20.0f}, true, 0),
        MultiVectorNeighbor(4, {20.0f, 25.0f, 40.0f}, true, 0)
    };

    std::vector<MultiVectorNeighbor> skyline;
    std::vector<MultiVectorNeighbor> remain_points;

    gps.findSkyline(points, skyline, remain_points);

    // 验证结果
    EXPECT_GT(skyline.size(), 0);
    EXPECT_TRUE(verifyParetoFrontier(skyline, remain_points, 3));

    // 打印结果
    std::cout << "3D Skyline points:" << std::endl;
    for (const auto &p : skyline)
    {
        std::cout << "  ID: " << p.id_ << ", distances: [" 
                  << p.distances_[0] << ", " << p.distances_[1] << ", " << p.distances_[2] << "]" << std::endl;
    }
}

// 测试7：5维向量Pareto前沿计算测试
TEST(MultiVectorGPSTest, FindSkyline5D)
{
    MultiVectorGPS gps(10, 5, 2, 5);

    // 创建测试点集
    std::vector<MultiVectorNeighbor> points = {
        MultiVectorNeighbor(0, {10.0f, 20.0f, 30.0f, 40.0f, 50.0f}, true, 0),
        MultiVectorNeighbor(1, {5.0f, 30.0f, 25.0f, 35.0f, 45.0f}, true, 0),
        MultiVectorNeighbor(2, {15.0f, 10.0f, 35.0f, 25.0f, 55.0f}, true, 0),
        MultiVectorNeighbor(3, {8.0f, 15.0f, 20.0f, 30.0f, 40.0f}, true, 0),
        MultiVectorNeighbor(4, {20.0f, 25.0f, 40.0f, 45.0f, 60.0f}, true, 0)
    };

    std::vector<MultiVectorNeighbor> skyline;
    std::vector<MultiVectorNeighbor> remain_points;

    gps.findSkyline(points, skyline, remain_points);

    // 验证结果
    EXPECT_GT(skyline.size(), 0);
    EXPECT_TRUE(verifyParetoFrontier(skyline, remain_points, 5));

    // 打印结果
    std::cout << "5D Skyline points:" << std::endl;
    for (const auto &p : skyline)
    {
        std::cout << "  ID: " << p.id_ << ", distances: [" 
                  << p.distances_[0] << ", " << p.distances_[1] << ", " << p.distances_[2] << ", "
                  << p.distances_[3] << ", " << p.distances_[4] << "]" << std::endl;
    }
}

// 测试8：初始化邻居测试
TEST(MultiVectorGPSTest, InitNeighbor)
{
    MultiVectorGPS gps(10, 5, 2, 2);

    std::vector<MultiVectorNeighbor> points = createTestPoints(20, 2);

    gps.initNeighbor(points);

    EXPECT_GT(gps.getPoolSize(), 0);
    EXPECT_GT(gps.getNumLayer(), 0);

    std::cout << "InitNeighbor - Pool size: " << gps.getPoolSize() 
              << ", Layers: " << gps.getNumLayer() << std::endl;
}

// 测试9：更新邻居测试
TEST(MultiVectorGPSTest, UpdateNeighbor)
{
    MultiVectorGPS gps(10, 5, 2, 2);

    std::vector<MultiVectorNeighbor> points = createTestPoints(20, 2);

    gps.initNeighbor(points);
    gps.updateNeighbor();

    EXPECT_GT(gps.getPoolSize(), 0);

    std::cout << "UpdateNeighbor - Pool size: " << gps.getPoolSize() << std::endl;
}

// 测试10：插入测试
TEST(MultiVectorGPSTest, Insert)
{
    MultiVectorGPS gps(10, 5, 2, 2);

    // 创建测试点集
    std::vector<MultiVectorNeighbor> points = {
        MultiVectorNeighbor(0, {10.0f, 20.0f}, true, 0),
        MultiVectorNeighbor(1, {5.0f, 30.0f}, true, 0),
        MultiVectorNeighbor(2, {15.0f, 10.0f}, true, 0)
    };

    // 使用initNeighbor初始化
    gps.initNeighbor(points);

    EXPECT_GT(gps.getPoolSize(), 0);
    EXPECT_EQ(gps.getPoolSize(), 3); // 应该有3个点
}

// 测试11：插入维度不匹配测试
TEST(MultiVectorGPSTest, InsertDimensionMismatch)
{
    MultiVectorGPS gps(10, 5, 2, 2);

    EXPECT_THROW(gps.insert(0, {10.0f, 20.0f, 30.0f}), std::invalid_argument);
}

// 测试12：清空池测试
TEST(MultiVectorGPSTest, Clear)
{
    MultiVectorGPS gps(10, 5, 2, 2);

    std::vector<MultiVectorNeighbor> points = createTestPoints(20, 2);

    gps.initNeighbor(points);
    EXPECT_GT(gps.getPoolSize(), 0);

    gps.clear();
    EXPECT_EQ(gps.getPoolSize(), 0);
    EXPECT_EQ(gps.getNumLayer(), 0);
}

// 测试13：性能测试 - 2维向量
TEST(MultiVectorGPSTest, Performance2D)
{
    MultiVectorGPS gps(100, 50, 10, 2);

    size_t num_points = 1000;
    std::vector<MultiVectorNeighbor> points = createTestPoints(num_points, 2);

    auto start = std::chrono::high_resolution_clock::now();

    gps.initNeighbor(points);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "2D Performance - Time: " << duration.count() << " ms, "
              << "Points: " << num_points << ", "
              << "Pool size: " << gps.getPoolSize() << ", "
              << "Layers: " << gps.getNumLayer() << std::endl;

    EXPECT_GT(gps.getPoolSize(), 0);
}

// 测试14：性能测试 - 3维向量
TEST(MultiVectorGPSTest, Performance3D)
{
    MultiVectorGPS gps(100, 50, 10, 3);

    size_t num_points = 1000;
    std::vector<MultiVectorNeighbor> points = createTestPoints(num_points, 3);

    auto start = std::chrono::high_resolution_clock::now();

    gps.initNeighbor(points);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "3D Performance - Time: " << duration.count() << " " << "ms, "
              << "Points: " << num_points << ", "
              << "Pool size: " << gps.getPoolSize() << ", "
              << "Layers: " << gps.getNumLayer() << std::endl;

    EXPECT_GT(gps.getPoolSize(), 0);
}

// 测试15：性能测试 - 5维向量
TEST(MultiVectorGPSTest, Performance5D)
{
    MultiVectorGPS gps(100, 50, 10, 5);

    size_t num_points = 1000;
    std::vector<MultiVectorNeighbor> points = createTestPoints(num_points, 5);

    auto start = std::chrono::high_resolution_clock::now();

    gps.initNeighbor(points);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "5D Performance - Time: " << duration.count() << " ms, "
              << "Points: " << num_points << ", "
              << "Pool size: " << gps.getPoolSize() << ", "
              << "Layers: " << gps.getNumLayer() << std::endl;

    EXPECT_GT(gps.getPoolSize(), 0);
}

// 测试16：MultiVectorSkylineQueue测试
TEST(MultiVectorSkylineQueueTest, BasicFunctionality)
{
    MultiVectorSkylineQueue queue(10, 2);

    std::vector<MultiVectorNeighbor> points = {
        MultiVectorNeighbor(0, {10.0f, 20.0f}, true, 0),
        MultiVectorNeighbor(1, {5.0f, 30.0f}, true, 0),
        MultiVectorNeighbor(2, {15.0f, 10.0f}, true, 0)
    };

    queue.initQueue(points);

    EXPECT_GT(queue.getPoolSize(), 0);
    EXPECT_GT(queue.getNumLayer(), 0);

    std::cout << "SkylineQueue - Pool size: " << queue.getPoolSize() 
              << ", Layers: " << queue.getNumLayer() << std::endl;
}

// 测试17：MultiVectorSkylineQueue 3D测试
TEST(MultiVectorSkylineQueueTest, ThreeDimensions)
{
    MultiVectorSkylineQueue queue(10, 3);

    std::vector<MultiVectorNeighbor> points = {
        MultiVectorNeighbor(0, {10.0f, 20.0f, 30.0f}, true, 0),
        MultiVectorNeighbor(1, {5.0f, 30.0f, 25.0f}, true, 0),
        MultiVectorNeighbor(2, {15.0f, 10.0f, 35.0f}, true, 0)
    };

    queue.initQueue(points);

    EXPECT_GT(queue.getPoolSize(), 0);
    EXPECT_GT(queue.getNumLayer(), 0);

    std::cout << "SkylineQueue 3D - Pool size: " << queue.getPoolSize() 
              << ", Layers: " << queue.getNumLayer() << std::endl;
}

// 测试18：空输入测试
TEST(MultiVectorGPSTest, EmptyInput)
{
    MultiVectorGPS gps(10, 5, 2, 2);

    std::vector<MultiVectorNeighbor> points;
    std::vector<MultiVectorNeighbor> skyline;
    std::vector<MultiVectorNeighbor> remain_points;

    gps.findSkyline(points, skyline, remain_points);

    EXPECT_EQ(skyline.size(), 0);
    EXPECT_EQ(remain_points.size(), 0);
}

// 测试19：单点测试
TEST(MultiVectorGPSTest, SinglePoint)
{
    MultiVectorGPS gps(10, 5, 2, 2);

    std::vector<MultiVectorNeighbor> points = {
        MultiVectorNeighbor(0, {10.0f, 20.0f}, true, 0)
    };

    std::vector<MultiVectorNeighbor> skyline;
    std::vector<MultiVectorNeighbor> remain_points;

    gps.findSkyline(points, skyline, remain_points);

    EXPECT_EQ(skyline.size(), 1);
    EXPECT_EQ(remain_points.size(), 0);
    EXPECT_EQ(skyline[0].id_, 0);
}

// 测试20：维度不一致测试
TEST(MultiVectorGPSTest, InconsistentDimensions)
{
    MultiVectorGPS gps(10, 5, 2, 2);

    std::vector<MultiVectorNeighbor> points = {
        MultiVectorNeighbor(0, {10.0f, 20.0f}, true, 0),
        MultiVectorNeighbor(1, {5.0f, 30.0f, 25.0f}, true, 0) // 维度不一致
    };

    std::vector<MultiVectorNeighbor> skyline;
    std::vector<MultiVectorNeighbor> remain_points;

    EXPECT_THROW(gps.findSkyline(points, skyline, remain_points), std::invalid_argument);
}

// 主函数
int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
