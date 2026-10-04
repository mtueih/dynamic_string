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

### 类型定义 [↑](#basic-concepts)

#### 动态字符串 [↑](#basic-concepts-type-definitions)

本库是一个抽象数据类型库，这意味着所有 API 函数都是对一类对象的某种操作的定义。在此库中，这类对象是**动态字符串**。出于安全和封装等目的，此库使用**不透明类型**来定义它：

```c
typedef struct dynamic_string dstr_adt;
```

`dstr` 是 **d**ynamic **str**ing 的缩写，而 `adt` 表示抽象数据类型。

不透明意味着您不能直接获取和控制其属性。由于编译器也无法知道该类型的大小，所以您只能通过其**不透明指针**（`dstr_adt *`）来使用它。

#### 查找/替换方向 [↑](#basic-concepts-type-definitions)

对于**查找与替换**类的操作，需要指示查找/替换的方向（从前往后/从后往前）。因此这类 API 函数需要一个参数，来表达这两种情况。出于对语义清晰性的考量，本库定义枚举类型来表达此信息：

```c
typedef enum {
	DSTR_DIR_FORWARD,  /* 从前往后 */
	DSTR_DIR_BACKWARD  /* 从后往前 */
} dstr_direction_t;
```

<a id="basic-concepts-status-codes"></a>

### 状态码 [↑](#basic-concepts)

本库由于大量涉及内存操作，因此对于此库所定义的大多数操作而言，其并不总是会成功执行，且其执行失败可能有多种不同的原因。为了让调用方必要时能够区分是什么原因导致执行失败的，因此定义状态码枚举类型，API 函数通过返回此类型的值，来向调用方传递一个状态，以表达操作是否成功执行，以及失败时，失败的具体原因。

状态码枚举类型定义如下：

```c
typedef enum {
	DSTR_SUCCESS = 0,         /* 成功 */
	DSTR_MEMORY_ALLOC_FAILED, /* 内存分配失败 */
	DSTR_INVALID_ARGUMENT,    /* 无效参数 */
} dstr_status_t;
```

<a id="basic-concepts-naming-conventions"></a>

### 命名约定 [↑](#basic-concepts)

#### 前缀 [↑](#basic-concepts-naming-conventions)

本库主要包含对动态字符串这种对象操作的 API 函数，它们都以 `dstr_` 开头。本库也提供了一些不依赖动态字符串这种对象，而是针对 C 字符串的 API 函数，它们则以 `cstr_` 开头。

#### 后缀 [↑](#basic-concepts-naming-conventions)

本库大多数操作，都针对允许的两种输入字符串类型（动态字符串/C 字符串）提供了两种不同的版本，输入类型为 C 字符串的版本，相对输入类型为动态字符串的版本，包含 `_cstr` 后缀。

<a id="basic-concepts-general-semantics-and-constraints"></a>

### 通用语义与约束 [↑](#basic-concepts)

以下约定适用于全库，除非具体函数另有说明。

1. **文本字符串库，而非二进制安全字符串库**。底层始终是合法 C 字符串，即数据始终以 `'\0'` 结尾，哪怕长度为 0。这意味着 API 函数 `dstr_cstr()` 对于有效输入，其返回值一定不会是空指针，其也一定指向一个以 `'\0'` 结尾的字符串缓冲区。
2. **编码无关性与容量**。本库并不关心字符串数据的编码，因此其长度以字节而不是字符个数为单位，且其长度不包括 `'\0'`。容量表示底层数据缓冲区的真实大小，同样以字节为单位，由于要存储结尾的 `'\0'`，因此容量始终至少比长度大 1（长度为 0 时，也至少是 1，这意味着容量值的下限保证是 1 而不是 0，且由于实现可能会做 SSO 等优化，因此实际的容量下限可能大于 1），但不保证始终等于长度 + 1（实现可能会做几何扩容等优化）。
3. **空指针与空字符串**。对于任何表示字符串的参数，通常都可以使用空指针来表示空字符串，这避免了为了表示空字符串而构造不必要的对象的麻烦。
4. **非重叠匹配**。所有查找、统计与替换类的操作均按**非重叠匹配**处理。
5. **禁止内存重叠**。对于任何可能涉及从一个缓冲区拷贝数据到另一个缓冲区的操作，不保证实现会做源缓冲区与目标缓冲区重叠的检查。因此涉及此类情况时，请勿让源缓冲区指针指向目标缓冲区，否则为未定义行为。

<a id="api-functions"></a>

## API 函数 [↑](#dynamic_string-api-参考)

<a id="api-functions-creation-and-destruction"></a>

### 创建与销毁 [↑](#api-functions)

此组 API 函数，主要用于控制一个「动态字符串」的生命周期。

包含的操作及对应 API 函数与说明如下：

<table>
  <thead>
    <tr>
      <th>操作类型</th>
      <th>操作</th>
      <th>API 函数</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td rowspan="6">创建</td>
      <td>创建</td>
      <td><a href="#api-functions-creation-and-destruction-dstr_create"><code>dstr_create()</code></a></td>
    </tr>
    <tr>
      <td>克隆</td>
      <td><a href="#api-functions-creation-and-destruction-dstr_clone"><code>dstr_clone()</code></a></td>
    </tr>
    <tr>
      <td rowspan="2">提取子串</td>
      <td><a href="#api-functions-creation-and-destruction-dstr_sub_cstr"><code>dstr_sub_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-creation-and-destruction-dstr_sub"><code>dstr_sub()</code></a></td>
    </tr>
    <tr>
      <td rowspan="2">格式化创建</td>
      <td><a href="#api-functions-creation-and-destruction-dstr_create_format"><code>dstr_create_format()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-creation-and-destruction-dstr_create_vformat"><code>dstr_create_vformat()</code></a></td>
    </tr>
    <tr>
      <td>销毁</td>
      <td>销毁</td>
      <td><a href="#api-functions-creation-and-destruction-dstr_destroy"><code>dstr_destroy()</code></a></td>
    </tr>
  </tbody>
</table>

<a id="api-functions-creation-and-destruction-dstr_create"></a>

#### `dstr_create()` [↑](#api-functions-creation-and-destruction)

创建一个「动态字符串」。

```c
dstr_adt *dstr_create(
	const char *cstr
);
```

| 参数   | 类型           | 说明                                                                      |
| ------ | -------------- | ------------------------------------------------------------------------- |
| `cstr` | `const char *` | 源「C 字符串」的指针。<br>为<em>「空字符串」</em>时创建空「动态字符串」。 |

| 返回值       | 说明                                                     |
| ------------ | -------------------------------------------------------- |
| `dstr_adt *` | 所创建的「动态字符串」的指针。<br>创建失败返回*空指针*。 |

> [!IMPORTANT]
>
> 返回值指向堆内存，请手动调用 `dstr_destroy()` 释放。

<a id="api-functions-creation-and-destruction-dstr_destroy"></a>

#### `dstr_destroy()` [↑](#api-functions-creation-and-destruction)

销毁一个「动态字符串」。

```c
void dstr_destroy(
	dstr_adt *dstr
);
```

| 参数   | 类型         | 说明                                                         |
| ------ | ------------ | ------------------------------------------------------------ |
| `dstr` | `dstr_adt *` | 目标「动态字符串」的指针。<br>为*空指针*时，函数会直接返回。 |

<a id="api-functions-creation-and-destruction-dstr_clone"></a>

#### `dstr_clone()` [↑](#api-functions-creation-and-destruction)

克隆一个「动态字符串」。

```c
dstr_adt *dstr_clone(
	const dstr_adt *dstr
);
```

| 参数   | 类型               | 说明                                                                        |
| ------ | ------------------ | --------------------------------------------------------------------------- |
| `dstr` | `const dstr_adt *` | 源「动态字符串」的指针。<br>为<em>「空字符串」</em>时创建空「动态字符串」。 |

| 返回值       | 说明                                                     |
| ------------ | -------------------------------------------------------- |
| `dstr_adt *` | 所创建的「动态字符串」的指针。<br>创建失败返回*空指针*。 |

> [!IMPORTANT]
>
> 返回值指向堆内存，请手动调用 `dstr_destroy()` 释放。

<a id="api-functions-creation-and-destruction-dstr_sub_cstr"></a>

#### `dstr_sub_cstr()` [↑](#api-functions-creation-and-destruction)

提取一个「C 字符串」的子串为一个新的「动态字符串」。

```c
dstr_adt *dstr_sub_cstr(
	const char *cstr,
	size_t sub_start,
	size_t sub_length
);
```

| 参数         | 类型           | 说明                                                                                                            |
| ------------ | -------------- | --------------------------------------------------------------------------------------------------------------- |
| `cstr`       | `const char *` | 源「C 字符串」的指针。<br>为<em>「空字符串」</em>时创建空「动态字符串」，此时忽略 `sub_start` 和 `sub_length`。 |
| `sub_start`  | `size_t`       | 子串的起始索引。<br>*越界*时，函数会直接返回*空指针*。                                                          |
| `sub_length` | `size_t`       | 子串的长度。<br>为 _`0`_ 表示到末尾。<br>*越界*时，函数会直接返回*空指针*。                                     |

| 返回值       | 说明                                                     |
| ------------ | -------------------------------------------------------- |
| `dstr_adt *` | 所创建的「动态字符串」的指针。<br>创建失败返回*空指针*。 |

> [!IMPORTANT]
>
> 返回值指向堆内存，请手动调用 `dstr_destroy()` 释放。

<a id="api-functions-creation-and-destruction-dstr_sub"></a>

#### `dstr_sub()` [↑](#api-functions-creation-and-destruction)

提取一个「动态字符串」的子串为一个新的「动态字符串」。

```c
dstr_adt *dstr_sub(
	const dstr_adt *dstr,
	size_t sub_start,
	size_t sub_length
);
```

| 参数         | 类型               | 说明                                                                                                              |
| ------------ | ------------------ | ----------------------------------------------------------------------------------------------------------------- |
| `dstr`       | `const dstr_adt *` | 源「动态字符串」的指针。<br>为<em>「空字符串」</em>时创建空「动态字符串」，此时忽略 `sub_start` 和 `sub_length`。 |
| `sub_start`  | `size_t`           | 子串的起始索引。<br>*越界*时，函数会直接返回*空指针*。                                                            |
| `sub_length` | `size_t`           | 子串的长度。<br>为 _`0`_ 表示到末尾。<br>*越界*时，函数会直接返回*空指针*。                                       |

| 返回值       | 说明                                                     |
| ------------ | -------------------------------------------------------- |
| `dstr_adt *` | 所创建的「动态字符串」的指针。<br>创建失败返回*空指针*。 |

> [!IMPORTANT]
>
> 返回值指向堆内存，请手动调用 `dstr_destroy()` 释放。

<a id="api-functions-creation-and-destruction-dstr_create_format"></a>

#### `dstr_create_format()` [↑](#api-functions-creation-and-destruction)

格式化创建一个「动态字符串」。

```c
dstr_adt *dstr_create_format(
	const char *format,
	...
);
```

| 参数     | 类型           | 说明                                                                        |
| -------- | -------------- | --------------------------------------------------------------------------- |
| `format` | `const char *` | 格式「C 字符串」的指针。<br>为<em>「空字符串」</em>时创建空「动态字符串」。 |
| `...`    | —              | 与 `format` 对应的可变参数列表。                                            |

| 返回值       | 说明                                                     |
| ------------ | -------------------------------------------------------- |
| `dstr_adt *` | 所创建的「动态字符串」的指针。<br>创建失败返回*空指针*。 |

> [!IMPORTANT]
>
> 返回值指向堆内存，请手动调用 `dstr_destroy()` 释放。

<a id="api-functions-creation-and-destruction-dstr_create_vformat"></a>

