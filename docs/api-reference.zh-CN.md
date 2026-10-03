# 动态字符串 API 参考

## 创建与销毁

### `dstr_create()`

创建一个「动态字符串」。

```c
dstr_adt *dstr_create(const char *cstr);
```

**参数**

- `cstr`

  源「C 字符串」的指针。

  为*空指针*或*指向空「C 字符串」*时创建空「动态字符串」。

**返回值**

（`dstr_adt *`）所创建的「动态字符串」的指针。

如果创建失败则返回*空指针*。

> [!CAUTION]
>
> 返回值指向堆内存，请手动调用 `dstr_destroy()` 释放。

### `dstr_destroy()`

销毁一个「动态字符串」。

```c
void dstr_destroy(dstr_adt *dstr);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

  如果为*空指针*，则函数会直接返回。

### `dstr_clone()`

克隆一个「动态字符串」。

```c
dstr_adt *dstr_clone(const dstr_adt *dstr);
```

**参数**

- `dstr`

  源「动态字符串」的指针。

  为*空指针*或*指向空「动态字符串」*时创建空「动态字符串」。

**返回值**

（`dstr_adt *`）所创建的「动态字符串」的指针。

如果创建失败则返回*空指针*。

> [!CAUTION]
>
> 返回值指向堆内存，请手动调用 `dstr_destroy()` 释放。

### `dstr_sub_cstr()`

提取一个「C 字符串」的子串为一个新的「动态字符串」。

```c
dstr_adt *dstr_sub_cstr(const char *cstr, size_t sub_start, size_t sub_length);
```

**参数**

- `cstr`

  源「C 字符串」的指针。

  为*空指针*或*指向空「C 字符串」*时创建空「动态字符串」，此时忽略 `sub_start` 和 `sub_length`。

- `sub_start`

  子串的起始索引。

  如果越界，则函数会直接返回*空指针*。

- `sub_length`

  子串的长度。

  为 0 表示到末尾。

  如果越界，则函数会直接返回*空指针*。

**返回值**

（`dstr_adt *`）所创建的「动态字符串」的指针。

如果创建失败则返回*空指针*。

> [!CAUTION]
>
> 返回值指向堆内存，请手动调用 `dstr_destroy()` 释放。

### `dstr_sub()`

提取一个「动态字符串」的子串为一个新的「动态字符串」。

```c
dstr_adt *dstr_sub(const dstr_adt *dstr, size_t sub_start, size_t sub_length);
```

**参数**

- `dstr`

  源「动态字符串」的指针。

  为*空指针*或*指向空「动态字符串」*时创建空「动态字符串」，此时忽略 `sub_start` 和 `sub_length`。

- `sub_start`

  子串的起始索引。

  如果越界，则函数会直接返回*空指针*。

- `sub_length`

  子串的长度。

  为 0 表示到末尾。

  如果越界，则函数会直接返回*空指针*。

**返回值**

（`dstr_adt *`）所创建的「动态字符串」的指针。

如果创建失败则返回*空指针*。

> [!CAUTION]
>
> 返回值指向堆内存，请手动调用 `dstr_destroy()` 释放。

### `dstr_create_format()`

格式化创建一个「动态字符串」。

```c
dstr_adt *dstr_create_format(const char *format, ...);
```

**参数**

- `format`

  格式「C 字符串」的指针。

  为*空指针*或*指向空「C 字符串」*时创建空「动态字符串」。

- `...`

  与 `format` 对应的可变参数列表。

**返回值**

（`dstr_adt *`）所创建的「动态字符串」的指针。

如果创建失败则返回*空指针*。

> [!CAUTION]
>
> 返回值指向堆内存，请手动调用 `dstr_destroy()` 释放。

### `dstr_create_vformat()`

格式化创建一个「动态字符串」（va_list 版本）。

```c
dstr_adt *dstr_create_vformat(const char *format, va_list args);
```

**参数**

- `format`

  格式「C 字符串」的指针。

  为*空指针*或*指向空「C 字符串」*时创建空「动态字符串」。

- `args`

  已通过 `va_start()` 初始化的 `va_list` 变量，包含与 `format` 对应的可变参数列表信息。

  该函数不会调用 `va_end()`，调用者需自行管理 `args` 的生命周期。

**返回值**

（`dstr_adt *`）所创建的「动态字符串」的指针。

如果创建失败则返回*空指针*。

> [!TIP]
>
> `args` 可能被本函数读取并消耗，调用后不应再次使用，除非重新 `va_start()` 或使用 `va_copy()`。

> [!CAUTION]
>
> 返回值指向堆内存，请手动调用 `dstr_destroy()` 释放。

## 属性获取与设置

### `dstr_cstr()`

获取一个「动态字符串」的内部「C 字符串」指针。

```c
const char *dstr_cstr(const dstr_adt *dstr);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

  如果为*空指针*，则函数会直接返回*空指针*。

**返回值**

（`const char *`）所获取的「C 字符串」指针。

对非空 `dstr` 保证非空。

> [!CAUTION]
>
> 返回的指针指向内部缓冲区，调用者不得修改其内容。该指针在 `dstr` 被修改、扩容、缩容或销毁后失效。对非空 `dstr`，保证返回非空指针；当 `dstr_length() == 0` 时，返回的指针指向 `""`。

### `dstr_length()`

获取一个「动态字符串」的长度。

```c
size_t dstr_length(const dstr_adt *dstr);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

  如果为*空指针*，则函数会直接返回 0。

**返回值**

（`size_t`）所获取的长度。

### `dstr_is_empty()`

判断一个「动态字符串」是否是空「动态字符串」。

```c
bool dstr_is_empty(const dstr_adt *dstr);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

  如果为*空指针*，则函数会直接返回 true。

**返回值**

（`bool`）如果目标「动态字符串」是空「动态字符串」则返回 true，否则返回 false。

### `dstr_capacity()`

获取一个「动态字符串」的容量。

```c
size_t dstr_capacity(const dstr_adt *dstr);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

  如果为*空指针*，则函数会直接返回 0。

**返回值**

（`size_t`）所获取的容量。

### `dstr_set_capacity()`

设置一个「动态字符串」的容量。

```c
dstr_status_t dstr_set_capacity(dstr_adt *dstr, size_t new_capacity);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

  如果为*空指针*，则视为不合法参数。

- `new_capacity`

  新的容量。

**返回值**

（`dstr_status_t`）全局状态码。

> [!TIP]
>
> 成功调用后，该请求值会成为后续自动扩容/缩容的保底容量下限，直到调用 `dstr_shrink_to_fit()` 清除。再次调用 `dstr_set_capacity()` 会覆盖旧下限。具体保底容量行为由实现决定，API 仅保证容量不小于请求值。

