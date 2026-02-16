/**
 * @file multi_vector_gps_example.cpp
 * @brief 多向量GPS算法使用示例
 * 
 * 该文件展示了如何使用扩展后的多向量GPS算法，
 * 包括2维、3维和更高维度的Pareto前沿计算。
 */

#include <iostream>
#include <vector>
#include <random>
#include <iomanip>
#include "../include/multi_vector_gps.h"

using namespace stkq;

/**
 * @brief 打印点集
 * @param points 点集
 * @param label 标签
 */
void printPoints(const std::vector<MultiVectorNeighbor> &points, const std::string &label)
{
    std::cout << label << " (" << points.size() << " points):" << std::endl;
    for (const auto &point : points)
    {
        std::cout << "  ID: " << point.id_ << ", distances: [";
        for (size_t i = 0; i < point.distances_.size(); ++i)
        {
            std::cout << std::fixed << std::setprecision(2) << point.distances_[i];
            if (i < point.distances_.size() - 1)
            {
                std::cout << ", ";
            }
        }
        std::cout << "]" << std::endl;
    }
}

/**
 * @brief 示例1：2维向量Pareto前沿计算
 */
void example2D()
{
    std::cout << "\n========== 示例1：2维向量Pareto前沿计算 ==========" << std::endl;

    MultiVectorGPS gps(10, 5, 2, 2); // 2个维度

    // 创建测试点集
    std::vector<MultiVectorNeighbor> points = {
        MultiVectorNeighbor(0, {10.0f, 20.0f}, true, 0),
        MultiVectorNeighbor(1, {5.0f, 30.0f}, true, 0),
        MultiVectorNeighbor(2, {15.0f, 10.0f}, true, 0),
        MultiVectorNeighbor(3, {8.0f, 15.0f}, true, 0),
        MultiVectorNeighbor(4, {20.0f, 25.0f}, true, 0),
        MultiVectorNeighbor(5, {3.0f, 35.0f}, true, 0),
        MultiVectorNeighbor(6, {12.0f, 18.0f}, true, 0)
    };

    printPoints(points, "原始点集");

    // 计算Pareto前沿
    std::vector<MultiVectorNeighbor> skyline;
    std::vector<MultiVectorNeighbor> remain_points;

    gps.findSkyline(points, skyline, remain_points);

    printPoints(skyline, "Pareto前沿");
    printPoints(remain_points, "剩余点集");
}

/**
 * @brief 示例2：3维向量Pareto前沿计算
 */
void example3D()
{
    std::cout << "\n========== 示例2：3维向量Pareto前沿计算 ==========" << std::endl;

    MultiVectorGPS gps(10, 5, 2, 3); // 3个维度

    // 创建测试点集
    std::vector<MultiVectorNeighbor> points = {
        MultiVectorNeighbor(0, {10.0f, 20.0f, 30.0f}, true, 0),
        MultiVectorNeighbor(1, {5.0f, 30.0f, 25.0f}, true, 0),
        MultiVectorNeighbor(2, {15.0f, 10.0f, 35.0f}, true, 0),
        MultiVectorNeighbor(3, {8.0f, 15.0f, 20.0f}, true, 0),
        MultiVectorNeighbor(4, {20.0f, 25.0f, 40.0f}, true, 0),
        MultiVectorNeighbor(5, {3.0f, 35.0f, 22.0f}, true, 0),
        MultiVectorNeighbor(6, {12.0f, 18.0f, 28.0f}, true, 0)
    };

    printPoints(points, "原始点集");

    // 计算Pareto前沿
    std::vector<MultiVectorNeighbor> skyline;
    std::vector<MultiVectorNeighbor> remain_points;

    gps.findSkyline(points, skyline, remain_points);

    printPoints(skyline, "Pareto前沿");
    printPoints(remain_points, "剩余点集");
}

/**
/**
 * @brief 示例3：5维向量Pareto前沿计算
 */
void example5D()
{
    std::cout << "\n========== 示例3：5维向量Pareto前沿计算 ==========" << std::endl;

    MultiVectorGPS gps(10, 5, 2, 5); // 5个维度

    // 创建测试点集
    std::vector<MultiVectorNeighbor> points = {
        MultiVectorNeighbor(0, {10.0f, 20.0f, 30.0f, 40.0f, 50.0f}, true, 0),
        MultiVectorNeighbor(1, {5.0f, 30.0f, 25.0f, 35.0f, 45.0f}, true, 0),
        MultiVectorNeighbor(2, {15.0f, 10.0f, 35.0f, 25.0f, 55.0f}, true, 0),
        MultiVectorNeighbor(3, {8.0f, 15.0f, 20.0f, 30.0f, 40.0f}, true, 0),
        MultiVectorNeighbor(4, {20.0f, 25.0f, 40.0f, 45.0f, 60.0f}, true, 0)
    };

    printPoints(points, "原始点集");

    // 计算Pareto前沿
    std::vector<MultiVectorNeighbor> skyline;
    std::vector<MultiVectorNeighbor> remain_points;

    gps.findSkyline(points, skyline, remain_points);

    printPoints(skyline, "Pareto前沿");
    printPoints(remain_points, "剩余点集");
}

/**
 * @brief 示例4：使用initNeighbor和updateNeighbor
 */
