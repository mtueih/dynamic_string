<div align="right">

**English** | [简体中文](api-reference.zh-CN.md)

</div>

# dynamic_string API Reference

## Table of Contents

- [Basic Concepts](#basic-concepts)
  - [Type Definitions](#basic-concepts-type-definitions)
  - [Status Codes](#basic-concepts-status-codes)
  - [Naming Conventions](#basic-concepts-naming-conventions)
  - [General Semantics and Constraints](#basic-concepts-general-semantics-and-constraints)
- [API Functions](#api-functions)
  - [Creation and Destruction](#api-functions-creation-and-destruction)
  - [Getters and Setters](#api-functions-getters-and-setters)
  - [Content Editing](#api-functions-content-editing)
  - [Relation and Comparison](#api-functions-relation-and-comparison)
  - [Find, Count, and Replace](#api-functions-find-count-and-replace)
  - [Split and Join](#api-functions-split-and-join)

<a id="basic-concepts"></a>

## Basic Concepts [↑](#dynamic_string-api-reference)

<a id="basic-concepts-type-definitions"></a>

### Type Definitions [↑](#basic-concepts)

#### Dynamic String [↑](#basic-concepts-type-definitions)

This library is an abstract-data-type library, which means every API function is a definition of some operation on a class of objects. In this library, that class of objects is the **dynamic string**. For safety and encapsulation purposes, this library defines it as an **opaque type**:

```c
typedef struct dynamic_string dstr_adt;
```

`dstr` is an abbreviation of **d**ynamic **str**ing, and `adt` stands for abstract data type.

"Opaque" means you cannot directly access or control its attributes. Because the compiler cannot know the size of this type either, you can only use it through its **opaque pointer** (`dstr_adt *`).

#### Find/Replace Direction [↑](#basic-concepts-type-definitions)

For **find and replace** operations, the direction of the search/replacement (front-to-back / back-to-front) must be indicated. Therefore such API functions require a parameter to express these two cases. For semantic clarity, this library defines an enumeration type to convey this information:

```c
typedef enum {
	DSTR_DIR_FORWARD,  /* From front to back. */
	DSTR_DIR_BACKWARD  /* From back to front. */
} dstr_direction_t;
```

<a id="basic-concepts-status-codes"></a>

### Status Codes [↑](#basic-concepts)

This library involves a great deal of memory operations, so most operations defined by this library do not always succeed, and their failures may have various different causes. To allow the caller to distinguish the cause of a failure when necessary, a status-code enumeration type is defined. API functions return a value of this type to convey a status to the caller, indicating whether the operation succeeded and, on failure, the specific cause.

The status-code enumeration type is defined as follows:

```c
typedef enum {
	DSTR_SUCCESS = 0,         /* Success. */
	DSTR_MEMORY_ALLOC_FAILED, /* Memory allocation failed. */
	DSTR_INVALID_ARGUMENT,    /* Invalid argument. */
} dstr_status_t;
```

<a id="basic-concepts-naming-conventions"></a>

### Naming Conventions [↑](#basic-concepts)

#### Prefix [↑](#basic-concepts-naming-conventions)

This library mainly provides API functions that operate on dynamic-string objects; they all start with `dstr_`. This library also provides some API functions that do not depend on dynamic-string objects but target C strings instead, which start with `cstr_`.

#### Suffix [↑](#basic-concepts-naming-conventions)

For most operations in this library, two versions are provided for the two allowed input string types (dynamic strings / C strings). The version whose input type is a C string carries the `_cstr` suffix relative to the version whose input type is a dynamic string.

<a id="basic-concepts-general-semantics-and-constraints"></a>

### General Semantics and Constraints [↑](#basic-concepts)

The following conventions apply library-wide unless a specific function states otherwise.

1. **A text-string library, not a binary-safe string library.** The underlying representation is always a valid C string, i.e. the data is always terminated by `'\0'`, even when its length is 0. This means that for valid input, the return value of the API function `dstr_cstr()` is guaranteed never to be a null pointer, and it always points to a buffer of a string terminated by `'\0'`.
2. **Encoding-independence and capacity.** This library does not care about the encoding of string data, so its length is measured in bytes rather than characters, and does not include the terminating `'\0'`. Capacity denotes the real size of the underlying data buffer, also measured in bytes. Because the terminating `'\0'` must be stored, capacity is always at least 1 greater than length (at least 1 when length is 0, which means the lower bound of capacity is guaranteed to be 1 rather than 0; and because the implementation may apply optimizations such as SSO, the actual lower bound of capacity may be greater than 1), but it is not guaranteed to always equal length + 1 (the implementation may apply optimizations such as geometric growth).
3. **Null pointers and empty strings.** For any parameter that represents a string, a null pointer can generally be used to represent an empty string, avoiding the trouble of constructing an unnecessary object just to represent an empty string.
4. **Non-overlapping matches.** All find, count, and replace operations treat matches as **non-overlapping**.
5. **No overlapping memory.** For any operation that may copy data from one buffer to another, the implementation is not guaranteed to check whether the source and destination buffers overlap. Therefore, in such cases, do not let the source-buffer pointer point to the destination buffer, otherwise the behavior is undefined.

<a id="api-functions"></a>

## API Functions [↑](#dynamic_string-api-reference)

<a id="api-functions-creation-and-destruction"></a>

### Creation and Destruction [↑](#api-functions)

This group of API functions is mainly used to control the lifecycle of a "dynamic string".

The operations and their corresponding API functions and descriptions are as follows:

<table>
  <thead>
    <tr>
      <th>Operation Type</th>
      <th>Operation</th>
      <th>API Function</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td rowspan="6">Create</td>
      <td>Create</td>
      <td><a href="#api-functions-creation-and-destruction-dstr_create"><code>dstr_create()</code></a></td>
    </tr>
    <tr>
      <td>Clone</td>
      <td><a href="#api-functions-creation-and-destruction-dstr_clone"><code>dstr_clone()</code></a></td>
    </tr>
    <tr>
      <td rowspan="2">Extract substring</td>
      <td><a href="#api-functions-creation-and-destruction-dstr_sub_cstr"><code>dstr_sub_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-creation-and-destruction-dstr_sub"><code>dstr_sub()</code></a></td>
    </tr>
    <tr>
      <td rowspan="2">Format creation</td>
      <td><a href="#api-functions-creation-and-destruction-dstr_create_format"><code>dstr_create_format()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-creation-and-destruction-dstr_create_vformat"><code>dstr_create_vformat()</code></a></td>
    </tr>
    <tr>
      <td>Destroy</td>
      <td>Destroy</td>
      <td><a href="#api-functions-creation-and-destruction-dstr_destroy"><code>dstr_destroy()</code></a></td>
    </tr>
  </tbody>
</table>

<a id="api-functions-creation-and-destruction-dstr_create"></a>

#### `dstr_create()` [↑](#api-functions-creation-and-destruction)

Creates a "dynamic string".

```c
dstr_adt *dstr_create(
	const char *cstr
);
```

| Parameter | Type           | Description                                                                                                   |
| --------- | -------------- | ------------------------------------------------------------------------------------------------------------- |
| `cstr`    | `const char *` | Pointer to the source "C string".<br>Creates an empty "dynamic string" when it is an <em>"empty string"</em>. |

| Return Value | Description                                                                      |
| ------------ | -------------------------------------------------------------------------------- |
| `dstr_adt *` | Pointer to the created "dynamic string".<br>Returns a _null pointer_ on failure. |

> [!IMPORTANT]
>
> The return value points to heap memory; call `dstr_destroy()` manually to release it.

<a id="api-functions-creation-and-destruction-dstr_destroy"></a>

#### `dstr_destroy()` [↑](#api-functions-creation-and-destruction)

Destroys a "dynamic string".

```c
void dstr_destroy(
	dstr_adt *dstr
);
```

| Parameter | Type         | Description                                                                                              |
| --------- | ------------ | -------------------------------------------------------------------------------------------------------- |
| `dstr`    | `dstr_adt *` | Pointer to the target "dynamic string".<br>The function returns immediately when it is a _null pointer_. |

<a id="api-functions-creation-and-destruction-dstr_clone"></a>

#### `dstr_clone()` [↑](#api-functions-creation-and-destruction)

Clones a "dynamic string".

```c
dstr_adt *dstr_clone(
	const dstr_adt *dstr
);
```

| Parameter | Type               | Description                                                                                                         |
| --------- | ------------------ | ------------------------------------------------------------------------------------------------------------------- |
| `dstr`    | `const dstr_adt *` | Pointer to the source "dynamic string".<br>Creates an empty "dynamic string" when it is an <em>"empty string"</em>. |

| Return Value | Description                                                                      |
| ------------ | -------------------------------------------------------------------------------- |
| `dstr_adt *` | Pointer to the created "dynamic string".<br>Returns a _null pointer_ on failure. |

> [!IMPORTANT]
>
> The return value points to heap memory; call `dstr_destroy()` manually to release it.

<a id="api-functions-creation-and-destruction-dstr_sub_cstr"></a>

#### `dstr_sub_cstr()` [↑](#api-functions-creation-and-destruction)

Extracts a substring of a "C string" into a new "dynamic string".

```c
dstr_adt *dstr_sub_cstr(
	const char *cstr,
	size_t sub_start,
	size_t sub_length
);
```

| Parameter    | Type           | Description                                                                                                                                          |
| ------------ | -------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------- |
| `cstr`       | `const char *` | Pointer to the source "C string".<br>Creates an empty "dynamic string" when it is an <em>"empty string"</em>, ignoring `sub_start` and `sub_length`. |
| `sub_start`  | `size_t`       | Starting index of the substring.<br>The function returns a _null pointer_ immediately when _out of bounds_.                                          |
| `sub_length` | `size_t`       | Length of the substring.<br>_`0`_ means to the end.<br>The function returns a _null pointer_ immediately when _out of bounds_.                       |

| Return Value | Description                                                                      |
| ------------ | -------------------------------------------------------------------------------- |
| `dstr_adt *` | Pointer to the created "dynamic string".<br>Returns a _null pointer_ on failure. |

> [!IMPORTANT]
>
> The return value points to heap memory; call `dstr_destroy()` manually to release it.

<a id="api-functions-creation-and-destruction-dstr_sub"></a>

#### `dstr_sub()` [↑](#api-functions-creation-and-destruction)

Extracts a substring of a "dynamic string" into a new "dynamic string".

```c
dstr_adt *dstr_sub(
	const dstr_adt *dstr,
	size_t sub_start,
	size_t sub_length
);
```

| Parameter    | Type               | Description                                                                                                                                                |
| ------------ | ------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `dstr`       | `const dstr_adt *` | Pointer to the source "dynamic string".<br>Creates an empty "dynamic string" when it is an <em>"empty string"</em>, ignoring `sub_start` and `sub_length`. |
| `sub_start`  | `size_t`           | Starting index of the substring.<br>The function returns a _null pointer_ immediately when _out of bounds_.                                                |
| `sub_length` | `size_t`           | Length of the substring.<br>_`0`_ means to the end.<br>The function returns a _null pointer_ immediately when _out of bounds_.                             |

| Return Value | Description                                                                      |
| ------------ | -------------------------------------------------------------------------------- |
| `dstr_adt *` | Pointer to the created "dynamic string".<br>Returns a _null pointer_ on failure. |

> [!IMPORTANT]
>
> The return value points to heap memory; call `dstr_destroy()` manually to release it.

<a id="api-functions-creation-and-destruction-dstr_create_format"></a>

#### `dstr_create_format()` [↑](#api-functions-creation-and-destruction)

Creates a "dynamic string" from a format string.

```c
dstr_adt *dstr_create_format(
	const char *format,
	...
);
```

| Parameter | Type           | Description                                                                                                   |
| --------- | -------------- | ------------------------------------------------------------------------------------------------------------- |
| `format`  | `const char *` | Pointer to the format "C string".<br>Creates an empty "dynamic string" when it is an <em>"empty string"</em>. |
| `...`     | —              | The variable argument list corresponding to `format`.                                                         |

| Return Value | Description                                                                      |
| ------------ | -------------------------------------------------------------------------------- |
| `dstr_adt *` | Pointer to the created "dynamic string".<br>Returns a _null pointer_ on failure. |

> [!IMPORTANT]
>
> The return value points to heap memory; call `dstr_destroy()` manually to release it.

<a id="api-functions-creation-and-destruction-dstr_create_vformat"></a>

#### `dstr_create_vformat()` [↑](#api-functions-creation-and-destruction)

Creates a "dynamic string" from a format string (`va_list` version).

```c
dstr_adt *dstr_create_vformat(
	const char *format,
	va_list args
);
```

| Parameter | Type           | Description                                                                                                                                                                                                                                            |
| --------- | -------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `format`  | `const char *` | Pointer to the format "C string".<br>Creates an empty "dynamic string" when it is an <em>"empty string"</em>.                                                                                                                                          |
| `args`    | `va_list`      | A `va_list` variable that has been initialized by `va_start()`, containing the variable argument list information corresponding to `format`.<br>This function does not call `va_end()`; the caller is responsible for managing the lifetime of `args`. |

| Return Value | Description                                                                      |
| ------------ | -------------------------------------------------------------------------------- |
| `dstr_adt *` | Pointer to the created "dynamic string".<br>Returns a _null pointer_ on failure. |

> [!NOTE]
>
> `args` may be read and consumed by this function and should not be reused after the call, unless reinitialized with `va_start()` or a copy is created with `va_copy()` for use with this function.

> [!IMPORTANT]
>
> The return value points to heap memory; call `dstr_destroy()` manually to release it.

<a id="api-functions-getters-and-setters"></a>

### Getters and Setters [↑](#api-functions)

This group of API functions is mainly used to get and set the attributes of a "dynamic string".

The operations and their corresponding API functions and descriptions are as follows:

<table>
  <thead>
    <tr>
      <th>Operation Type</th>
      <th>Operation</th>
      <th>API Function</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td rowspan="4">Get</td>
      <td>Get the internal "C string" pointer</td>
      <td><a href="#api-functions-getters-and-setters-dstr_cstr"><code>dstr_cstr()</code></a></td>
    </tr>
    <tr>
      <td>Get length</td>
      <td><a href="#api-functions-getters-and-setters-dstr_length"><code>dstr_length()</code></a></td>
    </tr>
    <tr>
      <td>Get capacity</td>
      <td><a href="#api-functions-getters-and-setters-dstr_capacity"><code>dstr_capacity()</code></a></td>
    </tr>
    <tr>
      <td>Check if empty</td>
      <td><a href="#api-functions-getters-and-setters-dstr_is_empty"><code>dstr_is_empty()</code></a></td>
    </tr>
    <tr>
      <td rowspan="2">Set</td>
      <td>Set capacity</td>
      <td><a href="#api-functions-getters-and-setters-dstr_set_capacity"><code>dstr_set_capacity()</code></a></td>
    </tr>
    <tr>
      <td>Shrink capacity to fit</td>
      <td><a href="#api-functions-getters-and-setters-dstr_shrink_to_fit"><code>dstr_shrink_to_fit()</code></a></td>
    </tr>
  </tbody>
</table>

<a id="api-functions-getters-and-setters-dstr_cstr"></a>

#### `dstr_cstr()` [↑](#api-functions-getters-and-setters)

Gets the internal "C string" pointer of a "dynamic string".

```c
const char *dstr_cstr(
	const dstr_adt *dstr
);
```

| Parameter | Type               | Description                                                                                                               |
| --------- | ------------------ | ------------------------------------------------------------------------------------------------------------------------- |
| `dstr`    | `const dstr_adt *` | Pointer to the target "dynamic string".<br>The function returns a _null pointer_ immediately when it is a _null pointer_. |

| Return Value   | Description                                |
| -------------- | ------------------------------------------ |
| `const char *` | The internal "C string" pointer of `dstr`. |

> [!NOTE]
>
> For valid input, this function guarantees that its return value is never a _null pointer_, and it always points to a string buffer terminated by `'\0'`.

> [!WARNING]
>
> The returned pointer points to an internal buffer; do not modify its contents through this pointer.
>
> This pointer is temporary and may be invalidated by changes to the content or capacity of `dstr`; do not rely on it.

<a id="api-functions-getters-and-setters-dstr_length"></a>

#### `dstr_length()` [↑](#api-functions-getters-and-setters)

Gets the length of a "dynamic string".

```c
size_t dstr_length(
	const dstr_adt *dstr
);
```

| Parameter | Type               | Description                                                                                                    |
| --------- | ------------------ | -------------------------------------------------------------------------------------------------------------- |
| `dstr`    | `const dstr_adt *` | Pointer to the target "dynamic string".<br>The function returns _`0`_ immediately when it is a _null pointer_. |

| Return Value | Description           |
| ------------ | --------------------- |
| `size_t`     | The length of `dstr`. |

<a id="api-functions-getters-and-setters-dstr_is_empty"></a>

#### `dstr_is_empty()` [↑](#api-functions-getters-and-setters)

Checks whether a "dynamic string" is an empty "dynamic string".

```c
bool dstr_is_empty(
	const dstr_adt *dstr
);
```

| Parameter | Type               | Description                                                                                                       |
| --------- | ------------------ | ----------------------------------------------------------------------------------------------------------------- |
| `dstr`    | `const dstr_adt *` | Pointer to the target "dynamic string".<br>The function returns _`true`_ immediately when it is a _null pointer_. |

| Return Value | Description                                                               |
| ------------ | ------------------------------------------------------------------------- |
| `bool`       | Returns _`true`_ if it is an empty "dynamic string", otherwise _`false`_. |

<a id="api-functions-getters-and-setters-dstr_capacity"></a>

#### `dstr_capacity()` [↑](#api-functions-getters-and-setters)

Gets the capacity of a "dynamic string".

```c
size_t dstr_capacity(
	const dstr_adt *dstr
);
```

| Parameter | Type               | Description                                                                                                    |
| --------- | ------------------ | -------------------------------------------------------------------------------------------------------------- |
| `dstr`    | `const dstr_adt *` | Pointer to the target "dynamic string".<br>The function returns _`0`_ immediately when it is a _null pointer_. |

| Return Value | Description             |
| ------------ | ----------------------- |
| `size_t`     | The capacity of `dstr`. |

<a id="api-functions-getters-and-setters-dstr_set_capacity"></a>

#### `dstr_set_capacity()` [↑](#api-functions-getters-and-setters)

Sets the capacity of a "dynamic string".

```c
dstr_status_t dstr_set_capacity(
	dstr_adt *dstr,
	size_t new_capacity
);
```

| Parameter      | Type         | Description                                                                                    |
| -------------- | ------------ | ---------------------------------------------------------------------------------------------- |
| `dstr`         | `dstr_adt *` | Pointer to the target "dynamic string".<br>A _null pointer_ is treated as an invalid argument. |
| `new_capacity` | `size_t`     | The new capacity.                                                                              |

| Return Value    | Description         |
| --------------- | ------------------- |
| `dstr_status_t` | Global status code. |

> [!NOTE]
>
> Because the API guarantees that the internal buffer is always a valid C string terminated by `'\0'`, the API constrains the capacity of a dynamic string to have a lower bound, which need not be 1 but must be greater than 0.
>
> This affects the behavior of this function: after it executes successfully, the actual capacity may equal `new_capacity`, or it may equal the implementation's capacity lower bound (when `new_capacity` is less than or equal to that bound).
>
> Furthermore, if `new_capacity` is greater than the implementation's capacity lower bound, then after this function executes successfully it also sets `new_capacity` as the guaranteed minimum capacity of `dstr` (which is not the same as the implementation's capacity lower bound). When the capacity is later shrunk automatically due to a decrease in the length of `dstr`'s content, the capacity will not drop below this guaranteed minimum. Calling this function again can override this value, and calling `dstr_shrink_to_fit()` can clear it.

> [!WARNING]
>
> When `new_capacity` is less than or equal to the current length of `dstr`, the content of `dstr` is truncated; the exact behavior is affected by the implementation's capacity lower bound.

<a id="api-functions-getters-and-setters-dstr_shrink_to_fit"></a>

#### `dstr_shrink_to_fit()` [↑](#api-functions-getters-and-setters)

Shrinks the capacity of a "dynamic string" to just fit its content.

```c
void dstr_shrink_to_fit(
	dstr_adt *dstr
);
```

| Parameter | Type         | Description                                                                                              |
| --------- | ------------ | -------------------------------------------------------------------------------------------------------- |
| `dstr`    | `dstr_adt *` | Pointer to the target "dynamic string".<br>The function returns immediately when it is a _null pointer_. |

> [!NOTE]
>
> After this function executes, the actual capacity is affected by the implementation's capacity lower bound.
>
> Executing this function also clears the guaranteed minimum capacity of `dstr` previously set by `dstr_set_capacity()`.

<a id="api-functions-content-editing"></a>

### Content Editing [↑](#api-functions)

This group of API functions is mainly used to edit the content of a "dynamic string".

The operations and their corresponding API functions and descriptions are as follows:

<table>
  <thead>
    <tr>
      <th>Operation Type</th>
      <th>Operation</th>
      <th>API Function</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td rowspan="6">Copy</td>
      <td rowspan="2">Copy</td>
      <td><a href="#api-functions-content-editing-dstr_cpy_cstr"><code>dstr_cpy_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-content-editing-dstr_cpy"><code>dstr_cpy()</code></a></td>
    </tr>
    <tr>
      <td rowspan="2">Copy substring</td>
      <td><a href="#api-functions-content-editing-dstr_cpy_sub_cstr"><code>dstr_cpy_sub_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-content-editing-dstr_cpy_sub"><code>dstr_cpy_sub()</code></a></td>
    </tr>
    <tr>
      <td rowspan="2">Format copy</td>
      <td><a href="#api-functions-content-editing-dstr_cpy_format"><code>dstr_cpy_format()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-content-editing-dstr_cpy_vformat"><code>dstr_cpy_vformat()</code></a></td>
    </tr>
    <tr>
      <td rowspan="6">Append</td>
      <td rowspan="2">Append</td>
      <td><a href="#api-functions-content-editing-dstr_cat_cstr"><code>dstr_cat_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-content-editing-dstr_cat"><code>dstr_cat()</code></a></td>
    </tr>
    <tr>
      <td rowspan="2">Append substring</td>
      <td><a href="#api-functions-content-editing-dstr_cat_sub_cstr"><code>dstr_cat_sub_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-content-editing-dstr_cat_sub"><code>dstr_cat_sub()</code></a></td>
    </tr>
    <tr>
      <td rowspan="2">Format append</td>
      <td><a href="#api-functions-content-editing-dstr_cat_format"><code>dstr_cat_format()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-content-editing-dstr_cat_vformat"><code>dstr_cat_vformat()</code></a></td>
    </tr>
    <tr>
      <td rowspan="6">Insert</td>
      <td rowspan="2">Insert</td>
      <td><a href="#api-functions-content-editing-dstr_insert_cstr"><code>dstr_insert_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-content-editing-dstr_insert"><code>dstr_insert()</code></a></td>
    </tr>
    <tr>
      <td rowspan="2">Insert substring</td>
      <td><a href="#api-functions-content-editing-dstr_insert_sub_cstr"><code>dstr_insert_sub_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-content-editing-dstr_insert_sub"><code>dstr_insert_sub()</code></a></td>
    </tr>
    <tr>
      <td rowspan="2">Format insert</td>
      <td><a href="#api-functions-content-editing-dstr_insert_format"><code>dstr_insert_format()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-content-editing-dstr_insert_vformat"><code>dstr_insert_vformat()</code></a></td>
    </tr>
    <tr>
      <td rowspan="3">Delete</td>
      <td>Clear</td>
      <td><a href="#api-functions-content-editing-dstr_clear"><code>dstr_clear()</code></a></td>
    </tr>
    <tr>
      <td>Remove substring</td>
      <td><a href="#api-functions-content-editing-dstr_remove"><code>dstr_remove()</code></a></td>
    </tr>
    <tr>
      <td>Trim</td>
      <td><a href="#api-functions-content-editing-dstr_trim"><code>dstr_trim()</code></a></td>
    </tr>
  </tbody>
</table>

<a id="api-functions-content-editing-dstr_cpy_cstr"></a>

#### `dstr_cpy_cstr()` [↑](#api-functions-content-editing)

Copies a "C string" to a "dynamic string".

```c
dstr_status_t dstr_cpy_cstr(
	dstr_adt *dest,
	const char *src
);
```

| Parameter | Type           | Description                                                                                                                         |
| --------- | -------------- | ----------------------------------------------------------------------------------------------------------------------------------- |
| `dest`    | `dstr_adt *`   | Pointer to the target "dynamic string".<br>A _null pointer_ is treated as an invalid argument.                                      |
| `src`     | `const char *` | Pointer to the source "C string".<br>Copies an empty string (clearing the content of `dest`) when it is an <em>"empty string"</em>. |

| Return Value    | Description         |
| --------------- | ------------------- |
| `dstr_status_t` | Global status code. |

> [!WARNING]
>
> `src` must not point to the internal buffer of `dest`; if self-copy is needed, create a temporary copy first.

<a id="api-functions-content-editing-dstr_cpy"></a>

#### `dstr_cpy()` [↑](#api-functions-content-editing)

Copies a "dynamic string" to another "dynamic string".

```c
dstr_status_t dstr_cpy(
	dstr_adt *dest,
	const dstr_adt *src
);
```

| Parameter | Type               | Description                                                                                                                               |
| --------- | ------------------ | ----------------------------------------------------------------------------------------------------------------------------------------- |
| `dest`    | `dstr_adt *`       | Pointer to the target "dynamic string".<br>A _null pointer_ is treated as an invalid argument.                                            |
| `src`     | `const dstr_adt *` | Pointer to the source "dynamic string".<br>Copies an empty string (clearing the content of `dest`) when it is an <em>"empty string"</em>. |

| Return Value    | Description         |
| --------------- | ------------------- |
| `dstr_status_t` | Global status code. |

> [!WARNING]
>
> `src` must not point to the internal buffer of `dest`; if self-copy is needed, create a temporary copy first.

<a id="api-functions-content-editing-dstr_cpy_sub_cstr"></a>

#### `dstr_cpy_sub_cstr()` [↑](#api-functions-content-editing)

Copies a substring of a "C string" to a "dynamic string".

```c
dstr_status_t dstr_cpy_sub_cstr(
	dstr_adt *dest,
	const char *src,
	size_t sub_start,
	size_t sub_length
);
```

| Parameter    | Type           | Description                                                                                                                                                                |
| ------------ | -------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `dest`       | `dstr_adt *`   | Pointer to the target "dynamic string".<br>A _null pointer_ is treated as an invalid argument.                                                                             |
| `src`        | `const char *` | Pointer to the source "C string".<br>Copies an empty string (clearing the content of `dest`) when it is an <em>"empty string"</em>, ignoring `sub_start` and `sub_length`. |
| `sub_start`  | `size_t`       | Starting index of the substring.<br>_Out of bounds_ is treated as an invalid argument.                                                                                     |
| `sub_length` | `size_t`       | Length of the substring.<br>_`0`_ means to the end.<br>_Out of bounds_ is treated as an invalid argument.                                                                  |

| Return Value    | Description         |
| --------------- | ------------------- |
| `dstr_status_t` | Global status code. |

> [!WARNING]
>
> `src` must not point to the internal buffer of `dest`; if self-copy is needed, create a temporary copy first.

<a id="api-functions-content-editing-dstr_cpy_sub"></a>

#### `dstr_cpy_sub()` [↑](#api-functions-content-editing)

Copies a substring of a "dynamic string" to another "dynamic string".

```c
dstr_status_t dstr_cpy_sub(
	dstr_adt *dest,
	const dstr_adt *src,
	size_t sub_start,
	size_t sub_length
);
```

| Parameter    | Type               | Description                                                                                                                                                                      |
| ------------ | ------------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `dest`       | `dstr_adt *`       | Pointer to the target "dynamic string".<br>A _null pointer_ is treated as an invalid argument.                                                                                   |
| `src`        | `const dstr_adt *` | Pointer to the source "dynamic string".<br>Copies an empty string (clearing the content of `dest`) when it is an <em>"empty string"</em>, ignoring `sub_start` and `sub_length`. |
| `sub_start`  | `size_t`           | Starting index of the substring.<br>_Out of bounds_ is treated as an invalid argument.                                                                                           |
| `sub_length` | `size_t`           | Length of the substring.<br>_`0`_ means to the end.<br>_Out of bounds_ is treated as an invalid argument.                                                                        |

| Return Value    | Description         |
| --------------- | ------------------- |
| `dstr_status_t` | Global status code. |

> [!WARNING]
>
> `src` must not point to the internal buffer of `dest`; if self-copy is needed, create a temporary copy first.

<a id="api-functions-content-editing-dstr_cpy_format"></a>

#### `dstr_cpy_format()` [↑](#api-functions-content-editing)

Copies a string to a "dynamic string" using a format string.

```c
dstr_status_t dstr_cpy_format(
	dstr_adt *dest,
	const char *format,
	...
);
```

| Parameter | Type           | Description                                                                                                                         |
| --------- | -------------- | ----------------------------------------------------------------------------------------------------------------------------------- |
| `dest`    | `dstr_adt *`   | Pointer to the target "dynamic string".<br>A _null pointer_ is treated as an invalid argument.                                      |
| `format`  | `const char *` | Pointer to the format "C string".<br>Copies an empty string (clearing the content of `dest`) when it is an <em>"empty string"</em>. |
| `...`     | —              | The variable argument list corresponding to `format`.                                                                               |

| Return Value    | Description         |
| --------------- | ------------------- |
| `dstr_status_t` | Global status code. |

> [!WARNING]
>
> `format` must not point to the internal buffer of `dest`; if self-copy is needed, create a temporary copy first.

<a id="api-functions-content-editing-dstr_cpy_vformat"></a>

#### `dstr_cpy_vformat()` [↑](#api-functions-content-editing)

Copies a string to a "dynamic string" using a format string (`va_list` version).

```c
dstr_status_t dstr_cpy_vformat(
	dstr_adt *dest,
	const char *format,
	va_list args
);
```

| Parameter | Type           | Description                                                                                                                                                                                                                                            |
| --------- | -------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `dest`    | `dstr_adt *`   | Pointer to the target "dynamic string".<br>A _null pointer_ is treated as an invalid argument.                                                                                                                                                         |
| `format`  | `const char *` | Pointer to the format "C string".<br>Copies an empty string (clearing the content of `dest`) when it is an <em>"empty string"</em>.                                                                                                                    |
| `args`    | `va_list`      | A `va_list` variable that has been initialized by `va_start()`, containing the variable argument list information corresponding to `format`.<br>This function does not call `va_end()`; the caller is responsible for managing the lifetime of `args`. |

| Return Value    | Description         |
| --------------- | ------------------- |
| `dstr_status_t` | Global status code. |

> [!NOTE]
>
> `args` may be read and consumed by this function and should not be reused after the call, unless reinitialized with `va_start()` or a copy is created with `va_copy()` for use with this function.

> [!WARNING]
>
> `format` must not point to the internal buffer of `dest`; if self-copy is needed, create a temporary copy first.

<a id="api-functions-content-editing-dstr_cat_cstr"></a>

#### `dstr_cat_cstr()` [↑](#api-functions-content-editing)

Appends a "C string" to a "dynamic string".

```c
dstr_status_t dstr_cat_cstr(
	dstr_adt *dest,
	const char *src
);
```

| Parameter | Type           | Description                                                                                                             |
| --------- | -------------- | ----------------------------------------------------------------------------------------------------------------------- |
| `dest`    | `dstr_adt *`   | Pointer to the target "dynamic string".<br>A _null pointer_ is treated as an invalid argument.                          |
| `src`     | `const char *` | Pointer to the source "C string".<br>Appends an empty string (appending nothing) when it is an <em>"empty string"</em>. |

| Return Value    | Description         |
| --------------- | ------------------- |
| `dstr_status_t` | Global status code. |

> [!WARNING]
>
> `src` must not point to the internal buffer of `dest`; if self-append is needed, create a temporary copy first.

<a id="api-functions-content-editing-dstr_cat"></a>

#### `dstr_cat()` [↑](#api-functions-content-editing)

Appends a "dynamic string" to another "dynamic string".

```c
dstr_status_t dstr_cat(
	dstr_adt *dest,
	const dstr_adt *src
);
```

| Parameter | Type               | Description                                                                                                                   |
| --------- | ------------------ | ----------------------------------------------------------------------------------------------------------------------------- |
| `dest`    | `dstr_adt *`       | Pointer to the target "dynamic string".<br>A _null pointer_ is treated as an invalid argument.                                |
| `src`     | `const dstr_adt *` | Pointer to the source "dynamic string".<br>Appends an empty string (appending nothing) when it is an <em>"empty string"</em>. |

| Return Value    | Description         |
| --------------- | ------------------- |
| `dstr_status_t` | Global status code. |

> [!WARNING]
>
> `src` must not point to the internal buffer of `dest`; if self-append is needed, create a temporary copy first.

<a id="api-functions-content-editing-dstr_cat_sub_cstr"></a>

#### `dstr_cat_sub_cstr()` [↑](#api-functions-content-editing)

Appends a substring of a "C string" to a "dynamic string".

```c
dstr_status_t dstr_cat_sub_cstr(
	dstr_adt *dest,
	const char *src,
	size_t sub_start,
	size_t sub_length
);
```

| Parameter    | Type           | Description                                                                                                                                                    |
| ------------ | -------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `dest`       | `dstr_adt *`   | Pointer to the target "dynamic string".<br>A _null pointer_ is treated as an invalid argument.                                                                 |
| `src`        | `const char *` | Pointer to the source "C string".<br>Appends an empty string (appending nothing) when it is an <em>"empty string"</em>, ignoring `sub_start` and `sub_length`. |
| `sub_start`  | `size_t`       | Starting index of the substring.<br>_Out of bounds_ is treated as an invalid argument.                                                                         |
| `sub_length` | `size_t`       | Length of the substring.<br>_`0`_ means to the end.<br>_Out of bounds_ is treated as an invalid argument.                                                      |

| Return Value    | Description         |
| --------------- | ------------------- |
| `dstr_status_t` | Global status code. |

> [!WARNING]
>
> `src` must not point to the internal buffer of `dest`; if self-append is needed, create a temporary copy first.

<a id="api-functions-content-editing-dstr_cat_sub"></a>

#### `dstr_cat_sub()` [↑](#api-functions-content-editing)

Appends a substring of a "dynamic string" to another "dynamic string".

```c
dstr_status_t dstr_cat_sub(
	dstr_adt *dest,
	const dstr_adt *src,
	size_t sub_start,
	size_t sub_length
);
```

| Parameter    | Type               | Description                                                                                                                                                          |
| ------------ | ------------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `dest`       | `dstr_adt *`       | Pointer to the target "dynamic string".<br>A _null pointer_ is treated as an invalid argument.                                                                       |
| `src`        | `const dstr_adt *` | Pointer to the source "dynamic string".<br>Appends an empty string (appending nothing) when it is an <em>"empty string"</em>, ignoring `sub_start` and `sub_length`. |
| `sub_start`  | `size_t`           | Starting index of the substring.<br>_Out of bounds_ is treated as an invalid argument.                                                                               |
| `sub_length` | `size_t`           | Length of the substring.<br>_`0`_ means to the end.<br>_Out of bounds_ is treated as an invalid argument.                                                            |

| Return Value    | Description         |
| --------------- | ------------------- |
| `dstr_status_t` | Global status code. |

> [!WARNING]
>
> `src` must not point to the internal buffer of `dest`; if self-append is needed, create a temporary copy first.

<a id="api-functions-content-editing-dstr_cat_format"></a>

#### `dstr_cat_format()` [↑](#api-functions-content-editing)

Appends a string to a "dynamic string" using a format string.

```c
dstr_status_t dstr_cat_format(
	dstr_adt *dest,
	const char *format,
	...
);
```

| Parameter | Type           | Description                                                                                                             |
| --------- | -------------- | ----------------------------------------------------------------------------------------------------------------------- |
| `dest`    | `dstr_adt *`   | Pointer to the target "dynamic string".<br>A _null pointer_ is treated as an invalid argument.                          |
| `format`  | `const char *` | Pointer to the format "C string".<br>Appends an empty string (appending nothing) when it is an <em>"empty string"</em>. |
| `...`     | —              | The variable argument list corresponding to `format`.                                                                   |

| Return Value    | Description         |
| --------------- | ------------------- |
| `dstr_status_t` | Global status code. |

> [!WARNING]
>
> `format` must not point to the internal buffer of `dest`; if self-append is needed, create a temporary copy first.

<a id="api-functions-content-editing-dstr_cat_vformat"></a>

#### `dstr_cat_vformat()` [↑](#api-functions-content-editing)

Appends a string to a "dynamic string" using a format string (`va_list` version).

```c
dstr_status_t dstr_cat_vformat(
	dstr_adt *dest,
	const char *format,
	va_list args
);
```

| Parameter | Type           | Description                                                                                                                                                                                                                                            |
| --------- | -------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `dest`    | `dstr_adt *`   | Pointer to the target "dynamic string".<br>A _null pointer_ is treated as an invalid argument.                                                                                                                                                         |
| `format`  | `const char *` | Pointer to the format "C string".<br>Appends an empty string (appending nothing) when it is an <em>"empty string"</em>.                                                                                                                                |
| `args`    | `va_list`      | A `va_list` variable that has been initialized by `va_start()`, containing the variable argument list information corresponding to `format`.<br>This function does not call `va_end()`; the caller is responsible for managing the lifetime of `args`. |

| Return Value    | Description         |
| --------------- | ------------------- |
| `dstr_status_t` | Global status code. |

> [!NOTE]
>
> `args` may be read and consumed by this function and should not be reused after the call, unless reinitialized with `va_start()` or a copy is created with `va_copy()` for use with this function.

> [!WARNING]
>
> `format` must not point to the internal buffer of `dest`; if self-append is needed, create a temporary copy first.

<a id="api-functions-content-editing-dstr_insert_cstr"></a>

#### `dstr_insert_cstr()` [↑](#api-functions-content-editing)

Inserts a "C string" into a "dynamic string".

```c
dstr_status_t dstr_insert_cstr(
	dstr_adt *dest,
	size_t index,
	const char *src
);
```

| Parameter | Type           | Description                                                                                                             |
| --------- | -------------- | ----------------------------------------------------------------------------------------------------------------------- |
| `dest`    | `dstr_adt *`   | Pointer to the target "dynamic string".<br>A _null pointer_ is treated as an invalid argument.                          |
| `index`   | `size_t`       | The index at which to insert.<br>_Out of bounds_ is treated as an invalid argument.                                     |
| `src`     | `const char *` | Pointer to the source "C string".<br>Inserts an empty string (inserting nothing) when it is an <em>"empty string"</em>. |

| Return Value    | Description         |
| --------------- | ------------------- |
| `dstr_status_t` | Global status code. |

> [!WARNING]
>
> `src` must not point to the internal buffer of `dest`; if self-insert is needed, create a temporary copy first.

<a id="api-functions-content-editing-dstr_insert"></a>

#### `dstr_insert()` [↑](#api-functions-content-editing)

Inserts a "dynamic string" into another "dynamic string".

```c
dstr_status_t dstr_insert(
	dstr_adt *dest,
	size_t index,
	const dstr_adt *src
);
```

| Parameter | Type               | Description                                                                                                                   |
| --------- | ------------------ | ----------------------------------------------------------------------------------------------------------------------------- |
| `dest`    | `dstr_adt *`       | Pointer to the target "dynamic string".<br>A _null pointer_ is treated as an invalid argument.                                |
| `index`   | `size_t`           | The index at which to insert.<br>_Out of bounds_ is treated as an invalid argument.                                           |
| `src`     | `const dstr_adt *` | Pointer to the source "dynamic string".<br>Inserts an empty string (inserting nothing) when it is an <em>"empty string"</em>. |

| Return Value    | Description         |
| --------------- | ------------------- |
| `dstr_status_t` | Global status code. |

> [!WARNING]
>
> `src` must not point to the internal buffer of `dest`; if self-insert is needed, create a temporary copy first.

<a id="api-functions-content-editing-dstr_insert_sub_cstr"></a>

#### `dstr_insert_sub_cstr()` [↑](#api-functions-content-editing)

Inserts a substring of a "C string" into a "dynamic string".

```c
dstr_status_t dstr_insert_sub_cstr(
	dstr_adt *dest,
	size_t index,
	const char *src,
	size_t sub_start,
	size_t sub_length
);
```

| Parameter    | Type           | Description                                                                                                                                                    |
| ------------ | -------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `dest`       | `dstr_adt *`   | Pointer to the target "dynamic string".<br>A _null pointer_ is treated as an invalid argument.                                                                 |
| `index`      | `size_t`       | The index at which to insert.<br>_Out of bounds_ is treated as an invalid argument.                                                                            |
| `src`        | `const char *` | Pointer to the source "C string".<br>Inserts an empty string (inserting nothing) when it is an <em>"empty string"</em>, ignoring `sub_start` and `sub_length`. |
| `sub_start`  | `size_t`       | Starting index of the substring.<br>_Out of bounds_ is treated as an invalid argument.                                                                         |
| `sub_length` | `size_t`       | Length of the substring.<br>_`0`_ means to the end.<br>_Out of bounds_ is treated as an invalid argument.                                                      |

| Return Value    | Description         |
| --------------- | ------------------- |
| `dstr_status_t` | Global status code. |

> [!WARNING]
>
> `src` must not point to the internal buffer of `dest`; if self-insert is needed, create a temporary copy first.

<a id="api-functions-content-editing-dstr_insert_sub"></a>

#### `dstr_insert_sub()` [↑](#api-functions-content-editing)

Inserts a substring of a "dynamic string" into another "dynamic string".

```c
dstr_status_t dstr_insert_sub(
	dstr_adt *dest,
	size_t index,
	const dstr_adt *src,
	size_t sub_start,
	size_t sub_length
);
```

| Parameter    | Type               | Description                                                                                                                                                          |
| ------------ | ------------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `dest`       | `dstr_adt *`       | Pointer to the target "dynamic string".<br>A _null pointer_ is treated as an invalid argument.                                                                       |
| `index`      | `size_t`           | The index at which to insert.<br>_Out of bounds_ is treated as an invalid argument.                                                                                  |
| `src`        | `const dstr_adt *` | Pointer to the source "dynamic string".<br>Inserts an empty string (inserting nothing) when it is an <em>"empty string"</em>, ignoring `sub_start` and `sub_length`. |
| `sub_start`  | `size_t`           | Starting index of the substring.<br>_Out of bounds_ is treated as an invalid argument.                                                                               |
| `sub_length` | `size_t`           | Length of the substring.<br>_`0`_ means to the end.<br>_Out of bounds_ is treated as an invalid argument.                                                            |

| Return Value    | Description         |
| --------------- | ------------------- |
| `dstr_status_t` | Global status code. |

> [!WARNING]
>
> `src` must not point to the internal buffer of `dest`; if self-insert is needed, create a temporary copy first.

<a id="api-functions-content-editing-dstr_insert_format"></a>

#### `dstr_insert_format()` [↑](#api-functions-content-editing)

Inserts a string into a "dynamic string" using a format string.

```c
dstr_status_t dstr_insert_format(
	dstr_adt *dest,
	size_t index,
	const char *format,
	...
);
```

| Parameter | Type           | Description                                                                                                             |
| --------- | -------------- | ----------------------------------------------------------------------------------------------------------------------- |
| `dest`    | `dstr_adt *`   | Pointer to the target "dynamic string".<br>A _null pointer_ is treated as an invalid argument.                          |
| `index`   | `size_t`       | The index at which to insert.<br>_Out of bounds_ is treated as an invalid argument.                                     |
| `format`  | `const char *` | Pointer to the format "C string".<br>Inserts an empty string (inserting nothing) when it is an <em>"empty string"</em>. |
| `...`     | —              | The variable argument list corresponding to `format`.                                                                   |

| Return Value    | Description         |
| --------------- | ------------------- |
| `dstr_status_t` | Global status code. |

> [!WARNING]
>
> `format` must not point to the internal buffer of `dest`; if self-insert is needed, create a temporary copy first.

<a id="api-functions-content-editing-dstr_insert_vformat"></a>

#### `dstr_insert_vformat()` [↑](#api-functions-content-editing)

Inserts a string into a "dynamic string" using a format string (`va_list` version).

```c
dstr_status_t dstr_insert_vformat(
	dstr_adt *dest,
	size_t index,
	const char *format,
	va_list args
);
```

| Parameter | Type           | Description                                                                                                                                                                                                                                            |
| --------- | -------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `dest`    | `dstr_adt *`   | Pointer to the target "dynamic string".<br>A _null pointer_ is treated as an invalid argument.                                                                                                                                                         |
| `index`   | `size_t`       | The index at which to insert.<br>_Out of bounds_ is treated as an invalid argument.                                                                                                                                                                    |
| `format`  | `const char *` | Pointer to the format "C string".<br>Inserts an empty string (inserting nothing) when it is an <em>"empty string"</em>.                                                                                                                                |
| `args`    | `va_list`      | A `va_list` variable that has been initialized by `va_start()`, containing the variable argument list information corresponding to `format`.<br>This function does not call `va_end()`; the caller is responsible for managing the lifetime of `args`. |

| Return Value    | Description         |
| --------------- | ------------------- |
| `dstr_status_t` | Global status code. |

> [!NOTE]
>
> `args` may be read and consumed by this function and should not be reused after the call, unless reinitialized with `va_start()` or a copy is created with `va_copy()` for use with this function.

> [!WARNING]
>
> `format` must not point to the internal buffer of `dest`; if self-insert is needed, create a temporary copy first.

<a id="api-functions-content-editing-dstr_clear"></a>

#### `dstr_clear()` [↑](#api-functions-content-editing)

Clears a "dynamic string".

```c
void dstr_clear(
	dstr_adt *dstr
);
```

| Parameter | Type         | Description                                                                                              |
| --------- | ------------ | -------------------------------------------------------------------------------------------------------- |
| `dstr`    | `dstr_adt *` | Pointer to the target "dynamic string".<br>The function returns immediately when it is a _null pointer_. |

> [!NOTE]
>
> This function only sets its length to 0; it does not release the capacity immediately.

<a id="api-functions-content-editing-dstr_remove"></a>

#### `dstr_remove()` [↑](#api-functions-content-editing)

Removes a substring from a "dynamic string".

```c
void dstr_remove(
	dstr_adt *dstr,
	size_t sub_start,
	size_t sub_length
);
```

| Parameter    | Type         | Description                                                                                                                                               |
| ------------ | ------------ | --------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `dstr`       | `dstr_adt *` | Pointer to the target "dynamic string".<br>The function returns immediately when it is an <em>"empty string"</em>, ignoring `sub_start` and `sub_length`. |
| `sub_start`  | `size_t`     | Starting index of the substring.<br>The function returns immediately when _out of bounds_.                                                                |
| `sub_length` | `size_t`     | Length of the substring.<br>_`0`_ means to the end.<br>The function returns immediately when _out of bounds_.                                             |

<a id="api-functions-content-editing-dstr_trim"></a>

#### `dstr_trim()` [↑](#api-functions-content-editing)

Trims whitespace characters or specified characters from both ends of a "dynamic string".

```c
void dstr_trim(
	dstr_adt *dstr,
	const char *trim_chars
);
```

| Parameter    | Type           | Description                                                                                                                        |
| ------------ | -------------- | ---------------------------------------------------------------------------------------------------------------------------------- |
| `dstr`       | `dstr_adt *`   | Pointer to the target "dynamic string".<br>The function returns immediately when it is an <em>"empty string"</em>.                 |
| `trim_chars` | `const char *` | Pointer to the "C string" containing the characters to trim.<br>Trims whitespace characters when it is an <em>"empty string"</em>. |

> [!NOTE]
>
> Whitespace detection uses the C standard library function `isspace()`. This library does not modify the locale, so the result depends on the locale at call time.

<a id="api-functions-relation-and-comparison"></a>

### Relation and Comparison [↑](#api-functions)

This group of API functions is mainly used to determine and compare relations between two strings.

This group of API functions treats empty strings in a special way, following the relevant theorems in string theory.

The operations and their corresponding API functions and descriptions are as follows:

<table>
  <thead>
    <tr>
      <th>Operation Type</th>
      <th>Operation</th>
      <th>API Function</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td rowspan="12">Check</td>
      <td rowspan="3">Prefix check</td>
      <td><a href="#api-functions-relation-and-comparison-dstr_starts_with_cstr"><code>dstr_starts_with_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-relation-and-comparison-dstr_starts_with"><code>dstr_starts_with()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-relation-and-comparison-cstr_starts_with"><code>cstr_starts_with()</code></a></td>
    </tr>
    <tr>
      <td rowspan="3">Suffix check</td>
      <td><a href="#api-functions-relation-and-comparison-dstr_ends_with_cstr"><code>dstr_ends_with_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-relation-and-comparison-dstr_ends_with"><code>dstr_ends_with()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-relation-and-comparison-cstr_ends_with"><code>cstr_ends_with()</code></a></td>
    </tr>
    <tr>
      <td rowspan="3">Contains check</td>
      <td><a href="#api-functions-relation-and-comparison-dstr_contains_cstr"><code>dstr_contains_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-relation-and-comparison-dstr_contains"><code>dstr_contains()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-relation-and-comparison-cstr_contains"><code>cstr_contains()</code></a></td>
    </tr>
    <tr>
      <td rowspan="3">Equality check</td>
      <td><a href="#api-functions-relation-and-comparison-dstr_equals_cstr"><code>dstr_equals_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-relation-and-comparison-dstr_equals"><code>dstr_equals()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-relation-and-comparison-cstr_equals"><code>cstr_equals()</code></a></td>
    </tr>
    <tr>
      <td rowspan="3">Compare</td>
      <td rowspan="3">Relational compare</td>
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

Checks whether a "dynamic string" starts with a specified "C string" prefix.

```c
bool dstr_starts_with_cstr(
	const dstr_adt *dstr,
	const char *prefix
);
```

| Parameter | Type               | Description                             |
| --------- | ------------------ | --------------------------------------- |
| `dstr`    | `const dstr_adt *` | Pointer to the target "dynamic string". |
| `prefix`  | `const char *`     | Pointer to the prefix "C string".       |

| Return Value | Description                                                                                                                                                                                              |
| ------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `bool`       | Returns _`true`_ if it is a prefix, otherwise _`false`_.<br>If `prefix` is an <em>"empty string"</em>, this always returns _`true`_ (the <em>"empty string"</em> is regarded as a prefix of any string). |

<a id="api-functions-relation-and-comparison-dstr_starts_with"></a>

#### `dstr_starts_with()` [↑](#api-functions-relation-and-comparison)

Checks whether a "dynamic string" starts with a specified "dynamic string" prefix.

```c
bool dstr_starts_with(
	const dstr_adt *dstr,
	const dstr_adt *prefix
);
```

| Parameter | Type               | Description                             |
| --------- | ------------------ | --------------------------------------- |
| `dstr`    | `const dstr_adt *` | Pointer to the target "dynamic string". |
| `prefix`  | `const dstr_adt *` | Pointer to the prefix "dynamic string". |

| Return Value | Description                                                                                                                                                                                              |
| ------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `bool`       | Returns _`true`_ if it is a prefix, otherwise _`false`_.<br>If `prefix` is an <em>"empty string"</em>, this always returns _`true`_ (the <em>"empty string"</em> is regarded as a prefix of any string). |

<a id="api-functions-relation-and-comparison-cstr_starts_with"></a>

#### `cstr_starts_with()` [↑](#api-functions-relation-and-comparison)

Checks whether a "C string" starts with a specified "C string" prefix.

```c
bool cstr_starts_with(
	const char *cstr,
	const char *prefix
);
```

| Parameter | Type           | Description                       |
| --------- | -------------- | --------------------------------- |
| `cstr`    | `const char *` | Pointer to the target "C string". |
| `prefix`  | `const char *` | Pointer to the prefix "C string". |

| Return Value | Description                                                                                                                                                                                              |
| ------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `bool`       | Returns _`true`_ if it is a prefix, otherwise _`false`_.<br>If `prefix` is an <em>"empty string"</em>, this always returns _`true`_ (the <em>"empty string"</em> is regarded as a prefix of any string). |

<a id="api-functions-relation-and-comparison-dstr_ends_with_cstr"></a>

#### `dstr_ends_with_cstr()` [↑](#api-functions-relation-and-comparison)

Checks whether a "dynamic string" ends with a specified "C string" suffix.

```c
bool dstr_ends_with_cstr(
	const dstr_adt *dstr,
	const char *suffix
);
```

| Parameter | Type               | Description                             |
| --------- | ------------------ | --------------------------------------- |
| `dstr`    | `const dstr_adt *` | Pointer to the target "dynamic string". |
| `suffix`  | `const char *`     | Pointer to the suffix "C string".       |

| Return Value | Description                                                                                                                                                                                              |
| ------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `bool`       | Returns _`true`_ if it is a suffix, otherwise _`false`_.<br>If `suffix` is an <em>"empty string"</em>, this always returns _`true`_ (the <em>"empty string"</em> is regarded as a suffix of any string). |

<a id="api-functions-relation-and-comparison-dstr_ends_with"></a>

#### `dstr_ends_with()` [↑](#api-functions-relation-and-comparison)

Checks whether a "dynamic string" ends with a specified "dynamic string" suffix.

```c
bool dstr_ends_with(
	const dstr_adt *dstr,
	const dstr_adt *suffix
);
```

| Parameter | Type               | Description                             |
| --------- | ------------------ | --------------------------------------- |
| `dstr`    | `const dstr_adt *` | Pointer to the target "dynamic string". |
| `suffix`  | `const dstr_adt *` | Pointer to the suffix "dynamic string". |

| Return Value | Description                                                                                                                                                                                              |
| ------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `bool`       | Returns _`true`_ if it is a suffix, otherwise _`false`_.<br>If `suffix` is an <em>"empty string"</em>, this always returns _`true`_ (the <em>"empty string"</em> is regarded as a suffix of any string). |

<a id="api-functions-relation-and-comparison-cstr_ends_with"></a>

#### `cstr_ends_with()` [↑](#api-functions-relation-and-comparison)

Checks whether a "C string" ends with a specified "C string" suffix.

```c
bool cstr_ends_with(
	const char *cstr,
	const char *suffix
);
```

| Parameter | Type           | Description                       |
| --------- | -------------- | --------------------------------- |
| `cstr`    | `const char *` | Pointer to the target "C string". |
| `suffix`  | `const char *` | Pointer to the suffix "C string". |

| Return Value | Description                                                                                                                                                                                              |
| ------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `bool`       | Returns _`true`_ if it is a suffix, otherwise _`false`_.<br>If `suffix` is an <em>"empty string"</em>, this always returns _`true`_ (the <em>"empty string"</em> is regarded as a suffix of any string). |

<a id="api-functions-relation-and-comparison-dstr_contains_cstr"></a>

#### `dstr_contains_cstr()` [↑](#api-functions-relation-and-comparison)

Checks whether a "dynamic string" contains a specified "C string" substring.

```c
bool dstr_contains_cstr(
	const dstr_adt *dstr,
	const char *sub
);
```

| Parameter | Type               | Description                             |
| --------- | ------------------ | --------------------------------------- |
| `dstr`    | `const dstr_adt *` | Pointer to the target "dynamic string". |
| `sub`     | `const char *`     | Pointer to the "C string" substring.    |

| Return Value | Description                                                                                                                                                                                              |
| ------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `bool`       | Returns _`true`_ if it contains it, otherwise _`false`_.<br>If `sub` is an <em>"empty string"</em>, this always returns _`true`_ (the <em>"empty string"</em> is regarded as a substring of any string). |

<a id="api-functions-relation-and-comparison-dstr_contains"></a>

#### `dstr_contains()` [↑](#api-functions-relation-and-comparison)

Checks whether a "dynamic string" contains a specified "dynamic string" substring.

```c
bool dstr_contains(
	const dstr_adt *dstr,
	const dstr_adt *sub
);
```

| Parameter | Type               | Description                                |
| --------- | ------------------ | ------------------------------------------ |
| `dstr`    | `const dstr_adt *` | Pointer to the target "dynamic string".    |
| `sub`     | `const dstr_adt *` | Pointer to the "dynamic string" substring. |

| Return Value | Description                                                                                                                                                                                              |
| ------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `bool`       | Returns _`true`_ if it contains it, otherwise _`false`_.<br>If `sub` is an <em>"empty string"</em>, this always returns _`true`_ (the <em>"empty string"</em> is regarded as a substring of any string). |

<a id="api-functions-relation-and-comparison-cstr_contains"></a>

#### `cstr_contains()` [↑](#api-functions-relation-and-comparison)

Checks whether a "C string" contains a specified "C string" substring.

```c
bool cstr_contains(
	const char *cstr,
	const char *sub
);
```

| Parameter | Type           | Description                          |
| --------- | -------------- | ------------------------------------ |
| `cstr`    | `const char *` | Pointer to the target "C string".    |
| `sub`     | `const char *` | Pointer to the "C string" substring. |

| Return Value | Description                                                                                                                                                                                              |
| ------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `bool`       | Returns _`true`_ if it contains it, otherwise _`false`_.<br>If `sub` is an <em>"empty string"</em>, this always returns _`true`_ (the <em>"empty string"</em> is regarded as a substring of any string). |

<a id="api-functions-relation-and-comparison-dstr_equals_cstr"></a>

#### `dstr_equals_cstr()` [↑](#api-functions-relation-and-comparison)

Checks whether a "dynamic string" is equal to a "C string".

```c
bool dstr_equals_cstr(
	const dstr_adt *lhs,
	const char *rhs
);
```

| Parameter | Type               | Description                      |
| --------- | ------------------ | -------------------------------- |
| `lhs`     | `const dstr_adt *` | Pointer to the "dynamic string". |
| `rhs`     | `const char *`     | Pointer to the "C string".       |

| Return Value | Description                                                                                                  |
| ------------ | ------------------------------------------------------------------------------------------------------------ |
| `bool`       | Returns _`true`_ if equal, otherwise _`false`_.<br>When both are <em>"empty strings"</em>, returns _`true`_. |

<a id="api-functions-relation-and-comparison-dstr_equals"></a>

#### `dstr_equals()` [↑](#api-functions-relation-and-comparison)

Checks whether two "dynamic strings" are equal.

```c
bool dstr_equals(
	const dstr_adt *lhs,
	const dstr_adt *rhs
);
```

| Parameter | Type               | Description                             |
| --------- | ------------------ | --------------------------------------- |
| `lhs`     | `const dstr_adt *` | Pointer to the first "dynamic string".  |
| `rhs`     | `const dstr_adt *` | Pointer to the second "dynamic string". |

| Return Value | Description                                                                                                  |
| ------------ | ------------------------------------------------------------------------------------------------------------ |
| `bool`       | Returns _`true`_ if equal, otherwise _`false`_.<br>When both are <em>"empty strings"</em>, returns _`true`_. |

<a id="api-functions-relation-and-comparison-cstr_equals"></a>

#### `cstr_equals()` [↑](#api-functions-relation-and-comparison)

Checks whether two "C strings" are equal.

```c
bool cstr_equals(
	const char *lhs,
	const char *rhs
);
```

| Parameter | Type           | Description                       |
| --------- | -------------- | --------------------------------- |
| `lhs`     | `const char *` | Pointer to the first "C string".  |
| `rhs`     | `const char *` | Pointer to the second "C string". |

| Return Value | Description                                                                                                  |
| ------------ | ------------------------------------------------------------------------------------------------------------ |
| `bool`       | Returns _`true`_ if equal, otherwise _`false`_.<br>When both are <em>"empty strings"</em>, returns _`true`_. |

<a id="api-functions-relation-and-comparison-dstr_compare_cstr"></a>

#### `dstr_compare_cstr()` [↑](#api-functions-relation-and-comparison)

Compares a "dynamic string" with a "C string".

```c
int dstr_compare_cstr(
	const dstr_adt *lhs,
	const char *rhs
);
```

| Parameter | Type               | Description                      |
| --------- | ------------------ | -------------------------------- |
| `lhs`     | `const dstr_adt *` | Pointer to the "dynamic string". |
| `rhs`     | `const char *`     | Pointer to the "C string".       |

| Return Value | Description                                                                                                                                                                                                                                                                                                                                                        |
| ------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `int`        | Returns _`0`_ if the two are equal, a _positive value_ if the former is greater than the latter, and a _negative value_ if the former is less than the latter.<br>When both are <em>"empty strings"</em>, returns _`0`_.<br>When only one of them is an <em>"empty string"</em>, the <em>"empty string"</em> one is less than the non-<em>"empty string"</em> one. |

<a id="api-functions-relation-and-comparison-dstr_compare"></a>

#### `dstr_compare()` [↑](#api-functions-relation-and-comparison)

Compares two "dynamic strings".

```c
int dstr_compare(
	const dstr_adt *lhs,
	const dstr_adt *rhs
);
```

| Parameter | Type               | Description                             |
| --------- | ------------------ | --------------------------------------- |
| `lhs`     | `const dstr_adt *` | Pointer to the first "dynamic string".  |
| `rhs`     | `const dstr_adt *` | Pointer to the second "dynamic string". |

| Return Value | Description                                                                                                                                                                                                                                                                                                                                                        |
| ------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `int`        | Returns _`0`_ if the two are equal, a _positive value_ if the former is greater than the latter, and a _negative value_ if the former is less than the latter.<br>When both are <em>"empty strings"</em>, returns _`0`_.<br>When only one of them is an <em>"empty string"</em>, the <em>"empty string"</em> one is less than the non-<em>"empty string"</em> one. |

<a id="api-functions-relation-and-comparison-cstr_compare"></a>

#### `cstr_compare()` [↑](#api-functions-relation-and-comparison)

Compares two "C strings".

```c
int cstr_compare(
	const char *lhs,
	const char *rhs
);
```

| Parameter | Type           | Description                       |
| --------- | -------------- | --------------------------------- |
| `lhs`     | `const char *` | Pointer to the first "C string".  |
| `rhs`     | `const char *` | Pointer to the second "C string". |

| Return Value | Description                                                                                                                                                                                                                                                                                                                                                        |
| ------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `int`        | Returns _`0`_ if the two are equal, a _positive value_ if the former is greater than the latter, and a _negative value_ if the former is less than the latter.<br>When both are <em>"empty strings"</em>, returns _`0`_.<br>When only one of them is an <em>"empty string"</em>, the <em>"empty string"</em> one is less than the non-<em>"empty string"</em> one. |

<a id="api-functions-find-count-and-replace"></a>

### Find, Count, and Replace [↑](#api-functions)

This group of API functions is mainly used to find, count, and replace within a "dynamic string".

The operations and their corresponding API functions and descriptions are as follows:

<table>
  <thead>
    <tr>
      <th>Operation Type</th>
      <th>Operation</th>
      <th>API Function</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td rowspan="9">Find</td>
      <td rowspan="3">First occurrence</td>
      <td><a href="#api-functions-find-count-and-replace-dstr_find_cstr"><code>dstr_find_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-find-count-and-replace-dstr_find"><code>dstr_find()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-find-count-and-replace-cstr_find"><code>cstr_find()</code></a></td>
    </tr>
    <tr>
      <td rowspan="3">nth occurrence</td>
      <td><a href="#api-functions-find-count-and-replace-dstr_find_nth_cstr"><code>dstr_find_nth_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-find-count-and-replace-dstr_find_nth"><code>dstr_find_nth()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-find-count-and-replace-cstr_find_nth"><code>cstr_find_nth()</code></a></td>
    </tr>
    <tr>
      <td rowspan="3">First n occurrences</td>
      <td><a href="#api-functions-find-count-and-replace-dstr_find_indexes_cstr"><code>dstr_find_indexes_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-find-count-and-replace-dstr_find_indexes"><code>dstr_find_indexes()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-find-count-and-replace-cstr_find_indexes"><code>cstr_find_indexes()</code></a></td>
    </tr>
    <tr>
      <td rowspan="3">Count</td>
      <td rowspan="3">Occurrences</td>
      <td><a href="#api-functions-find-count-and-replace-dstr_count_cstr"><code>dstr_count_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-find-count-and-replace-dstr_count"><code>dstr_count()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-find-count-and-replace-cstr_count"><code>cstr_count()</code></a></td>
    </tr>
    <tr>
      <td rowspan="4">Replace</td>
      <td rowspan="2">Replace n occurrences</td>
      <td><a href="#api-functions-find-count-and-replace-dstr_replace_cstr"><code>dstr_replace_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-find-count-and-replace-dstr_replace"><code>dstr_replace()</code></a></td>
    </tr>
    <tr>
      <td rowspan="2">Replace nth occurrence</td>
      <td><a href="#api-functions-find-count-and-replace-dstr_replace_nth_cstr"><code>dstr_replace_nth_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-find-count-and-replace-dstr_replace_nth"><code>dstr_replace_nth()</code></a></td>
    </tr>
  </tbody>
</table>

<a id="api-functions-find-count-and-replace-dstr_find_cstr"></a>

#### `dstr_find_cstr()` [↑](#api-functions-find-count-and-replace)

Finds the position of the first occurrence of a specified "C string" substring in a "dynamic string".

```c
bool dstr_find_cstr(
	const dstr_adt *dstr,
	const char *sub,
	size_t *out_index,
	dstr_direction_t direction
);
```

| Parameter   | Type               | Description                                                                                                                       |
| ----------- | ------------------ | --------------------------------------------------------------------------------------------------------------------------------- |
| `dstr`      | `const dstr_adt *` | Pointer to the target "dynamic string".<br>The function returns _`false`_ immediately when it is an <em>"empty string"</em>.      |
| `sub`       | `const char *`     | Pointer to the "C string" substring.<br>The function returns _`false`_ immediately when it is an <em>"empty string"</em>.         |
| `out_index` | `size_t *`         | Pointer to a `size_t` variable that stores the search result (position index).<br>Nothing is written when it is a _null pointer_. |
| `direction` | `dstr_direction_t` | Search direction.                                                                                                                 |

| Return Value | Description                                     |
| ------------ | ----------------------------------------------- |
| `bool`       | Returns _`true`_ if found, otherwise _`false`_. |

<a id="api-functions-find-count-and-replace-dstr_find"></a>

#### `dstr_find()` [↑](#api-functions-find-count-and-replace)

Finds the position of the first occurrence of a specified "dynamic string" substring in a "dynamic string".

```c
bool dstr_find(
	const dstr_adt *dstr,
	const dstr_adt *sub,
	size_t *out_index,
	dstr_direction_t direction
);
```

| Parameter   | Type               | Description                                                                                                                       |
| ----------- | ------------------ | --------------------------------------------------------------------------------------------------------------------------------- |
| `dstr`      | `const dstr_adt *` | Pointer to the target "dynamic string".<br>The function returns _`false`_ immediately when it is an <em>"empty string"</em>.      |
| `sub`       | `const dstr_adt *` | Pointer to the "dynamic string" substring.<br>The function returns _`false`_ immediately when it is an <em>"empty string"</em>.   |
| `out_index` | `size_t *`         | Pointer to a `size_t` variable that stores the search result (position index).<br>Nothing is written when it is a _null pointer_. |
| `direction` | `dstr_direction_t` | Search direction.                                                                                                                 |

| Return Value | Description                                     |
| ------------ | ----------------------------------------------- |
| `bool`       | Returns _`true`_ if found, otherwise _`false`_. |

<a id="api-functions-find-count-and-replace-cstr_find"></a>

#### `cstr_find()` [↑](#api-functions-find-count-and-replace)

Finds the position of the first occurrence of a specified "C string" substring in a "C string".

```c
bool cstr_find(
	const char *cstr,
	const char *sub,
	size_t *out_index,
	dstr_direction_t direction
);
```

| Parameter   | Type               | Description                                                                                                                       |
| ----------- | ------------------ | --------------------------------------------------------------------------------------------------------------------------------- |
| `cstr`      | `const char *`     | Pointer to the target "C string".<br>The function returns _`false`_ immediately when it is an <em>"empty string"</em>.            |
| `sub`       | `const char *`     | Pointer to the "C string" substring.<br>The function returns _`false`_ immediately when it is an <em>"empty string"</em>.         |
| `out_index` | `size_t *`         | Pointer to a `size_t` variable that stores the search result (position index).<br>Nothing is written when it is a _null pointer_. |
| `direction` | `dstr_direction_t` | Search direction.                                                                                                                 |

| Return Value | Description                                     |
| ------------ | ----------------------------------------------- |
| `bool`       | Returns _`true`_ if found, otherwise _`false`_. |

<a id="api-functions-find-count-and-replace-dstr_find_nth_cstr"></a>

#### `dstr_find_nth_cstr()` [↑](#api-functions-find-count-and-replace)

Finds the position of the nth occurrence of a specified "C string" substring in a "dynamic string".

```c
bool dstr_find_nth_cstr(
	const dstr_adt *dstr,
	const char *sub,
	size_t *out_index,
	dstr_direction_t direction,
	size_t n
);
```

| Parameter   | Type               | Description                                                                                                                                                               |
| ----------- | ------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `dstr`      | `const dstr_adt *` | Pointer to the target "dynamic string".<br>The function returns _`false`_ immediately when it is an <em>"empty string"</em>.                                              |
| `sub`       | `const char *`     | Pointer to the "C string" substring.<br>The function returns _`false`_ immediately when it is an <em>"empty string"</em>.                                                 |
| `out_index` | `size_t *`         | Pointer to a `size_t` variable that stores the search result (position index).<br>Nothing is written when it is a _null pointer_.                                         |
| `direction` | `dstr_direction_t` | Search direction.                                                                                                                                                         |
| `n`         | `size_t`           | The order of the occurrence.<br>Starts from _`1`_.<br>_`0`_ means the last one.<br>If it is greater than the actual number of occurrences, it is treated as the last one. |

| Return Value | Description                                     |
| ------------ | ----------------------------------------------- |
| `bool`       | Returns _`true`_ if found, otherwise _`false`_. |

<a id="api-functions-find-count-and-replace-dstr_find_nth"></a>

#### `dstr_find_nth()` [↑](#api-functions-find-count-and-replace)

Finds the position of the nth occurrence of a specified "dynamic string" substring in a "dynamic string".

```c
bool dstr_find_nth(
	const dstr_adt *dstr,
	const dstr_adt *sub,
	size_t *out_index,
	dstr_direction_t direction,
	size_t n
);
```

| Parameter   | Type               | Description                                                                                                                                                               |
| ----------- | ------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `dstr`      | `const dstr_adt *` | Pointer to the target "dynamic string".<br>The function returns _`false`_ immediately when it is an <em>"empty string"</em>.                                              |
| `sub`       | `const dstr_adt *` | Pointer to the "dynamic string" substring.<br>The function returns _`false`_ immediately when it is an <em>"empty string"</em>.                                           |
| `out_index` | `size_t *`         | Pointer to a `size_t` variable that stores the search result (position index).<br>Nothing is written when it is a _null pointer_.                                         |
| `direction` | `dstr_direction_t` | Search direction.                                                                                                                                                         |
| `n`         | `size_t`           | The order of the occurrence.<br>Starts from _`1`_.<br>_`0`_ means the last one.<br>If it is greater than the actual number of occurrences, it is treated as the last one. |

| Return Value | Description                                     |
| ------------ | ----------------------------------------------- |
| `bool`       | Returns _`true`_ if found, otherwise _`false`_. |

<a id="api-functions-find-count-and-replace-cstr_find_nth"></a>

#### `cstr_find_nth()` [↑](#api-functions-find-count-and-replace)

Finds the position of the nth occurrence of a specified "C string" substring in a "C string".

```c
bool cstr_find_nth(
	const char *cstr,
	const char *sub,
	size_t *out_index,
	dstr_direction_t direction,
	size_t n
);
```

| Parameter   | Type               | Description                                                                                                                                                               |
| ----------- | ------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `cstr`      | `const char *`     | Pointer to the target "C string".<br>The function returns _`false`_ immediately when it is an <em>"empty string"</em>.                                                    |
| `sub`       | `const char *`     | Pointer to the "C string" substring.<br>The function returns _`false`_ immediately when it is an <em>"empty string"</em>.                                                 |
| `out_index` | `size_t *`         | Pointer to a `size_t` variable that stores the search result (position index).<br>Nothing is written when it is a _null pointer_.                                         |
| `direction` | `dstr_direction_t` | Search direction.                                                                                                                                                         |
| `n`         | `size_t`           | The order of the occurrence.<br>Starts from _`1`_.<br>_`0`_ means the last one.<br>If it is greater than the actual number of occurrences, it is treated as the last one. |

| Return Value | Description                                     |
| ------------ | ----------------------------------------------- |
| `bool`       | Returns _`true`_ if found, otherwise _`false`_. |

<a id="api-functions-find-count-and-replace-dstr_find_indexes_cstr"></a>

#### `dstr_find_indexes_cstr()` [↑](#api-functions-find-count-and-replace)

Finds the positions of the first n occurrences of a specified "C string" substring in a "dynamic string".

```c
size_t dstr_find_indexes_cstr(
	const dstr_adt *dstr,
	const char *sub,
	size_t *out_indexes,
	dstr_direction_t direction,
	size_t n
);
```

| Parameter     | Type               | Description                                                                                                                                                                            |
| ------------- | ------------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `dstr`        | `const dstr_adt *` | Pointer to the target "dynamic string".<br>The function returns _`0`_ immediately when it is an <em>"empty string"</em>.                                                               |
| `sub`         | `const char *`     | Pointer to the "C string" substring.<br>The function returns _`0`_ immediately when it is an <em>"empty string"</em>.                                                                  |
| `out_indexes` | `size_t *`         | Pointer to a `size_t` array that stores the search results (position indexes).<br>Nothing is written when it is a _null pointer_.<br>Please ensure the array capacity is large enough. |
| `direction`   | `dstr_direction_t` | Search direction.                                                                                                                                                                      |
| `n`           | `size_t`           | The number of searches.<br>Starts from _`1`_.<br>_`0`_ means search for all.<br>If it is greater than the actual number of occurrences, all occurrences will be searched.              |

| Return Value | Description                                                                                                                              |
| ------------ | ---------------------------------------------------------------------------------------------------------------------------------------- |
| `size_t`     | The actual number of occurrences up to the `n`-th one, which is also the number of elements actually written to the `out_indexes` array. |

<a id="api-functions-find-count-and-replace-dstr_find_indexes"></a>

#### `dstr_find_indexes()` [↑](#api-functions-find-count-and-replace)

Finds the positions of the first n occurrences of a specified "dynamic string" substring in a "dynamic string".

```c
size_t dstr_find_indexes(
	const dstr_adt *dstr,
	const dstr_adt *sub,
	size_t *out_indexes,
	dstr_direction_t direction,
	size_t n
);
```

| Parameter     | Type               | Description                                                                                                                                                                            |
| ------------- | ------------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `dstr`        | `const dstr_adt *` | Pointer to the target "dynamic string".<br>The function returns _`0`_ immediately when it is an <em>"empty string"</em>.                                                               |
| `sub`         | `const dstr_adt *` | Pointer to the "dynamic string" substring.<br>The function returns _`0`_ immediately when it is an <em>"empty string"</em>.                                                            |
| `out_indexes` | `size_t *`         | Pointer to a `size_t` array that stores the search results (position indexes).<br>Nothing is written when it is a _null pointer_.<br>Please ensure the array capacity is large enough. |
| `direction`   | `dstr_direction_t` | Search direction.                                                                                                                                                                      |
| `n`           | `size_t`           | The number of searches.<br>Starts from _`1`_.<br>_`0`_ means search for all.<br>If it is greater than the actual number of occurrences, all occurrences will be searched.              |

| Return Value | Description                                                                                                                              |
| ------------ | ---------------------------------------------------------------------------------------------------------------------------------------- |
| `size_t`     | The actual number of occurrences up to the `n`-th one, which is also the number of elements actually written to the `out_indexes` array. |

<a id="api-functions-find-count-and-replace-cstr_find_indexes"></a>

#### `cstr_find_indexes()` [↑](#api-functions-find-count-and-replace)

Finds the positions of the first n occurrences of a specified "C string" substring in a "C string".

```c
size_t cstr_find_indexes(
	const char *cstr,
	const char *sub,
	size_t *out_indexes,
	dstr_direction_t direction,
	size_t n
);
```

| Parameter     | Type               | Description                                                                                                                                                                            |
| ------------- | ------------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `cstr`        | `const char *`     | Pointer to the target "C string".<br>The function returns _`0`_ immediately when it is an <em>"empty string"</em>.                                                                     |
| `sub`         | `const char *`     | Pointer to the "C string" substring.<br>The function returns _`0`_ immediately when it is an <em>"empty string"</em>.                                                                  |
| `out_indexes` | `size_t *`         | Pointer to a `size_t` array that stores the search results (position indexes).<br>Nothing is written when it is a _null pointer_.<br>Please ensure the array capacity is large enough. |
| `direction`   | `dstr_direction_t` | Search direction.                                                                                                                                                                      |
| `n`           | `size_t`           | The number of searches.<br>Starts from _`1`_.<br>_`0`_ means search for all.<br>If it is greater than the actual number of occurrences, all occurrences will be searched.              |

| Return Value | Description                                                                                                                              |
| ------------ | ---------------------------------------------------------------------------------------------------------------------------------------- |
| `size_t`     | The actual number of occurrences up to the `n`-th one, which is also the number of elements actually written to the `out_indexes` array. |

<a id="api-functions-find-count-and-replace-dstr_count_cstr"></a>

#### `dstr_count_cstr()` [↑](#api-functions-find-count-and-replace)

Counts the number of occurrences of a specified "C string" substring in a "dynamic string".

```c
size_t dstr_count_cstr(
	const dstr_adt *dstr,
	const char *sub
);
```

| Parameter | Type               | Description                                                                                                              |
| --------- | ------------------ | ------------------------------------------------------------------------------------------------------------------------ |
| `dstr`    | `const dstr_adt *` | Pointer to the target "dynamic string".<br>The function returns _`0`_ immediately when it is an <em>"empty string"</em>. |
| `sub`     | `const char *`     | Pointer to the "C string" substring.<br>The function returns _`0`_ immediately when it is an <em>"empty string"</em>.    |

| Return Value | Description                |
| ------------ | -------------------------- |
| `size_t`     | The number of occurrences. |

<a id="api-functions-find-count-and-replace-dstr_count"></a>

#### `dstr_count()` [↑](#api-functions-find-count-and-replace)

Counts the number of occurrences of a specified "dynamic string" substring in a "dynamic string".

```c
size_t dstr_count(
	const dstr_adt *dstr,
	const dstr_adt *sub
);
```

| Parameter | Type               | Description                                                                                                                 |
| --------- | ------------------ | --------------------------------------------------------------------------------------------------------------------------- |
| `dstr`    | `const dstr_adt *` | Pointer to the target "dynamic string".<br>The function returns _`0`_ immediately when it is an <em>"empty string"</em>.    |
| `sub`     | `const dstr_adt *` | Pointer to the "dynamic string" substring.<br>The function returns _`0`_ immediately when it is an <em>"empty string"</em>. |

| Return Value | Description                |
| ------------ | -------------------------- |
| `size_t`     | The number of occurrences. |

<a id="api-functions-find-count-and-replace-cstr_count"></a>

#### `cstr_count()` [↑](#api-functions-find-count-and-replace)

Counts the number of occurrences of a specified "C string" substring in a "C string".

```c
size_t cstr_count(
	const char *cstr,
	const char *sub
);
```

| Parameter | Type           | Description                                                                                                           |
| --------- | -------------- | --------------------------------------------------------------------------------------------------------------------- |
| `cstr`    | `const char *` | Pointer to the target "C string".<br>The function returns _`0`_ immediately when it is an <em>"empty string"</em>.    |
| `sub`     | `const char *` | Pointer to the "C string" substring.<br>The function returns _`0`_ immediately when it is an <em>"empty string"</em>. |

| Return Value | Description                |
| ------------ | -------------------------- |
| `size_t`     | The number of occurrences. |

<a id="api-functions-find-count-and-replace-dstr_replace_cstr"></a>

#### `dstr_replace_cstr()` [↑](#api-functions-find-count-and-replace)

Replaces the specified old "C string" in a "dynamic string" with the specified new "C string" n times.

```c
dstr_status_t dstr_replace_cstr(
	dstr_adt *dstr,
	const char *old_str,
	const char *new_str,
	dstr_direction_t direction,
	size_t n
);
```

| Parameter   | Type               | Description                                                                                                                                                                                                                           |
| ----------- | ------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `dstr`      | `dstr_adt *`       | Pointer to the target "dynamic string".<br>An <em>"empty string"</em> is treated as an invalid argument.                                                                                                                              |
| `old_str`   | `const char *`     | Pointer to the old "C string".<br>An <em>"empty string"</em> is treated as an invalid argument.<br>It is treated as an invalid argument if it never appears in `dstr`, or if it appears fewer than `n` times (when `n` is not _`0`_). |
| `new_str`   | `const char *`     | Pointer to the new "C string".                                                                                                                                                                                                        |
| `direction` | `dstr_direction_t` | Replacement direction.                                                                                                                                                                                                                |
| `n`         | `size_t`           | The number of replacements.<br>_`0`_ means replace all.                                                                                                                                                                               |

| Return Value    | Description         |
| --------------- | ------------------- |
| `dstr_status_t` | Global status code. |

<a id="api-functions-find-count-and-replace-dstr_replace"></a>

#### `dstr_replace()` [↑](#api-functions-find-count-and-replace)

Replaces the specified old "dynamic string" in a "dynamic string" with the specified new "dynamic string" n times.

```c
dstr_status_t dstr_replace(
	dstr_adt *dstr,
	const dstr_adt *old_str,
	const dstr_adt *new_str,
	dstr_direction_t direction,
	size_t n
);
```

| Parameter   | Type               | Description                                                                                                                                                                                                                                 |
| ----------- | ------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `dstr`      | `dstr_adt *`       | Pointer to the target "dynamic string".<br>An <em>"empty string"</em> is treated as an invalid argument.                                                                                                                                    |
| `old_str`   | `const dstr_adt *` | Pointer to the old "dynamic string".<br>An <em>"empty string"</em> is treated as an invalid argument.<br>It is treated as an invalid argument if it never appears in `dstr`, or if it appears fewer than `n` times (when `n` is not _`0`_). |
| `new_str`   | `const dstr_adt *` | Pointer to the new "dynamic string".                                                                                                                                                                                                        |
| `direction` | `dstr_direction_t` | Replacement direction.                                                                                                                                                                                                                      |
| `n`         | `size_t`           | The number of replacements.<br>_`0`_ means replace all.                                                                                                                                                                                     |

| Return Value    | Description         |
| --------------- | ------------------- |
| `dstr_status_t` | Global status code. |

<a id="api-functions-find-count-and-replace-dstr_replace_nth_cstr"></a>

#### `dstr_replace_nth_cstr()` [↑](#api-functions-find-count-and-replace)

Replaces the nth occurrence of the specified old "C string" in a "dynamic string" with the specified new "C string".

```c
dstr_status_t dstr_replace_nth_cstr(
	dstr_adt *dstr,
	const char *old_str,
	const char *new_str,
	dstr_direction_t direction,
	size_t n
);
```

| Parameter   | Type               | Description                                                                                                                                                                                                                           |
| ----------- | ------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `dstr`      | `dstr_adt *`       | Pointer to the target "dynamic string".<br>An <em>"empty string"</em> is treated as an invalid argument.                                                                                                                              |
| `old_str`   | `const char *`     | Pointer to the old "C string".<br>An <em>"empty string"</em> is treated as an invalid argument.<br>It is treated as an invalid argument if it never appears in `dstr`, or if it appears fewer than `n` times (when `n` is not _`0`_). |
| `new_str`   | `const char *`     | Pointer to the new "C string".                                                                                                                                                                                                        |
| `direction` | `dstr_direction_t` | Replacement direction.                                                                                                                                                                                                                |
| `n`         | `size_t`           | The order of the replacement.<br>Starts from _`1`_.<br>_`0`_ means the last one.                                                                                                                                                      |

| Return Value    | Description         |
| --------------- | ------------------- |
| `dstr_status_t` | Global status code. |

<a id="api-functions-find-count-and-replace-dstr_replace_nth"></a>

#### `dstr_replace_nth()` [↑](#api-functions-find-count-and-replace)

Replaces the nth occurrence of the specified old "dynamic string" in a "dynamic string" with the specified new "dynamic string".

```c
dstr_status_t dstr_replace_nth(
	dstr_adt *dstr,
	const dstr_adt *old_str,
	const dstr_adt *new_str,
	dstr_direction_t direction,
	size_t n
);
```

| Parameter   | Type               | Description                                                                                                                                                                                                                                 |
| ----------- | ------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `dstr`      | `dstr_adt *`       | Pointer to the target "dynamic string".<br>An <em>"empty string"</em> is treated as an invalid argument.                                                                                                                                    |
| `old_str`   | `const dstr_adt *` | Pointer to the old "dynamic string".<br>An <em>"empty string"</em> is treated as an invalid argument.<br>It is treated as an invalid argument if it never appears in `dstr`, or if it appears fewer than `n` times (when `n` is not _`0`_). |
| `new_str`   | `const dstr_adt *` | Pointer to the new "dynamic string".                                                                                                                                                                                                        |
| `direction` | `dstr_direction_t` | Replacement direction.                                                                                                                                                                                                                      |
| `n`         | `size_t`           | The order of the replacement.<br>Starts from _`1`_.<br>_`0`_ means the last one.                                                                                                                                                            |

| Return Value    | Description         |
| --------------- | ------------------- |
| `dstr_status_t` | Global status code. |

<a id="api-functions-split-and-join"></a>

### Split and Join [↑](#api-functions)

This group of API functions is mainly used to split a string and join multiple strings.

The operations and their corresponding API functions and descriptions are as follows:

<table>
  <thead>
    <tr>
      <th>Operation Type</th>
      <th>Operation</th>
      <th>API Function</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td rowspan="3">Split</td>
      <td rowspan="3">Split</td>
      <td><a href="#api-functions-split-and-join-dstr_split_cstr"><code>dstr_split_cstr()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-split-and-join-dstr_split"><code>dstr_split()</code></a></td>
    </tr>
    <tr>
      <td><a href="#api-functions-split-and-join-cstr_split"><code>cstr_split()</code></a></td>
    </tr>
    <tr>
      <td rowspan="3">Join</td>
      <td rowspan="3">Join</td>
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

Splits a "C string" into multiple "dynamic strings".

```c
dstr_adt **dstr_split_cstr(
	const char *cstr,
	const char *separator,
	size_t *out_dstr_count
);
```

| Parameter        | Type           | Description                                                                                                                                                                                                                                  |
| ---------------- | -------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `cstr`           | `const char *` | Pointer to the target "C string".<br>The function returns a _null pointer_ immediately when it is an <em>"empty string"</em>.                                                                                                                |
| `separator`      | `const char *` | Pointer to the separator "C string".<br>The function returns a _null pointer_ immediately when it is an <em>"empty string"</em>.<br>The function returns a _null pointer_ when it never appears in `cstr`.                                   |
| `out_dstr_count` | `size_t *`     | Pointer to a `size_t` variable that stores the number of "dynamic strings" after splitting.<br>The function returns a _null pointer_ immediately when it is a _null pointer_.<br>It is written only when the function executes successfully. |

| Return Value  | Description                                                                                                  |
| ------------- | ------------------------------------------------------------------------------------------------------------ |
| `dstr_adt **` | Pointer to the array of "dynamic strings" after splitting.<br>Returns a _null pointer_ on splitting failure. |

> [!NOTE]
>
> When splitting, if the length of a substring between two separators, or between a separator and a leading/trailing boundary, is _0_, it is stored in the array as a _null pointer_ (rather than an <em>empty "dynamic string"</em>).

> [!IMPORTANT]
>
> The return value points to heap memory, which may in turn contain pointers to other heap memory.
>
> Therefore, when releasing, first call `dstr_destroy()` on each element one by one, then call `free()` to release the array itself.

<a id="api-functions-split-and-join-dstr_split"></a>

#### `dstr_split()` [↑](#api-functions-split-and-join)

Splits a "dynamic string" into multiple "dynamic strings".

```c
dstr_adt **dstr_split(
	const dstr_adt *dstr,
	const dstr_adt *separator,
	size_t *out_dstr_count
);
```

| Parameter        | Type               | Description                                                                                                                                                                                                                                  |
| ---------------- | ------------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `dstr`           | `const dstr_adt *` | Pointer to the target "dynamic string".<br>The function returns a _null pointer_ immediately when it is an <em>"empty string"</em>.                                                                                                          |
| `separator`      | `const dstr_adt *` | Pointer to the separator "dynamic string".<br>The function returns a _null pointer_ immediately when it is an <em>"empty string"</em>.<br>The function returns a _null pointer_ when it never appears in `dstr`.                             |
| `out_dstr_count` | `size_t *`         | Pointer to a `size_t` variable that stores the number of "dynamic strings" after splitting.<br>The function returns a _null pointer_ immediately when it is a _null pointer_.<br>It is written only when the function executes successfully. |

| Return Value  | Description                                                                                                  |
| ------------- | ------------------------------------------------------------------------------------------------------------ |
| `dstr_adt **` | Pointer to the array of "dynamic strings" after splitting.<br>Returns a _null pointer_ on splitting failure. |

> [!NOTE]
>
> When splitting, if the length of a substring between two separators, or between a separator and a leading/trailing boundary, is _0_, it is stored in the array as a _null pointer_ (rather than an <em>empty "dynamic string"</em>).

> [!IMPORTANT]
>
> The return value points to heap memory, which may in turn contain pointers to other heap memory.
>
> Therefore, when releasing, first call `dstr_destroy()` on each element one by one, then call `free()` to release the array itself.

<a id="api-functions-split-and-join-cstr_split"></a>

#### `cstr_split()` [↑](#api-functions-split-and-join)

Splits a "C string" into multiple "C strings".

```c
char **cstr_split(
	const char *cstr,
	const char *separator,
	size_t *out_cstr_count
);
```

| Parameter        | Type           | Description                                                                                                                                                                                                                            |
| ---------------- | -------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `cstr`           | `const char *` | Pointer to the target "C string".<br>The function returns a _null pointer_ immediately when it is an <em>"empty string"</em>.                                                                                                          |
| `separator`      | `const char *` | Pointer to the separator "C string".<br>The function returns a _null pointer_ immediately when it is an <em>"empty string"</em>.<br>The function returns a _null pointer_ when it never appears in `cstr`.                             |
| `out_cstr_count` | `size_t *`     | Pointer to a `size_t` variable that stores the number of "C strings" after splitting.<br>The function returns a _null pointer_ immediately when it is a _null pointer_.<br>It is written only when the function executes successfully. |

| Return Value | Description                                                                                            |
| ------------ | ------------------------------------------------------------------------------------------------------ |
| `char **`    | Pointer to the array of "C strings" after splitting.<br>Returns a _null pointer_ on splitting failure. |

> [!NOTE]
>
> When splitting, if the length of a substring between two separators, or between a separator and a leading/trailing boundary, is _0_, it is stored in the array as a _null pointer_ (rather than an <em>empty "C string"</em>).

> [!IMPORTANT]
>
> The return value points to heap memory, which may in turn contain pointers to other heap memory.
>
> Therefore, when releasing, first call `free()` on each element one by one, then call `free()` to release the array itself.

<a id="api-functions-split-and-join-dstr_join_cstr"></a>

#### `dstr_join_cstr()` [↑](#api-functions-split-and-join)

Joins multiple "C strings" into one "dynamic string".

```c
dstr_adt *dstr_join_cstr(
	const char *const *cstrs,
	size_t cstr_count,
	const char *separator
);
```

| Parameter    | Type                  | Description                                                                                                                                                                                             |
| ------------ | --------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `cstrs`      | `const char *const *` | Pointer to the array of source "C strings".<br>The function returns a _null pointer_ immediately when it is a _null pointer_.<br>A _null pointer_ element in the array is treated as an "empty string". |
| `cstr_count` | `size_t`              | The number of source "C strings".<br>The function returns a _null pointer_ immediately when it is _`0`_.                                                                                                |
| `separator`  | `const char *`        | Pointer to the separator "C string".                                                                                                                                                                    |

| Return Value | Description                                                                          |
| ------------ | ------------------------------------------------------------------------------------ |
| `dstr_adt *` | Pointer to the joined "dynamic string".<br>Returns a _null pointer_ on join failure. |

> [!IMPORTANT]
>
> The return value points to heap memory; call `dstr_destroy()` manually to release it.

<a id="api-functions-split-and-join-dstr_join"></a>

#### `dstr_join()` [↑](#api-functions-split-and-join)

Joins multiple "dynamic strings" into one "dynamic string".

```c
dstr_adt *dstr_join(
	const dstr_adt *const *dstrs,
	size_t dstr_count,
	const dstr_adt *separator
);
```

| Parameter    | Type                      | Description                                                                                                                                                                                                   |
| ------------ | ------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `dstrs`      | `const dstr_adt *const *` | Pointer to the array of source "dynamic strings".<br>The function returns a _null pointer_ immediately when it is a _null pointer_.<br>A _null pointer_ element in the array is treated as an "empty string". |
| `dstr_count` | `size_t`                  | The number of source "dynamic strings".<br>The function returns a _null pointer_ immediately when it is _`0`_.                                                                                                |
| `separator`  | `const dstr_adt *`        | Pointer to the separator "dynamic string".                                                                                                                                                                    |

| Return Value | Description                                                                          |
| ------------ | ------------------------------------------------------------------------------------ |
| `dstr_adt *` | Pointer to the joined "dynamic string".<br>Returns a _null pointer_ on join failure. |

> [!IMPORTANT]
>
> The return value points to heap memory; call `dstr_destroy()` manually to release it.

<a id="api-functions-split-and-join-cstr_join"></a>

#### `cstr_join()` [↑](#api-functions-split-and-join)

Joins multiple "C strings" into one "C string".

```c
char *cstr_join(
	const char *const *cstrs,
	size_t cstr_count,
	const char *separator
);
```

| Parameter    | Type                  | Description                                                                                                                                                                                             |
| ------------ | --------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `cstrs`      | `const char *const *` | Pointer to the array of source "C strings".<br>The function returns a _null pointer_ immediately when it is a _null pointer_.<br>A _null pointer_ element in the array is treated as an "empty string". |
| `cstr_count` | `size_t`              | The number of source "C strings".<br>The function returns a _null pointer_ immediately when it is _`0`_.                                                                                                |
| `separator`  | `const char *`        | Pointer to the separator "C string".                                                                                                                                                                    |

| Return Value | Description                                                                    |
| ------------ | ------------------------------------------------------------------------------ |
| `char *`     | Pointer to the joined "C string".<br>Returns a _null pointer_ on join failure. |

> [!IMPORTANT]
>
> The return value points to heap memory; call `free()` manually to release it.