> [!CAUTION]
>
> 参数 `new_capacity` 是期望的数据缓冲区字节数，包含结尾 `'\0'`。函数成功执行时，实际容量可能等于 `new_capacity`，也可能等于实现所采用的最小容量下限（当 `new_capacity` 小于该下限时）。因此实际容量不会小于 `new_capacity`，调用者应以 `dstr_capacity()` 返回值为准。如果 `new_capacity <= dstr_length()`，则字符串会被截断，新的长度为实际容量 - 1（即新容量能容纳的最大长度）。如果 `new_capacity == 0`，视为请求最小容量。

### `dstr_shrink_to_fit()`

调整一个「动态字符串」的容量到刚合适。

```c
void dstr_shrink_to_fit(dstr_adt *dstr);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

  如果为*空指针*，则函数会直接返回。

> [!CAUTION]
>
> 执行此函数会同时取消目标「动态字符串」由 `dstr_set_capacity()` 所设置的保底容量下限。容量会缩到至少能容纳 `len + 1` 个字节，但可能受实现最小容量下限影响。

## 内容编辑

### `dstr_cpy_cstr()`

复制一个「C 字符串」到一个「动态字符串」。

```c
dstr_status_t dstr_cpy_cstr(dstr_adt *dest, const char *src);
```

**参数**

- `dest`

  目标「动态字符串」的指针。

  如果为*空指针*，则视为不合法参数。

- `src`

  源「C 字符串」的指针。

  为*空指针*或*指向空「C 字符串」*时复制空字符串（清空目标「动态字符串」的内容）。

**返回值**

（`dstr_status_t`）全局状态码。

> [!WARNING]
>
> `src` 不得指向 `dest` 的内部缓冲区。若需自复制，请先创建临时副本。

### `dstr_cpy()`

复制一个「动态字符串」到另一个「动态字符串」。

```c
dstr_status_t dstr_cpy(dstr_adt *dest, const dstr_adt *src);
```

**参数**

- `dest`

  目标「动态字符串」的指针。

  如果为*空指针*，则视为不合法参数。

- `src`

  源「动态字符串」的指针。

  为*空指针*或*指向空「动态字符串」*时复制空字符串（清空目标「动态字符串」的内容）。

**返回值**

（`dstr_status_t`）全局状态码。

> [!WARNING]
>
> `src` 不得指向 `dest` 的内部缓冲区。若需自复制，请先创建临时副本。

### `dstr_cpy_sub_cstr()`

复制一个「C 字符串」的子串到一个「动态字符串」。

```c
dstr_status_t dstr_cpy_sub_cstr(dstr_adt *dest, const char *src, size_t sub_start, size_t sub_length);
```

**参数**

- `dest`

  目标「动态字符串」的指针。

  如果为*空指针*，则视为不合法参数。

- `src`

  源「C 字符串」的指针。

  为*空指针*或*指向空「C 字符串」*时复制空字符串（清空目标「动态字符串」的内容），此时忽略 `sub_start` 和 `sub_length`。

- `sub_start`

  子串的起始索引。

  如果越界，则视为不合法参数。

- `sub_length`

  子串的长度。

  为 0 表示到末尾。

  如果越界，则视为不合法参数。

**返回值**

（`dstr_status_t`）全局状态码。

> [!WARNING]
>
> `src` 不得指向 `dest` 的内部缓冲区。若需自复制，请先创建临时副本。

### `dstr_cpy_sub()`

复制一个「动态字符串」的子串到另一个「动态字符串」。

```c
dstr_status_t dstr_cpy_sub(dstr_adt *dest, const dstr_adt *src, size_t sub_start, size_t sub_length);
```

**参数**

- `dest`

  目标「动态字符串」的指针。

  如果为*空指针*，则视为不合法参数。

- `src`

  源「动态字符串」的指针。

  为*空指针*或*指向空「动态字符串」*时复制空字符串（清空目标「动态字符串」的内容），此时忽略 `sub_start` 和 `sub_length`。

- `sub_start`

  子串的起始索引。

  如果越界，则视为不合法参数。

- `sub_length`

  子串的长度。

  为 0 表示到末尾。

  如果越界，则视为不合法参数。

**返回值**

（`dstr_status_t`）全局状态码。

> [!WARNING]
>
> `src` 不得指向 `dest` 的内部缓冲区。若需自复制，请先创建临时副本。

### `dstr_cpy_format()`

格式化复制一个字符串到一个「动态字符串」。

```c
dstr_status_t dstr_cpy_format(dstr_adt *dest, const char *format, ...);
```

**参数**

- `dest`

  目标「动态字符串」的指针。

  如果为*空指针*，则视为不合法参数。

- `format`

  格式「C 字符串」的指针。

  为*空指针*或*指向空「C 字符串」*时复制空字符串（清空目标「动态字符串」的内容）。

- `...`

  与 `format` 对应的可变参数列表。

**返回值**

（`dstr_status_t`）全局状态码。

> [!WARNING]
>
> `format` 不得指向 `dest` 的内部缓冲区。若需自复制，请先创建临时副本。

### `dstr_cpy_vformat()`

格式化复制一个字符串到一个「动态字符串」（va_list 版本）。

```c
dstr_status_t dstr_cpy_vformat(dstr_adt *dest, const char *format, va_list args);
```

**参数**

- `dest`

  目标「动态字符串」的指针。

  如果为*空指针*，则视为不合法参数。

- `format`

  格式「C 字符串」的指针。

  为*空指针*或*指向空「C 字符串」*时复制空字符串（清空目标「动态字符串」的内容）。

- `args`

  已通过 `va_start()` 初始化的 `va_list` 变量，包含与 `format` 对应的可变参数列表信息。

  该函数不会调用 `va_end()`，调用者需自行管理 `args` 的生命周期。

**返回值**

（`dstr_status_t`）全局状态码。

> [!TIP]
>
> `args` 可能被本函数读取并消耗，调用后不应再次使用，除非重新 `va_start()` 或使用 `va_copy()`。

> [!WARNING]
>
> `format` 不得指向 `dest` 的内部缓冲区。若需自复制，请先创建临时副本。

### `dstr_cat_cstr()`

追加一个「C 字符串」到一个「动态字符串」。

```c
dstr_status_t dstr_cat_cstr(dstr_adt *dest, const char *src);
```

**参数**

- `dest`

  目标「动态字符串」的指针。

  如果为*空指针*，则视为不合法参数。

- `src`

  源「C 字符串」的指针。

  为*空指针*或*指向空「C 字符串」*时追加空字符串。

**返回值**

（`dstr_status_t`）全局状态码。

> [!WARNING]
>
> `src` 不得指向 `dest` 的内部缓冲区。若需自追加，请先创建临时副本。

### `dstr_cat()`

追加一个「动态字符串」到另一个「动态字符串」。

```c
dstr_status_t dstr_cat(dstr_adt *dest, const dstr_adt *src);
```

**参数**

- `dest`

  目标「动态字符串」的指针。

  如果为*空指针*，则视为不合法参数。

- `src`

  源「动态字符串」的指针。

  为*空指针*或*指向空「动态字符串」*时追加空字符串。

**返回值**

（`dstr_status_t`）全局状态码。

> [!WARNING]
>
> `src` 不得指向 `dest` 的内部缓冲区。若需自追加，请先创建临时副本。

### `dstr_cat_sub_cstr()`

追加一个「C 字符串」的子串到一个「动态字符串」。

```c
dstr_status_t dstr_cat_sub_cstr(dstr_adt *dest, const char *src, size_t sub_start, size_t sub_length);
```

**参数**

- `dest`

  目标「动态字符串」的指针。

  如果为*空指针*，则视为不合法参数。

- `src`

  源「C 字符串」的指针。

  为*空指针*或*指向空「C 字符串」*时追加空字符串，此时忽略 `sub_start` 和 `sub_length`。

- `sub_start`

  子串的起始索引。

  如果越界，则视为不合法参数。

- `sub_length`

  子串的长度。

  为 0 表示到末尾。

  如果越界，则视为不合法参数。

**返回值**

（`dstr_status_t`）全局状态码。

> [!WARNING]
>
> `src` 不得指向 `dest` 的内部缓冲区。若需自追加，请先创建临时副本。

### `dstr_cat_sub()`

追加一个「动态字符串」的子串到另一个「动态字符串」。

```c
dstr_status_t dstr_cat_sub(dstr_adt *dest, const dstr_adt *src, size_t sub_start, size_t sub_length);
```

**参数**

- `dest`

  目标「动态字符串」的指针。

  如果为*空指针*，则视为不合法参数。

- `src`

  源「动态字符串」的指针。

  为*空指针*或*指向空「动态字符串」*时追加空字符串，此时忽略 `sub_start` 和 `sub_length`。

- `sub_start`

  子串的起始索引。

  如果越界，则视为不合法参数。

- `sub_length`

  子串的长度。

  为 0 表示到末尾。

  如果越界，则视为不合法参数。

**返回值**

（`dstr_status_t`）全局状态码。

> [!WARNING]
>
> `src` 不得指向 `dest` 的内部缓冲区。若需自追加，请先创建临时副本。

### `dstr_cat_format()`

格式化追加一个字符串到一个「动态字符串」。

```c
dstr_status_t dstr_cat_format(dstr_adt *dest, const char *format, ...);
```

**参数**

- `dest`

  目标「动态字符串」的指针。

  如果为*空指针*，则视为不合法参数。

- `format`

  格式「C 字符串」的指针。

  为*空指针*或*指向空「C 字符串」*时追加空字符串。

- `...`

  与 `format` 对应的可变参数列表。

**返回值**

（`dstr_status_t`）全局状态码。

> [!WARNING]
>
> `format` 不得指向 `dest` 的内部缓冲区。若需自追加，请先创建临时副本。

### `dstr_cat_vformat()`

格式化追加一个字符串到一个「动态字符串」（va_list 版本）。

```c
dstr_status_t dstr_cat_vformat(dstr_adt *dest, const char *format, va_list args);
```

**参数**

- `dest`

  目标「动态字符串」的指针。

  如果为*空指针*，则视为不合法参数。

- `format`

  格式「C 字符串」的指针。

  为*空指针*或*指向空「C 字符串」*时追加空字符串。

- `args`

  已通过 `va_start()` 初始化的 `va_list` 变量，包含与 `format` 对应的可变参数列表信息。

  该函数不会调用 `va_end()`，调用者需自行管理 `args` 的生命周期。

**返回值**

（`dstr_status_t`）全局状态码。

> [!TIP]
>
> `args` 可能被本函数读取并消耗，调用后不应再次使用，除非重新 `va_start()` 或使用 `va_copy()`。

> [!WARNING]
>
> `format` 不得指向 `dest` 的内部缓冲区。若需自追加，请先创建临时副本。

### `dstr_insert_cstr()`

插入一个「C 字符串」到一个「动态字符串」。

```c
dstr_status_t dstr_insert_cstr(dstr_adt *dest, size_t index, const char *src);
```

**参数**

- `dest`

  目标「动态字符串」的指针。

  如果为*空指针*，则视为不合法参数。

- `index`

  插入位置的索引。

  如果越界，则视为不合法参数。

- `src`

  源「C 字符串」的指针。

  为*空指针*或*指向空「C 字符串」*时插入空字符串。

**返回值**

（`dstr_status_t`）全局状态码。

> [!WARNING]
>
> `src` 不得指向 `dest` 的内部缓冲区。若需自插入，请先创建临时副本。

### `dstr_insert()`

插入一个「动态字符串」到另一个「动态字符串」。

```c
dstr_status_t dstr_insert(dstr_adt *dest, size_t index, const dstr_adt *src);
```

**参数**

- `dest`

  目标「动态字符串」的指针。

  如果为*空指针*，则视为不合法参数。

- `index`

  插入位置的索引。

  如果越界，则视为不合法参数。

- `src`

  源「动态字符串」的指针。

  为*空指针*或*指向空「动态字符串」*时插入空字符串。

**返回值**

（`dstr_status_t`）全局状态码。

> [!WARNING]
>
> `src` 不得指向 `dest` 的内部缓冲区。若需自插入，请先创建临时副本。

### `dstr_insert_sub_cstr()`

插入一个「C 字符串」的子串到一个「动态字符串」。

```c
dstr_status_t dstr_insert_sub_cstr(dstr_adt *dest, size_t index, const char *src, size_t sub_start, size_t sub_length);
```

**参数**

- `dest`

  目标「动态字符串」的指针。

  如果为*空指针*，则视为不合法参数。

- `index`

  插入位置的索引。

  如果越界，则视为不合法参数。

- `src`

  源「C 字符串」的指针。

  为*空指针*或*指向空「C 字符串」*时插入空字符串，此时忽略 `sub_start` 和 `sub_length`。

- `sub_start`

  子串的起始索引。

  如果越界，则视为不合法参数。

- `sub_length`

  子串的长度。

  为 0 表示到末尾。

  如果越界，则视为不合法参数。

**返回值**

（`dstr_status_t`）全局状态码。

> [!WARNING]
>
> `src` 不得指向 `dest` 的内部缓冲区。若需自插入，请先创建临时副本。

### `dstr_insert_sub()`

插入一个「动态字符串」的子串到另一个「动态字符串」。

```c
dstr_status_t dstr_insert_sub(dstr_adt *dest, size_t index, const dstr_adt *src, size_t sub_start, size_t sub_length);
```

**参数**

- `dest`

  目标「动态字符串」的指针。

  如果为*空指针*，则视为不合法参数。

- `index`

  插入位置的索引。

  如果越界，则视为不合法参数。

- `src`

  源「动态字符串」的指针。

  为*空指针*或*指向空「动态字符串」*时插入空字符串，此时忽略 `sub_start` 和 `sub_length`。

- `sub_start`

  子串的起始索引。

  如果越界，则视为不合法参数。

- `sub_length`

  子串的长度。

  为 0 表示到末尾。

  如果越界，则视为不合法参数。

**返回值**

（`dstr_status_t`）全局状态码。

> [!WARNING]
>
> `src` 不得指向 `dest` 的内部缓冲区。若需自插入，请先创建临时副本。

### `dstr_insert_format()`

格式化插入一个字符串到一个「动态字符串」。

```c
dstr_status_t dstr_insert_format(dstr_adt *dest, size_t index, const char *format, ...);
```

**参数**

- `dest`

  目标「动态字符串」的指针。

  如果为*空指针*，则视为不合法参数。

- `index`

  插入位置的索引。

  如果越界，则视为不合法参数。

- `format`

  格式「C 字符串」的指针。

  为*空指针*或*指向空「C 字符串」*时插入空字符串。

- `...`

  与 `format` 对应的可变参数列表。

**返回值**

（`dstr_status_t`）全局状态码。

> [!WARNING]
>
> `format` 不得指向 `dest` 的内部缓冲区。若需自插入，请先创建临时副本。

### `dstr_insert_vformat()`

格式化插入一个字符串到一个「动态字符串」（va_list 版本）。

```c
dstr_status_t dstr_insert_vformat(dstr_adt *dest, size_t index, const char *format, va_list args);
```

**参数**

- `dest`

  目标「动态字符串」的指针。

  如果为*空指针*，则视为不合法参数。

- `index`

  插入位置的索引。

  如果越界，则视为不合法参数。

- `format`

  格式「C 字符串」的指针。

  为*空指针*或*指向空「C 字符串」*时插入空字符串。

- `args`

  已通过 `va_start()` 初始化的 `va_list` 变量，包含与 `format` 对应的可变参数列表信息。

  该函数不会调用 `va_end()`，调用者需自行管理 `args` 的生命周期。

**返回值**

（`dstr_status_t`）全局状态码。

> [!TIP]
>
> `args` 可能被本函数读取并消耗，调用后不应再次使用，除非重新 `va_start()` 或使用 `va_copy()`。

> [!WARNING]
>
> `format` 不得指向 `dest` 的内部缓冲区。若需自插入，请先创建临时副本。

### `dstr_clear()`

清空一个「动态字符串」。

```c
void dstr_clear(dstr_adt *dstr);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

  如果为*空指针*，则函数会直接返回。

