<div align="right">

[English](README.md) | **简体中文**

</div>

# dynamic_string

[![C Standard](https://img.shields.io/badge/C-C99+-blue.svg)](https://zh.cppreference.com/c)
[![CMake](https://img.shields.io/badge/CMake-3.24+-green.svg)](https://cmake.org/)
[![GitHub License](https://img.shields.io/github/license/mtueih/dynamic_string)](LICENSE)
[![CI](https://github.com/mtueih/dynamic_string/actions/workflows/ci.yml/badge.svg)](https://github.com/mtueih/dynamic_string/actions/workflows/ci.yml)

`dynamic_string` 是一个用纯 C 语言实现的动态字符串抽象数据类型（ADT）库。它提供了创建、拼接、查找、替换、格式化等丰富的字符串操作接口，底层始终维护合法的以空字符（`\0`）结尾的 C 字符串，专注于文本处理而非二进制安全。

## 目录

- [API](#api)
- [在其他项目中使用](#use-in-other-projects)
  - [添加依赖](#use-in-other-projects-add-dependencies)
    - [CPM.cmake](#use-in-other-projects-add-dependencies-cpm-cmake)
    - [CMake find_package（需已安装）](#use-in-other-projects-add-dependencies-cmake-find-package-must-be-installed)
  - [链接库](#use-in-other-projects-link-library)
  - [在代码中使用](#use-in-other-projects-use-in-code)
    - [引入头文件](#use-in-other-projects-use-in-code-include-header-files)
    - [使用库函数](#use-in-other-projects-use-in-code-using-library-functions)
- [从源码构建](#build-from-source-code)
  - [环境要求](#build-from-source-code-environmental-requirements)
  - [构建步骤](#build-from-source-code-build-steps)
    - [克隆仓库](#build-from-source-code-build-steps-clone-repository)
    - [配置、构建与安装](#build-from-source-code-build-steps-configure-build-and-install)
- [许可协议](#license)

<a id="api"></a>

## API [↑](#dynamic_string)

本库提供丰富且完整的 API，按功能分为以下几组（每组仅列举典型函数）：

- **创建与销毁**：`dstr_create()`、`dstr_destroy()`、`dstr_clone()`。

- **属性获取与设置**：`dstr_cstr()`、`dstr_length()`、`dstr_is_empty()`、`dstr_capacity()`。

- **内容编辑**：`dstr_cpy_cstr()`、`dstr_cat_cstr()`、`dstr_cat_format()`、`dstr_insert_cstr()`。

- **关系判断与比较**：`dstr_starts_with_cstr()`、`dstr_ends_with_cstr()`、`dstr_contains_cstr()`、`dstr_equals()`。

- **查找、统计与替换**：`dstr_find()`、`dstr_count()`、`dstr_replace()`。

- **分隔与合并**：`dstr_split()`、`dstr_join()`。

有关各函数的完整说明与完整列表，请参阅 [API 参考](docs/api-reference.zh-CN.md)。

<a id="use-in-other-projects"></a>

## 在其他项目中使用 [↑](#dynamic_string)

<a id="use-in-other-projects-add-dependencies"></a>

### 添加依赖 [↑](#dynamic_string)

<a id="use-in-other-projects-add-dependencies-cpm-cmake"></a>

#### CPM.cmake [↑](#dynamic_string)

环境要求：[CPM.cmake](https://github.com/cpm-cmake/CPM.cmake)。

在 `CMakeLists.txt` 中：

```cmake
include(${PROJECT_SOURCE_DIR}/cmake/CPM.cmake)

CPMAddPackage("gh:mtueih/dynamic_string#v1.0.1")
```

<a id="use-in-other-projects-add-dependencies-cmake-find-package-must-be-installed"></a>

#### CMake find_package（需已安装） [↑](#dynamic_string)

在 `CMakeLists.txt` 中：

```cmake
find_package(dynamic_string REQUIRED)
```

<a id="use-in-other-projects-link-library"></a>

### 链接库 [↑](#dynamic_string)

在 `CMakeLists.txt` 中：

```cmake
target_link_libraries(your_target PRIVATE dynamic_string::dynamic_string)
```

<a id="use-in-other-projects-use-in-code"></a>

### 在代码中使用 [↑](#dynamic_string)

<a id="use-in-other-projects-use-in-code-include-header-files"></a>

#### 引入头文件 [↑](#dynamic_string)

```c
#include <dynamic_string/dynamic_string.h>
```

<a id="use-in-other-projects-use-in-code-using-library-functions"></a>

#### 使用库函数 [↑](#dynamic_string)

```c
#include <dynamic_string/dynamic_string.h>
#include <stdio.h>

int main(void)
{
    /* 创建动态字符串。 */
    dstr_adt *s = dstr_create("Hello");
    if (s == NULL)
    {
        return 1;
    }

    /* 追加字符串。 */
    if (dstr_cat_cstr(s, ", World!") != DSTR_SUCCESS)
    {
        dstr_destroy(s);
        return 1;
    }

    /* 格式化追加。 */
    if (dstr_cat_format(s, " num=%d", 42) != DSTR_SUCCESS)
    {
        dstr_destroy(s);
        return 1;
    }

    /* 输出结果。 */
    printf("%s\n", dstr_cstr(s)); /* "Hello, World! num=42"。 */
    printf("length: %zu\n", dstr_length(s));

    /* 销毁。 */
    dstr_destroy(s);
    return 0;
}
```

<a id="build-from-source-code"></a>

## 从源码构建 [↑](#dynamic_string)

<a id="build-from-source-code-environmental-requirements"></a>

### 环境要求 [↑](#dynamic_string)

- [CMake](https://cmake.org/) 3.24+。
- 支持 [C99](https://zh.cppreference.com/c/99)+ 的 [C 编译器](https://zh.cppreference.com/c/compiler_support)（MSVC / MinGW-w64 / Clang）。

<a id="build-from-source-code-build-steps"></a>

### 构建步骤 [↑](#dynamic_string)

<a id="build-from-source-code-build-steps-clone-repository"></a>

#### 克隆仓库 [↑](#dynamic_string)

```bash
git clone https://github.com/mtueih/dynamic_string.git --depth 1 -b v1.0.1
cd dynamic_string
```

<a id="build-from-source-code-build-steps-configure-build-and-install"></a>

#### 配置、构建与安装 [↑](#dynamic_string)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DDYNAMIC_STRING_INSTALL=ON
cmake --build build --config Release --parallel
cmake --install build --config Release --strip --prefix install
```

有关上述命令的说明：

- 安装命令。通过 `--prefix install` 将产物安装在了 `install` 目录下，而不是全局安装，以便你按自己的方式使用安装产物。如果你希望全局安装，则删除它即可。

<a id="license"></a>

## 许可协议 [↑](#dynamic_string)

本项目采用 [ISC 许可证](https://www.isc.org/licenses/) 授权——详情请参阅 [LICENSE](LICENSE) 文件。
