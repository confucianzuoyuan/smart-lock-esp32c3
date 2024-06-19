# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
# 需要的cmake的最低版本是3.16
cmake_minimum_required(VERSION 3.16)

# 将esp-idf的源码包含进来，类似于STM32的HAL库源码
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# 项目名称是`hello_world`
# 编译出来的二进制文件叫做: `hello_world.bin`
project(smart-lock)