> [!TIP]
>
> 使其长度为 0，不会立即释放容量。

### `dstr_remove()`

删除一个「动态字符串」的子串。

```c
void dstr_remove(dstr_adt *dstr, size_t sub_start, size_t sub_length);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

  如果为*空指针*或*指向空「动态字符串」*，则函数会直接返回。

- `sub_start`

  子串的起始索引。

  如果越界，则函数会直接返回（视为参数非法，静默失败）。

- `sub_length`

  子串的长度。

  为 0 表示到末尾。

  如果越界，则函数会直接返回。

### `dstr_trim()`

删除一个「动态字符串」首尾的空白字符或指定字符。

```c
void dstr_trim(dstr_adt *dstr, const char *trim_chars);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

  如果为*空指针*或*指向空「动态字符串」*，则函数会直接返回。

- `trim_chars`

  包含要删除的字符的「C 字符串」的指针。

  为*空指针*或*指向空「C 字符串」*时删除空白字符。

> [!TIP]
>
> 空白字符判定使用 C 标准库函数 `isspace()`。本库不修改 locale，因此判定结果取决于调用时的 locale。如需不受 locale 影响，请显式指定 `trim_chars`。

## 关系判断与比较

### `dstr_starts_with_cstr()`

判断一个「动态字符串」是否以指定「C 字符串」前缀开头。