#### `dstr_create_vformat()` [↑](#api-functions-creation-and-destruction)

格式化创建一个「动态字符串」（`va_list` 版本）。

```c
dstr_adt *dstr_create_vformat(
	const char *format,
	va_list args
);
```

| 参数     | 类型           | 说明                                                                                                                                                     |
| -------- | -------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `format` | `const char *` | 格式「C 字符串」的指针。<br>为<em>「空字符串」</em>时创建空「动态字符串」。                                                                              |
| `args`   | `va_list`      | 已通过 `va_start()` 初始化的 `va_list` 变量，包含与 `format` 对应的可变参数列表信息。<br>该函数不会调用 `va_end()`，调用者需自行管理 `args` 的生命周期。 |

| 返回值       | 说明                                                     |
| ------------ | -------------------------------------------------------- |
| `dstr_adt *` | 所创建的「动态字符串」的指针。<br>创建失败返回*空指针*。 |

> [!NOTE]
>
> `args` 可能被此函数读取并消耗，调用后不应再次使用，除非重新 `va_start()` 或使用 `va_copy()` 创建副本以使用此函数。

> [!IMPORTANT]
>
> 返回值指向堆内存，请手动调用 `dstr_destroy()` 释放。

<a id="api-functions-getters-and-setters"></a>

### 属性获取与设置 [↑](#api-functions)

此组 API 函数，主要用于获取与设置一个「动态字符串」的属性。

包含的操作及对应 API 函数与说明如下：

<table>
  <thead>
    <tr>
      <th>操作类型</th>
      <th>操作</th>
      <th>API 函数</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td rowspan="4">获取</td>
      <td>获取内部「C 字符串」指针</td>
      <td><a href="#api-functions-getters-and-setters-dstr_cstr"><code>dstr_cstr()</code></a></td>
    </tr>
    <tr>
      <td>获取长度</td>
      <td><a href="#api-functions-getters-and-setters-dstr_length"><code>dstr_length()</code></a></td>
    </tr>
    <tr>
      <td>获取容量</td>
      <td><a href="#api-functions-getters-and-setters-dstr_capacity"><code>dstr_capacity()</code></a></td>
    </tr>
    <tr>
      <td>判断是否为空</td>
      <td><a href="#api-functions-getters-and-setters-dstr_is_empty"><code>dstr_is_empty()</code></a></td>
    </tr>
    <tr>
      <td rowspan="2">设置</td>
      <td>设置容量</td>
      <td><a href="#api-functions-getters-and-setters-dstr_set_capacity"><code>dstr_set_capacity()</code></a></td>
    </tr>
    <tr>
      <td>调整容量到刚够</td>
      <td><a href="#api-functions-getters-and-setters-dstr_shrink_to_fit"><code>dstr_shrink_to_fit()</code></a></td>
    </tr>
  </tbody>
</table>

<a id="api-functions-getters-and-setters-dstr_cstr"></a>

#### `dstr_cstr()` [↑](#api-functions-getters-and-setters)

获取一个「动态字符串」的内部「C 字符串」指针。

```c
const char *dstr_cstr(
	const dstr_adt *dstr
);
```

| 参数   | 类型               | 说明                                                                 |
| ------ | ------------------ | -------------------------------------------------------------------- |
| `dstr` | `const dstr_adt *` | 目标「动态字符串」的指针。<br>为*空指针*时，函数会直接返回*空指针*。 |

| 返回值         | 说明                            |
| -------------- | ------------------------------- |
| `const char *` | `dstr` 的内部「C 字符串」指针。 |

> [!NOTE]
>
> 此函数对于有效输入，保证其返回值一定不会是*空指针*，其也一定指向一个以 `'\0'` 结尾的字符串缓冲区。

> [!WARNING]
>
> 返回的指针指向内部缓冲区，请勿通过该指针修改其内容。
>
> 该指针是临时的，可能因 `dstr` 的内容或容量的变化而失效，切勿依赖。

<a id="api-functions-getters-and-setters-dstr_length"></a>

#### `dstr_length()` [↑](#api-functions-getters-and-setters)

获取一个「动态字符串」的长度。

```c
size_t dstr_length(
	const dstr_adt *dstr
);
```

| 参数   | 类型               | 说明                                                               |
| ------ | ------------------ | ------------------------------------------------------------------ |
| `dstr` | `const dstr_adt *` | 目标「动态字符串」的指针。<br>为*空指针*时，函数会直接返回 _`0`_。 |

| 返回值   | 说明            |
| -------- | --------------- |
| `size_t` | `dstr` 的长度。 |

<a id="api-functions-getters-and-setters-dstr_is_empty"></a>

#### `dstr_is_empty()` [↑](#api-functions-getters-and-setters)

判断一个「动态字符串」是否是空「动态字符串」。

```c
bool dstr_is_empty(
	const dstr_adt *dstr
);
```

| 参数   | 类型               | 说明                                                                  |
| ------ | ------------------ | --------------------------------------------------------------------- |
| `dstr` | `const dstr_adt *` | 目标「动态字符串」的指针。<br>为*空指针*时，函数会直接返回 _`true`_。 |

| 返回值 | 说明                                                    |
| ------ | ------------------------------------------------------- |
| `bool` | 是空「动态字符串」则返回 _`true`_，否则返回 _`false`_。 |

<a id="api-functions-getters-and-setters-dstr_capacity"></a>

#### `dstr_capacity()` [↑](#api-functions-getters-and-setters)

获取一个「动态字符串」的容量。

```c
size_t dstr_capacity(
	const dstr_adt *dstr
);
```

| 参数   | 类型               | 说明                                                               |
| ------ | ------------------ | ------------------------------------------------------------------ |
| `dstr` | `const dstr_adt *` | 目标「动态字符串」的指针。<br>为*空指针*时，函数会直接返回 _`0`_。 |

| 返回值   | 说明            |
| -------- | --------------- |
| `size_t` | `dstr` 的容量。 |

<a id="api-functions-getters-and-setters-dstr_set_capacity"></a>

#### `dstr_set_capacity()` [↑](#api-functions-getters-and-setters)

设置一个「动态字符串」的容量。

```c
dstr_status_t dstr_set_capacity(
	dstr_adt *dstr,
	size_t new_capacity
);
```

| 参数           | 类型         | 说明                                                 |
| -------------- | ------------ | ---------------------------------------------------- |
| `dstr`         | `dstr_adt *` | 目标「动态字符串」的指针。<br>*空指针*视为无效参数。 |
| `new_capacity` | `size_t`     | 新的容量。                                           |

| 返回值          | 说明         |
| --------------- | ------------ |
| `dstr_status_t` | 全局状态码。 |

> [!NOTE]
>
> 由于 API 保证内部缓冲区始终是以 `'\0'` 结尾的合法 C 字符串，因此 API 层面约束了一个动态字符串的容量存在下限，这个下限可以不是 1，但必须大于 0。
>
> 这影响了此函数的行为，当此函数成功执行后，实际容量可能等于 `new_capacity`，也可能等于实现所采用的容量下限（当 `new_capacity` 小于等于该下限时）。
>
> 另外，如果 `new_capacity` 大于实现所采用的容量下限，那么当此函数成功执行后，会将 `new_capacity` 同时设置为 `dstr` 的保底容量（和实现所采用的容量下限不是一回事），在后续由于 `dstr` 内容长度的减小所引起的容量的自动缩小时，容量将维持不低于保底容量值。再次调用此函数可覆盖此值，调用 `dstr_shrink_to_fit()` 函数可清除此值。

> [!WARNING]
>
> 当 `new_capacity` 小于等于 `dstr` 当前长度时，`dstr` 的内容会被截断，具体行为受实现容量下限影响。

<a id="api-functions-getters-and-setters-dstr_shrink_to_fit"></a>

#### `dstr_shrink_to_fit()` [↑](#api-functions-getters-and-setters)

调整一个「动态字符串」的容量到刚合适。

```c
void dstr_shrink_to_fit(
	dstr_adt *dstr
);
```

| 参数   | 类型         | 说明                                                         |
| ------ | ------------ | ------------------------------------------------------------ |
| `dstr` | `dstr_adt *` | 目标「动态字符串」的指针。<br>为*空指针*时，函数会直接返回。 |

> [!NOTE]
>
> 此函数执行后，实际容量受实现的容量下限影响。
>
> 执行此函数会同时清除 `dstr` 由 `dstr_set_capacity()` 函数所设置的保底容量。

<a id="api-functions-content-editing"></a>

### 内容编辑 [↑](#api-functions)

此组 API 函数，主要用于编辑一个「动态字符串」的内容。

包含的操作及对应 API 函数与说明如下：

<table>
  <thead>
    <tr>
      <th>操作类型</th>
      <th>操作</th>
      <th>API 函数</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td rowspan="6">复制</td>
      <td rowspan="2">复制</td>
      <td><a href="#api-functions-content-editing-dstr_cpy_cstr"><code>dstr_cpy_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-content-editing-dstr_cpy"><code>dstr_cpy()</code></a></td>
    </tr>
    <tr>
      <td rowspan="2">复制子串</td>
      <td><a href="#api-functions-content-editing-dstr_cpy_sub_cstr"><code>dstr_cpy_sub_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-content-editing-dstr_cpy_sub"><code>dstr_cpy_sub()</code></a></td>
    </tr>
    <tr>
      <td rowspan="2">格式化复制</td>
      <td><a href="#api-functions-content-editing-dstr_cpy_format"><code>dstr_cpy_format()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-content-editing-dstr_cpy_vformat"><code>dstr_cpy_vformat()</code></a></td>
    </tr>
    <tr>
      <td rowspan="6">追加</td>
      <td rowspan="2">追加</td>
      <td><a href="#api-functions-content-editing-dstr_cat_cstr"><code>dstr_cat_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-content-editing-dstr_cat"><code>dstr_cat()</code></a></td>
    </tr>
    <tr>
      <td rowspan="2">追加子串</td>
      <td><a href="#api-functions-content-editing-dstr_cat_sub_cstr"><code>dstr_cat_sub_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-content-editing-dstr_cat_sub"><code>dstr_cat_sub()</code></a></td>
    </tr>
    <tr>
      <td rowspan="2">格式化追加</td>
      <td><a href="#api-functions-content-editing-dstr_cat_format"><code>dstr_cat_format()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-content-editing-dstr_cat_vformat"><code>dstr_cat_vformat()</code></a></td>
    </tr>
    <tr>
      <td rowspan="6">插入</td>
      <td rowspan="2">插入</td>
      <td><a href="#api-functions-content-editing-dstr_insert_cstr"><code>dstr_insert_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-content-editing-dstr_insert"><code>dstr_insert()</code></a></td>
    </tr>
    <tr>
      <td rowspan="2">插入子串</td>
      <td><a href="#api-functions-content-editing-dstr_insert_sub_cstr"><code>dstr_insert_sub_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-content-editing-dstr_insert_sub"><code>dstr_insert_sub()</code></a></td>
    </tr>
    <tr>
      <td rowspan="2">格式化插入</td>
      <td><a href="#api-functions-content-editing-dstr_insert_format"><code>dstr_insert_format()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-content-editing-dstr_insert_vformat"><code>dstr_insert_vformat()</code></a></td>
    </tr>
    <tr>
      <td rowspan="3">删除</td>
      <td>清空</td>
      <td><a href="#api-functions-content-editing-dstr_clear"><code>dstr_clear()</code></a></td>
    </tr>
    <tr>
      <td>删除子串</td>
      <td><a href="#api-functions-content-editing-dstr_remove"><code>dstr_remove()</code></a></td>
    </tr>
    <tr>
      <td>删除首尾字符</td>
      <td><a href="#api-functions-content-editing-dstr_trim"><code>dstr_trim()</code></a></td>
    </tr>
  </tbody>
