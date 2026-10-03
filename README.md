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

- [Use in Other Projects](#use-in-other-projects)
  - [Add Dependencies](#use-in-other-projects-add-dependencies)
    - [CPM.cmake](#use-in-other-projects-add-dependencies-cpm-cmake)
    - [CMake find_package (must be installed)](#use-in-other-projects-add-dependencies-cmake-find-package-must-be-installed)
  - [Link the Library](#use-in-other-projects-link-library)
  - [Use in Code](#use-in-other-projects-use-in-code)
    - [Include Header Files](#use-in-other-projects-use-in-code-include-header-files)
    - [Use the Library Functions](#use-in-other-projects-use-in-code-using-library-functions)
- [Build from Source](#build-from-source-code)
  - [Requirements](#build-from-source-code-environmental-requirements)
  - [Build Steps](#build-from-source-code-build-steps)
    - [Clone the Repository](#build-from-source-code-build-steps-clone-repository)
    - [Configure, Build and Install](#build-from-source-code-build-steps-configure-build-and-install)
- [License](#license)

<a id="use-in-other-projects"></a>

## Use in Other Projects [↑](#dynamic_string)

<a id="use-in-other-projects-add-dependencies"></a>

### Add Dependencies [↑](#dynamic_string)

<a id="use-in-other-projects-add-dependencies-cpm-cmake"></a>

#### CPM.cmake [↑](#dynamic_string)

Requirements: [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake).

In your `CMakeLists.txt`:

```cmake
include(${PROJECT_SOURCE_DIR}/cmake/CPM.cmake)

CPMAddPackage("gh:mtueih/dynamic_string#v1.0.1")
```

<a id="use-in-other-projects-add-dependencies-cmake-find-package-must-be-installed"></a>

#### CMake find_package (must be installed) [↑](#dynamic_string)

In your `CMakeLists.txt`:

```cmake
find_package(dynamic_string REQUIRED)
```

<a id="use-in-other-projects-link-library"></a>

### Link the Library [↑](#dynamic_string)

In your `CMakeLists.txt`:

```cmake
target_link_libraries(your_target PRIVATE dynamic_string::dynamic_string)
```

<a id="use-in-other-projects-use-in-code"></a>

### Use in Code [↑](#dynamic_string)

<a id="use-in-other-projects-use-in-code-include-header-files"></a>

#### Include Header Files [↑](#dynamic_string)

```c
#include <dynamic_string/dynamic_string.h>
```

<a id="use-in-other-projects-use-in-code-using-library-functions"></a>

#### Use the Library Functions [↑](#dynamic_string)

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

### Requirements [↑](#dynamic_string)

- [CMake](https://cmake.org/) 3.24+.
- A [C compiler](https://en.cppreference.com/c/compiler_support) supporting [C99](https://en.cppreference.com/c/99)+ (MSVC / MinGW-w64 / Clang).

<a id="build-from-source-code-build-steps"></a>

### Build Steps [↑](#dynamic_string)

<a id="build-from-source-code-build-steps-clone-repository"></a>

#### Clone the Repository [↑](#dynamic_string)

```bash
git clone https://github.com/mtueih/dynamic_string.git --depth 1 -b v1.0.1
cd dynamic_string
```

<a id="build-from-source-code-build-steps-configure-build-and-install"></a>

#### Configure, Build and Install [↑](#dynamic_string)

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