```c
bool dstr_starts_with_cstr(const dstr_adt *dstr, const char *prefix);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

  如果为*空指针*或*指向空「动态字符串」*，则函数会直接返回 false。

- `prefix`

  前缀「C 字符串」的指针。

  如果为*空指针*或*指向空「C 字符串」*，则函数会直接返回 false。

**返回值**

（`bool`）如果目标「动态字符串」以指定「C 字符串」前缀开头则返回 true，否则返回 false。

### `dstr_starts_with()`

判断一个「动态字符串」是否以指定「动态字符串」前缀开头。

```c
bool dstr_starts_with(const dstr_adt *dstr, const dstr_adt *prefix);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

  如果为*空指针*或*指向空「动态字符串」*，则函数会直接返回 false。

- `prefix`

  前缀「动态字符串」的指针。

  如果为*空指针*或*指向空「动态字符串」*，则函数会直接返回 false。

**返回值**

（`bool`）如果目标「动态字符串」以指定「动态字符串」前缀开头则返回 true，否则返回 false。

### `cstr_starts_with()`

判断一个「C 字符串」是否以指定「C 字符串」前缀开头。

```c
bool cstr_starts_with(const char *cstr, const char *prefix);
```

**参数**

- `cstr`

  目标「C 字符串」的指针。

  如果为*空指针*或*指向空「C 字符串」*，则函数会直接返回 false。

- `prefix`

  前缀「C 字符串」的指针。

  如果为*空指针*或*指向空「C 字符串」*，则函数会直接返回 false。

**返回值**

（`bool`）如果目标「C 字符串」以指定「C 字符串」前缀开头则返回 true，否则返回 false。

### `dstr_ends_with_cstr()`

判断一个「动态字符串」是否以指定「C 字符串」后缀结尾。

```c
bool dstr_ends_with_cstr(const dstr_adt *dstr, const char *suffix);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

  如果为*空指针*或*指向空「动态字符串」*，则函数会直接返回 false。

- `suffix`

  后缀「C 字符串」的指针。

  如果为*空指针*或*指向空「C 字符串」*，则函数会直接返回 false。

**返回值**

（`bool`）如果目标「动态字符串」以指定「C 字符串」后缀结尾则返回 true，否则返回 false。

### `dstr_ends_with()`

判断一个「动态字符串」是否以指定「动态字符串」后缀结尾。

```c
bool dstr_ends_with(const dstr_adt *dstr, const dstr_adt *suffix);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

  如果为*空指针*或*指向空「动态字符串」*，则函数会直接返回 false。

- `suffix`

  后缀「动态字符串」的指针。

  如果为*空指针*或*指向空「动态字符串」*，则函数会直接返回 false。

**返回值**

（`bool`）如果目标「动态字符串」以指定「动态字符串」后缀结尾则返回 true，否则返回 false。

### `cstr_ends_with()`

判断一个「C 字符串」是否以指定「C 字符串」后缀结尾。

```c
bool cstr_ends_with(const char *cstr, const char *suffix);
```

**参数**

- `cstr`

  目标「C 字符串」的指针。

  如果为*空指针*或*指向空「C 字符串」*，则函数会直接返回 false。

- `suffix`

  后缀「C 字符串」的指针。

  如果为*空指针*或*指向空「C 字符串」*，则函数会直接返回 false。

**返回值**

（`bool`）如果目标「C 字符串」以指定「C 字符串」后缀结尾则返回 true，否则返回 false。

### `dstr_contains_cstr()`

判断一个「动态字符串」是否包含指定子「C 字符串」。