</table>

<a id="api-functions-content-editing-dstr_cpy_cstr"></a>

#### `dstr_cpy_cstr()` [↑](#api-functions-content-editing)

复制一个「C 字符串」到一个「动态字符串」。

```c
dstr_status_t dstr_cpy_cstr(
	dstr_adt *dest,
	const char *src
);
```

| 参数   | 类型           | 说明                                                                                    |
| ------ | -------------- | --------------------------------------------------------------------------------------- |
| `dest` | `dstr_adt *`   | 目标「动态字符串」的指针。<br>*空指针*视为无效参数。                                    |
| `src`  | `const char *` | 源「C 字符串」的指针。<br>为<em>「空字符串」</em>时复制空字符串（清空 `dest` 的内容）。 |

| 返回值          | 说明         |
| --------------- | ------------ |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
>
> `src` 不得指向 `dest` 的内部缓冲区，若需自复制，请先创建临时副本。

<a id="api-functions-content-editing-dstr_cpy"></a>

#### `dstr_cpy()` [↑](#api-functions-content-editing)

复制一个「动态字符串」到另一个「动态字符串」。

```c
dstr_status_t dstr_cpy(
	dstr_adt *dest,
	const dstr_adt *src
);
```

| 参数   | 类型               | 说明                                                                                      |
| ------ | ------------------ | ----------------------------------------------------------------------------------------- |
| `dest` | `dstr_adt *`       | 目标「动态字符串」的指针。<br>*空指针*视为无效参数。                                      |
| `src`  | `const dstr_adt *` | 源「动态字符串」的指针。<br>为<em>「空字符串」</em>时复制空字符串（清空 `dest` 的内容）。 |

| 返回值          | 说明         |
| --------------- | ------------ |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
>
> `src` 不得指向 `dest` 的内部缓冲区，若需自复制，请先创建临时副本。

<a id="api-functions-content-editing-dstr_cpy_sub_cstr"></a>

#### `dstr_cpy_sub_cstr()` [↑](#api-functions-content-editing)

复制一个「C 字符串」的子串到一个「动态字符串」。

```c
dstr_status_t dstr_cpy_sub_cstr(
	dstr_adt *dest,
	const char *src,
	size_t sub_start,
	size_t sub_length
);
```

| 参数         | 类型           | 说明                                                                                                                          |
| ------------ | -------------- | ----------------------------------------------------------------------------------------------------------------------------- |
| `dest`       | `dstr_adt *`   | 目标「动态字符串」的指针。<br>*空指针*视为无效参数。                                                                          |
| `src`        | `const char *` | 源「C 字符串」的指针。<br>为<em>「空字符串」</em>时复制空字符串（清空 `dest` 的内容），此时忽略 `sub_start` 和 `sub_length`。 |
| `sub_start`  | `size_t`       | 子串的起始索引。<br>*越界*视为无效参数。                                                                                      |
| `sub_length` | `size_t`       | 子串的长度。<br>为 _`0`_ 表示到末尾。<br>*越界*视为无效参数。                                                                 |

| 返回值          | 说明         |
| --------------- | ------------ |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
>
> `src` 不得指向 `dest` 的内部缓冲区，若需自复制，请先创建临时副本。

<a id="api-functions-content-editing-dstr_cpy_sub"></a>

#### `dstr_cpy_sub()` [↑](#api-functions-content-editing)

复制一个「动态字符串」的子串到另一个「动态字符串」。

```c
dstr_status_t dstr_cpy_sub(
	dstr_adt *dest,
	const dstr_adt *src,
	size_t sub_start,
	size_t sub_length
);
```

| 参数         | 类型               | 说明                                                                                                                            |
| ------------ | ------------------ | ------------------------------------------------------------------------------------------------------------------------------- |
| `dest`       | `dstr_adt *`       | 目标「动态字符串」的指针。<br>*空指针*视为无效参数。                                                                            |
| `src`        | `const dstr_adt *` | 源「动态字符串」的指针。<br>为<em>「空字符串」</em>时复制空字符串（清空 `dest` 的内容），此时忽略 `sub_start` 和 `sub_length`。 |
| `sub_start`  | `size_t`           | 子串的起始索引。<br>*越界*视为无效参数。                                                                                        |
| `sub_length` | `size_t`           | 子串的长度。<br>为 _`0`_ 表示到末尾。<br>*越界*视为无效参数。                                                                   |

| 返回值          | 说明         |
| --------------- | ------------ |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
>
> `src` 不得指向 `dest` 的内部缓冲区，若需自复制，请先创建临时副本。

<a id="api-functions-content-editing-dstr_cpy_format"></a>

#### `dstr_cpy_format()` [↑](#api-functions-content-editing)

格式化复制一个字符串到一个「动态字符串」。

```c
dstr_status_t dstr_cpy_format(
	dstr_adt *dest,
	const char *format,
	...
);
```

| 参数     | 类型           | 说明                                                                                      |
| -------- | -------------- | ----------------------------------------------------------------------------------------- |
| `dest`   | `dstr_adt *`   | 目标「动态字符串」的指针。<br>*空指针*视为无效参数。                                      |
| `format` | `const char *` | 格式「C 字符串」的指针。<br>为<em>「空字符串」</em>时复制空字符串（清空 `dest` 的内容）。 |
| `...`    | —              | 与 `format` 对应的可变参数列表。                                                          |

| 返回值          | 说明         |
| --------------- | ------------ |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
>
> `format` 不得指向 `dest` 的内部缓冲区，若需自复制，请先创建临时副本。

<a id="api-functions-content-editing-dstr_cpy_vformat"></a>

#### `dstr_cpy_vformat()` [↑](#api-functions-content-editing)

格式化复制一个字符串到一个「动态字符串」（`va_list` 版本）。

```c
dstr_status_t dstr_cpy_vformat(
	dstr_adt *dest,
	const char *format,
	va_list args
);
```

| 参数     | 类型           | 说明                                                                                                                                                     |
| -------- | -------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `dest`   | `dstr_adt *`   | 目标「动态字符串」的指针。<br>*空指针*视为无效参数。                                                                                                     |
| `format` | `const char *` | 格式「C 字符串」的指针。<br>为<em>「空字符串」</em>时复制空字符串（清空 `dest` 的内容）。                                                                |
| `args`   | `va_list`      | 已通过 `va_start()` 初始化的 `va_list` 变量，包含与 `format` 对应的可变参数列表信息。<br>该函数不会调用 `va_end()`，调用者需自行管理 `args` 的生命周期。 |

| 返回值          | 说明         |
| --------------- | ------------ |
| `dstr_status_t` | 全局状态码。 |

> [!NOTE]
>
> `args` 可能被此函数读取并消耗，调用后不应再次使用，除非重新 `va_start()` 或使用 `va_copy()` 创建副本以使用此函数。

> [!WARNING]
>
> `format` 不得指向 `dest` 的内部缓冲区，若需自复制，请先创建临时副本。

<a id="api-functions-content-editing-dstr_cat_cstr"></a>

#### `dstr_cat_cstr()` [↑](#api-functions-content-editing)

追加一个「C 字符串」到一个「动态字符串」。

```c
dstr_status_t dstr_cat_cstr(
	dstr_adt *dest,
	const char *src
);
```

| 参数   | 类型           | 说明                                                                              |
| ------ | -------------- | --------------------------------------------------------------------------------- |
| `dest` | `dstr_adt *`   | 目标「动态字符串」的指针。<br>*空指针*视为无效参数。                              |
| `src`  | `const char *` | 源「C 字符串」的指针。<br>为<em>「空字符串」</em>时追加空字符串（什么都不追加）。 |

| 返回值          | 说明         |
| --------------- | ------------ |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
>
> `src` 不得指向 `dest` 的内部缓冲区，若需自追加，请先创建临时副本。

<a id="api-functions-content-editing-dstr_cat"></a>

#### `dstr_cat()` [↑](#api-functions-content-editing)

追加一个「动态字符串」到另一个「动态字符串」。

```c
dstr_status_t dstr_cat(
	dstr_adt *dest,
	const dstr_adt *src
);
```

| 参数   | 类型               | 说明                                                                                |
| ------ | ------------------ | ----------------------------------------------------------------------------------- |
| `dest` | `dstr_adt *`       | 目标「动态字符串」的指针。<br>*空指针*视为无效参数。                                |
| `src`  | `const dstr_adt *` | 源「动态字符串」的指针。<br>为<em>「空字符串」</em>时追加空字符串（什么都不追加）。 |

| 返回值          | 说明         |
| --------------- | ------------ |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
>
> `src` 不得指向 `dest` 的内部缓冲区，若需自追加，请先创建临时副本。

<a id="api-functions-content-editing-dstr_cat_sub_cstr"></a>

#### `dstr_cat_sub_cstr()` [↑](#api-functions-content-editing)

追加一个「C 字符串」的子串到一个「动态字符串」。

```c
dstr_status_t dstr_cat_sub_cstr(
	dstr_adt *dest,
	const char *src,
	size_t sub_start,
	size_t sub_length
);
```

| 参数         | 类型           | 说明                                                                                                                    |
| ------------ | -------------- | ----------------------------------------------------------------------------------------------------------------------- |
| `dest`       | `dstr_adt *`   | 目标「动态字符串」的指针。<br>*空指针*视为无效参数。                                                                    |
| `src`        | `const char *` | 源「C 字符串」的指针。<br>为<em>「空字符串」</em>时追加空字符串（什么都不追加），此时忽略 `sub_start` 和 `sub_length`。 |
| `sub_start`  | `size_t`       | 子串的起始索引。<br>*越界*视为无效参数。                                                                                |
| `sub_length` | `size_t`       | 子串的长度。<br>为 _`0`_ 表示到末尾。<br>*越界*视为无效参数。                                                           |

| 返回值          | 说明         |
| --------------- | ------------ |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
>
> `src` 不得指向 `dest` 的内部缓冲区，若需自追加，请先创建临时副本。

<a id="api-functions-content-editing-dstr_cat_sub"></a>

#### `dstr_cat_sub()` [↑](#api-functions-content-editing)

追加一个「动态字符串」的子串到另一个「动态字符串」。

```c
dstr_status_t dstr_cat_sub(
	dstr_adt *dest,
	const dstr_adt *src,
	size_t sub_start,
	size_t sub_length
);
```

| 参数         | 类型               | 说明                                                                                                                      |
| ------------ | ------------------ | ------------------------------------------------------------------------------------------------------------------------- |
| `dest`       | `dstr_adt *`       | 目标「动态字符串」的指针。<br>*空指针*视为无效参数。                                                                      |
| `src`        | `const dstr_adt *` | 源「动态字符串」的指针。<br>为<em>「空字符串」</em>时追加空字符串（什么都不追加），此时忽略 `sub_start` 和 `sub_length`。 |
| `sub_start`  | `size_t`           | 子串的起始索引。<br>*越界*视为无效参数。                                                                                  |
| `sub_length` | `size_t`           | 子串的长度。<br>为 _`0`_ 表示到末尾。<br>*越界*视为无效参数。                                                             |

| 返回值          | 说明         |
| --------------- | ------------ |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
>
> `src` 不得指向 `dest` 的内部缓冲区，若需自追加，请先创建临时副本。

<a id="api-functions-content-editing-dstr_cat_format"></a>

#### `dstr_cat_format()` [↑](#api-functions-content-editing)

格式化追加一个字符串到一个「动态字符串」。

```c
dstr_status_t dstr_cat_format(
	dstr_adt *dest,
	const char *format,
	...
);
```

