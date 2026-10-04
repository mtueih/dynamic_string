<div align="right">

**English** | [简体中文](README.zh-CN.md)

</div>

# dynamic_string

[![C Standard](https://img.shields.io/badge/C-C99+-blue.svg)](https://en.cppreference.com/c)
[![CMake](https://img.shields.io/badge/CMake-3.24+-green.svg)](https://cmake.org/)
[![GitHub License](https://img.shields.io/github/license/mtueih/dynamic_string)](LICENSE)
[![CI](https://github.com/mtueih/dynamic_string/actions/workflows/ci.yml/badge.svg)](https://github.com/mtueih/dynamic_string/actions/workflows/ci.yml)

`dynamic_string` is a pure C implementation of a dynamic string abstract data type (ADT). It provides a rich set of string operations—including creation, concatenation, searching, replacement, and formatting—while always maintaining a valid null-terminated (`\0`) C string underneath, focusing on text processing rather than binary safety.

## Table of Contents

- [API](#api)
- [Use in Other Projects](#use-in-other-projects)
  - [Add Dependencies](#use-in-other-projects-add-dependencies)
  - [Link the Library](#use-in-other-projects-link-library)
  - [Use in Code](#use-in-other-projects-use-in-code)
- [Build from Source](#build-from-source-code)
  - [Requirements](#build-from-source-code-environmental-requirements)
  - [Build Steps](#build-from-source-code-build-steps)
- [License](#license)

<a id="api"></a>

## API [↑](#dynamic_string)

This library provides a rich and complete API, organized into the following groups:

- **Creation and Destruction**: Create, Destroy, Clone.
- **Property Accessors**: Get C-String, Get Length, Get Capacity, Set Capacity.
- **Content Editing**: Copy, Append, Insert, Remove.
- **Relations and Comparison**: Prefix/Suffix Check, Contains Check, Size Comparison.
- **Search, Count and Replace**.
- **Split and Join**.

See the [API Reference](docs/api-reference.md) for the complete reference of every function.

<a id="use-in-other-projects"></a>

## Use in Other Projects [↑](#dynamic_string)

<a id="use-in-other-projects-add-dependencies"></a>

### Add Dependencies [↑](#use-in-other-projects)

#### CPM.cmake [↑](#use-in-other-projects-add-dependencies)

Requirements: [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake).

In your `CMakeLists.txt`:

```cmake
include(${PROJECT_SOURCE_DIR}/cmake/CPM.cmake)

CPMAddPackage("gh:mtueih/dynamic_string#v1.0.1")
```

#### CMake find_package (must be installed) [↑](#use-in-other-projects-add-dependencies)

In your `CMakeLists.txt`:

```cmake
find_package(dynamic_string REQUIRED)
```

<a id="use-in-other-projects-link-library"></a>

### Link the Library [↑](#use-in-other-projects)

In your `CMakeLists.txt`:

```cmake
target_link_libraries(your_target PRIVATE dynamic_string::dynamic_string)
```

<a id="use-in-other-projects-use-in-code"></a>

### Use in Code [↑](#use-in-other-projects)

#### Include Header Files [↑](#use-in-other-projects-use-in-code)

```c
#include <dynamic_string/dynamic_string.h>
```

#### Use the Library Functions [↑](#use-in-other-projects-use-in-code)

```c
#include <dynamic_string/dynamic_string.h>
#include <stdio.h>

int main(void)
{
    /* Create a dynamic string. */
    dstr_adt *s = dstr_create("Hello");
    if (s == NULL)
    {
        return 1;
    }

    /* Append a string. */
    if (dstr_cat_cstr(s, ", World!") != DSTR_SUCCESS)
    {
        dstr_destroy(s);
        return 1;
    }

    /* Append using a format string. */
    if (dstr_cat_format(s, " num=%d", 42) != DSTR_SUCCESS)
    {
        dstr_destroy(s);
        return 1;
    }

    /* Print the result. */
    printf("%s\n", dstr_cstr(s)); /* "Hello, World! num=42". */
    printf("length: %zu\n", dstr_length(s));

    /* Destroy. */
    dstr_destroy(s);
    return 0;
}
```

<a id="build-from-source-code"></a>

## Build from Source [↑](#dynamic_string)

<a id="build-from-source-code-environmental-requirements"></a>

### Requirements [↑](#build-from-source-code)

- [CMake](https://cmake.org/) 3.24+.
- A [C compiler](https://en.cppreference.com/c/compiler_support) supporting [C99](https://en.cppreference.com/c/99)+ (MSVC / MinGW-w64 / Clang).

<a id="build-from-source-code-build-steps"></a>

### Build Steps [↑](#build-from-source-code)

#### Clone the Repository [↑](#build-from-source-code-build-steps)

```bash
git clone https://github.com/mtueih/dynamic_string.git --depth 1 -b v1.0.1
cd dynamic_string
```

#### Configure, Build and Install [↑](#build-from-source-code-build-steps)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DDYNAMIC_STRING_INSTALL=ON
cmake --build build --config Release --parallel
cmake --install build --config Release --strip --prefix install
```

Notes on the commands above:

- The install command. Using `--prefix install` installs the artifacts into the `install` directory instead of installing them globally, so you can use the artifacts in your own way. To install globally, simply remove it.

<a id="license"></a>

## License [↑](#dynamic_string)

This project is licensed under the [ISC License](https://www.isc.org/licenses/) — see the [LICENSE](LICENSE) file for details.