```c
bool dstr_contains_cstr(const dstr_adt *dstr, const char *sub);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

- `sub`

  子「C 字符串」的指针。

**返回值**

（`bool`）包含则返回 true，否则返回 false。

如果 `sub` 为「空字符串」，则一定返回 true，遵循 C 标准规定：空字符串是任何字符串的子串。

> [!NOTE]
>
> 本函数与查找相关函数不同：空子串按 C 标准视为任意字符串的子串，因此返回 true；而查找相关函数将空子串视为非法查找目标。

### `dstr_contains()`

判断一个「动态字符串」是否包含指定子「动态字符串」。

```c
bool dstr_contains(const dstr_adt *dstr, const dstr_adt *sub);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

- `sub`

  子「动态字符串」的指针。

**返回值**

（`bool`）包含则返回 true，否则返回 false。

如果 `sub` 为「空字符串」，则一定返回 true，遵循 C 标准规定：空字符串是任何字符串的子串。

> [!NOTE]
>
> 本函数与查找相关函数不同：空子串按 C 标准视为任意字符串的子串，因此返回 true；而查找相关函数将空子串视为非法查找目标。

### `cstr_contains()`

判断一个「C 字符串」是否包含指定子「C 字符串」。

```c
bool cstr_contains(const char *cstr, const char *sub);
```

**参数**

- `cstr`

  目标「C 字符串」的指针。

- `sub`

  子「C 字符串」的指针。

**返回值**

（`bool`）包含则返回 true，否则返回 false。

如果 `sub` 为「空字符串」，则一定返回 true，遵循 C 标准规定：空字符串是任何字符串的子串。

> [!NOTE]
>
> 本函数与查找相关函数不同：空子串按 C 标准视为任意字符串的子串，因此返回 true；而查找相关函数将空子串视为非法查找目标。

### `dstr_equals_cstr()`

判断一个「动态字符串」是否与一个「C 字符串」相等。

```c
bool dstr_equals_cstr(const dstr_adt *lhs, const char *rhs);
```

**参数**

- `lhs`

  「动态字符串」的指针。

  如果为*空指针*，则视其为「空字符串」。

- `rhs`

  「C 字符串」的指针。

  如果为*空指针*，则视其为「空字符串」。

**返回值**

（`bool`）相等则返回 true，否则返回 false。

两者同为「空字符串」时，返回 true。

### `dstr_equals()`

判断两个「动态字符串」是否相等。

```c
bool dstr_equals(const dstr_adt *lhs, const dstr_adt *rhs);
```

**参数**

- `lhs`

  第一个「动态字符串」的指针。

  如果为*空指针*，则视其为「空字符串」。

- `rhs`

  第二个「动态字符串」的指针。

  如果为*空指针*，则视其为「空字符串」。

**返回值**

（`bool`）相等则返回 true，否则返回 false。

两者同为「空字符串」时，返回 true。

### `cstr_equals()`

判断两个「C 字符串」是否相等。

```c
bool cstr_equals(const char *lhs, const char *rhs);
```

**参数**

- `lhs`

  第一个「C 字符串」的指针。

  如果为*空指针*，则视其为「空字符串」。

- `rhs`

  第二个「C 字符串」的指针。

  如果为*空指针*，则视其为「空字符串」。

**返回值**

（`bool`）相等则返回 true，否则返回 false。

两者同为「空字符串」时，返回 true。

### `dstr_compare_cstr()`

比较一个「动态字符串」与一个「C 字符串」。

```c
int dstr_compare_cstr(const dstr_adt *lhs, const char *rhs);
```

**参数**

- `lhs`

  「动态字符串」的指针。

  如果为*空指针*，则视其为「空字符串」。

- `rhs`

  「C 字符串」的指针。

  如果为*空指针*，则视其为「空字符串」。

**返回值**

（`int`）两者相等返回 0，前者大于后者返回正值，前者小于后者返回负值。

两者同为「空字符串」时，返回 0。

两者只有一者为「空字符串」时，为「空字符串」者小于非「空字符串」者。

### `dstr_compare()`

比较两个「动态字符串」。

```c
int dstr_compare(const dstr_adt *lhs, const dstr_adt *rhs);
```

**参数**

- `lhs`

  第一个「动态字符串」的指针。

  如果为*空指针*，则视其为「空字符串」。

- `rhs`

  第二个「动态字符串」的指针。

  如果为*空指针*，则视其为「空字符串」。

**返回值**

（`int`）两者相等返回 0，前者大于后者返回正值，前者小于后者返回负值。

两者同为「空字符串」时，返回 0。

两者只有一者为「空字符串」时，为「空字符串」者小于非「空字符串」者。

### `cstr_compare()`

比较两个「C 字符串」。

```c
int cstr_compare(const char *lhs, const char *rhs);
```

**参数**

- `lhs`

  第一个「C 字符串」的指针。

  如果为*空指针*，则视其为「空字符串」。

- `rhs`

  第二个「C 字符串」的指针。

  如果为*空指针*，则视其为「空字符串」。

**返回值**

（`int`）两者相等返回 0，前者大于后者返回正值，前者小于后者返回负值。

两者同为「空字符串」时，返回 0。

两者只有一者为「空字符串」时，为「空字符串」者小于非「空字符串」者。

## 查找、统计与替换

> [!NOTE]
>
> 本库所有查找、统计、替换操作均按"非重叠匹配"处理。即一旦在位置 i 找到一个匹配，下一次查找从 i + 子串长度处继续；反向查找同理，下一次从匹配位置之前继续。因此，"aaa" 中查找 "aa" 只算出现 1 次，而不是 2 次。替换时，新插入的内容不会参与同一轮再次匹配。

### `dstr_find_cstr()`

查找一个「动态字符串」中指定子「C 字符串」首次出现的位置。

```c
bool dstr_find_cstr(const dstr_adt *dstr, const char *sub, size_t *out_index, dstr_direction_t direction);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

  如果为*空指针*或*指向空「动态字符串」*，则函数会直接返回 false。

- `sub`

  子「C 字符串」的指针。

  如果为*空指针*或*指向空「C 字符串」*，则函数会直接返回 false。

- `out_index`

  存储查找结果（位置索引）的 `size_t` 变量的指针。

  为*空指针*时不写入。

- `direction`

  查找方向。

**返回值**

（`bool`）找到则返回 true，否则返回 false。

### `dstr_find()`

查找一个「动态字符串」中指定子「动态字符串」首次出现的位置。

```c
bool dstr_find(const dstr_adt *dstr, const dstr_adt *sub, size_t *out_index, dstr_direction_t direction);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

  如果为*空指针*或*指向空「动态字符串」*，则函数会直接返回 false。

- `sub`

  子「动态字符串」的指针。

  如果为*空指针*或*指向空「动态字符串」*，则函数会直接返回 false。

- `out_index`

  存储查找结果（位置索引）的 `size_t` 变量的指针。

  为*空指针*时不写入。

- `direction`

  查找方向。

**返回值**

（`bool`）找到则返回 true，否则返回 false。

### `cstr_find()`