| 参数     | 类型           | 说明                                                                                |
| -------- | -------------- | ----------------------------------------------------------------------------------- |
| `dest`   | `dstr_adt *`   | 目标「动态字符串」的指针。<br>*空指针*视为无效参数。                                |
| `format` | `const char *` | 格式「C 字符串」的指针。<br>为<em>「空字符串」</em>时追加空字符串（什么都不追加）。 |
| `...`    | —              | 与 `format` 对应的可变参数列表。                                                    |

| 返回值          | 说明         |
| --------------- | ------------ |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
>
> `format` 不得指向 `dest` 的内部缓冲区，若需自追加，请先创建临时副本。

<a id="api-functions-content-editing-dstr_cat_vformat"></a>

#### `dstr_cat_vformat()` [↑](#api-functions-content-editing)

格式化追加一个字符串到一个「动态字符串」（`va_list` 版本）。

```c
dstr_status_t dstr_cat_vformat(
	dstr_adt *dest,
	const char *format,
	va_list args
);
```

| 参数     | 类型           | 说明                                                                                                                                                     |
| -------- | -------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `dest`   | `dstr_adt *`   | 目标「动态字符串」的指针。<br>*空指针*视为无效参数。                                                                                                     |
| `format` | `const char *` | 格式「C 字符串」的指针。<br>为<em>「空字符串」</em>时追加空字符串（什么都不追加）。                                                                      |
| `args`   | `va_list`      | 已通过 `va_start()` 初始化的 `va_list` 变量，包含与 `format` 对应的可变参数列表信息。<br>该函数不会调用 `va_end()`，调用者需自行管理 `args` 的生命周期。 |

| 返回值          | 说明         |
| --------------- | ------------ |
| `dstr_status_t` | 全局状态码。 |

> [!NOTE]
>
> `args` 可能被此函数读取并消耗，调用后不应再次使用，除非重新 `va_start()` 或使用 `va_copy()` 创建副本以使用此函数。

> [!WARNING]
>
> `format` 不得指向 `dest` 的内部缓冲区，若需自追加，请先创建临时副本。

<a id="api-functions-content-editing-dstr_insert_cstr"></a>

#### `dstr_insert_cstr()` [↑](#api-functions-content-editing)

插入一个「C 字符串」到一个「动态字符串」。

```c
dstr_status_t dstr_insert_cstr(
	dstr_adt *dest,
	size_t index,
	const char *src
);
```

| 参数    | 类型           | 说明                                                                              |
| ------- | -------------- | --------------------------------------------------------------------------------- |
| `dest`  | `dstr_adt *`   | 目标「动态字符串」的指针。<br>*空指针*视为无效参数。                              |
| `index` | `size_t`       | 插入位置的索引。<br>*越界*视为无效参数。                                          |
| `src`   | `const char *` | 源「C 字符串」的指针。<br>为<em>「空字符串」</em>时插入空字符串（什么都不插入）。 |

| 返回值          | 说明         |
| --------------- | ------------ |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
>
> `src` 不得指向 `dest` 的内部缓冲区，若需自插入，请先创建临时副本。

<a id="api-functions-content-editing-dstr_insert"></a>

#### `dstr_insert()` [↑](#api-functions-content-editing)

插入一个「动态字符串」到另一个「动态字符串」。

```c
dstr_status_t dstr_insert(
	dstr_adt *dest,
	size_t index,
	const dstr_adt *src
);
```

| 参数    | 类型               | 说明                                                                                |
| ------- | ------------------ | ----------------------------------------------------------------------------------- |
| `dest`  | `dstr_adt *`       | 目标「动态字符串」的指针。<br>*空指针*视为无效参数。                                |
| `index` | `size_t`           | 插入位置的索引。<br>*越界*视为无效参数。                                            |
| `src`   | `const dstr_adt *` | 源「动态字符串」的指针。<br>为<em>「空字符串」</em>时插入空字符串（什么都不插入）。 |

| 返回值          | 说明         |
| --------------- | ------------ |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
>
> `src` 不得指向 `dest` 的内部缓冲区，若需自插入，请先创建临时副本。

<a id="api-functions-content-editing-dstr_insert_sub_cstr"></a>

#### `dstr_insert_sub_cstr()` [↑](#api-functions-content-editing)

插入一个「C 字符串」的子串到一个「动态字符串」。

```c
dstr_status_t dstr_insert_sub_cstr(
	dstr_adt *dest,
	size_t index,
	const char *src,
	size_t sub_start,
	size_t sub_length
);
```

| 参数         | 类型           | 说明                                                                                                                    |
| ------------ | -------------- | ----------------------------------------------------------------------------------------------------------------------- |
| `dest`       | `dstr_adt *`   | 目标「动态字符串」的指针。<br>*空指针*视为无效参数。                                                                    |
| `index`      | `size_t`       | 插入位置的索引。<br>*越界*视为无效参数。                                                                                |
| `src`        | `const char *` | 源「C 字符串」的指针。<br>为<em>「空字符串」</em>时插入空字符串（什么都不插入），此时忽略 `sub_start` 和 `sub_length`。 |
| `sub_start`  | `size_t`       | 子串的起始索引。<br>*越界*视为无效参数。                                                                                |
| `sub_length` | `size_t`       | 子串的长度。<br>为 _`0`_ 表示到末尾。<br>*越界*视为无效参数。                                                           |

| 返回值          | 说明         |
| --------------- | ------------ |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
>
> `src` 不得指向 `dest` 的内部缓冲区，若需自插入，请先创建临时副本。

<a id="api-functions-content-editing-dstr_insert_sub"></a>

#### `dstr_insert_sub()` [↑](#api-functions-content-editing)

插入一个「动态字符串」的子串到另一个「动态字符串」。

```c
dstr_status_t dstr_insert_sub(
	dstr_adt *dest,
	size_t index,
	const dstr_adt *src,
	size_t sub_start,
	size_t sub_length
);
```

| 参数         | 类型               | 说明                                                                                                                      |
| ------------ | ------------------ | ------------------------------------------------------------------------------------------------------------------------- |
| `dest`       | `dstr_adt *`       | 目标「动态字符串」的指针。<br>*空指针*视为无效参数。                                                                      |
| `index`      | `size_t`           | 插入位置的索引。<br>*越界*视为无效参数。                                                                                  |
| `src`        | `const dstr_adt *` | 源「动态字符串」的指针。<br>为<em>「空字符串」</em>时插入空字符串（什么都不插入），此时忽略 `sub_start` 和 `sub_length`。 |
| `sub_start`  | `size_t`           | 子串的起始索引。<br>*越界*视为无效参数。                                                                                  |
| `sub_length` | `size_t`           | 子串的长度。<br>为 _`0`_ 表示到末尾。<br>*越界*视为无效参数。                                                             |

| 返回值          | 说明         |
| --------------- | ------------ |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
>
> `src` 不得指向 `dest` 的内部缓冲区，若需自插入，请先创建临时副本。

<a id="api-functions-content-editing-dstr_insert_format"></a>

#### `dstr_insert_format()` [↑](#api-functions-content-editing)

格式化插入一个字符串到一个「动态字符串」。

```c
dstr_status_t dstr_insert_format(
	dstr_adt *dest,
	size_t index,
	const char *format,
	...
);
```

| 参数     | 类型           | 说明                                                                                |
| -------- | -------------- | ----------------------------------------------------------------------------------- |
| `dest`   | `dstr_adt *`   | 目标「动态字符串」的指针。<br>*空指针*视为无效参数。                                |
| `index`  | `size_t`       | 插入位置的索引。<br>*越界*视为无效参数。                                            |
| `format` | `const char *` | 格式「C 字符串」的指针。<br>为<em>「空字符串」</em>时插入空字符串（什么都不插入）。 |
| `...`    | —              | 与 `format` 对应的可变参数列表。                                                    |

| 返回值          | 说明         |
| --------------- | ------------ |
| `dstr_status_t` | 全局状态码。 |

> [!WARNING]
>
> `format` 不得指向 `dest` 的内部缓冲区，若需自插入，请先创建临时副本。

<a id="api-functions-content-editing-dstr_insert_vformat"></a>

#### `dstr_insert_vformat()` [↑](#api-functions-content-editing)

格式化插入一个字符串到一个「动态字符串」（`va_list` 版本）。

```c
dstr_status_t dstr_insert_vformat(
	dstr_adt *dest,
	size_t index,
	const char *format,
	va_list args
);
```

| 参数     | 类型           | 说明                                                                                                                                                     |
| -------- | -------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `dest`   | `dstr_adt *`   | 目标「动态字符串」的指针。<br>*空指针*视为无效参数。                                                                                                     |
| `index`  | `size_t`       | 插入位置的索引。<br>*越界*视为无效参数。                                                                                                                 |
| `format` | `const char *` | 格式「C 字符串」的指针。<br>为<em>「空字符串」</em>时插入空字符串（什么都不插入）。                                                                      |
| `args`   | `va_list`      | 已通过 `va_start()` 初始化的 `va_list` 变量，包含与 `format` 对应的可变参数列表信息。<br>该函数不会调用 `va_end()`，调用者需自行管理 `args` 的生命周期。 |

| 返回值          | 说明         |
| --------------- | ------------ |
| `dstr_status_t` | 全局状态码。 |

> [!NOTE]
>
> `args` 可能被此函数读取并消耗，调用后不应再次使用，除非重新 `va_start()` 或使用 `va_copy()` 创建副本以使用此函数。

> [!WARNING]
>
> `format` 不得指向 `dest` 的内部缓冲区，若需自插入，请先创建临时副本。

<a id="api-functions-content-editing-dstr_clear"></a>

#### `dstr_clear()` [↑](#api-functions-content-editing)

清空一个「动态字符串」。

```c
void dstr_clear(
	dstr_adt *dstr
);
```

| 参数   | 类型         | 说明                                                         |
| ------ | ------------ | ------------------------------------------------------------ |
| `dstr` | `dstr_adt *` | 目标「动态字符串」的指针。<br>为*空指针*时，函数会直接返回。 |

> [!NOTE]
>
> 此函数只会使其长度为 0，不会立即释放容量。

<a id="api-functions-content-editing-dstr_remove"></a>

#### `dstr_remove()` [↑](#api-functions-content-editing)

删除一个「动态字符串」的子串。

```c
void dstr_remove(
	dstr_adt *dstr,
	size_t sub_start,
	size_t sub_length
);
```

| 参数         | 类型         | 说明                                                                                                            |
| ------------ | ------------ | --------------------------------------------------------------------------------------------------------------- |
| `dstr`       | `dstr_adt *` | 目标「动态字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回，此时忽略 `sub_start` 和 `sub_length`。 |
| `sub_start`  | `size_t`     | 子串的起始索引。<br>*越界*时，函数会直接返回。                                                                  |
| `sub_length` | `size_t`     | 子串的长度。<br>为 _`0`_ 表示到末尾。<br>*越界*时，函数会直接返回。                                             |

<a id="api-functions-content-editing-dstr_trim"></a>

#### `dstr_trim()` [↑](#api-functions-content-editing)

删除一个「动态字符串」首尾的空白字符或指定字符。

```c
void dstr_trim(
	dstr_adt *dstr,
	const char *trim_chars
);
```

| 参数         | 类型           | 说明                                                                              |
| ------------ | -------------- | --------------------------------------------------------------------------------- |
| `dstr`       | `dstr_adt *`   | 目标「动态字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回。         |
| `trim_chars` | `const char *` | 包含要删除的字符的「C 字符串」的指针。<br>为<em>「空字符串」</em>时删除空白字符。 |

> [!NOTE]
>
> 空白字符判定使用 C 标准库函数 `isspace()`。本库不修改 locale，因此判定结果取决于调用时的 locale。

<a id="api-functions-relation-and-comparison"></a>

### 关系判断与比较 [↑](#api-functions)

