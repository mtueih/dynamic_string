<div align="right">

[English](README.md) | **简体中文**

</div>

# dynamic_string

[![C Standard](https://img.shields.io/badge/C-C99+-blue.svg)](https://zh.cppreference.com/c)
[![CMake](https://img.shields.io/badge/CMake-3.24+-green.svg)](https://cmake.org/)
[![GitHub License](https://img.shields.io/github/license/mtueih/dynamic_string)](LICENSE)
[![CI](https://github.com/mtueih/dynamic_string/actions/workflows/ci.yml/badge.svg)](https://github.com/mtueih/dynamic_string/actions/workflows/ci.yml)

一个动态字符串抽象数据类型 C 库。

## 在其他项目中使用

### 添加依赖

#### CPM.cmake

环境要求：[CPM.cmake](https://github.com/cpm-cmake/CPM.cmake)。

在 `CMakeLists.txt` 中：

```cmake
include(${PROJECT_SOURCE_DIR}/cmake/CPM.cmake)

CPMAddPackage("gh:mtueih/dynamic_string#v1.0.1")
```

#### CMake find_package（需已安装）

在 `CMakeLists.txt` 中：

```cmake
find_package(dynamic_string REQUIRED)
```

### 链接库

在 `CMakeLists.txt` 中：

```cmake
target_link_libraries(your_target PRIVATE dynamic_string::dynamic_string)
```

### 在代码中使用

#### 引入头文件

```c
#include <dynamic_string/dynamic_string.h>
```

#### 使用库函数

```c
#include <dynamic_string/dynamic_string.h>
#include <stdio.h>

int main(void)
{
    /* 创建动态字符串 */
    dstr_adt *s = dstr_create("Hello");
    if (s == NULL)
    {
        return 1;
    }

    /* 追加字符串 */
    if (dstr_cat_cstr(s, ", World!") != DSTR_SUCCESS)
    {
        dstr_destroy(s);
        return 1;
    }

    /* 格式化追加 */
    if (dstr_cat_format(s, " num=%d", 42) != DSTR_SUCCESS)
    {
        dstr_destroy(s);
        return 1;
    }

    /* 输出结果 */
    printf("%s\n", dstr_cstr(s)); /* Hello, World! num=42 */
    printf("length: %zu\n", dstr_length(s));

    /* 销毁 */
    dstr_destroy(s);
    return 0;
}
```

## 从源码构建

### 环境要求

- [CMake](https://cmake.org/) 3.24+。
- 支持 [C99](https://zh.cppreference.com/c/99)+ 的 [C 编译器](https://zh.cppreference.com/c/compiler_support)（MSVC / MinGW-w64 / Clang）。

### 构建步骤

#### 克隆仓库

```bash
git clone https://github.com/mtueih/dynamic_string.git --depth 1 -b v1.0.1
cd dynamic_string
```

#### 配置、构建与安装

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DDYNAMIC_STRING_INSTALL=ON
cmake --build build --config Release --parallel
cmake --install build --config Release --strip --prefix install
```

有关上述命令的说明：

- 安装命令。通过 `--prefix install` 将产物安装在了 `install` 目录下，而不是全局安装，以便你按自己的方式使用安装产物。如果你希望全局安装，则删除它即可。

## 许可协议

本项目采用 [ISC 许可证](https://www.isc.org/licenses/) 授权——详情请参阅 [LICENSE](LICENSE) 文件。