查找一个「C 字符串」中指定子「C 字符串」首次出现的位置。

```c
bool cstr_find(const char *cstr, const char *sub, size_t *out_index, dstr_direction_t direction);
```

**参数**

- `cstr`

  目标「C 字符串」的指针。

  如果为*空指针*或*指向空「C 字符串」*，则函数会直接返回 false。

- `sub`

  子「C 字符串」的指针。

  如果为*空指针*或*指向空「C 字符串」*，则函数会直接返回 false。

- `out_index`

  存储查找结果（位置索引）的 `size_t` 变量的指针。

  为*空指针*时不写入。

- `direction`

  查找方向。

**返回值**

（`bool`）找到则返回 true，否则返回 false。

### `dstr_find_nth_cstr()`

查找一个「动态字符串」中指定子「C 字符串」第 n 次出现的位置。

```c
bool dstr_find_nth_cstr(const dstr_adt *dstr, const char *sub, size_t *out_index, dstr_direction_t direction, size_t n);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

  如果为*空指针*或*指向空「动态字符串」*，则函数会直接返回 false。

- `sub`

  子「C 字符串」的指针。

  如果为*空指针*或*指向空「C 字符串」*，则函数会直接返回 false。

- `out_index`

  存储查找结果（位置索引）的 `size_t` 变量的指针。

  为*空指针*时不写入。

- `direction`

  查找方向。

- `n`

  出现的次序，从 1 开始。

  为 0 表示该方向的最后一次出现。

  具体：`direction` 为 `DSTR_DIR_FORWARD` 时，`n>0` 表示从前往后第 n 次，`n=0` 表示从前往后最后一次（等价于 `DSTR_DIR_BACKWARD, n=1`）；`direction` 为 `DSTR_DIR_BACKWARD` 时，`n>0` 表示从后往前第 n 次，`n=0` 表示从后往前最后一次（等价于 `DSTR_DIR_FORWARD, n=1`）。

  如果大于实际出现次数，则视为最后一次。

**返回值**

（`bool`）找到则返回 true，否则返回 false。

### `dstr_find_nth()`

查找一个「动态字符串」中指定子「动态字符串」第 n 次出现的位置。

```c
bool dstr_find_nth(const dstr_adt *dstr, const dstr_adt *sub, size_t *out_index, dstr_direction_t direction, size_t n);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

  如果为*空指针*或*指向空「动态字符串」*，则函数会直接返回 false。

- `sub`

  子「动态字符串」的指针。

  如果为*空指针*或*指向空「动态字符串」*，则函数会直接返回 false。

- `out_index`

  存储查找结果（位置索引）的 `size_t` 变量的指针。

  为*空指针*时不写入。

- `direction`

  查找方向。

- `n`

  出现的次序，从 1 开始。

  为 0 表示该方向的最后一次出现。

  具体：`direction` 为 `DSTR_DIR_FORWARD` 时，`n>0` 表示从前往后第 n 次，`n=0` 表示从前往后最后一次（等价于 `DSTR_DIR_BACKWARD, n=1`）；`direction` 为 `DSTR_DIR_BACKWARD` 时，`n>0` 表示从后往前第 n 次，`n=0` 表示从后往前最后一次（等价于 `DSTR_DIR_FORWARD, n=1`）。

  如果大于实际出现次数，则视为最后一次。

**返回值**

（`bool`）找到则返回 true，否则返回 false。

### `cstr_find_nth()`

查找一个「C 字符串」中指定子「C 字符串」第 n 次出现的位置。

```c
bool cstr_find_nth(const char *cstr, const char *sub, size_t *out_index, dstr_direction_t direction, size_t n);
```

**参数**

- `cstr`

  目标「C 字符串」的指针。

  如果为*空指针*或*指向空「C 字符串」*，则函数会直接返回 false。

- `sub`

  子「C 字符串」的指针。

  如果为*空指针*或*指向空「C 字符串」*，则函数会直接返回 false。

- `out_index`

  存储查找结果（位置索引）的 `size_t` 变量的指针。

  为*空指针*时不写入。

- `direction`

  查找方向。

- `n`

  出现的次序，从 1 开始。

  为 0 表示该方向的最后一次出现。

  具体：`direction` 为 `DSTR_DIR_FORWARD` 时，`n>0` 表示从前往后第 n 次，`n=0` 表示从前往后最后一次（等价于 `DSTR_DIR_BACKWARD, n=1`）；`direction` 为 `DSTR_DIR_BACKWARD` 时，`n>0` 表示从后往前第 n 次，`n=0` 表示从后往前最后一次（等价于 `DSTR_DIR_FORWARD, n=1`）。

  如果大于实际出现次数，则视为最后一次。

**返回值**

（`bool`）找到则返回 true，否则返回 false。

### `dstr_find_indexes_cstr()`

查找一个「动态字符串」中指定子「C 字符串」前 n 次出现的位置。

```c
size_t dstr_find_indexes_cstr(const dstr_adt *dstr, const char *sub, size_t *out_indexes, dstr_direction_t direction, size_t n);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

  如果为*空指针*或*指向空「动态字符串」*，则函数会直接返回 0。

- `sub`

  子「C 字符串」的指针。

  如果为*空指针*或*指向空「C 字符串」*，则函数会直接返回 0。

- `out_indexes`

  存储查找结果（位置索引）的 `size_t` 数组的指针。

  为*空指针*时不写入。

  请自行确保数组容量够大。

- `direction`

  查找方向。

- `n`

  查找的次数，从 1 开始。

  为 0 表示查找全部。

  如果大于实际出现次数，也会查找全部。

**返回值**

（`size_t`）截止第 n 次，实际出现的次数，也表示实际向 `out_indexes` 数组中写入的元素个数。

### `dstr_find_indexes()`

查找一个「动态字符串」中指定子「动态字符串」前 n 次出现的位置。

```c
size_t dstr_find_indexes(const dstr_adt *dstr, const dstr_adt *sub, size_t *out_indexes, dstr_direction_t direction, size_t n);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

  如果为*空指针*或*指向空「动态字符串」*，则函数会直接返回 0。

- `sub`

  子「动态字符串」的指针。

  如果为*空指针*或*指向空「动态字符串」*，则函数会直接返回 0。

- `out_indexes`

  存储查找结果（位置索引）的 `size_t` 数组的指针。

  为*空指针*时不写入。

  请自行确保数组容量够大。

- `direction`

  查找方向。

- `n`

  查找的次数，从 1 开始。

  为 0 表示查找全部。

  如果大于实际出现次数，也会查找全部。

**返回值**

（`size_t`）截止第 n 次，实际出现的次数，也表示实际向 `out_indexes` 数组中写入的元素个数。

### `cstr_find_indexes()`

查找一个「C 字符串」中指定子「C 字符串」前 n 次出现的位置。

```c
size_t cstr_find_indexes(const char *cstr, const char *sub, size_t *out_indexes, dstr_direction_t direction, size_t n);
```

**参数**