此组 API 函数，主要用于判断与比较两个字符串之间的关系。

此组 API 函数对空字符串的处理方式较特殊，遵循字符串理论中的相关定理。

包含的操作及对应 API 函数与说明如下：

<table>
  <thead>
    <tr>
      <th>操作类型</th>
      <th>操作</th>
      <th>API 函数</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td rowspan="12">判断</td>
      <td rowspan="3">前缀判断</td>
      <td><a href="#api-functions-relation-and-comparison-dstr_starts_with_cstr"><code>dstr_starts_with_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-relation-and-comparison-dstr_starts_with"><code>dstr_starts_with()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-relation-and-comparison-cstr_starts_with"><code>cstr_starts_with()</code></a></td>
    </tr>
    <tr>
      <td rowspan="3">后缀判断</td>
      <td><a href="#api-functions-relation-and-comparison-dstr_ends_with_cstr"><code>dstr_ends_with_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-relation-and-comparison-dstr_ends_with"><code>dstr_ends_with()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-relation-and-comparison-cstr_ends_with"><code>cstr_ends_with()</code></a></td>
    </tr>
    <tr>
      <td rowspan="3">包含判断</td>
      <td><a href="#api-functions-relation-and-comparison-dstr_contains_cstr"><code>dstr_contains_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-relation-and-comparison-dstr_contains"><code>dstr_contains()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-relation-and-comparison-cstr_contains"><code>cstr_contains()</code></a></td>
    </tr>
    <tr>
      <td rowspan="3">相等判断</td>
      <td><a href="#api-functions-relation-and-comparison-dstr_equals_cstr"><code>dstr_equals_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-relation-and-comparison-dstr_equals"><code>dstr_equals()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-relation-and-comparison-cstr_equals"><code>cstr_equals()</code></a></td>
    </tr>
    <tr>
      <td rowspan="3">比较</td>
      <td rowspan="3">大小比较</td>
      <td><a href="#api-functions-relation-and-comparison-dstr_compare_cstr"><code>dstr_compare_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-relation-and-comparison-dstr_compare"><code>dstr_compare()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-relation-and-comparison-cstr_compare"><code>cstr_compare()</code></a></td>
    </tr>
  </tbody>
</table>

<a id="api-functions-relation-and-comparison-dstr_starts_with_cstr"></a>

#### `dstr_starts_with_cstr()` [↑](#api-functions-relation-and-comparison)

判断一个「动态字符串」是否以指定「C 字符串」前缀开头。

```c
bool dstr_starts_with_cstr(
	const dstr_adt *dstr,
	const char *prefix
);
```

| 参数     | 类型               | 说明                       |
| -------- | ------------------ | -------------------------- |
| `dstr`   | `const dstr_adt *` | 目标「动态字符串」的指针。 |
| `prefix` | `const char *`     | 前缀「C 字符串」的指针。   |

| 返回值 | 说明                                                                                                                                                     |
| ------ | -------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `bool` | 是前缀则返回 _`true`_，否则返回 _`false`_。<br>如果 `prefix` 为<em>「空字符串」</em>，则一定返回 _`true`_（视<em>「空字符串」</em>是任何字符串的前缀）。 |

<a id="api-functions-relation-and-comparison-dstr_starts_with"></a>

#### `dstr_starts_with()` [↑](#api-functions-relation-and-comparison)

判断一个「动态字符串」是否以指定「动态字符串」前缀开头。

```c
bool dstr_starts_with(
	const dstr_adt *dstr,
	const dstr_adt *prefix
);
```

| 参数     | 类型               | 说明                       |
| -------- | ------------------ | -------------------------- |
| `dstr`   | `const dstr_adt *` | 目标「动态字符串」的指针。 |
| `prefix` | `const dstr_adt *` | 前缀「动态字符串」的指针。 |

| 返回值 | 说明                                                                                                                                                     |
| ------ | -------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `bool` | 是前缀则返回 _`true`_，否则返回 _`false`_。<br>如果 `prefix` 为<em>「空字符串」</em>，则一定返回 _`true`_（视<em>「空字符串」</em>是任何字符串的前缀）。 |

<a id="api-functions-relation-and-comparison-cstr_starts_with"></a>

#### `cstr_starts_with()` [↑](#api-functions-relation-and-comparison)

判断一个「C 字符串」是否以指定「C 字符串」前缀开头。

```c
bool cstr_starts_with(
	const char *cstr,
	const char *prefix
);
```

| 参数     | 类型           | 说明                     |
| -------- | -------------- | ------------------------ |
| `cstr`   | `const char *` | 目标「C 字符串」的指针。 |
| `prefix` | `const char *` | 前缀「C 字符串」的指针。 |

| 返回值 | 说明                                                                                                                                                     |
| ------ | -------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `bool` | 是前缀则返回 _`true`_，否则返回 _`false`_。<br>如果 `prefix` 为<em>「空字符串」</em>，则一定返回 _`true`_（视<em>「空字符串」</em>是任何字符串的前缀）。 |

<a id="api-functions-relation-and-comparison-dstr_ends_with_cstr"></a>

#### `dstr_ends_with_cstr()` [↑](#api-functions-relation-and-comparison)

判断一个「动态字符串」是否以指定「C 字符串」后缀结尾。

```c
bool dstr_ends_with_cstr(
	const dstr_adt *dstr,
	const char *suffix
);
```

| 参数     | 类型               | 说明                       |
| -------- | ------------------ | -------------------------- |
| `dstr`   | `const dstr_adt *` | 目标「动态字符串」的指针。 |
| `suffix` | `const char *`     | 后缀「C 字符串」的指针。   |

| 返回值 | 说明                                                                                                                                                     |
| ------ | -------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `bool` | 是后缀则返回 _`true`_，否则返回 _`false`_。<br>如果 `suffix` 为<em>「空字符串」</em>，则一定返回 _`true`_（视<em>「空字符串」</em>是任何字符串的后缀）。 |

<a id="api-functions-relation-and-comparison-dstr_ends_with"></a>

#### `dstr_ends_with()` [↑](#api-functions-relation-and-comparison)

判断一个「动态字符串」是否以指定「动态字符串」后缀结尾。

```c
bool dstr_ends_with(
	const dstr_adt *dstr,
	const dstr_adt *suffix
);
```

| 参数     | 类型               | 说明                       |
| -------- | ------------------ | -------------------------- |
| `dstr`   | `const dstr_adt *` | 目标「动态字符串」的指针。 |
| `suffix` | `const dstr_adt *` | 后缀「动态字符串」的指针。 |

| 返回值 | 说明                                                                                                                                                     |
| ------ | -------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `bool` | 是后缀则返回 _`true`_，否则返回 _`false`_。<br>如果 `suffix` 为<em>「空字符串」</em>，则一定返回 _`true`_（视<em>「空字符串」</em>是任何字符串的后缀）。 |

<a id="api-functions-relation-and-comparison-cstr_ends_with"></a>

#### `cstr_ends_with()` [↑](#api-functions-relation-and-comparison)

判断一个「C 字符串」是否以指定「C 字符串」后缀结尾。

```c
bool cstr_ends_with(
	const char *cstr,
	const char *suffix
);
```

| 参数     | 类型           | 说明                     |
| -------- | -------------- | ------------------------ |
| `cstr`   | `const char *` | 目标「C 字符串」的指针。 |
| `suffix` | `const char *` | 后缀「C 字符串」的指针。 |

| 返回值 | 说明                                                                                                                                                     |
| ------ | -------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `bool` | 是后缀则返回 _`true`_，否则返回 _`false`_。<br>如果 `suffix` 为<em>「空字符串」</em>，则一定返回 _`true`_（视<em>「空字符串」</em>是任何字符串的后缀）。 |

<a id="api-functions-relation-and-comparison-dstr_contains_cstr"></a>

#### `dstr_contains_cstr()` [↑](#api-functions-relation-and-comparison)

判断一个「动态字符串」是否包含指定子「C 字符串」。

```c
bool dstr_contains_cstr(
	const dstr_adt *dstr,
	const char *sub
);
```

| 参数   | 类型               | 说明                       |
| ------ | ------------------ | -------------------------- |
| `dstr` | `const dstr_adt *` | 目标「动态字符串」的指针。 |
| `sub`  | `const char *`     | 子「C 字符串」的指针。     |

| 返回值 | 说明                                                                                                                                                |
| ------ | --------------------------------------------------------------------------------------------------------------------------------------------------- |
| `bool` | 包含则返回 _`true`_，否则返回 _`false`_。<br>如果 `sub` 为<em>「空字符串」</em>，则一定返回 _`true`_（视<em>「空字符串」</em>是任何字符串的子串）。 |

<a id="api-functions-relation-and-comparison-dstr_contains"></a>

#### `dstr_contains()` [↑](#api-functions-relation-and-comparison)

判断一个「动态字符串」是否包含指定子「动态字符串」。

```c
bool dstr_contains(
	const dstr_adt *dstr,
	const dstr_adt *sub
);
```

| 参数   | 类型               | 说明                       |
| ------ | ------------------ | -------------------------- |
| `dstr` | `const dstr_adt *` | 目标「动态字符串」的指针。 |
| `sub`  | `const dstr_adt *` | 子「动态字符串」的指针。   |

| 返回值 | 说明                                                                                                                                                |
| ------ | --------------------------------------------------------------------------------------------------------------------------------------------------- |
| `bool` | 包含则返回 _`true`_，否则返回 _`false`_。<br>如果 `sub` 为<em>「空字符串」</em>，则一定返回 _`true`_（视<em>「空字符串」</em>是任何字符串的子串）。 |

<a id="api-functions-relation-and-comparison-cstr_contains"></a>

#### `cstr_contains()` [↑](#api-functions-relation-and-comparison)

判断一个「C 字符串」是否包含指定子「C 字符串」。

```c
bool cstr_contains(
	const char *cstr,
	const char *sub
);
```

| 参数   | 类型           | 说明                     |
| ------ | -------------- | ------------------------ |
| `cstr` | `const char *` | 目标「C 字符串」的指针。 |
| `sub`  | `const char *` | 子「C 字符串」的指针。   |

| 返回值 | 说明                                                                                                                                                |
| ------ | --------------------------------------------------------------------------------------------------------------------------------------------------- |
| `bool` | 包含则返回 _`true`_，否则返回 _`false`_。<br>如果 `sub` 为<em>「空字符串」</em>，则一定返回 _`true`_（视<em>「空字符串」</em>是任何字符串的子串）。 |

<a id="api-functions-relation-and-comparison-dstr_equals_cstr"></a>

#### `dstr_equals_cstr()` [↑](#api-functions-relation-and-comparison)

判断一个「动态字符串」是否与一个「C 字符串」相等。

```c
bool dstr_equals_cstr(
	const dstr_adt *lhs,
	const char *rhs
);
```

| 参数  | 类型               | 说明                   |
| ----- | ------------------ | ---------------------- |
| `lhs` | `const dstr_adt *` | 「动态字符串」的指针。 |
| `rhs` | `const char *`     | 「C 字符串」的指针。   |

| 返回值 | 说明                                                                                          |
| ------ | --------------------------------------------------------------------------------------------- |
| `bool` | 相等则返回 _`true`_，否则返回 _`false`_。<br>两者同为<em>「空字符串」</em>时，返回 _`true`_。 |

<a id="api-functions-relation-and-comparison-dstr_equals"></a>

#### `dstr_equals()` [↑](#api-functions-relation-and-comparison)

判断两个「动态字符串」是否相等。

```c
bool dstr_equals(
	const dstr_adt *lhs,
	const dstr_adt *rhs
);
```

| 参数  | 类型               | 说明                         |
| ----- | ------------------ | ---------------------------- |
| `lhs` | `const dstr_adt *` | 第一个「动态字符串」的指针。 |
| `rhs` | `const dstr_adt *` | 第二个「动态字符串」的指针。 |