void exampleInitAndUpdate()
{
    std::cout << "\n========== 示例4：使用initNeighbor和updateNeighbor ==========" << std::endl;

    MultiVectorGPS gps(10, 5, 2, 2);

    // 创建随机点集
    std::vector<MultiVectorNeighbor> points;
    std::random_device rd;
    std::mt19937 gen(42);
    std::uniform_real_distribution<float> dis(0.0f, 100.0f);

    for (size_t i = 0; i < 20; ++i)
    {
        std::vector<float> distances = {dis(gen), dis(gen)};
        points.emplace_back(i, distances, true, 0);
    }

    printPoints(points, "原始点集");

    // 初始化邻居
    gps.initNeighbor(points);

    std::cout << "初始化后 - 池大小: " << gps.getPoolSize() 
              << ", 层数: " << gps.getNumLayer() << std::endl;

    printPoints(gps.getPool(), "池中的点");

    // 更新邻居
    gps.updateNeighbor();

    std::cout << "更新后 - 池大小: " << gps.getPoolSize() << std::endl;
}

/**
 * @brief 示例5：使用insert方法
 */
void exampleInsert()
{
    std::cout << "\n========== 示例5：使用insert方法 ==========" << std::endl;

    MultiVectorGPS gps(10, 5, 2, 3);

    // 插入点
    gps.insert(0, {10.0f, 20.0f, 30.0f});
    gps.insert(1, {5.0f, 30.0f, 25.0f});
    gps.insert(2, {15.0f, 10.0f, 35.0f});

    std::cout << "插入3个点后 - 池大小: " << gps.getPoolSize() << std::endl;

    // 尝试插入相同的点（应该被忽略）
    gps.insert(0, {10.0f, 20.0f, 30.0f});

    std::cout << "尝试插入重复点后 - 池大小: " << gps.getPoolSize() << std::endl;
}

/**
 * @brief 示例6：动态设置维度
 */
void exampleDynamicDimension()
{
    std::cout << "\n========== 示例6：动态设置维度 ==========" << std::endl;

    MultiVectorGPS gps; // 默认2维

    std::cout << "默认维度: " << gps.getNumDimensions() << std::endl;

    // 动态设置维度
    gps.setNumDimensions(4);
    std::cout << "设置后维度: " << gps.getNumDimensions() << std::endl;

    // 创建4维点集
    std::vector<MultiVectorNeighbor> points = {
        MultiVectorNeighbor(0, {10.0f, 20.0f, 30.0f, 40.0f}, true, 0),
        MultiVectorNeighbor(1, {5.0f, 30.0f, 25.0f, 35.0f}, true, 0),
        MultiVectorNeighbor(2, {15.0f, 10.0f, 35.0f, 25.0f}, true, 0)
    };

    // 计算Pareto前沿
    std::vector<MultiVectorNeighbor> skyline;
    std::vector<MultiVectorNeighbor> remain_points;

    gps.findSkyline(points, skyline, remain_points);

    printPoints(skyline, "4维Pareto前沿");
}

/**
 * @brief 示例7：使用MultiVectorSkylineQueue
 */
void exampleSkylineQueue()
{
    std::cout << "\n========== 示例7：使用MultiVectorSkylineQueue ==========" << std::endl;

    MultiVectorSkylineQueue queue(10, 2);

    // 创建点集
    std::vector<MultiVectorNeighbor> points = {
        MultiVectorNeighbor(0, {10.0f, 20.0f}, true, 0),
        MultiVectorNeighbor(1, {5.0f, 30.0f}, true, 0),
        MultiVectorNeighbor(2, {15.0f, 10.0f}, true, 0),
        MultiVectorNeighbor(3, {8.0f, 15.0f}, true, 0),
        MultiVectorNeighbor(4, {20.0f, 25.0f}, true, 0)
    };

    printPoints(points, "原始点集");

    // 初始化队列
    queue.initQueue(points);

    std::cout << "队列初始化后 - 池大小: " << queue.getPoolSize() 
              << ", 层数: " << queue.getNumLayer() << std::endl;

    printPoints(queue.getPool(), "队列中的点");
}

/**
 * @brief 示例8：参数验证
 */
void exampleParameterValidation()
{
    std::cout << "\n========== 示例8：参数验证 ==========" << std::endl;

    try
    {
        // 尝试创建维度为0的GPS对象（应该抛出异常）
        MultiVectorGPS gps(10, 5, 2, 0);
        std::cout << "错误：应该抛出异常但没有" << std::endl;
    }
    catch (const std::invalid_argument &e)
    {
        std::cout << "捕获到预期异常: " << e.what() << std::endl;
    }

    try
    {
        // 尝试设置无效的维度（应该抛出异常）
        MultiVectorGPS gps;
        gps.setNumDimensions(0);
        std::cout << "错误：应该抛出异常但没有" << std::endl;
    }
    catch (const std::invalid_argument &e)
    {
        std::cout << "捕获到预期异常: " << e.what() << std::endl;
    }

    try
    {
        // 尝试插入维度不匹配的点（应该抛出异常）
        MultiVectorGPS gps(10, 5, 2, 2);
        gps.insert(0, {10.0f, 20.0f, 30.0f});
        std::cout << "错误：应该抛出异常但没有" << std::endl;
    }
    catch (const std::invalid_argument &e)
    {
        std::cout << "捕获到预期异常: " << e.what() << std::endl;
    }
}

/**
 * @brief 主函数
 */
int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "  多向量GPS算法使用示例" << std::endl;
    std::cout << "========================================" << std::endl;

    // 运行各个示例
    example2D();
    example3D();
    example5D();
    exampleInitAndUpdate();
    exampleInsert();
    exampleDynamicDimension();
    exampleSkylineQueue();
    exampleParameterValidation();

    std::cout << "\n========================================" << std::endl;
    std::cout << "  所有示例运行完成" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