- `cstr`

  目标「C 字符串」的指针。

  如果为*空指针*或*指向空「C 字符串」*，则函数会直接返回 0。

- `sub`

  子「C 字符串」的指针。

  如果为*空指针*或*指向空「C 字符串」*，则函数会直接返回 0。

- `out_indexes`

  存储查找结果（位置索引）的 `size_t` 数组的指针。

  为*空指针*时不写入。

  请自行确保数组容量够大。

- `direction`

  查找方向。

- `n`

  查找的次数，从 1 开始。

  为 0 表示查找全部。

  如果大于实际出现次数，也会查找全部。

**返回值**

（`size_t`）截止第 n 次，实际出现的次数，也表示实际向 `out_indexes` 数组中写入的元素个数。

### `dstr_count_cstr()`

统计一个「动态字符串」中指定子「C 字符串」出现的次数。

```c
size_t dstr_count_cstr(const dstr_adt *dstr, const char *sub);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

  如果为*空指针*或*指向空「动态字符串」*，则函数会直接返回 0。

- `sub`

  子「C 字符串」的指针。

  如果为*空指针*或*指向空「C 字符串」*，则函数会直接返回 0。

**返回值**

（`size_t`）出现的次数。

### `dstr_count()`

统计一个「动态字符串」中指定子「动态字符串」出现的次数。

```c
size_t dstr_count(const dstr_adt *dstr, const dstr_adt *sub);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

  如果为*空指针*或*指向空「动态字符串」*，则函数会直接返回 0。

- `sub`

  子「动态字符串」的指针。

  如果为*空指针*或*指向空「动态字符串」*，则函数会直接返回 0。

**返回值**

（`size_t`）出现的次数。

### `cstr_count()`

统计一个「C 字符串」中指定子「C 字符串」出现的次数。

```c
size_t cstr_count(const char *cstr, const char *sub);
```

**参数**

- `cstr`

  目标「C 字符串」的指针。

  如果为*空指针*或*指向空「C 字符串」*，则函数会直接返回 0。

- `sub`

  子「C 字符串」的指针。

  如果为*空指针*或*指向空「C 字符串」*，则函数会直接返回 0。

**返回值**

（`size_t`）出现的次数。

### `dstr_replace_cstr()`

替换一个「动态字符串」中指定旧「C 字符串」为指定新「C 字符串」n 次。

```c
dstr_status_t dstr_replace_cstr(dstr_adt *dstr, const char *old_str, const char *new_str, dstr_direction_t direction, size_t n);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

  如果为*空指针*或*指向空「动态字符串」*，则视为不合法参数。

- `old_str`

  旧「C 字符串」的指针。

  如果为*空指针*或*指向空「C 字符串」*，则视为不合法参数。

  如果在 `dstr` 中一次都没有出现或出现次数不足 n 次（n 不为 0 时），则视为不合法参数。

- `new_str`

  新「C 字符串」的指针。

  为*空指针*或*指向空「C 字符串」*时，替换为空。

- `direction`

  替换方向。

- `n`

  替换的次数。

  为 0 表示替换所有。

  如果大于旧「C 字符串」实际出现的次数，则视为不合法参数，将一次替换都不进行。

**返回值**

（`dstr_status_t`）全局状态码。

### `dstr_replace()`

替换一个「动态字符串」中指定旧「动态字符串」为指定新「动态字符串」n 次。

```c
dstr_status_t dstr_replace(dstr_adt *dstr, const dstr_adt *old_str, const dstr_adt *new_str, dstr_direction_t direction, size_t n);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

  如果为*空指针*或*指向空「动态字符串」*，则视为不合法参数。

- `old_str`

  旧「动态字符串」的指针。

  如果为*空指针*或*指向空「动态字符串」*，则视为不合法参数。

  如果在 `dstr` 中一次都没有出现或出现次数不足 n 次（n 不为 0 时），则视为不合法参数。

- `new_str`

  新「动态字符串」的指针。

  为*空指针*或*指向空「动态字符串」*时，替换为空。

- `direction`

  替换方向。

- `n`

  替换的次数。

  为 0 表示替换所有。

  如果大于旧「动态字符串」实际出现的次数，则视为不合法参数，将一次替换都不进行。

**返回值**

（`dstr_status_t`）全局状态码。

### `dstr_replace_nth_cstr()`

替换一个「动态字符串」中指定旧「C 字符串」第 n 次为指定新「C 字符串」。

```c
dstr_status_t dstr_replace_nth_cstr(dstr_adt *dstr, const char *old_str, const char *new_str, dstr_direction_t direction, size_t n);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

  如果为*空指针*或*指向空「动态字符串」*，则视为不合法参数。

- `old_str`

  旧「C 字符串」的指针。

  如果为*空指针*或*指向空「C 字符串」*，则视为不合法参数。

  如果在 `dstr` 中一次都没有出现或出现次数不足 n 次（n 不为 0 时），则视为不合法参数。

- `new_str`

  新「C 字符串」的指针。

  为*空指针*或*指向空「C 字符串」*时，替换为空。

- `direction`

  替换方向。

- `n`

  替换的次序，从 1 开始。

  为 0 表示该方向的最后一次出现。

  具体：`direction` 为 `DSTR_DIR_FORWARD` 时，`n>0` 表示从前往后第 n 次，`n=0` 表示从前往后最后一次（等价于 `DSTR_DIR_BACKWARD, n=1`）；`direction` 为 `DSTR_DIR_BACKWARD` 时，`n>0` 表示从后往前第 n 次，`n=0` 表示从后往前最后一次（等价于 `DSTR_DIR_FORWARD, n=1`）。

  如果大于旧「C 字符串」实际出现的次数，则视为不合法参数，将一次替换都不进行。

**返回值**

（`dstr_status_t`）全局状态码。

### `dstr_replace_nth()`

替换一个「动态字符串」中指定旧「动态字符串」第 n 次为指定新「动态字符串」。

```c
dstr_status_t dstr_replace_nth(dstr_adt *dstr, const dstr_adt *old_str, const dstr_adt *new_str, dstr_direction_t direction, size_t n);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

  如果为*空指针*或*指向空「动态字符串」*，则视为不合法参数。

- `old_str`

  旧「动态字符串」的指针。

  如果为*空指针*或*指向空「动态字符串」*，则视为不合法参数。

  如果在 `dstr` 中一次都没有出现或出现次数不足 n 次（n 不为 0 时），则视为不合法参数。

- `new_str`

  新「动态字符串」的指针。

  为*空指针*或*指向空「动态字符串」*时，替换为空。

- `direction`

  替换方向。

- `n`

  替换的次序，从 1 开始。

  为 0 表示该方向的最后一次出现。

  具体：`direction` 为 `DSTR_DIR_FORWARD` 时，`n>0` 表示从前往后第 n 次，`n=0` 表示从前往后最后一次（等价于 `DSTR_DIR_BACKWARD, n=1`）；`direction` 为 `DSTR_DIR_BACKWARD` 时，`n>0` 表示从后往前第 n 次，`n=0` 表示从后往前最后一次（等价于 `DSTR_DIR_FORWARD, n=1`）。

  如果大于旧「动态字符串」实际出现的次数，则视为不合法参数，将一次替换都不进行。