| 返回值 | 说明                                                                                          |
| ------ | --------------------------------------------------------------------------------------------- |
| `bool` | 相等则返回 _`true`_，否则返回 _`false`_。<br>两者同为<em>「空字符串」</em>时，返回 _`true`_。 |

<a id="api-functions-relation-and-comparison-cstr_equals"></a>

#### `cstr_equals()` [↑](#api-functions-relation-and-comparison)

判断两个「C 字符串」是否相等。

```c
bool cstr_equals(
	const char *lhs,
	const char *rhs
);
```

| 参数  | 类型           | 说明                       |
| ----- | -------------- | -------------------------- |
| `lhs` | `const char *` | 第一个「C 字符串」的指针。 |
| `rhs` | `const char *` | 第二个「C 字符串」的指针。 |

| 返回值 | 说明                                                                                          |
| ------ | --------------------------------------------------------------------------------------------- |
| `bool` | 相等则返回 _`true`_，否则返回 _`false`_。<br>两者同为<em>「空字符串」</em>时，返回 _`true`_。 |

<a id="api-functions-relation-and-comparison-dstr_compare_cstr"></a>

#### `dstr_compare_cstr()` [↑](#api-functions-relation-and-comparison)

比较一个「动态字符串」与一个「C 字符串」。

```c
int dstr_compare_cstr(
	const dstr_adt *lhs,
	const char *rhs
);
```

| 参数  | 类型               | 说明                   |
| ----- | ------------------ | ---------------------- |
| `lhs` | `const dstr_adt *` | 「动态字符串」的指针。 |
| `rhs` | `const char *`     | 「C 字符串」的指针。   |

| 返回值 | 说明                                                                                                                                                                                                                     |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `int`  | 两者相等返回 _`0`_，前者大于后者返回*正值*，前者小于后者返回*负值*。<br>两者同为<em>「空字符串」</em>时，返回 _`0`_。<br>两者只有一者为<em>「空字符串」</em>时，为<em>「空字符串」</em>者小于非<em>「空字符串」</em>者。 |

<a id="api-functions-relation-and-comparison-dstr_compare"></a>

#### `dstr_compare()` [↑](#api-functions-relation-and-comparison)

比较两个「动态字符串」。

```c
int dstr_compare(
	const dstr_adt *lhs,
	const dstr_adt *rhs
);
```

| 参数  | 类型               | 说明                         |
| ----- | ------------------ | ---------------------------- |
| `lhs` | `const dstr_adt *` | 第一个「动态字符串」的指针。 |
| `rhs` | `const dstr_adt *` | 第二个「动态字符串」的指针。 |

| 返回值 | 说明                                                                                                                                                                                                                     |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `int`  | 两者相等返回 _`0`_，前者大于后者返回*正值*，前者小于后者返回*负值*。<br>两者同为<em>「空字符串」</em>时，返回 _`0`_。<br>两者只有一者为<em>「空字符串」</em>时，为<em>「空字符串」</em>者小于非<em>「空字符串」</em>者。 |

<a id="api-functions-relation-and-comparison-cstr_compare"></a>

#### `cstr_compare()` [↑](#api-functions-relation-and-comparison)

比较两个「C 字符串」。

```c
int cstr_compare(
	const char *lhs,
	const char *rhs
);
```

| 参数  | 类型           | 说明                       |
| ----- | -------------- | -------------------------- |
| `lhs` | `const char *` | 第一个「C 字符串」的指针。 |
| `rhs` | `const char *` | 第二个「C 字符串」的指针。 |

| 返回值 | 说明                                                                                                                                                                                                                     |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `int`  | 两者相等返回 _`0`_，前者大于后者返回*正值*，前者小于后者返回*负值*。<br>两者同为<em>「空字符串」</em>时，返回 _`0`_。<br>两者只有一者为<em>「空字符串」</em>时，为<em>「空字符串」</em>者小于非<em>「空字符串」</em>者。 |

<a id="api-functions-find-count-and-replace"></a>

### 查找、统计与替换 [↑](#api-functions)

此组 API 函数，主要用于对一个「动态字符串」进行查找、统计与替换。

包含的操作及对应 API 函数与说明如下：

<table>
  <thead>
    <tr>
      <th>操作类型</th>
      <th>操作</th>
      <th>API 函数</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td rowspan="9">查找</td>
      <td rowspan="3">首次出现位置</td>
      <td><a href="#api-functions-find-count-and-replace-dstr_find_cstr"><code>dstr_find_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-find-count-and-replace-dstr_find"><code>dstr_find()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-find-count-and-replace-cstr_find"><code>cstr_find()</code></a></td>
    </tr>
    <tr>
      <td rowspan="3">第 n 次出现位置</td>
      <td><a href="#api-functions-find-count-and-replace-dstr_find_nth_cstr"><code>dstr_find_nth_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-find-count-and-replace-dstr_find_nth"><code>dstr_find_nth()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-find-count-and-replace-cstr_find_nth"><code>cstr_find_nth()</code></a></td>
    </tr>
    <tr>
      <td rowspan="3">前 n 次全部位置</td>
      <td><a href="#api-functions-find-count-and-replace-dstr_find_indexes_cstr"><code>dstr_find_indexes_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-find-count-and-replace-dstr_find_indexes"><code>dstr_find_indexes()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-find-count-and-replace-cstr_find_indexes"><code>cstr_find_indexes()</code></a></td>
    </tr>
    <tr>
      <td rowspan="3">统计</td>
      <td rowspan="3">出现次数</td>
      <td><a href="#api-functions-find-count-and-replace-dstr_count_cstr"><code>dstr_count_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-find-count-and-replace-dstr_count"><code>dstr_count()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-find-count-and-replace-cstr_count"><code>cstr_count()</code></a></td>
    </tr>
    <tr>
      <td rowspan="4">替换</td>
      <td rowspan="2">替换 n 次</td>
      <td><a href="#api-functions-find-count-and-replace-dstr_replace_cstr"><code>dstr_replace_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-find-count-and-replace-dstr_replace"><code>dstr_replace()</code></a></td>
    </tr>
    <tr>
      <td rowspan="2">替换第 n 次</td>
      <td><a href="#api-functions-find-count-and-replace-dstr_replace_nth_cstr"><code>dstr_replace_nth_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-find-count-and-replace-dstr_replace_nth"><code>dstr_replace_nth()</code></a></td>
    </tr>
  </tbody>
</table>

<a id="api-functions-find-count-and-replace-dstr_find_cstr"></a>

#### `dstr_find_cstr()` [↑](#api-functions-find-count-and-replace)

查找一个「动态字符串」中指定子「C 字符串」首次出现的位置。

```c
bool dstr_find_cstr(
	const dstr_adt *dstr,
	const char *sub,
	size_t *out_index,
	dstr_direction_t direction
);
```

| 参数        | 类型               | 说明                                                                                |
| ----------- | ------------------ | ----------------------------------------------------------------------------------- |
| `dstr`      | `const dstr_adt *` | 目标「动态字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回 _`false`_。 |
| `sub`       | `const char *`     | 子「C 字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回 _`false`_。     |
| `out_index` | `size_t *`         | 存储查找结果（位置索引）的 `size_t` 变量的指针。<br>为*空指针*时不写入。            |
| `direction` | `dstr_direction_t` | 查找方向。                                                                          |

| 返回值 | 说明                                      |
| ------ | ----------------------------------------- |
| `bool` | 找到则返回 _`true`_，否则返回 _`false`_。 |

<a id="api-functions-find-count-and-replace-dstr_find"></a>

#### `dstr_find()` [↑](#api-functions-find-count-and-replace)

查找一个「动态字符串」中指定子「动态字符串」首次出现的位置。

```c
bool dstr_find(
	const dstr_adt *dstr,
	const dstr_adt *sub,
	size_t *out_index,
	dstr_direction_t direction
);
```

| 参数        | 类型               | 说明                                                                                |
| ----------- | ------------------ | ----------------------------------------------------------------------------------- |
| `dstr`      | `const dstr_adt *` | 目标「动态字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回 _`false`_。 |
| `sub`       | `const dstr_adt *` | 子「动态字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回 _`false`_。   |
| `out_index` | `size_t *`         | 存储查找结果（位置索引）的 `size_t` 变量的指针。<br>为*空指针*时不写入。            |
| `direction` | `dstr_direction_t` | 查找方向。                                                                          |

| 返回值 | 说明                                      |
| ------ | ----------------------------------------- |
| `bool` | 找到则返回 _`true`_，否则返回 _`false`_。 |

<a id="api-functions-find-count-and-replace-cstr_find"></a>

#### `cstr_find()` [↑](#api-functions-find-count-and-replace)

查找一个「C 字符串」中指定子「C 字符串」首次出现的位置。

```c
bool cstr_find(
	const char *cstr,
	const char *sub,
	size_t *out_index,
	dstr_direction_t direction
);
```

| 参数        | 类型               | 说明                                                                              |
| ----------- | ------------------ | --------------------------------------------------------------------------------- |
| `cstr`      | `const char *`     | 目标「C 字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回 _`false`_。 |
| `sub`       | `const char *`     | 子「C 字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回 _`false`_。   |
| `out_index` | `size_t *`         | 存储查找结果（位置索引）的 `size_t` 变量的指针。<br>为*空指针*时不写入。          |
| `direction` | `dstr_direction_t` | 查找方向。                                                                        |

| 返回值 | 说明                                      |
| ------ | ----------------------------------------- |
| `bool` | 找到则返回 _`true`_，否则返回 _`false`_。 |

<a id="api-functions-find-count-and-replace-dstr_find_nth_cstr"></a>

#### `dstr_find_nth_cstr()` [↑](#api-functions-find-count-and-replace)

查找一个「动态字符串」中指定子「C 字符串」第 n 次出现的位置。

```c
bool dstr_find_nth_cstr(
	const dstr_adt *dstr,
	const char *sub,
	size_t *out_index,
	dstr_direction_t direction,
	size_t n
);
```

| 参数        | 类型               | 说明                                                                                                 |
| ----------- | ------------------ | ---------------------------------------------------------------------------------------------------- |
| `dstr`      | `const dstr_adt *` | 目标「动态字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回 _`false`_。                  |
| `sub`       | `const char *`     | 子「C 字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回 _`false`_。                      |
| `out_index` | `size_t *`         | 存储查找结果（位置索引）的 `size_t` 变量的指针。<br>为*空指针*时不写入。                             |
| `direction` | `dstr_direction_t` | 查找方向。                                                                                           |
| `n`         | `size_t`           | 出现的次序。<br>从 _`1`_ 开始。<br>为 _`0`_ 表示最后一次。<br>如果大于实际出现次数，则视为最后一次。 |

| 返回值 | 说明                                      |
| ------ | ----------------------------------------- |
| `bool` | 找到则返回 _`true`_，否则返回 _`false`_。 |

<a id="api-functions-find-count-and-replace-dstr_find_nth"></a>

#### `dstr_find_nth()` [↑](#api-functions-find-count-and-replace)

查找一个「动态字符串」中指定子「动态字符串」第 n 次出现的位置。

```c
bool dstr_find_nth(
	const dstr_adt *dstr,
	const dstr_adt *sub,
	size_t *out_index,
	dstr_direction_t direction,
	size_t n
);
```

