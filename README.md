# dynamic_string

[![C Standard](https://img.shields.io/badge/C-C99/C11/C17/C23-blue.svg)](https://cppreference.cn/w/c)
[![CMake](https://img.shields.io/badge/CMake-3.14+-green.svg)](https://cmake.org/)

## 安装

**环境要求**：

- CMake 3.14 或更高版本。

**依赖**：

- [`attrs`](https://github.com/mtueih/attrs);
- [`safe_calc`](https://github.com/mtueih/safe_calc)。

```bash
# 克隆仓库。
git clone https://github.com/mtueih/dynamic_string.git
cd dynamic_string

# 创建构建目录。
mkdir build && cd build

# 配置并安装。
cmake ..
cmake --build .
cmake --install .
```

## 使用

```cmake
find_package(dynamic_string REQUIRED)
target_link_libraries(your_target PRIVATE dynamic_string::dynamic_string)
```