**返回值**

（`dstr_status_t`）全局状态码。

## 分隔与合并

### `dstr_split_cstr()`

分隔一个「C 字符串」为多个「动态字符串」。

```c
dstr_adt **dstr_split_cstr(const char *cstr, const char *separator, size_t *out_dstr_count);
```

**参数**

- `cstr`

  目标「C 字符串」的指针。

  如果为*空指针*或*指向空「C 字符串」*，则函数会直接返回*空指针*。

- `separator`

  分隔「C 字符串」的指针。

  如果为*空指针*或*指向空「C 字符串」*，则函数会直接返回*空指针*。

- `out_dstr_count`

  存储分隔后的「动态字符串」的个数的 `size_t` 变量的指针。

  如果为*空指针*，则函数会直接返回*空指针*。

  仅在函数成功执行时写入。

**返回值**

（`dstr_adt **`）分隔后的「动态字符串」数组的指针。

如果分隔失败则返回*空指针*。

> [!TIP]
>
> 分隔时，如果两个分隔串之间，或分隔串与首尾边界之间的子串长度为 0，将以*空指针*（而不是空「动态字符串」）形式存储在数组中，而不是跳过。

> [!CAUTION]
>
> 返回值指向堆内存，其中又可能有指向其他堆内存的指针。因此，释放时，请先手动依次调用 `dstr_destroy()` 释放每个元素，然后手动调用 `free()` 释放数组本身。

### `dstr_split()`

分隔一个「动态字符串」为多个「动态字符串」。

```c
dstr_adt **dstr_split(const dstr_adt *dstr, const dstr_adt *separator, size_t *out_dstr_count);
```

**参数**

- `dstr`

  目标「动态字符串」的指针。

  如果为*空指针*或*指向空「动态字符串」*，则函数会直接返回*空指针*。

- `separator`

  分隔「动态字符串」的指针。

  如果为*空指针*或*指向空「动态字符串」*，则函数会直接返回*空指针*。

- `out_dstr_count`

  存储分隔后的「动态字符串」的个数的 `size_t` 变量的指针。

  如果为*空指针*，则函数会直接返回*空指针*。

  仅在函数成功执行时写入。

**返回值**

（`dstr_adt **`）分隔后的「动态字符串」数组的指针。

如果分隔失败则返回*空指针*。

> [!TIP]
>
> 分隔时，如果两个分隔串之间，或分隔串与首尾边界之间的子串长度为 0，将以*空指针*（而不是空「动态字符串」）形式存储在数组中，而不是跳过。

> [!CAUTION]
>
> 返回值指向堆内存，其中又可能有指向其他堆内存的指针。因此，释放时，请先手动依次调用 `dstr_destroy()` 释放每个元素，然后手动调用 `free()` 释放数组本身。

### `cstr_split()`

分隔一个「C 字符串」为多个「C 字符串」。

```c
char **cstr_split(const char *cstr, const char *separator, size_t *out_cstr_count);
```

**参数**

- `cstr`

  目标「C 字符串」的指针。

  如果为*空指针*或*指向空「C 字符串」*，则函数会直接返回*空指针*。

- `separator`

  分隔「C 字符串」的指针。

  如果为*空指针*或*指向空「C 字符串」*，则函数会直接返回*空指针*。

- `out_cstr_count`

  存储分隔后的「C 字符串」的个数的 `size_t` 变量的指针。

  如果为*空指针*，则函数会直接返回*空指针*。

  仅在函数成功执行时写入。

**返回值**

（`char **`）分隔后的「C 字符串」数组的指针。

如果分隔失败则返回*空指针*。

> [!TIP]
>
> 分隔时，如果两个分隔串之间，或分隔串与首尾边界之间的子串长度为 0，将以*空指针*（而不是空「C 字符串」）形式存储在数组中，而不是跳过。

> [!CAUTION]
>
> 返回值指向堆内存，其中又可能有指向其他堆内存的指针。因此，释放时，请先手动依次调用 `free()` 释放每个元素，然后手动调用 `free()` 释放数组本身。

### `dstr_join_cstr()`

合并多个「C 字符串」为一个「动态字符串」。

```c
dstr_adt *dstr_join_cstr(const char *const *cstrs, size_t cstr_count, const char *separator);
```

**参数**

- `cstrs`

  源「C 字符串」数组的指针。

  如果为*空指针*，则函数会直接返回*空指针*。

- `cstr_count`

  源「C 字符串」的个数。

  如果为 0，则函数会直接返回*空指针*。

- `separator`

  分隔「C 字符串」的指针。

  为*空指针*或*指向空「C 字符串」*时使用空字符串合并。

**返回值**

（`dstr_adt *`）合并后的「动态字符串」的指针。

如果合并失败则返回*空指针*。

> [!TIP]
>
> 合并时，其中的*空指针*或空「C 字符串」会以空字符串形式被合并，而不是被跳过。

> [!CAUTION]
>
> 返回值指向堆内存，请手动调用 `dstr_destroy()` 释放。

### `dstr_join()`

合并多个「动态字符串」为一个「动态字符串」。

```c
dstr_adt *dstr_join(const dstr_adt *const *dstrs, size_t dstr_count, const dstr_adt *separator);
```

**参数**

- `dstrs`

  源「动态字符串」数组的指针。

  如果为*空指针*，则函数会直接返回*空指针*。

- `dstr_count`

  源「动态字符串」的个数。

  如果为 0，则函数会直接返回*空指针*。

- `separator`

  分隔「动态字符串」的指针。

  为*空指针*或*指向空「动态字符串」*时使用空字符串合并。

**返回值**

（`dstr_adt *`）合并后的「动态字符串」的指针。

如果合并失败则返回*空指针*。

> [!TIP]
>
> 合并时，其中的*空指针*或空「动态字符串」会以空字符串形式被合并，而不是被跳过。

> [!CAUTION]
>
> 返回值指向堆内存，请手动调用 `dstr_destroy()` 释放。

### `cstr_join()`

合并多个「C 字符串」为一个「C 字符串」。

```c
char *cstr_join(const char *const *cstrs, size_t cstr_count, const char *separator);
```

**参数**

- `cstrs`

  源「C 字符串」数组的指针。

  如果为*空指针*，则函数会直接返回*空指针*。

- `cstr_count`

  源「C 字符串」的个数。

  如果为 0，则函数会直接返回*空指针*。

- `separator`

  分隔「C 字符串」的指针。

  为*空指针*或*指向空「C 字符串」*时使用空字符串合并。

**返回值**

（`char *`）合并后的「C 字符串」的指针。

如果合并失败则返回*空指针*。

> [!TIP]
>
> 合并时，其中的*空指针*或空「C 字符串」会以空字符串形式被合并，而不是被跳过。

> [!CAUTION]
>
> 返回值指向堆内存，请手动调用 `free()` 释放。
