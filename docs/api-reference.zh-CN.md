# dynamic_string API 参考

## 目录

- [基础概念](#basic-concepts)
  - [类型定义](#basic-concepts-type-definitions)
  - [状态码](#basic-concepts-status-codes)
  - [命名约定](#basic-concepts-naming-conventions)
  - [通用语义与约束](#basic-concepts-general-semantics-and-constraints)
- [API 函数](#api-functions)
  - [创建与销毁](#api-functions-creation-and-destruction)
  - [属性获取与设置](#api-functions-getters-and-setters)
  - [内容编辑](#api-functions-content-editing)
  - [关系判断与比较](#api-functions-relation-and-comparison)
  - [查找、统计与替换](#api-functions-find-count-and-replace)
  - [分隔与合并](#api-functions-split-and-join)

<a id="basic-concepts"></a>

## 基础概念 [↑](#dynamic_string-api-参考)

<a id="basic-concepts-type-definitions"></a>

### 类型定义 [↑](#dynamic_string-api-参考)

#### 动态字符串 [↑](#dynamic_string-api-参考)

本库是一个抽象数据类型库，这意味着所有 API 函数都是对某种对象的某种操作的定义。在此库中，这种对象是**动态字符串**。出于安全和封装等目的，此库使用**不透明类型**来定义它：

```c
typedef struct dynamic_string dstr_adt;
```

`dstr` 是 **d**ynamic **str**ing 的缩写，而 `adt` 表示抽象数据类型。

不透明意味着您不能直接获取和控制其属性。由于编译器也无法知道该类型的大小，所以您只能通过其**不透明指针**（`dstr_adt *`）来使用它。

#### 查找/替换方向 [↑](#dynamic_string-api-参考)

对于**查找与替换**类的操作，需要指示查找/替换的方向（从前往后/从后往前）。因此这类 API 函数需要一个参数，来表达这两种情况。出于对语义清晰性的考量，本库定义枚举类型来表达此信息：

```c
typedef enum 
{
    DSTR_DIR_FORWARD,  /* 从前往后 */
    DSTR_DIR_BACKWARD  /* 从后往前 */
} dstr_direction_t;
```

<a id="basic-concepts-status-codes"></a>

### 状态码 [↑](#dynamic_string-api-参考)

本库由于大量涉及内存操作，因此对于此库所定义的大所数操作而言，其并不总是会成功执行，且其执行失败可能有多种不同的原因。为了让对调用方必要时能够区分是什么原因导致执行失败的，因此定义状态码枚举类型，API 函数通过返回此类型的值，来向调用方传递一个状态，以表达操作是否成功执行，以及失败时，失败的具体原因。

状态码枚举类型定义如下：

```c
typedef enum 
{
    DSTR_SUCCESS = 0,         /* 成功 */
    DSTR_MEMORY_ALLOC_FAILED, /* 内存分配失败 */
    DSTR_INVALID_ARGUMENT,    /* 无效参数 */
} dstr_status_t;
```

<a id="basic-concepts-naming-conventions"></a>

### 命名约定 [↑](#dynamic_string-api-参考)

#### 前缀 [↑](#dynamic_string-api-参考)

本库主要包含对动态字符串这种对象操作的 API 函数，它们都以 `dstr_` 开头。本库也提供了一些不依赖动态字符串这种对象，而是针对 C 字符串的 API 函数，它们则以 `cstr_` 开头。

#### 后缀 [↑](#dynamic_string-api-参考)

本库大多数操作，都针对允许的两种输入字符串类型（动态字符串/ C 字符串）提供了两种不同的版本，输入类型为 C 字符串的版本，相对输入类型为动态字符串的版本，包含 `_cstr` 后缀。

<a id="basic-concepts-general-semantics-and-constraints"></a>

### 通用语义与约束 [↑](#dynamic_string-api-参考)

以下约定适用于全库，除非具体函数另有说明。

1. **文本字符串库，而非二进制安全字符串库**。底层始终是合法 C 字符串，即数据始终以 `'\0'` 结尾，哪怕长度为 0。这意味着 API 函数 `dstr_cstr()` 对于有效输入，其返回值一定不会是空指针，其也一定指向一个以 `'\0'` 结尾的字符串缓冲区。
2. **编码无关性与容量**。本库并不关心字符串数据的编码，因此其长度以字节而不是字符个数为单位，且其长度不包括 `'\0'`。容量表示底层数据缓冲区的真实大小，同样以字节为单位，由于要存储结尾的 `'\0'`，因此容量始终至少比长度大 1（长度为 0 时，也至少是 1，这意味着容量值的下限保证是 1 而不是 0，且由于实现可能会做 SSO 等优化，因此实际的容量下限可能大于 1），但不保证始终等于长度 + 1（实现可能会做几何扩容等优化）。
3. **空指针与空字符串**。对于任何可以接受空字符串，且不是用于表示被修改的对象的参数，通常都可以使用空指针来表示空字符串，这避免了为了表示空字符串而构造不必要的对象的麻烦。
4. **非重叠匹配**。所有查找、统计与替换类的操作均按**非重叠匹配**处理。
5. **禁止内存重叠**。对于任何可能涉及从一个缓冲区拷贝数据到另一个缓冲区的操作，不保证实现会做源缓冲区与目标缓冲区重叠的检查。因此涉及此类情况时，请勿让源缓冲区指针指向目标缓冲区，否则为未定义行为。

<a id="api-functions"></a>

## API 函数 [↑](#dynamic_string-api-参考)

<a id="api-functions-creation-and-destruction"></a>

### 创建与销毁 [↑](#dynamic_string-api-参考)

此组 API 函数，主要用于控制一个「动态字符串」的生命周期。

包含的操作及对应 API 函数与说明如下：

| 操作 | API 函数 |
| --- | --- |
| 创建 | `dstr_create()` |
| 销毁 | `dstr_destroy()` |
| 克隆 | `dstr_clone()` |
| 提取子串 | `dstr_sub_cstr()` / `dstr_sub()` |
| 格式化创建 | `dstr_create_format()` / `dstr_create_vformat()` |

#### `dstr_create()` [↑](#dynamic_string-api-参考)

创建一个「动态字符串」。

```c
dstr_adt *dstr_create(const char *cstr);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `cstr` | `const char *` | 源「C 字符串」；<br>为*空*或*空串*时创建空「动态字符串」。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_adt *` | 创建的「动态字符串」；失败返回 *`NULL`*。 |

> [!CAUTION]
> 返回值指向堆内存，需手动调用 `dstr_destroy()` 释放。

#### `dstr_destroy()` [↑](#dynamic_string-api-参考)

销毁一个「动态字符串」。

```c
void dstr_destroy(dstr_adt *dstr);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `dstr_adt *` | 目标「动态字符串」；为空则直接返回。 |

#### `dstr_clone()` [↑](#dynamic_string-api-参考)

克隆一个「动态字符串」。

```c
dstr_adt *dstr_clone(const dstr_adt *dstr);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `const dstr_adt *` | 源「动态字符串」；为空或空串时创建空「动态字符串」。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_adt *` | 克隆出的「动态字符串」；失败返回 `NULL`。 |

> [!CAUTION]
> 返回值指向堆内存，需手动调用 `dstr_destroy()` 释放。

#### `dstr_sub_cstr()` [↑](#dynamic_string-api-参考)

提取一个「C 字符串」的子串为新的「动态字符串」。

```c
dstr_adt *dstr_sub_cstr(const char *cstr, size_t sub_start, size_t sub_length);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `cstr` | `const char *` | 源「C 字符串」；为空或空串时创建空「动态字符串」，忽略后两项。 |
| `sub_start` | `size_t` | 子串起始索引；越界直接返回 `NULL`。 |
| `sub_length` | `size_t` | 子串长度；`0` 表示到末尾；越界直接返回 `NULL`。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_adt *` | 提取出的「动态字符串」；失败返回 `NULL`。 |

> [!CAUTION]
> 返回值指向堆内存，需手动调用 `dstr_destroy()` 释放。

#### `dstr_sub()` [↑](#dynamic_string-api-参考)

提取一个「动态字符串」的子串为新的「动态字符串」。

```c
dstr_adt *dstr_sub(const dstr_adt *dstr, size_t sub_start, size_t sub_length);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `const dstr_adt *` | 源「动态字符串」；为空或空串时创建空「动态字符串」，忽略后两项。 |
| `sub_start` | `size_t` | 子串起始索引；越界直接返回 `NULL`。 |
| `sub_length` | `size_t` | 子串长度；`0` 表示到末尾；越界直接返回 `NULL`。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_adt *` | 提取出的「动态字符串」；失败返回 `NULL`。 |

> [!CAUTION]
> 返回值指向堆内存，需手动调用 `dstr_destroy()` 释放。

#### `dstr_create_format()` [↑](#dynamic_string-api-参考)

格式化创建一个「动态字符串」。

```c
dstr_adt *dstr_create_format(const char *format, ...);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `format` | `const char *` | 格式「C 字符串」；为空或空串时创建空「动态字符串」。 |
| `...` | — | 与 `format` 对应的可变实参。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_adt *` | 创建的「动态字符串」；失败返回 `NULL`。 |

> [!CAUTION]
> 返回值指向堆内存，需手动调用 `dstr_destroy()` 释放。

#### `dstr_create_vformat()` [↑](#dynamic_string-api-参考)

格式化创建一个「动态字符串」（`va_list` 版本）。

```c
dstr_adt *dstr_create_vformat(const char *format, va_list args);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `format` | `const char *` | 格式「C 字符串」；为空或空串时创建空「动态字符串」。 |
| `args` | `va_list` | 已 `va_start()` 的可变参数列表；本函数不调用 `va_end()`。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_adt *` | 创建的「动态字符串」；失败返回 `NULL`。 |

> [!WARNING]
> `args` 会被本函数读取并消耗，调用后不应再次使用，除非重新 `va_start()` 或使用 `va_copy()`。

> [!CAUTION]
> 返回值指向堆内存，需手动调用 `dstr_destroy()` 释放。

<a id="api-functions-getters-and-setters"></a>

### 属性获取与设置 [↑](#dynamic_string-api-参考)

此组 API 函数，主要用于获取与设置一个「动态字符串」的属性。

包含的操作及对应 API 函数与说明如下：

| 操作 | API 函数 |
| --- | --- |
| 获取内部「C 字符串」指针 | `dstr_cstr()` |
| 获取长度 | `dstr_length()` |
| 判断是否为空 | `dstr_is_empty()` |
| 获取容量 | `dstr_capacity()` |
| 设置容量 | `dstr_set_capacity()` |
| 调整容量到刚够 | `dstr_shrink_to_fit()` |

#### `dstr_cstr()` [↑](#dynamic_string-api-参考)

获取内部「C 字符串」指针。

```c
const char *dstr_cstr(const dstr_adt *dstr);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `const dstr_adt *` | 目标「动态字符串」；为空直接返回 `NULL`。 |

| 返回值 | 说明 |
| --- | --- |
| `const char *` | 内部缓冲区指针；非空 `dstr` 保证非空，长度为 0 时指向 `""`。 |

> [!WARNING]
> 返回指针指向内部缓冲区，调用者不得修改；`dstr` 被修改、扩容、缩容或销毁后该指针失效。

#### `dstr_length()` [↑](#dynamic_string-api-参考)

获取字符串长度（不含结尾 `'\0'`）。

```c
size_t dstr_length(const dstr_adt *dstr);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `const dstr_adt *` | 目标「动态字符串」；为空直接返回 `0`。 |

| 返回值 | 说明 |
| --- | --- |
| `size_t` | 字符串长度。 |

#### `dstr_is_empty()` [↑](#dynamic_string-api-参考)

判断是否为「空动态字符串」。

```c
bool dstr_is_empty(const dstr_adt *dstr);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `const dstr_adt *` | 目标「动态字符串」；为空直接返回 `true`。 |

| 返回值 | 说明 |
| --- | --- |
| `bool` | 为空则 `true`，否则 `false`。 |

#### `dstr_capacity()` [↑](#dynamic_string-api-参考)

获取内部缓冲区容量（含结尾 `'\0'`）。

```c
size_t dstr_capacity(const dstr_adt *dstr);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `const dstr_adt *` | 目标「动态字符串」；为空直接返回 `0`。 |

| 返回值 | 说明 |
| --- | --- |
| `size_t` | 缓冲区字节数。 |

#### `dstr_set_capacity()` [↑](#dynamic_string-api-参考)

设置容量。

```c
dstr_status_t dstr_set_capacity(dstr_adt *dstr, size_t new_capacity);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `dstr_adt *` | 目标「动态字符串」；为空视为无效参数。 |
| `new_capacity` | `size_t` | 请求的缓冲区字节数（含 `'\0'`）；`0` 视为请求最小容量。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_status_t` | 全局状态码。 |

> [!NOTE]
> 实际容量可能大于请求值（受最小容量下限影响）；若 `new_capacity <= dstr_length()` 字符串会被截断。成功后该值成为后续自动扩容/缩容的保底下限，直到再次调用 `dstr_set_capacity()` 覆盖或 `dstr_shrink_to_fit()` 清除。

#### `dstr_shrink_to_fit()` [↑](#dynamic_string-api-参考)

将容量收缩到刚好容纳当前内容。

```c
void dstr_shrink_to_fit(dstr_adt *dstr);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `dstr_adt *` | 目标「动态字符串」；为空直接返回。 |

> [!NOTE]
> 同时取消 `dstr_set_capacity()` 设置的保底容量下限；收缩后至少能容纳 `len + 1` 字节，但可能受实现最小容量下限影响。

<a id="api-functions-content-editing"></a>

### 内容编辑 [↑](#dynamic_string-api-参考)

此组 API 函数，主要用于编辑一个「动态字符串」的内容。

包含的操作及对应 API 函数与说明如下：

| 操作 | API 函数 |
| --- | --- |
| 复制 | `dstr_cpy_cstr()` / `dstr_cpy()` |
| 追加 | `dstr_cat_cstr()` / `dstr_cat()` |
| 插入 | `dstr_insert_cstr()` / `dstr_insert()` |
| 删除子串 | `dstr_clear()`、`dstr_remove()`、`dstr_trim()` |

| 函数 | 说明 |
| --- | --- |
| `dstr_cpy_cstr()` / `dstr_cpy()` | 复制（C 字符串 / 动态字符串）。 |
| `dstr_cpy_sub_cstr()` / `dstr_cpy_sub()` | 复制子串。 |
| `dstr_cpy_format()` / `dstr_cpy_vformat()` | 格式化复制（`...` / `va_list`）。 |
| `dstr_cat_cstr()` / `dstr_cat()` | 追加。 |
| `dstr_cat_sub_cstr()` / `dstr_cat_sub()` | 追加子串。 |
| `dstr_cat_format()` / `dstr_cat_vformat()` | 格式化追加（`...` / `va_list`）。 |
| `dstr_insert_cstr()` / `dstr_insert()` | 插入。 |
| `dstr_insert_sub_cstr()` / `dstr_insert_sub()` | 插入子串。 |
| `dstr_insert_format()` / `dstr_insert_vformat()` | 格式化插入（`...` / `va_list`）。 |
| `dstr_clear()` | 清空。 |
| `dstr_remove()` | 删除子串。 |
| `dstr_trim()` | 删除首尾空白或指定字符。 |

#### `dstr_cpy_cstr()` [↑](#dynamic_string-api-参考)

复制一个「C 字符串」到「动态字符串」。

```c
dstr_status_t dstr_cpy_cstr(dstr_adt *dest, const char *src);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dest` | `dstr_adt *` | 目标「动态字符串」。 |
| `src` | `const char *` | 源「C 字符串」；为空或空串时清空 `dest`。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
> `src` 不得指向 `dest` 的内部缓冲区；若需自复制请先创建临时副本。

#### `dstr_cpy()` [↑](#dynamic_string-api-参考)

复制一个「动态字符串」到另一个「动态字符串」。

```c
dstr_status_t dstr_cpy(dstr_adt *dest, const dstr_adt *src);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dest` | `dstr_adt *` | 目标「动态字符串」。 |
| `src` | `const dstr_adt *` | 源「动态字符串」；为空或空串时清空 `dest`。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
> `src` 不得指向 `dest` 的内部缓冲区；若需自复制请先创建临时副本。

#### `dstr_cpy_sub_cstr()` [↑](#dynamic_string-api-参考)

复制一个「C 字符串」的子串到「动态字符串」。

```c
dstr_status_t dstr_cpy_sub_cstr(dstr_adt *dest, const char *src, size_t sub_start, size_t sub_length);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dest` | `dstr_adt *` | 目标「动态字符串」。 |
| `src` | `const char *` | 源「C 字符串」；为空或空串时清空 `dest`，忽略后两项。 |
| `sub_start` | `size_t` | 子串起始索引；越界视为无效参数。 |
| `sub_length` | `size_t` | 子串长度；`0` 表示到末尾；越界视为无效参数。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
> `src` 不得指向 `dest` 的内部缓冲区；若需自复制请先创建临时副本。

#### `dstr_cpy_sub()` [↑](#dynamic_string-api-参考)

复制一个「动态字符串」的子串到另一个「动态字符串」。

```c
dstr_status_t dstr_cpy_sub(dstr_adt *dest, const dstr_adt *src, size_t sub_start, size_t sub_length);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dest` | `dstr_adt *` | 目标「动态字符串」。 |
| `src` | `const dstr_adt *` | 源「动态字符串」；为空或空串时清空 `dest`，忽略后两项。 |
| `sub_start` | `size_t` | 子串起始索引；越界视为无效参数。 |
| `sub_length` | `size_t` | 子串长度；`0` 表示到末尾；越界视为无效参数。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
> `src` 不得指向 `dest` 的内部缓冲区；若需自复制请先创建临时副本。

#### `dstr_cpy_format()` [↑](#dynamic_string-api-参考)

格式化复制一个字符串到「动态字符串」。

```c
dstr_status_t dstr_cpy_format(dstr_adt *dest, const char *format, ...);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dest` | `dstr_adt *` | 目标「动态字符串」。 |
| `format` | `const char *` | 格式「C 字符串」；为空或空串时清空 `dest`。 |
| `...` | — | 与 `format` 对应的可变实参。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
> `format` 不得指向 `dest` 的内部缓冲区；若需自复制请先创建临时副本。

#### `dstr_cpy_vformat()` [↑](#dynamic_string-api-参考)

格式化复制一个字符串到「动态字符串」（`va_list` 版本）。

```c
dstr_status_t dstr_cpy_vformat(dstr_adt *dest, const char *format, va_list args);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dest` | `dstr_adt *` | 目标「动态字符串」。 |
| `format` | `const char *` | 格式「C 字符串」；为空或空串时清空 `dest`。 |
| `args` | `va_list` | 已 `va_start()` 的可变参数列表；本函数不调用 `va_end()`。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
> `format` 不得指向 `dest` 的内部缓冲区；`args` 会被读取并消耗，调用后不应再次使用（除非重新 `va_start()` 或 `va_copy()`）。

#### `dstr_cat_cstr()` [↑](#dynamic_string-api-参考)

追加一个「C 字符串」到「动态字符串」。

```c
dstr_status_t dstr_cat_cstr(dstr_adt *dest, const char *src);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dest` | `dstr_adt *` | 目标「动态字符串」。 |
| `src` | `const char *` | 源「C 字符串」；为空或空串时追加空字符串。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
> `src` 不得指向 `dest` 的内部缓冲区；若需自追加请先创建临时副本。

#### `dstr_cat()` [↑](#dynamic_string-api-参考)

追加一个「动态字符串」到另一个「动态字符串」。

```c
dstr_status_t dstr_cat(dstr_adt *dest, const dstr_adt *src);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dest` | `dstr_adt *` | 目标「动态字符串」。 |
| `src` | `const dstr_adt *` | 源「动态字符串」；为空或空串时追加空字符串。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
> `src` 不得指向 `dest` 的内部缓冲区；若需自追加请先创建临时副本。

#### `dstr_cat_sub_cstr()` [↑](#dynamic_string-api-参考)

追加一个「C 字符串」的子串到「动态字符串」。

```c
dstr_status_t dstr_cat_sub_cstr(dstr_adt *dest, const char *src, size_t sub_start, size_t sub_length);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dest` | `dstr_adt *` | 目标「动态字符串」。 |
| `src` | `const char *` | 源「C 字符串」；为空或空串时追加空串，忽略后两项。 |
| `sub_start` | `size_t` | 子串起始索引；越界视为无效参数。 |
| `sub_length` | `size_t` | 子串长度；`0` 表示到末尾；越界视为无效参数。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
> `src` 不得指向 `dest` 的内部缓冲区；若需自追加请先创建临时副本。

#### `dstr_cat_sub()` [↑](#dynamic_string-api-参考)

追加一个「动态字符串」的子串到另一个「动态字符串」。

```c
dstr_status_t dstr_cat_sub(dstr_adt *dest, const dstr_adt *src, size_t sub_start, size_t sub_length);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dest` | `dstr_adt *` | 目标「动态字符串」。 |
| `src` | `const dstr_adt *` | 源「动态字符串」；为空或空串时追加空串，忽略后两项。 |
| `sub_start` | `size_t` | 子串起始索引；越界视为无效参数。 |
| `sub_length` | `size_t` | 子串长度；`0` 表示到末尾；越界视为无效参数。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
> `src` 不得指向 `dest` 的内部缓冲区；若需自追加请先创建临时副本。

#### `dstr_cat_format()` [↑](#dynamic_string-api-参考)

格式化追加一个字符串到「动态字符串」。

```c
dstr_status_t dstr_cat_format(dstr_adt *dest, const char *format, ...);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dest` | `dstr_adt *` | 目标「动态字符串」。 |
| `format` | `const char *` | 格式「C 字符串」；为空或空串时追加空字符串。 |
| `...` | — | 与 `format` 对应的可变实参。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
> `format` 不得指向 `dest` 的内部缓冲区；若需自追加请先创建临时副本。

#### `dstr_cat_vformat()` [↑](#dynamic_string-api-参考)

格式化追加一个字符串到「动态字符串」（`va_list` 版本）。

```c
dstr_status_t dstr_cat_vformat(dstr_adt *dest, const char *format, va_list args);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dest` | `dstr_adt *` | 目标「动态字符串」。 |
| `format` | `const char *` | 格式「C 字符串」；为空或空串时追加空字符串。 |
| `args` | `va_list` | 已 `va_start()` 的可变参数列表；本函数不调用 `va_end()`。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
> `format` 不得指向 `dest` 的内部缓冲区；`args` 会被读取并消耗，调用后不应再次使用（除非重新 `va_start()` 或 `va_copy()`）。

#### `dstr_insert_cstr()` [↑](#dynamic_string-api-参考)

插入一个「C 字符串」到「动态字符串」。

```c
dstr_status_t dstr_insert_cstr(dstr_adt *dest, size_t index, const char *src);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dest` | `dstr_adt *` | 目标「动态字符串」。 |
| `index` | `size_t` | 插入位置索引；越界视为无效参数。 |
| `src` | `const char *` | 源「C 字符串」；为空或空串时插入空字符串。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
> `src` 不得指向 `dest` 的内部缓冲区；若需自插入请先创建临时副本。

#### `dstr_insert()` [↑](#dynamic_string-api-参考)

插入一个「动态字符串」到另一个「动态字符串」。

```c
dstr_status_t dstr_insert(dstr_adt *dest, size_t index, const dstr_adt *src);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dest` | `dstr_adt *` | 目标「动态字符串」。 |
| `index` | `size_t` | 插入位置索引；越界视为无效参数。 |
| `src` | `const dstr_adt *` | 源「动态字符串」；为空或空串时插入空字符串。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
> `src` 不得指向 `dest` 的内部缓冲区；若需自插入请先创建临时副本。

#### `dstr_insert_sub_cstr()` [↑](#dynamic_string-api-参考)

插入一个「C 字符串」的子串到「动态字符串」。

```c
dstr_status_t dstr_insert_sub_cstr(dstr_adt *dest, size_t index, const char *src, size_t sub_start, size_t sub_length);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dest` | `dstr_adt *` | 目标「动态字符串」。 |
| `index` | `size_t` | 插入位置索引；越界视为无效参数。 |
| `src` | `const char *` | 源「C 字符串」；为空或空串时插入空串，忽略后两项。 |
| `sub_start` | `size_t` | 子串起始索引；越界视为无效参数。 |
| `sub_length` | `size_t` | 子串长度；`0` 表示到末尾；越界视为无效参数。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
> `src` 不得指向 `dest` 的内部缓冲区；若需自插入请先创建临时副本。

#### `dstr_insert_sub()` [↑](#dynamic_string-api-参考)

插入一个「动态字符串」的子串到另一个「动态字符串」。

```c
dstr_status_t dstr_insert_sub(dstr_adt *dest, size_t index, const dstr_adt *src, size_t sub_start, size_t sub_length);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dest` | `dstr_adt *` | 目标「动态字符串」。 |
| `index` | `size_t` | 插入位置索引；越界视为无效参数。 |
| `src` | `const dstr_adt *` | 源「动态字符串」；为空或空串时插入空串，忽略后两项。 |
| `sub_start` | `size_t` | 子串起始索引；越界视为无效参数。 |
| `sub_length` | `size_t` | 子串长度；`0` 表示到末尾；越界视为无效参数。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
> `src` 不得指向 `dest` 的内部缓冲区；若需自插入请先创建临时副本。

#### `dstr_insert_format()` [↑](#dynamic_string-api-参考)

格式化插入一个字符串到「动态字符串」。

```c
dstr_status_t dstr_insert_format(dstr_adt *dest, size_t index, const char *format, ...);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dest` | `dstr_adt *` | 目标「动态字符串」。 |
| `index` | `size_t` | 插入位置索引；越界视为无效参数。 |
| `format` | `const char *` | 格式「C 字符串」；为空或空串时插入空字符串。 |
| `...` | — | 与 `format` 对应的可变实参。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
> `format` 不得指向 `dest` 的内部缓冲区；若需自插入请先创建临时副本。

#### `dstr_insert_vformat()` [↑](#dynamic_string-api-参考)

格式化插入一个字符串到「动态字符串」（`va_list` 版本）。

```c
dstr_status_t dstr_insert_vformat(dstr_adt *dest, size_t index, const char *format, va_list args);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dest` | `dstr_adt *` | 目标「动态字符串」。 |
| `index` | `size_t` | 插入位置索引；越界视为无效参数。 |
| `format` | `const char *` | 格式「C 字符串」；为空或空串时插入空字符串。 |
| `args` | `va_list` | 已 `va_start()` 的可变参数列表；本函数不调用 `va_end()`。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
> `format` 不得指向 `dest` 的内部缓冲区；`args` 会被读取并消耗，调用后不应再次使用（除非重新 `va_start()` 或 `va_copy()`）。

#### `dstr_clear()` [↑](#dynamic_string-api-参考)

清空一个「动态字符串」。

```c
void dstr_clear(dstr_adt *dstr);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `dstr_adt *` | 目标「动态字符串」；为空直接返回。 |

> [!NOTE]
> 使长度为 0，但不立即释放容量。

#### `dstr_remove()` [↑](#dynamic_string-api-参考)

删除一个「动态字符串」的子串。

```c
void dstr_remove(dstr_adt *dstr, size_t sub_start, size_t sub_length);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `dstr_adt *` | 目标「动态字符串」；为空或空串直接返回。 |
| `sub_start` | `size_t` | 子串起始索引；越界直接返回。 |
| `sub_length` | `size_t` | 子串长度；`0` 表示到末尾；越界直接返回。 |

#### `dstr_trim()` [↑](#dynamic_string-api-参考)

删除首尾的空白字符或指定字符。

```c
void dstr_trim(dstr_adt *dstr, const char *trim_chars);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `dstr_adt *` | 目标「动态字符串」；为空或空串直接返回。 |
| `trim_chars` | `const char *` | 要删除的字符集；为空或空串时删除空白字符。 |

> [!NOTE]
> 空白字符判定使用 C 标准库 `isspace()`，结果取决于调用时的 locale；若需不受 locale 影响，请显式指定 `trim_chars`。

<a id="api-functions-relation-and-comparison"></a>

### 关系判断与比较 [↑](#dynamic_string-api-参考)

判断类函数返回 `bool`，比较类函数返回 `int`；各参数的空指针处理见参数说明。

| 函数 | 说明 |
| --- | --- |
| `dstr_starts_with_cstr()` / `dstr_starts_with()` / `cstr_starts_with()` | 前缀判断。 |
| `dstr_ends_with_cstr()` / `dstr_ends_with()` / `cstr_ends_with()` | 后缀判断。 |
| `dstr_contains_cstr()` / `dstr_contains()` / `cstr_contains()` | 包含判断。 |
| `dstr_equals_cstr()` / `dstr_equals()` / `cstr_equals()` | 相等判断。 |
| `dstr_compare_cstr()` / `dstr_compare()` / `cstr_compare()` | 大小比较。 |

#### `dstr_starts_with_cstr()` [↑](#dynamic_string-api-参考)

判断「动态字符串」是否以指定「C 字符串」前缀开头。

```c
bool dstr_starts_with_cstr(const dstr_adt *dstr, const char *prefix);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `const dstr_adt *` | 目标「动态字符串」；为空或空串返回 `false`。 |
| `prefix` | `const char *` | 前缀「C 字符串」；为空或空串返回 `false`。 |

| 返回值 | 说明 |
| --- | --- |
| `bool` | 以前缀开头则 `true`，否则 `false`。 |

#### `dstr_starts_with()` [↑](#dynamic_string-api-参考)

判断「动态字符串」是否以指定「动态字符串」前缀开头。

```c
bool dstr_starts_with(const dstr_adt *dstr, const dstr_adt *prefix);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `const dstr_adt *` | 目标「动态字符串」；为空或空串返回 `false`。 |
| `prefix` | `const dstr_adt *` | 前缀「动态字符串」；为空或空串返回 `false`。 |

| 返回值 | 说明 |
| --- | --- |
| `bool` | 以前缀开头则 `true`，否则 `false`。 |

#### `cstr_starts_with()` [↑](#dynamic_string-api-参考)

判断「C 字符串」是否以指定「C 字符串」前缀开头。

```c
bool cstr_starts_with(const char *cstr, const char *prefix);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `cstr` | `const char *` | 目标「C 字符串」；为空或空串返回 `false`。 |
| `prefix` | `const char *` | 前缀「C 字符串」；为空或空串返回 `false`。 |

| 返回值 | 说明 |
| --- | --- |
| `bool` | 以前缀开头则 `true`，否则 `false`。 |

#### `dstr_ends_with_cstr()` [↑](#dynamic_string-api-参考)

判断「动态字符串」是否以指定「C 字符串」后缀结尾。

```c
bool dstr_ends_with_cstr(const dstr_adt *dstr, const char *suffix);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `const dstr_adt *` | 目标「动态字符串」；为空或空串返回 `false`。 |
| `suffix` | `const char *` | 后缀「C 字符串」；为空或空串返回 `false`。 |

| 返回值 | 说明 |
| --- | --- |
| `bool` | 以后缀结尾则 `true`，否则 `false`。 |

#### `dstr_ends_with()` [↑](#dynamic_string-api-参考)

判断「动态字符串」是否以指定「动态字符串」后缀结尾。

```c
bool dstr_ends_with(const dstr_adt *dstr, const dstr_adt *suffix);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `const dstr_adt *` | 目标「动态字符串」；为空或空串返回 `false`。 |
| `suffix` | `const dstr_adt *` | 后缀「动态字符串」；为空或空串返回 `false`。 |

| 返回值 | 说明 |
| --- | --- |
| `bool` | 以后缀结尾则 `true`，否则 `false`。 |

#### `cstr_ends_with()` [↑](#dynamic_string-api-参考)

判断「C 字符串」是否以指定「C 字符串」后缀结尾。

```c
bool cstr_ends_with(const char *cstr, const char *suffix);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `cstr` | `const char *` | 目标「C 字符串」；为空或空串返回 `false`。 |
| `suffix` | `const char *` | 后缀「C 字符串」；为空或空串返回 `false`。 |

| 返回值 | 说明 |
| --- | --- |
| `bool` | 以后缀结尾则 `true`，否则 `false`。 |

#### `dstr_contains_cstr()` [↑](#dynamic_string-api-参考)

判断「动态字符串」是否包含指定子「C 字符串」。

```c
bool dstr_contains_cstr(const dstr_adt *dstr, const char *sub);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `const dstr_adt *` | 目标「动态字符串」。 |
| `sub` | `const char *` | 子「C 字符串」。 |

| 返回值 | 说明 |
| --- | --- |
| `bool` | 包含则 `true`；`sub` 为空串时恒为 `true`。 |

> [!NOTE]
> 与 `find` 系列不同：空子串按 C 标准视为任意字符串的子串，因此返回 `true`。

#### `dstr_contains()` [↑](#dynamic_string-api-参考)

判断「动态字符串」是否包含指定子「动态字符串」。

```c
bool dstr_contains(const dstr_adt *dstr, const dstr_adt *sub);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `const dstr_adt *` | 目标「动态字符串」。 |
| `sub` | `const dstr_adt *` | 子「动态字符串」。 |

| 返回值 | 说明 |
| --- | --- |
| `bool` | 包含则 `true`；`sub` 为空串时恒为 `true`。 |

> [!NOTE]
> 与 `find` 系列不同：空子串按 C 标准视为任意字符串的子串，因此返回 `true`。

#### `cstr_contains()` [↑](#dynamic_string-api-参考)

判断「C 字符串」是否包含指定子「C 字符串」。

```c
bool cstr_contains(const char *cstr, const char *sub);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `cstr` | `const char *` | 目标「C 字符串」。 |
| `sub` | `const char *` | 子「C 字符串」。 |

| 返回值 | 说明 |
| --- | --- |
| `bool` | 包含则 `true`；`sub` 为空串时恒为 `true`。 |

> [!NOTE]
> 与 `find` 系列不同：空子串按 C 标准视为任意字符串的子串，因此返回 `true`。

#### `dstr_equals_cstr()` [↑](#dynamic_string-api-参考)

判断「动态字符串」是否与「C 字符串」相等。

```c
bool dstr_equals_cstr(const dstr_adt *lhs, const char *rhs);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `lhs` | `const dstr_adt *` | 左操作数；为空视为空串。 |
| `rhs` | `const char *` | 右操作数；为空视为空串。 |

| 返回值 | 说明 |
| --- | --- |
| `bool` | 相等则 `true`；两者同为空串返回 `true`。 |

#### `dstr_equals()` [↑](#dynamic_string-api-参考)

判断两个「动态字符串」是否相等。

```c
bool dstr_equals(const dstr_adt *lhs, const dstr_adt *rhs);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `lhs` | `const dstr_adt *` | 左操作数；为空视为空串。 |
| `rhs` | `const dstr_adt *` | 右操作数；为空视为空串。 |

| 返回值 | 说明 |
| --- | --- |
| `bool` | 相等则 `true`；两者同为空串返回 `true`。 |

#### `cstr_equals()` [↑](#dynamic_string-api-参考)

判断两个「C 字符串」是否相等。

```c
bool cstr_equals(const char *lhs, const char *rhs);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `lhs` | `const char *` | 左操作数；为空视为空串。 |
| `rhs` | `const char *` | 右操作数；为空视为空串。 |

| 返回值 | 说明 |
| --- | --- |
| `bool` | 相等则 `true`；两者同为空串返回 `true`。 |

#### `dstr_compare_cstr()` [↑](#dynamic_string-api-参考)

比较「动态字符串」与「C 字符串」。

```c
int dstr_compare_cstr(const dstr_adt *lhs, const char *rhs);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `lhs` | `const dstr_adt *` | 左操作数；为空视为空串。 |
| `rhs` | `const char *` | 右操作数；为空视为空串。 |

| 返回值 | 说明 |
| --- | --- |
| `int` | 相等返回 `0`；`lhs` 大返回正值，小返回负值；同为空串返回 `0`。 |

#### `dstr_compare()` [↑](#dynamic_string-api-参考)

比较两个「动态字符串」。

```c
int dstr_compare(const dstr_adt *lhs, const dstr_adt *rhs);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `lhs` | `const dstr_adt *` | 左操作数；为空视为空串。 |
| `rhs` | `const dstr_adt *` | 右操作数；为空视为空串。 |

| 返回值 | 说明 |
| --- | --- |
| `int` | 相等返回 `0`；`lhs` 大返回正值，小返回负值；同为空串返回 `0`。 |

#### `cstr_compare()` [↑](#dynamic_string-api-参考)

比较两个「C 字符串」。

```c
int cstr_compare(const char *lhs, const char *rhs);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `lhs` | `const char *` | 左操作数；为空视为空串。 |
| `rhs` | `const char *` | 右操作数；为空视为空串。 |

| 返回值 | 说明 |
| --- | --- |
| `int` | 相等返回 `0`；`lhs` 大返回正值，小返回负值；同为空串返回 `0`。 |

<a id="api-functions-find-count-and-replace"></a>

### 查找、统计与替换 [↑](#dynamic_string-api-参考)

所有查找、统计、替换均按「非重叠匹配」处理；方向由 `dstr_direction_t` 指定。

| 函数 | 说明 |
| --- | --- |
| `dstr_find_cstr()` / `dstr_find()` / `cstr_find()` | 首次出现位置。 |
| `dstr_find_nth_cstr()` / `dstr_find_nth()` / `cstr_find_nth()` | 第 n 次出现位置。 |
| `dstr_find_indexes_cstr()` / `dstr_find_indexes()` / `cstr_find_indexes()` | 前 n 次全部位置。 |
| `dstr_count_cstr()` / `dstr_count()` / `cstr_count()` | 出现次数。 |
| `dstr_replace_cstr()` / `dstr_replace()` | 替换 n 次。 |
| `dstr_replace_nth_cstr()` / `dstr_replace_nth()` | 替换第 n 次。 |

#### `dstr_find_cstr()` [↑](#dynamic_string-api-参考)

查找「动态字符串」中子「C 字符串」首次出现的位置。

```c
bool dstr_find_cstr(const dstr_adt *dstr, const char *sub, size_t *out_index, dstr_direction_t direction);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `const dstr_adt *` | 目标「动态字符串」；为空或空串返回 `false`。 |
| `sub` | `const char *` | 子「C 字符串」；为空或空串返回 `false`。 |
| `out_index` | `size_t *` | 接收位置索引；为空则不写入。 |
| `direction` | `dstr_direction_t` | 查找方向。 |

| 返回值 | 说明 |
| --- | --- |
| `bool` | 找到则 `true`，否则 `false`。 |

#### `dstr_find()` [↑](#dynamic_string-api-参考)

查找「动态字符串」中子「动态字符串」首次出现的位置。

```c
bool dstr_find(const dstr_adt *dstr, const dstr_adt *sub, size_t *out_index, dstr_direction_t direction);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `const dstr_adt *` | 目标「动态字符串」；为空或空串返回 `false`。 |
| `sub` | `const dstr_adt *` | 子「动态字符串」；为空或空串返回 `false`。 |
| `out_index` | `size_t *` | 接收位置索引；为空则不写入。 |
| `direction` | `dstr_direction_t` | 查找方向。 |

| 返回值 | 说明 |
| --- | --- |
| `bool` | 找到则 `true`，否则 `false`。 |

#### `cstr_find()` [↑](#dynamic_string-api-参考)

查找「C 字符串」中子「C 字符串」首次出现的位置。

```c
bool cstr_find(const char *cstr, const char *sub, size_t *out_index, dstr_direction_t direction);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `cstr` | `const char *` | 目标「C 字符串」；为空或空串返回 `false`。 |
| `sub` | `const char *` | 子「C 字符串」；为空或空串返回 `false`。 |
| `out_index` | `size_t *` | 接收位置索引；为空则不写入。 |
| `direction` | `dstr_direction_t` | 查找方向。 |

| 返回值 | 说明 |
| --- | --- |
| `bool` | 找到则 `true`，否则 `false`。 |

#### `dstr_find_nth_cstr()` [↑](#dynamic_string-api-参考)

查找「动态字符串」中子「C 字符串」第 n 次出现的位置。

```c
bool dstr_find_nth_cstr(const dstr_adt *dstr, const char *sub, size_t *out_index, dstr_direction_t direction, size_t n);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `const dstr_adt *` | 目标「动态字符串」；为空或空串返回 `false`。 |
| `sub` | `const char *` | 子「C 字符串」；为空或空串返回 `false`。 |
| `out_index` | `size_t *` | 接收位置索引；为空则不写入。 |
| `direction` | `dstr_direction_t` | 查找方向。 |
| `n` | `size_t` | 次序（从 1 起）；`0` 表示该方向最后一次；超过实际次数按最后一次。 |

| 返回值 | 说明 |
| --- | --- |
| `bool` | 找到则 `true`，否则 `false`。 |

#### `dstr_find_nth()` [↑](#dynamic_string-api-参考)

查找「动态字符串」中子「动态字符串」第 n 次出现的位置。

```c
bool dstr_find_nth(const dstr_adt *dstr, const dstr_adt *sub, size_t *out_index, dstr_direction_t direction, size_t n);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `const dstr_adt *` | 目标「动态字符串」；为空或空串返回 `false`。 |
| `sub` | `const dstr_adt *` | 子「动态字符串」；为空或空串返回 `false`。 |
| `out_index` | `size_t *` | 接收位置索引；为空则不写入。 |
| `direction` | `dstr_direction_t` | 查找方向。 |
| `n` | `size_t` | 次序（从 1 起）；`0` 表示该方向最后一次；超过实际次数按最后一次。 |

| 返回值 | 说明 |
| --- | --- |
| `bool` | 找到则 `true`，否则 `false`。 |

#### `cstr_find_nth()` [↑](#dynamic_string-api-参考)

查找「C 字符串」中子「C 字符串」第 n 次出现的位置。

```c
bool cstr_find_nth(const char *cstr, const char *sub, size_t *out_index, dstr_direction_t direction, size_t n);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `cstr` | `const char *` | 目标「C 字符串」；为空或空串返回 `false`。 |
| `sub` | `const char *` | 子「C 字符串」；为空或空串返回 `false`。 |
| `out_index` | `size_t *` | 接收位置索引；为空则不写入。 |
| `direction` | `dstr_direction_t` | 查找方向。 |
| `n` | `size_t` | 次序（从 1 起）；`0` 表示该方向最后一次；超过实际次数按最后一次。 |

| 返回值 | 说明 |
| --- | --- |
| `bool` | 找到则 `true`，否则 `false`。 |

#### `dstr_find_indexes_cstr()` [↑](#dynamic_string-api-参考)

查找「动态字符串」中子「C 字符串」前 n 次出现的位置。

```c
size_t dstr_find_indexes_cstr(const dstr_adt *dstr, const char *sub, size_t *out_indexes, dstr_direction_t direction,
                              size_t n);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `const dstr_adt *` | 目标「动态字符串」；为空或空串返回 `0`。 |
| `sub` | `const char *` | 子「C 字符串」；为空或空串返回 `0`。 |
| `out_indexes` | `size_t *` | 接收位置索引数组；为空则不写入（需确保容量够大）。 |
| `direction` | `dstr_direction_t` | 查找方向。 |
| `n` | `size_t` | 查找次数（从 1 起）；`0` 表示全部；超过实际次数也查找全部。 |

| 返回值 | 说明 |
| --- | --- |
| `size_t` | 实际出现次数，即写入 `out_indexes` 的元素个数。 |

#### `dstr_find_indexes()` [↑](#dynamic_string-api-参考)

查找「动态字符串」中子「动态字符串」前 n 次出现的位置。

```c
size_t dstr_find_indexes(const dstr_adt *dstr, const dstr_adt *sub, size_t *out_indexes, dstr_direction_t direction,
                         size_t n);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `const dstr_adt *` | 目标「动态字符串」；为空或空串返回 `0`。 |
| `sub` | `const dstr_adt *` | 子「动态字符串」；为空或空串返回 `0`。 |
| `out_indexes` | `size_t *` | 接收位置索引数组；为空则不写入（需确保容量够大）。 |
| `direction` | `dstr_direction_t` | 查找方向。 |
| `n` | `size_t` | 查找次数（从 1 起）；`0` 表示全部；超过实际次数也查找全部。 |

| 返回值 | 说明 |
| --- | --- |
| `size_t` | 实际出现次数，即写入 `out_indexes` 的元素个数。 |

#### `cstr_find_indexes()` [↑](#dynamic_string-api-参考)

查找「C 字符串」中子「C 字符串」前 n 次出现的位置。

```c
size_t cstr_find_indexes(const char *cstr, const char *sub, size_t *out_indexes, dstr_direction_t direction, size_t n);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `cstr` | `const char *` | 目标「C 字符串」；为空或空串返回 `0`。 |
| `sub` | `const char *` | 子「C 字符串」；为空或空串返回 `0`。 |
| `out_indexes` | `size_t *` | 接收位置索引数组；为空则不写入（需确保容量够大）。 |
| `direction` | `dstr_direction_t` | 查找方向。 |
| `n` | `size_t` | 查找次数（从 1 起）；`0` 表示全部；超过实际次数也查找全部。 |

| 返回值 | 说明 |
| --- | --- |
| `size_t` | 实际出现次数，即写入 `out_indexes` 的元素个数。 |

#### `dstr_count_cstr()` [↑](#dynamic_string-api-参考)

统计「动态字符串」中子「C 字符串」出现的次数。

```c
size_t dstr_count_cstr(const dstr_adt *dstr, const char *sub);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `const dstr_adt *` | 目标「动态字符串」；为空或空串返回 `0`。 |
| `sub` | `const char *` | 子「C 字符串」；为空或空串返回 `0`。 |

| 返回值 | 说明 |
| --- | --- |
| `size_t` | 非重叠匹配的出现次数。 |

#### `dstr_count()` [↑](#dynamic_string-api-参考)

统计「动态字符串」中子「动态字符串」出现的次数。

```c
size_t dstr_count(const dstr_adt *dstr, const dstr_adt *sub);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `const dstr_adt *` | 目标「动态字符串」；为空或空串返回 `0`。 |
| `sub` | `const dstr_adt *` | 子「动态字符串」；为空或空串返回 `0`。 |

| 返回值 | 说明 |
| --- | --- |
| `size_t` | 非重叠匹配的出现次数。 |

#### `cstr_count()` [↑](#dynamic_string-api-参考)

统计「C 字符串」中子「C 字符串」出现的次数。

```c
size_t cstr_count(const char *cstr, const char *sub);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `cstr` | `const char *` | 目标「C 字符串」；为空或空串返回 `0`。 |
| `sub` | `const char *` | 子「C 字符串」；为空或空串返回 `0`。 |

| 返回值 | 说明 |
| --- | --- |
| `size_t` | 非重叠匹配的出现次数。 |

#### `dstr_replace_cstr()` [↑](#dynamic_string-api-参考)

替换「动态字符串」中旧「C 字符串」为新「C 字符串」n 次。

```c
dstr_status_t dstr_replace_cstr(dstr_adt *dstr, const char *old_str, const char *new_str, dstr_direction_t direction,
                                size_t n);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `dstr_adt *` | 目标「动态字符串」；为空或空串视为无效参数。 |
| `old_str` | `const char *` | 旧「C 字符串」；为空、空串或出现次数不足时视为无效参数。 |
| `new_str` | `const char *` | 新「C 字符串」；为空或空串时替换为空。 |
| `direction` | `dstr_direction_t` | 替换方向。 |
| `n` | `size_t` | 替换次数；`0` 表示全部；超过实际次数视为无效参数（一次都不替换）。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_status_t` | 全局状态码。 |

#### `dstr_replace()` [↑](#dynamic_string-api-参考)

替换「动态字符串」中旧「动态字符串」为新「动态字符串」n 次。

```c
dstr_status_t dstr_replace(dstr_adt *dstr, const dstr_adt *old_str, const dstr_adt *new_str, dstr_direction_t direction,
                           size_t n);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `dstr_adt *` | 目标「动态字符串」；为空或空串视为无效参数。 |
| `old_str` | `const dstr_adt *` | 旧「动态字符串」；为空、空串或出现次数不足时视为无效参数。 |
| `new_str` | `const dstr_adt *` | 新「动态字符串」；为空或空串时替换为空。 |
| `direction` | `dstr_direction_t` | 替换方向。 |
| `n` | `size_t` | 替换次数；`0` 表示全部；超过实际次数视为无效参数（一次都不替换）。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_status_t` | 全局状态码。 |

#### `dstr_replace_nth_cstr()` [↑](#dynamic_string-api-参考)

替换「动态字符串」中旧「C 字符串」第 n 次为新「C 字符串」。

```c
dstr_status_t dstr_replace_nth_cstr(dstr_adt *dstr, const char *old_str, const char *new_str,
                                    dstr_direction_t direction, size_t n);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `dstr_adt *` | 目标「动态字符串」；为空或空串视为无效参数。 |
| `old_str` | `const char *` | 旧「C 字符串」；为空、空串或出现次数不足时视为无效参数。 |
| `new_str` | `const char *` | 新「C 字符串」；为空或空串时替换为空。 |
| `direction` | `dstr_direction_t` | 替换方向。 |
| `n` | `size_t` | 次序（从 1 起）；`0` 表示该方向最后一次；超过实际次数视为无效参数。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_status_t` | 全局状态码。 |

#### `dstr_replace_nth()` [↑](#dynamic_string-api-参考)

替换「动态字符串」中旧「动态字符串」第 n 次为新「动态字符串」。

```c
dstr_status_t dstr_replace_nth(dstr_adt *dstr, const dstr_adt *old_str, const dstr_adt *new_str,
                               dstr_direction_t direction, size_t n);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `dstr_adt *` | 目标「动态字符串」；为空或空串视为无效参数。 |
| `old_str` | `const dstr_adt *` | 旧「动态字符串」；为空、空串或出现次数不足时视为无效参数。 |
| `new_str` | `const dstr_adt *` | 新「动态字符串」；为空或空串时替换为空。 |
| `direction` | `dstr_direction_t` | 替换方向。 |
| `n` | `size_t` | 次序（从 1 起）；`0` 表示该方向最后一次；超过实际次数视为无效参数。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_status_t` | 全局状态码。 |

<a id="api-functions-split-and-join"></a>

### 分隔与合并 [↑](#dynamic_string-api-参考)

分隔函数返回数组（其大小由输出参数给出）；合并函数返回单个字符串对象。

| 函数 | 说明 |
| --- | --- |
| `dstr_split_cstr()` / `dstr_split()` / `cstr_split()` | 分隔。 |
| `dstr_join_cstr()` / `dstr_join()` / `cstr_join()` | 合并。 |

#### `dstr_split_cstr()` [↑](#dynamic_string-api-参考)

分隔一个「C 字符串」为多个「动态字符串」。

```c
dstr_adt **dstr_split_cstr(const char *cstr, const char *separator, size_t *out_dstr_count);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `cstr` | `const char *` | 目标「C 字符串」；为空或空串返回 `NULL`。 |
| `separator` | `const char *` | 分隔「C 字符串」；为空或空串返回 `NULL`。 |
| `out_dstr_count` | `size_t *` | 接收个数；为空返回 `NULL`；仅在成功时写入。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_adt **` | 「动态字符串」数组；失败返回 `NULL`。 |

> [!NOTE]
> 两分隔串之间或分隔串与边界之间的空子串，以空指针（而非空串）存入数组，不会跳过。

> [!CAUTION]
> 释放时先依次 `dstr_destroy()` 每个元素，再 `free()` 数组本身。

#### `dstr_split()` [↑](#dynamic_string-api-参考)

分隔一个「动态字符串」为多个「动态字符串」。

```c
dstr_adt **dstr_split(const dstr_adt *dstr, const dstr_adt *separator, size_t *out_dstr_count);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstr` | `const dstr_adt *` | 目标「动态字符串」；为空或空串返回 `NULL`。 |
| `separator` | `const dstr_adt *` | 分隔「动态字符串」；为空或空串返回 `NULL`。 |
| `out_dstr_count` | `size_t *` | 接收个数；为空返回 `NULL`；仅在成功时写入。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_adt **` | 「动态字符串」数组；失败返回 `NULL`。 |

> [!NOTE]
> 两分隔串之间或分隔串与边界之间的空子串，以空指针（而非空串）存入数组，不会跳过。

> [!CAUTION]
> 释放时先依次 `dstr_destroy()` 每个元素，再 `free()` 数组本身。

#### `cstr_split()` [↑](#dynamic_string-api-参考)

分隔一个「C 字符串」为多个「C 字符串」。

```c
char **cstr_split(const char *cstr, const char *separator, size_t *out_cstr_count);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `cstr` | `const char *` | 目标「C 字符串」；为空或空串返回 `NULL`。 |
| `separator` | `const char *` | 分隔「C 字符串」；为空或空串返回 `NULL`。 |
| `out_cstr_count` | `size_t *` | 接收个数；为空返回 `NULL`；仅在成功时写入。 |

| 返回值 | 说明 |
| --- | --- |
| `char **` | 「C 字符串」数组；失败返回 `NULL`。 |

> [!NOTE]
> 两分隔串之间或分隔串与边界之间的空子串，以空指针（而非空串）存入数组，不会跳过。

> [!CAUTION]
> 释放时先依次 `free()` 每个元素，再 `free()` 数组本身。

#### `dstr_join_cstr()` [↑](#dynamic_string-api-参考)

合并多个「C 字符串」为一个「动态字符串」。

```c
dstr_adt *dstr_join_cstr(const char *const *cstrs, size_t cstr_count, const char *separator);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `cstrs` | `const char *const *` | 「C 字符串」数组；为空返回 `NULL`。 |
| `cstr_count` | `size_t` | 元素个数；为 `0` 返回 `NULL`。 |
| `separator` | `const char *` | 分隔「C 字符串」；为空或空串时不插入分隔。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_adt *` | 合并后的「动态字符串」；失败返回 `NULL`。 |

> [!NOTE]
> 数组中的空指针或空串会以空字符串形式被合并，不会跳过。

> [!CAUTION]
> 返回值指向堆内存，需手动调用 `dstr_destroy()` 释放。

#### `dstr_join()` [↑](#dynamic_string-api-参考)

合并多个「动态字符串」为一个「动态字符串」。

```c
dstr_adt *dstr_join(const dstr_adt *const *dstrs, size_t dstr_count, const dstr_adt *separator);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `dstrs` | `const dstr_adt *const *` | 「动态字符串」数组；为空返回 `NULL`。 |
| `dstr_count` | `size_t` | 元素个数；为 `0` 返回 `NULL`。 |
| `separator` | `const dstr_adt *` | 分隔「动态字符串」；为空或空串时不插入分隔。 |

| 返回值 | 说明 |
| --- | --- |
| `dstr_adt *` | 合并后的「动态字符串」；失败返回 `NULL`。 |

> [!NOTE]
> 数组中的空指针或空串会以空字符串形式被合并，不会跳过。

> [!CAUTION]
> 返回值指向堆内存，需手动调用 `dstr_destroy()` 释放。

#### `cstr_join()` [↑](#dynamic_string-api-参考)

合并多个「C 字符串」为一个「C 字符串」。

```c
char *cstr_join(const char *const *cstrs, size_t cstr_count, const char *separator);
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `cstrs` | `const char *const *` | 「C 字符串」数组；为空返回 `NULL`。 |
| `cstr_count` | `size_t` | 元素个数；为 `0` 返回 `NULL`。 |
| `separator` | `const char *` | 分隔「C 字符串」；为空或空串时不插入分隔。 |

| 返回值 | 说明 |
| --- | --- |
| `char *` | 合并后的「C 字符串」；失败返回 `NULL`。 |

> [!NOTE]
> 数组中的空指针或空串会以空字符串形式被合并，不会跳过。

> [!CAUTION]
> 返回值指向堆内存，需手动调用 `free()` 释放。