| 参数        | 类型               | 说明                                                                                                 |
| ----------- | ------------------ | ---------------------------------------------------------------------------------------------------- |
| `dstr`      | `const dstr_adt *` | 目标「动态字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回 _`false`_。                  |
| `sub`       | `const dstr_adt *` | 子「动态字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回 _`false`_。                    |
| `out_index` | `size_t *`         | 存储查找结果（位置索引）的 `size_t` 变量的指针。<br>为*空指针*时不写入。                             |
| `direction` | `dstr_direction_t` | 查找方向。                                                                                           |
| `n`         | `size_t`           | 出现的次序。<br>从 _`1`_ 开始。<br>为 _`0`_ 表示最后一次。<br>如果大于实际出现次数，则视为最后一次。 |

| 返回值 | 说明                                      |
| ------ | ----------------------------------------- |
| `bool` | 找到则返回 _`true`_，否则返回 _`false`_。 |

<a id="api-functions-find-count-and-replace-cstr_find_nth"></a>

#### `cstr_find_nth()` [↑](#api-functions-find-count-and-replace)

查找一个「C 字符串」中指定子「C 字符串」第 n 次出现的位置。

```c
bool cstr_find_nth(
	const char *cstr,
	const char *sub,
	size_t *out_index,
	dstr_direction_t direction,
	size_t n
);
```

| 参数        | 类型               | 说明                                                                                                 |
| ----------- | ------------------ | ---------------------------------------------------------------------------------------------------- |
| `cstr`      | `const char *`     | 目标「C 字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回 _`false`_。                    |
| `sub`       | `const char *`     | 子「C 字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回 _`false`_。                      |
| `out_index` | `size_t *`         | 存储查找结果（位置索引）的 `size_t` 变量的指针。<br>为*空指针*时不写入。                             |
| `direction` | `dstr_direction_t` | 查找方向。                                                                                           |
| `n`         | `size_t`           | 出现的次序。<br>从 _`1`_ 开始。<br>为 _`0`_ 表示最后一次。<br>如果大于实际出现次数，则视为最后一次。 |

| 返回值 | 说明                                      |
| ------ | ----------------------------------------- |
| `bool` | 找到则返回 _`true`_，否则返回 _`false`_。 |

<a id="api-functions-find-count-and-replace-dstr_find_indexes_cstr"></a>

#### `dstr_find_indexes_cstr()` [↑](#api-functions-find-count-and-replace)

查找一个「动态字符串」中指定子「C 字符串」前 n 次出现的位置。

```c
size_t dstr_find_indexes_cstr(
	const dstr_adt *dstr,
	const char *sub,
	size_t *out_indexes,
	dstr_direction_t direction,
	size_t n
);
```

| 参数          | 类型               | 说明                                                                                                 |
| ------------- | ------------------ | ---------------------------------------------------------------------------------------------------- |
| `dstr`        | `const dstr_adt *` | 目标「动态字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回 _`0`_。                      |
| `sub`         | `const char *`     | 子「C 字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回 _`0`_。                          |
| `out_indexes` | `size_t *`         | 存储查找结果（位置索引）的 `size_t` 数组的指针。<br>为*空指针*时不写入。<br>请自行确保数组容量够大。 |
| `direction`   | `dstr_direction_t` | 查找方向。                                                                                           |
| `n`           | `size_t`           | 查找的次数。<br>从 _`1`_ 开始。<br>为 _`0`_ 表示查找全部。<br>如果大于实际出现次数，也会查找全部。   |

| 返回值   | 说明                                                                             |
| -------- | -------------------------------------------------------------------------------- |
| `size_t` | 截止第 `n` 次，实际出现的次数，也表示实际向 `out_indexes` 数组中写入的元素个数。 |

<a id="api-functions-find-count-and-replace-dstr_find_indexes"></a>

#### `dstr_find_indexes()` [↑](#api-functions-find-count-and-replace)

查找一个「动态字符串」中指定子「动态字符串」前 n 次出现的位置。

```c
size_t dstr_find_indexes(
	const dstr_adt *dstr,
	const dstr_adt *sub,
	size_t *out_indexes,
	dstr_direction_t direction,
	size_t n
);
```

| 参数          | 类型               | 说明                                                                                                 |
| ------------- | ------------------ | ---------------------------------------------------------------------------------------------------- |
| `dstr`        | `const dstr_adt *` | 目标「动态字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回 _`0`_。                      |
| `sub`         | `const dstr_adt *` | 子「动态字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回 _`0`_。                        |
| `out_indexes` | `size_t *`         | 存储查找结果（位置索引）的 `size_t` 数组的指针。<br>为*空指针*时不写入。<br>请自行确保数组容量够大。 |
| `direction`   | `dstr_direction_t` | 查找方向。                                                                                           |
| `n`           | `size_t`           | 查找的次数。<br>从 _`1`_ 开始。<br>为 _`0`_ 表示查找全部。<br>如果大于实际出现次数，也会查找全部。   |

| 返回值   | 说明                                                                             |
| -------- | -------------------------------------------------------------------------------- |
| `size_t` | 截止第 `n` 次，实际出现的次数，也表示实际向 `out_indexes` 数组中写入的元素个数。 |

<a id="api-functions-find-count-and-replace-cstr_find_indexes"></a>

#### `cstr_find_indexes()` [↑](#api-functions-find-count-and-replace)

查找一个「C 字符串」中指定子「C 字符串」前 n 次出现的位置。

```c
size_t cstr_find_indexes(
	const char *cstr,
	const char *sub,
	size_t *out_indexes,
	dstr_direction_t direction,
	size_t n
);
```

| 参数          | 类型               | 说明                                                                                                 |
| ------------- | ------------------ | ---------------------------------------------------------------------------------------------------- |
| `cstr`        | `const char *`     | 目标「C 字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回 _`0`_。                        |
| `sub`         | `const char *`     | 子「C 字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回 _`0`_。                          |
| `out_indexes` | `size_t *`         | 存储查找结果（位置索引）的 `size_t` 数组的指针。<br>为*空指针*时不写入。<br>请自行确保数组容量够大。 |
| `direction`   | `dstr_direction_t` | 查找方向。                                                                                           |
| `n`           | `size_t`           | 查找的次数。<br>从 _`1`_ 开始。<br>为 _`0`_ 表示查找全部。<br>如果大于实际出现次数，也会查找全部。   |

| 返回值   | 说明                                                                             |
| -------- | -------------------------------------------------------------------------------- |
| `size_t` | 截止第 `n` 次，实际出现的次数，也表示实际向 `out_indexes` 数组中写入的元素个数。 |

<a id="api-functions-find-count-and-replace-dstr_count_cstr"></a>

#### `dstr_count_cstr()` [↑](#api-functions-find-count-and-replace)

统计一个「动态字符串」中指定子「C 字符串」出现的次数。

```c
size_t dstr_count_cstr(
	const dstr_adt *dstr,
	const char *sub
);
```

| 参数   | 类型               | 说明                                                                            |
| ------ | ------------------ | ------------------------------------------------------------------------------- |
| `dstr` | `const dstr_adt *` | 目标「动态字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回 _`0`_。 |
| `sub`  | `const char *`     | 子「C 字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回 _`0`_。     |

| 返回值   | 说明         |
| -------- | ------------ |
| `size_t` | 出现的次数。 |

<a id="api-functions-find-count-and-replace-dstr_count"></a>

#### `dstr_count()` [↑](#api-functions-find-count-and-replace)

统计一个「动态字符串」中指定子「动态字符串」出现的次数。

```c
size_t dstr_count(
	const dstr_adt *dstr,
	const dstr_adt *sub
);
```

| 参数   | 类型               | 说明                                                                            |
| ------ | ------------------ | ------------------------------------------------------------------------------- |
| `dstr` | `const dstr_adt *` | 目标「动态字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回 _`0`_。 |
| `sub`  | `const dstr_adt *` | 子「动态字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回 _`0`_。   |

| 返回值   | 说明         |
| -------- | ------------ |
| `size_t` | 出现的次数。 |

<a id="api-functions-find-count-and-replace-cstr_count"></a>

#### `cstr_count()` [↑](#api-functions-find-count-and-replace)

统计一个「C 字符串」中指定子「C 字符串」出现的次数。

```c
size_t cstr_count(
	const char *cstr,
	const char *sub
);
```

| 参数   | 类型           | 说明                                                                          |
| ------ | -------------- | ----------------------------------------------------------------------------- |
| `cstr` | `const char *` | 目标「C 字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回 _`0`_。 |
| `sub`  | `const char *` | 子「C 字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回 _`0`_。   |

| 返回值   | 说明         |
| -------- | ------------ |
| `size_t` | 出现的次数。 |

<a id="api-functions-find-count-and-replace-dstr_replace_cstr"></a>

#### `dstr_replace_cstr()` [↑](#api-functions-find-count-and-replace)

替换一个「动态字符串」中指定旧「C 字符串」为指定新「C 字符串」n 次。

```c
dstr_status_t dstr_replace_cstr(
	dstr_adt *dstr,
	const char *old_str,
	const char *new_str,
	dstr_direction_t direction,
	size_t n
);
```

| 参数        | 类型               | 说明                                                                                                                                                          |
| ----------- | ------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `dstr`      | `dstr_adt *`       | 目标「动态字符串」的指针。<br><em>「空字符串」</em>视为无效参数。                                                                                             |
| `old_str`   | `const char *`     | 旧「C 字符串」的指针。<br><em>「空字符串」</em>视为无效参数。<br>如果在 `dstr` 中一次都没有出现或出现次数不足 `n` 次（`n` 不为 _`0`_ 时）时，则视为无效参数。 |
| `new_str`   | `const char *`     | 新「C 字符串」的指针。                                                                                                                                        |
| `direction` | `dstr_direction_t` | 替换方向。                                                                                                                                                    |
| `n`         | `size_t`           | 替换的次数。<br>为 _`0`_ 表示替换所有。                                                                                                                       |

| 返回值          | 说明         |
| --------------- | ------------ |
| `dstr_status_t` | 全局状态码。 |

<a id="api-functions-find-count-and-replace-dstr_replace"></a>

#### `dstr_replace()` [↑](#api-functions-find-count-and-replace)

替换一个「动态字符串」中指定旧「动态字符串」为指定新「动态字符串」n 次。

```c
dstr_status_t dstr_replace(
	dstr_adt *dstr,
	const dstr_adt *old_str,
	const dstr_adt *new_str,
	dstr_direction_t direction,
	size_t n
);
```

| 参数        | 类型               | 说明                                                                                                                                                            |
| ----------- | ------------------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `dstr`      | `dstr_adt *`       | 目标「动态字符串」的指针。<br><em>「空字符串」</em>视为无效参数。                                                                                               |
| `old_str`   | `const dstr_adt *` | 旧「动态字符串」的指针。<br><em>「空字符串」</em>视为无效参数。<br>如果在 `dstr` 中一次都没有出现或出现次数不足 `n` 次（`n` 不为 _`0`_ 时）时，则视为无效参数。 |
| `new_str`   | `const dstr_adt *` | 新「动态字符串」的指针。                                                                                                                                        |
| `direction` | `dstr_direction_t` | 替换方向。                                                                                                                                                      |
| `n`         | `size_t`           | 替换的次数。<br>为 _`0`_ 表示替换所有。                                                                                                                         |

| 返回值          | 说明         |
| --------------- | ------------ |
| `dstr_status_t` | 全局状态码。 |

<a id="api-functions-find-count-and-replace-dstr_replace_nth_cstr"></a>

#### `dstr_replace_nth_cstr()` [↑](#api-functions-find-count-and-replace)

替换一个「动态字符串」中指定旧「C 字符串」第 n 次为指定新「C 字符串」。

```c
dstr_status_t dstr_replace_nth_cstr(
	dstr_adt *dstr,
	const char *old_str,
	const char *new_str,
	dstr_direction_t direction,
	size_t n
);
```

