# CMakeLists.txt for MultiVectorGPS tests
cmake_minimum_required(VERSION 3.10)

# 设置C++标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找Google Test
find_package(GTest REQUIRED)
if(NOT GTest_FOUND)
    message(FATAL_ERROR "Google Test not found. Please install gtest.")
endif()

# 包含头文件目录
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${GTEST_INCLUDE_DIRS})

# 创建测试可执行文件
add_executable(test_multi_vector_gps test_multi_vector_gps.cpp)
add_executable(test_multi_vector_deg test_multi_vector_deg.cpp)

# 链接Google Test库
target_link_libraries(test_multi_vector_gps
    ${GTEST_LIBRARIES}
    pthread
)

target_link_libraries(test_multi_vector_deg
    ${GTEST_LIBRARIES}
    pthread
)

# 启用测试
enable_testing()

# 添加测试用例
add_test(NAME MultiVectorGPSTest COMMAND test_multi_vector_gps)
add_test(NAME MultiVectorDEGTest COMMAND test_multi_vector_deg)

# 打印配置信息
message(STATUS "Building MultiVectorGPS and MultiVectorDEG tests")
message(STATUS "GTest libraries: ${GTEST_LIBRARIES}")
message(STATUS "GTest include dirs: ${GTEST_INCLUDE_DIRS}")