| 参数        | 类型               | 说明                                                                                                                                                          |
| ----------- | ------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `dstr`      | `dstr_adt *`       | 目标「动态字符串」的指针。<br><em>「空字符串」</em>视为无效参数。                                                                                             |
| `old_str`   | `const char *`     | 旧「C 字符串」的指针。<br><em>「空字符串」</em>视为无效参数。<br>如果在 `dstr` 中一次都没有出现或出现次数不足 `n` 次（`n` 不为 _`0`_ 时）时，则视为无效参数。 |
| `new_str`   | `const char *`     | 新「C 字符串」的指针。                                                                                                                                        |
| `direction` | `dstr_direction_t` | 替换方向。                                                                                                                                                    |
| `n`         | `size_t`           | 替换的次序。<br>从 _`1`_ 开始。<br>为 _`0`_ 表示最后一次。                                                                                                    |

| 返回值          | 说明         |
| --------------- | ------------ |
| `dstr_status_t` | 全局状态码。 |

<a id="api-functions-find-count-and-replace-dstr_replace_nth"></a>

#### `dstr_replace_nth()` [↑](#api-functions-find-count-and-replace)

替换一个「动态字符串」中指定旧「动态字符串」第 n 次为指定新「动态字符串」。

```c
dstr_status_t dstr_replace_nth(
	dstr_adt *dstr,
	const dstr_adt *old_str,
	const dstr_adt *new_str,
	dstr_direction_t direction,
	size_t n
);
```

| 参数        | 类型               | 说明                                                                                                                                                            |
| ----------- | ------------------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `dstr`      | `dstr_adt *`       | 目标「动态字符串」的指针。<br><em>「空字符串」</em>视为无效参数。                                                                                               |
| `old_str`   | `const dstr_adt *` | 旧「动态字符串」的指针。<br><em>「空字符串」</em>视为无效参数。<br>如果在 `dstr` 中一次都没有出现或出现次数不足 `n` 次（`n` 不为 _`0`_ 时）时，则视为无效参数。 |
| `new_str`   | `const dstr_adt *` | 新「动态字符串」的指针。                                                                                                                                        |
| `direction` | `dstr_direction_t` | 替换方向。                                                                                                                                                      |
| `n`         | `size_t`           | 替换的次序。<br>从 _`1`_ 开始。<br>为 _`0`_ 表示最后一次。                                                                                                      |

| 返回值          | 说明         |
| --------------- | ------------ |
| `dstr_status_t` | 全局状态码。 |

<a id="api-functions-split-and-join"></a>

### 分隔与合并 [↑](#api-functions)

此组 API 函数，主要用于分隔一个字符串与合并多个字符串。

包含的操作及对应 API 函数与说明如下：

<table>
  <thead>
    <tr>
      <th>操作类型</th>
      <th>操作</th>
      <th>API 函数</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td rowspan="3">分隔</td>
      <td rowspan="3">分隔</td>
      <td><a href="#api-functions-split-and-join-dstr_split_cstr"><code>dstr_split_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-split-and-join-dstr_split"><code>dstr_split()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-split-and-join-cstr_split"><code>cstr_split()</code></a></td>
    </tr>
    <tr>
      <td rowspan="3">合并</td>
      <td rowspan="3">合并</td>
      <td><a href="#api-functions-split-and-join-dstr_join_cstr"><code>dstr_join_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-split-and-join-dstr_join"><code>dstr_join()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-split-and-join-cstr_join"><code>cstr_join()</code></a></td>
    </tr>
  </tbody>
</table>

<a id="api-functions-split-and-join-dstr_split_cstr"></a>

#### `dstr_split_cstr()` [↑](#api-functions-split-and-join)

分隔一个「C 字符串」为多个「动态字符串」。

```c
dstr_adt **dstr_split_cstr(
	const char *cstr,
	const char *separator,
	size_t *out_dstr_count
);
```

| 参数             | 类型           | 说明                                                                                                                           |
| ---------------- | -------------- | ------------------------------------------------------------------------------------------------------------------------------ |
| `cstr`           | `const char *` | 目标「C 字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回*空指针*。                                                |
| `separator`      | `const char *` | 分隔「C 字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回*空指针*。                                                |
| `out_dstr_count` | `size_t *`     | 存储分隔后的「动态字符串」的个数的 `size_t` 变量的指针。<br>为*空指针*时，函数会直接返回*空指针*。<br>仅在函数成功执行时写入。 |

| 返回值        | 说明                                                         |
| ------------- | ------------------------------------------------------------ |
| `dstr_adt **` | 分隔后的「动态字符串」数组的指针。<br>分隔失败返回*空指针*。 |

> [!NOTE]
>
> 分隔时，如果两个分隔串之间，或分隔串与首尾边界之间的子串长度为 _0_，将以*空指针*（而不是<em>空「动态字符串」</em>）形式存储在数组中。

> [!IMPORTANT]
>
> 返回值指向堆内存，其中又可能有指向其他堆内存的指针。
>
> 因此，释放时，请先手动依次调用 `dstr_destroy()` 释放每个元素，然后手动调用 `free()` 释放数组本身。

<a id="api-functions-split-and-join-dstr_split"></a>

#### `dstr_split()` [↑](#api-functions-split-and-join)

分隔一个「动态字符串」为多个「动态字符串」。

```c
dstr_adt **dstr_split(
	const dstr_adt *dstr,
	const dstr_adt *separator,
	size_t *out_dstr_count
);
```

| 参数             | 类型               | 说明                                                                                                                           |
| ---------------- | ------------------ | ------------------------------------------------------------------------------------------------------------------------------ |
| `dstr`           | `const dstr_adt *` | 目标「动态字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回*空指针*。                                              |
| `separator`      | `const dstr_adt *` | 分隔「动态字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回*空指针*。                                              |
| `out_dstr_count` | `size_t *`         | 存储分隔后的「动态字符串」的个数的 `size_t` 变量的指针。<br>为*空指针*时，函数会直接返回*空指针*。<br>仅在函数成功执行时写入。 |

| 返回值        | 说明                                                         |
| ------------- | ------------------------------------------------------------ |
| `dstr_adt **` | 分隔后的「动态字符串」数组的指针。<br>分隔失败返回*空指针*。 |

> [!NOTE]
>
> 分隔时，如果两个分隔串之间，或分隔串与首尾边界之间的子串长度为 _0_，将以*空指针*（而不是<em>空「动态字符串」</em>）形式存储在数组中。

> [!IMPORTANT]
>
> 返回值指向堆内存，其中又可能有指向其他堆内存的指针。
>
> 因此，释放时，请先手动依次调用 `dstr_destroy()` 释放每个元素，然后手动调用 `free()` 释放数组本身。

<a id="api-functions-split-and-join-cstr_split"></a>

#### `cstr_split()` [↑](#api-functions-split-and-join)

分隔一个「C 字符串」为多个「C 字符串」。

```c
char **cstr_split(
	const char *cstr,
	const char *separator,
	size_t *out_cstr_count
);
```

| 参数             | 类型           | 说明                                                                                                                         |
| ---------------- | -------------- | ---------------------------------------------------------------------------------------------------------------------------- |
| `cstr`           | `const char *` | 目标「C 字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回*空指针*。                                              |
| `separator`      | `const char *` | 分隔「C 字符串」的指针。<br>为<em>「空字符串」</em>时，函数会直接返回*空指针*。                                              |
| `out_cstr_count` | `size_t *`     | 存储分隔后的「C 字符串」的个数的 `size_t` 变量的指针。<br>为*空指针*时，函数会直接返回*空指针*。<br>仅在函数成功执行时写入。 |

| 返回值    | 说明                                                       |
| --------- | ---------------------------------------------------------- |
| `char **` | 分隔后的「C 字符串」数组的指针。<br>分隔失败返回*空指针*。 |

> [!NOTE]
>
> 分隔时，如果两个分隔串之间，或分隔串与首尾边界之间的子串长度为 _0_，将以*空指针*（而不是<em>空「C 字符串」</em>）形式存储在数组中。

> [!IMPORTANT]
>
> 返回值指向堆内存，其中又可能有指向其他堆内存的指针。
>
> 因此，释放时，请先手动依次调用 `free()` 释放每个元素，然后手动调用 `free()` 释放数组本身。

<a id="api-functions-split-and-join-dstr_join_cstr"></a>

#### `dstr_join_cstr()` [↑](#api-functions-split-and-join)

合并多个「C 字符串」为一个「动态字符串」。

```c
dstr_adt *dstr_join_cstr(
	const char *const *cstrs,
	size_t cstr_count,
	const char *separator
);
```

| 参数         | 类型                  | 说明                                                                                                       |
| ------------ | --------------------- | ---------------------------------------------------------------------------------------------------------- |
| `cstrs`      | `const char *const *` | 源「C 字符串」数组的指针。<br>为*空指针*时，函数会直接返回*空指针*。<br>数组中的*空指针*视为「空字符串」。 |
| `cstr_count` | `size_t`              | 源「C 字符串」的个数。<br>为 _`0`_ 时，函数会直接返回*空指针*。                                            |
| `separator`  | `const char *`        | 分隔「C 字符串」的指针。                                                                                   |

| 返回值       | 说明                                                     |
| ------------ | -------------------------------------------------------- |
| `dstr_adt *` | 合并后的「动态字符串」的指针。<br>合并失败返回*空指针*。 |

> [!IMPORTANT]
>
> 返回值指向堆内存，请手动调用 `dstr_destroy()` 释放。

<a id="api-functions-split-and-join-dstr_join"></a>

#### `dstr_join()` [↑](#api-functions-split-and-join)

合并多个「动态字符串」为一个「动态字符串」。

```c
dstr_adt *dstr_join(
	const dstr_adt *const *dstrs,
	size_t dstr_count,
	const dstr_adt *separator
);
```

| 参数         | 类型                      | 说明                                                                                                         |
| ------------ | ------------------------- | ------------------------------------------------------------------------------------------------------------ |
| `dstrs`      | `const dstr_adt *const *` | 源「动态字符串」数组的指针。<br>为*空指针*时，函数会直接返回*空指针*。<br>数组中的*空指针*视为「空字符串」。 |
| `dstr_count` | `size_t`                  | 源「动态字符串」的个数。<br>为 _`0`_ 时，函数会直接返回*空指针*。                                            |
| `separator`  | `const dstr_adt *`        | 分隔「动态字符串」的指针。                                                                                   |

| 返回值       | 说明                                                     |
| ------------ | -------------------------------------------------------- |
| `dstr_adt *` | 合并后的「动态字符串」的指针。<br>合并失败返回*空指针*。 |

> [!IMPORTANT]
>
> 返回值指向堆内存，请手动调用 `dstr_destroy()` 释放。

<a id="api-functions-split-and-join-cstr_join"></a>

#### `cstr_join()` [↑](#api-functions-split-and-join)

合并多个「C 字符串」为一个「C 字符串」。

```c
char *cstr_join(
	const char *const *cstrs,
	size_t cstr_count,
	const char *separator
);
```

| 参数         | 类型                  | 说明                                                                                                       |
| ------------ | --------------------- | ---------------------------------------------------------------------------------------------------------- |
| `cstrs`      | `const char *const *` | 源「C 字符串」数组的指针。<br>为*空指针*时，函数会直接返回*空指针*。<br>数组中的*空指针*视为「空字符串」。 |
| `cstr_count` | `size_t`              | 源「C 字符串」的个数。<br>为 _`0`_ 时，函数会直接返回*空指针*。                                            |
| `separator`  | `const char *`        | 分隔「C 字符串」的指针。                                                                                   |

| 返回值   | 说明                                                   |
| -------- | ------------------------------------------------------ |
| `char *` | 合并后的「C 字符串」的指针。<br>合并失败返回*空指针*。 |

> [!IMPORTANT]
>
> 返回值指向堆内存，请手动调用 `free()` 释放。
