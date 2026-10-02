/*
 * SPDX-FileCopyrightText: 2026 mtueih
 * SPDX-License-Identifier: ISC
 */

/*======================================================================================================================
 * include/dynamic_string.h - 项目主库头文件
 *====================================================================================================================*/
#ifndef DYNAMIC_STRING_H
#define DYNAMIC_STRING_H

/*======================================================================================================================
 * 总述
 *====================================================================================================================*/
/*
 * 1. 本库提供一个「动态字符串」ADT，底层数据始终是一个合法的「C 字符串」。
 *    字符串内容不得包含嵌入的 '\0'，且总是以 '\0' 结尾。
 *    因此本库不是二进制安全字符串库，而是文本字符串库。
 *
 * 2. 长度与容量：
 *    - dstr_length() 返回字符串长度，不包含结尾 '\0'。
 *    - dstr_capacity() 返回内部数据缓冲区字节数，包含结尾 '\0'。
 *    - 当前可保存的最大字符串长度为 dstr_capacity() - 1。
 *    - 实现可能设置最小容量下限，因此 dstr_set_capacity() 的参数是“请求容量”，
 *      实际容量可能大于请求值，调用者应以 dstr_capacity() 返回值为准。
 *    - 成功创建的「动态字符串」，其内部数据缓冲区始终非空，且至少能存放一个 '\0'。
 *
 * 3. 空指针与空字符串：
 *    - 本库约定：在需要字符串作为输入的参数位置，空指针通常被视为空字符串。
 *      具体以各函数注释为准。
 *    - 目标参数通常不允许为空指针，具体见各函数注释。
 *
 * 4. 查找、统计与替换：
 *    - 所有查找、统计与替换均按“非重叠匹配”处理。
 *      例如在 "aaa" 中查找 "aa"，只算出现 1 次，而不是 2 次。
 *    - 查找方向由 dstr_direction_t 指定。
 *    - contains 系列与 find 系列语义不同：
 *      contains 中空子串按 C 标准视为任意字符串的子串，返回 true；
 *      find 系列中空子串视为非法查找目标，返回 false。
 *
 * 5. 别名与内存重叠：
 *    - 当源参数可能指向目标「动态字符串」内部缓冲区时，调用者必须确保不发生内存重叠，
 *      否则可能导致未定义行为。具体接口若有此约束，会在函数注释中警告。
 *
 * 6. 内存所有权：
 *    - 所有返回 dstr_adt* 的创建类函数，返回值均指向堆内存，必须调用 dstr_destroy() 释放，
 *      除非函数注释另有说明。
 *    - dstr_split() 等返回数组的函数，释放方式见对应函数注释。
 *
 * 7. 全局状态码：
 *    - 可能失败的操作返回 dstr_status_t。
 *    - 无失败可能的简单操作返回 void 或直接返回查询结果。
 *
 * 8. C++ 兼容：
 *    - 本头文件通过 DYNAMIC_STRING_EXTERN_C_BEGIN / END 支持 C++ 调用。
 */

/*----------------------------------------------------------------------------------------------------------------------
 * 头文件包含
 *--------------------------------------------------------------------------------------------------------------------*/
#include <stdarg.h>
#include <stddef.h>

/* C23 标准已将 bool/true/false 收为内置关键字，因此按标准仅需在 C23 之前包含 stdbool.h。 */
#if !defined(__STDC_VERSION__) || (defined(__STDC_VERSION__) && __STDC_VERSION__ < 202311L)
#include <stdbool.h>
#endif

/*----------------------------------------------------------------------------------------------------------------------
 * 宏定义
 *--------------------------------------------------------------------------------------------------------------------*/

/* C++ 兼容包裹宏。 */
#ifdef __cplusplus
#define DYNAMIC_STRING_EXTERN_C_BEGIN                                                                                  \
    extern "C"                                                                                                         \
    {
#define DYNAMIC_STRING_EXTERN_C_END }
#else
#define DYNAMIC_STRING_EXTERN_C_BEGIN
#define DYNAMIC_STRING_EXTERN_C_END
#endif

/* C++ 兼容包裹宏-开始。 */
DYNAMIC_STRING_EXTERN_C_BEGIN

/*----------------------------------------------------------------------------------------------------------------------
 * ADT 类型别名声明
 *--------------------------------------------------------------------------------------------------------------------*/

/* 「动态字符串」ADT 类型别名。 */
typedef struct dynamic_string dstr_adt;

/*----------------------------------------------------------------------------------------------------------------------
 * 全局状态码
 *--------------------------------------------------------------------------------------------------------------------*/

/* 全局状态码枚举类型。 */
typedef enum
{
    DSTR_SUCCESS = 0,         /* 成功。 */
    DSTR_MEMORY_ALLOC_FAILED, /* 内存分配失败。 */
    DSTR_INVALID_ARGUMENT,    /* 无效参数。 */
} dstr_status_t;

/*----------------------------------------------------------------------------------------------------------------------
 * 其他类型定义
 *--------------------------------------------------------------------------------------------------------------------*/

/* 方向枚举类型。主要用于查找与替换系列函数，表示查找/替换的方向。 */
typedef enum
{
    DSTR_DIR_FORWARD, /* 从前往后。 */
    DSTR_DIR_BACKWARD /* 从后往前。 */
} dstr_direction_t;

/*----------------------------------------------------------------------------------------------------------------------
 * 接口函数原型（声明）
 *--------------------------------------------------------------------------------------------------------------------*/

/* 创建与销毁。 */

/**
 * @brief 创建一个「动态字符串」。
 *
 * @attention 返回值指向堆内存，请手动调用 dstr_destroy() 释放。
 *
 * @param cstr[in] 源「C 字符串」的指针。为空指针或指向空「C 字符串」时创建空「动态字符串」。
 *
 * @return 所创建的「动态字符串」的指针。如果创建失败则返回空指针。
 */
dstr_adt *dstr_create(const char *cstr);

/**
 * @brief 销毁一个「动态字符串」。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针，则函数会直接返回。
 */
void dstr_destroy(dstr_adt *dstr);

/**
 * @brief 克隆一个「动态字符串」。
 *
 * @attention 返回值指向堆内存，请手动调用 dstr_destroy() 释放。
 *
 * @param dstr[in] 源「动态字符串」的指针。为空指针或指向空「动态字符串」时创建空「动态字符串」。
 *
 * @return 所创建的「动态字符串」的指针。如果创建失败则返回空指针。
 */
dstr_adt *dstr_clone(const dstr_adt *dstr);

/**
 * @brief 提取一个「C 字符串」的子串为一个新的「动态字符串」。
 *
 * @attention 返回值指向堆内存，请手动调用 dstr_destroy() 释放。
 *
 * @param cstr[in] 源「C 字符串」的指针。为空指针或指向空「C 字符串」时创建空「动态字符串」，
 *                 此时忽略 sub_start 和 sub_length。
 * @param sub_start[in] 子串的起始索引。如果越界，则函数会直接返回空指针。
 * @param sub_length[in] 子串的长度。为 0 表示到末尾。如果越界，则函数会直接返回空指针。
 *
 * @return 所创建的「动态字符串」的指针。如果创建失败则返回空指针。
 */
dstr_adt *dstr_sub_cstr(const char *cstr, size_t sub_start, size_t sub_length);

/**
 * @brief 提取一个「动态字符串」的子串为一个新的「动态字符串」。
 *
 * @attention 返回值指向堆内存，请手动调用 dstr_destroy() 释放。
 *
 * @param dstr[in] 源「动态字符串」的指针。为空指针或指向空「动态字符串」时创建空「动态字符串」，
 *                 此时忽略 sub_start 和 sub_length。
 * @param sub_start[in] 子串的起始索引。如果越界，则函数会直接返回空指针。
 * @param sub_length[in] 子串的长度。为 0 表示到末尾。如果越界，则函数会直接返回空指针。
 *
 * @return 所创建的「动态字符串」的指针。如果创建失败则返回空指针。
 */
dstr_adt *dstr_sub(const dstr_adt *dstr, size_t sub_start, size_t sub_length);

/**
 * @brief 格式化创建一个「动态字符串」。
 *
 * @attention 返回值指向堆内存，请手动调用 dstr_destroy() 释放。
 *
 * @param format[in] 格式「C 字符串」的指针。为空指针或指向空「C 字符串」时创建空「动态字符串」。
 * @param ...[in] 与 format 对应的可变参数列表。
 *
 * @return 所创建的「动态字符串」的指针。如果创建失败则返回空指针。
 */
dstr_adt *dstr_create_format(const char *format, ...);

/**
 * @brief 格式化创建一个「动态字符串」（va_list 版本）。
 *
 * @attention 返回值指向堆内存，请手动调用 dstr_destroy() 释放。
 *
 * @remark args 可能被本函数读取并消耗，调用后不应再次使用，除非重新 va_start() 或使用 va_copy()。
 *
 * @param format[in] 格式「C 字符串」的指针。为空指针或指向空「C 字符串」时创建空「动态字符串」。
 * @param args[in] 已通过 va_start() 初始化的 va_list 变量，包含与 format 对应的可变参数列表信息。
 *                 该函数不会调用 va_end()，调用者需自行管理 args 的生命周期。
 *
 * @return 所创建的「动态字符串」的指针。如果创建失败则返回空指针。
 */
dstr_adt *dstr_create_vformat(const char *format, va_list args);

/* 属性获取与设置。 */

/**
 * @brief 获取一个「动态字符串」的内部「C 字符串」指针。
 *
 * @attention 返回的指针指向内部缓冲区，调用者不得修改其内容。
 *            该指针在 dstr 被修改、扩容、缩容或销毁后失效。
 *            对非空 dstr，保证返回非空指针；当 dstr_length() == 0 时，返回的指针指向 ""。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针，则函数会直接返回空指针。
 *
 * @return 所获取的「C 字符串」指针。对非空 dstr 保证非空。
 */
const char *dstr_cstr(const dstr_adt *dstr);

/**
 * @brief 获取一个「动态字符串」的长度。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针，则函数会直接返回 0。
 *
 * @return 所获取的长度。
 */
size_t dstr_length(const dstr_adt *dstr);

/**
 * @brief 判断一个「动态字符串」是否是空「动态字符串」。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针，则函数会直接返回 true。
 *
 * @return 如果目标「动态字符串」是空「动态字符串」则返回 true，否则返回 false。
 */
bool dstr_is_empty(const dstr_adt *dstr);

/**
 * @brief 获取一个「动态字符串」的容量。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针，则函数会直接返回 0。
 *
 * @return 所获取的容量。
 */
size_t dstr_capacity(const dstr_adt *dstr);

/**
 * @brief 设置一个「动态字符串」的容量。
 *
 * @attention 参数 new_capacity 是期望的数据缓冲区字节数，包含结尾 '\0'。
 *            函数成功执行时，实际容量可能等于 new_capacity，也可能等于实现所采用的最小容量下限
 *            （当 new_capacity 小于该下限时）。因此实际容量不会小于 new_capacity。
 *            调用者应以 dstr_capacity() 返回值为准。
 *            如果 new_capacity <= dstr_length()，则字符串会被截断，
 *            新的长度为实际容量 - 1（即新容量能容纳的最大长度）。
 *            如果 new_capacity == 0，视为请求最小容量。
 *
 * @remark 成功调用后，该请求值会成为后续自动扩容/缩容的保底容量下限，
 *         直到调用 dstr_shrink_to_fit() 清除。再次调用 dstr_set_capacity() 会覆盖旧下限。
 *         具体保底容量行为由实现决定，API 仅保证容量不小于请求值。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针，则视为不合法参数。
 * @param new_capacity[in] 新的容量。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_set_capacity(dstr_adt *dstr, size_t new_capacity);

/**
 * @brief 调整一个「动态字符串」的容量到刚合适。
 *
 * @attention 执行此函数会同时取消目标「动态字符串」由 dstr_set_capacity() 所设置的保底容量下限。
 *            容量会缩到至少能容纳 len + 1 个字节，但可能受实现最小容量下限影响。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针，则函数会直接返回。
 */
void dstr_shrink_to_fit(dstr_adt *dstr);

/* 内容编辑。 */

/**
 * @brief 复制一个「C 字符串」到一个「动态字符串」。
 *
 * @warning src 不得指向 dest 的内部缓冲区。若需自复制，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为不合法参数。
 * @param src[in] 源「C 字符串」的指针。为空指针或指向空「C 字符串」时复制空字符串（清空目标「动态字符串」的内容）。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cpy_cstr(dstr_adt *dest, const char *src);

/**
 * @brief 复制一个「动态字符串」到另一个「动态字符串」。
 *
 * @warning src 不得指向 dest 的内部缓冲区。若需自复制，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为不合法参数。
 * @param src[in] 源「动态字符串」的指针。为空指针或指向空「动态字符串」时复制空字符串（清空目标「动态字符串」的内容）。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cpy(dstr_adt *dest, const dstr_adt *src);

/**
 * @brief 复制一个「C 字符串」的子串到一个「动态字符串」。
 *
 * @warning src 不得指向 dest 的内部缓冲区。若需自复制，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为不合法参数。
 * @param src[in] 源「C 字符串」的指针。为空指针或指向空「C 字符串」时复制空字符串（清空目标「动态字符串」的内容），
 *                 此时忽略 sub_start 和 sub_length。
 * @param sub_start[in] 子串的起始索引。如果越界，则视为不合法参数。
 * @param sub_length[in] 子串的长度。为 0 表示到末尾。如果越界，则视为不合法参数。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cpy_sub_cstr(dstr_adt *dest, const char *src, size_t sub_start, size_t sub_length);

/**
 * @brief 复制一个「动态字符串」的子串到另一个「动态字符串」。
 *
 * @warning src 不得指向 dest 的内部缓冲区。若需自复制，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为不合法参数。
 * @param src[in] 源「动态字符串」的指针。为空指针或指向空「动态字符串」时复制空字符串（清空目标「动态字符串」的内容），
 *                 此时忽略 sub_start 和 sub_length。
 * @param sub_start[in] 子串的起始索引。如果越界，则视为不合法参数。
 * @param sub_length[in] 子串的长度。为 0 表示到末尾。如果越界，则视为不合法参数。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cpy_sub(dstr_adt *dest, const dstr_adt *src, size_t sub_start, size_t sub_length);

/**
 * @brief 格式化复制一个字符串到一个「动态字符串」。
 *
 * @warning format 不得指向 dest 的内部缓冲区。若需自复制，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为不合法参数。
 * @param format[in] 格式「C 字符串」的指针。
 *                   为空指针或指向空「C 字符串」时复制空字符串（清空目标「动态字符串」的内容）。
 * @param ...[in] 与 format 对应的可变参数列表。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cpy_format(dstr_adt *dest, const char *format, ...);

/**
 * @brief 格式化复制一个字符串到一个「动态字符串」（va_list 版本）。
 *
 * @warning format 不得指向 dest 的内部缓冲区。若需自复制，请先创建临时副本。
 *
 * @remark args 可能被本函数读取并消耗，调用后不应再次使用，除非重新 va_start() 或使用 va_copy()。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为不合法参数。
 * @param format[in] 格式「C 字符串」的指针。
 *                   为空指针或指向空「C 字符串」时复制空字符串（清空目标「动态字符串」的内容）。
 * @param args[in] 已通过 va_start() 初始化的 va_list 变量，包含与 format 对应的可变参数列表信息。
 *                 该函数不会调用 va_end()，调用者需自行管理 args 的生命周期。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cpy_vformat(dstr_adt *dest, const char *format, va_list args);

/**
 * @brief 追加一个「C 字符串」到一个「动态字符串」。
 *
 * @warning src 不得指向 dest 的内部缓冲区。若需自追加，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为不合法参数。
 * @param src[in] 源「C 字符串」的指针。为空指针或指向空「C 字符串」时追加空字符串。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cat_cstr(dstr_adt *dest, const char *src);

/**
 * @brief 追加一个「动态字符串」到另一个「动态字符串」。
 *
 * @warning src 不得指向 dest 的内部缓冲区。若需自追加，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为不合法参数。
 * @param src[in] 源「动态字符串」的指针。为空指针或指向空「动态字符串」时追加空字符串。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cat(dstr_adt *dest, const dstr_adt *src);

/**
 * @brief 追加一个「C 字符串」的子串到一个「动态字符串」。
 *
 * @warning src 不得指向 dest 的内部缓冲区。若需自追加，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为不合法参数。
 * @param src[in] 源「C 字符串」的指针。为空指针或指向空「C 字符串」时追加空字符串，
 *                 此时忽略 sub_start 和 sub_length。
 * @param sub_start[in] 子串的起始索引。如果越界，则视为不合法参数。
 * @param sub_length[in] 子串的长度。为 0 表示到末尾。如果越界，则视为不合法参数。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cat_sub_cstr(dstr_adt *dest, const char *src, size_t sub_start, size_t sub_length);

/**
 * @brief 追加一个「动态字符串」的子串到另一个「动态字符串」。
 *
 * @warning src 不得指向 dest 的内部缓冲区。若需自追加，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为不合法参数。
 * @param src[in] 源「动态字符串」的指针。为空指针或指向空「动态字符串」时追加空字符串，
 *                 此时忽略 sub_start 和 sub_length。
 * @param sub_start[in] 子串的起始索引。如果越界，则视为不合法参数。
 * @param sub_length[in] 子串的长度。为 0 表示到末尾。如果越界，则视为不合法参数。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cat_sub(dstr_adt *dest, const dstr_adt *src, size_t sub_start, size_t sub_length);

/**
 * @brief 格式化追加一个字符串到一个「动态字符串」。
 *
 * @warning format 不得指向 dest 的内部缓冲区。若需自追加，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为不合法参数。
 * @param format[in] 格式「C 字符串」的指针。为空指针或指向空「C 字符串」时追加空字符串。
 * @param ...[in] 与 format 对应的可变参数列表。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cat_format(dstr_adt *dest, const char *format, ...);

/**
 * @brief 格式化追加一个字符串到一个「动态字符串」（va_list 版本）。
 *
 * @warning format 不得指向 dest 的内部缓冲区。若需自追加，请先创建临时副本。
 *
 * @remark args 可能被本函数读取并消耗，调用后不应再次使用，除非重新 va_start() 或使用 va_copy()。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为不合法参数。
 * @param format[in] 格式「C 字符串」的指针。为空指针或指向空「C 字符串」时追加空字符串。
 * @param args[in] 已通过 va_start() 初始化的 va_list 变量，包含与 format 对应的可变参数列表信息。
 *                 该函数不会调用 va_end()，调用者需自行管理 args 的生命周期。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cat_vformat(dstr_adt *dest, const char *format, va_list args);

/**
 * @brief 插入一个「C 字符串」到一个「动态字符串」。
 *
 * @warning src 不得指向 dest 的内部缓冲区。若需自插入，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为不合法参数。
 * @param index[in] 插入位置的索引。如果越界，则视为不合法参数。
 * @param src[in] 源「C 字符串」的指针。为空指针或指向空「C 字符串」时插入空字符串。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_insert_cstr(dstr_adt *dest, size_t index, const char *src);

/**
 * @brief 插入一个「动态字符串」到另一个「动态字符串」。
 *
 * @warning src 不得指向 dest 的内部缓冲区。若需自插入，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为不合法参数。
 * @param index[in] 插入位置的索引。如果越界，则视为不合法参数。
 * @param src[in] 源「动态字符串」的指针。为空指针或指向空「动态字符串」时插入空字符串。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_insert(dstr_adt *dest, size_t index, const dstr_adt *src);

/**
 * @brief 插入一个「C 字符串」的子串到一个「动态字符串」。
 *
 * @warning src 不得指向 dest 的内部缓冲区。若需自插入，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为不合法参数。
 * @param index[in] 插入位置的索引。如果越界，则视为不合法参数。
 * @param src[in] 源「C 字符串」的指针。为空指针或指向空「C 字符串」时插入空字符串，
 *                 此时忽略 sub_start 和 sub_length。
 * @param sub_start[in] 子串的起始索引。如果越界，则视为不合法参数。
 * @param sub_length[in] 子串的长度。为 0 表示到末尾。如果越界，则视为不合法参数。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_insert_sub_cstr(dstr_adt *dest, size_t index, const char *src, size_t sub_start, size_t sub_length);

/**
 * @brief 插入一个「动态字符串」的子串到另一个「动态字符串」。
 *
 * @warning src 不得指向 dest 的内部缓冲区。若需自插入，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为不合法参数。
 * @param index[in] 插入位置的索引。如果越界，则视为不合法参数。
 * @param src[in] 源「动态字符串」的指针。为空指针或指向空「动态字符串」时插入空字符串，
 *                 此时忽略 sub_start 和 sub_length。
 * @param sub_start[in] 子串的起始索引。如果越界，则视为不合法参数。
 * @param sub_length[in] 子串的长度。为 0 表示到末尾。如果越界，则视为不合法参数。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_insert_sub(dstr_adt *dest, size_t index, const dstr_adt *src, size_t sub_start, size_t sub_length);

/**
 * @brief 格式化插入一个字符串到一个「动态字符串」。
 *
 * @warning format 不得指向 dest 的内部缓冲区。若需自插入，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为不合法参数。
 * @param index[in] 插入位置的索引。如果越界，则视为不合法参数。
 * @param format[in] 格式「C 字符串」的指针。为空指针或指向空「C 字符串」时插入空字符串。
 * @param ...[in] 与 format 对应的可变参数列表。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_insert_format(dstr_adt *dest, size_t index, const char *format, ...);

/**
 * @brief 格式化插入一个字符串到一个「动态字符串」（va_list 版本）。
 *
 * @warning format 不得指向 dest 的内部缓冲区。若需自插入，请先创建临时副本。
 *
 * @remark args 可能被本函数读取并消耗，调用后不应再次使用，除非重新 va_start() 或使用 va_copy()。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为不合法参数。
 * @param index[in] 插入位置的索引。如果越界，则视为不合法参数。
 * @param format[in] 格式「C 字符串」的指针。为空指针或指向空「C 字符串」时插入空字符串。
 * @param args[in] 已通过 va_start() 初始化的 va_list 变量，包含与 format 对应的可变参数列表信息。
 *                 该函数不会调用 va_end()，调用者需自行管理 args 的生命周期。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_insert_vformat(dstr_adt *dest, size_t index, const char *format, va_list args);

/**
 * @brief 清空一个「动态字符串」。
 *
 * @remark 使其长度为 0，不会立即释放容量。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针，则函数会直接返回。
 */
void dstr_clear(dstr_adt *dstr);

/**
 * @brief 删除一个「动态字符串」的子串。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针或指向空「动态字符串」，则函数会直接返回。
 * @param sub_start[in] 子串的起始索引。如果越界，则函数会直接返回（视为参数非法，静默失败）。
 * @param sub_length[in] 子串的长度。为 0 表示到末尾。如果越界，则函数会直接返回。
 */
void dstr_remove(dstr_adt *dstr, size_t sub_start, size_t sub_length);

/**
 * @brief 删除一个「动态字符串」首尾的空白字符或指定字符。
 *
 * @remark 空白字符判定使用 C 标准库函数 isspace()。本库不修改 locale，因此判定结果取决于调用时的 locale。
 *         如需不受 locale 影响，请显式指定 trim_chars。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针或指向空「动态字符串」，则函数会直接返回。
 * @param trim_chars[in] 包含要删除的字符的「C 字符串」的指针。为空指针或指向空「C 字符串」时删除空白字符。
 */
void dstr_trim(dstr_adt *dstr, const char *trim_chars);

/* 关系判断与比较。 */

/**
 * @brief 判断一个「动态字符串」是否以指定「C 字符串」前缀开头。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针或指向空「动态字符串」，则函数会直接返回 false。
 * @param prefix[in] 前缀「C 字符串」的指针。如果为空指针或指向空「C 字符串」，则函数会直接返回 false。
 *
 * @return 如果目标「动态字符串」以指定「C 字符串」前缀开头则返回 true，否则返回 false。
 */
bool dstr_starts_with_cstr(const dstr_adt *dstr, const char *prefix);

/**
 * @brief 判断一个「动态字符串」是否以指定「动态字符串」前缀开头。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针或指向空「动态字符串」，则函数会直接返回 false。
 * @param prefix[in] 前缀「动态字符串」的指针。如果为空指针或指向空「动态字符串」，则函数会直接返回 false。
 *
 * @return 如果目标「动态字符串」以指定「动态字符串」前缀开头则返回 true，否则返回 false。
 */
bool dstr_starts_with(const dstr_adt *dstr, const dstr_adt *prefix);

/**
 * @brief 判断一个「C 字符串」是否以指定「C 字符串」前缀开头。
 *
 * @param cstr[in] 目标「C 字符串」的指针。如果为空指针或指向空「C 字符串」，则函数会直接返回 false。
 * @param prefix[in] 前缀「C 字符串」的指针。如果为空指针或指向空「C 字符串」，则函数会直接返回 false。
 *
 * @return 如果目标「C 字符串」以指定「C 字符串」前缀开头则返回 true，否则返回 false。
 */
bool cstr_starts_with(const char *cstr, const char *prefix);

/**
 * @brief 判断一个「动态字符串」是否以指定「C 字符串」后缀结尾。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针或指向空「动态字符串」，则函数会直接返回 false。
 * @param suffix[in] 后缀「C 字符串」的指针。如果为空指针或指向空「C 字符串」，则函数会直接返回 false。
 *
 * @return 如果目标「动态字符串」以指定「C 字符串」后缀结尾则返回 true，否则返回 false。
 */
bool dstr_ends_with_cstr(const dstr_adt *dstr, const char *suffix);

/**
 * @brief 判断一个「动态字符串」是否以指定「动态字符串」后缀结尾。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针或指向空「动态字符串」，则函数会直接返回 false。
 * @param suffix[in] 后缀「动态字符串」的指针。如果为空指针或指向空「动态字符串」，则函数会直接返回 false。
 *
 * @return 如果目标「动态字符串」以指定「动态字符串」后缀结尾则返回 true，否则返回 false。
 */
bool dstr_ends_with(const dstr_adt *dstr, const dstr_adt *suffix);

/**
 * @brief 判断一个「C 字符串」是否以指定「C 字符串」后缀结尾。
 *
 * @param cstr[in] 目标「C 字符串」的指针。如果为空指针或指向空「C 字符串」，则函数会直接返回 false。
 * @param suffix[in] 后缀「C 字符串」的指针。如果为空指针或指向空「C 字符串」，则函数会直接返回 false。
 *
 * @return 如果目标「C 字符串」以指定「C 字符串」后缀结尾则返回 true，否则返回 false。
 */
bool cstr_ends_with(const char *cstr, const char *suffix);

/**
 * @brief 判断一个「动态字符串」是否包含指定子「C 字符串」。
 *
 * @note 本函数与查找相关函数不同：空子串按 C 标准视为任意字符串的子串，因此返回 true；
 *       而查找相关函数将空子串视为非法查找目标。
 *
 * @param dstr[in] 目标「动态字符串」的指针。
 * @param sub[in] 子「C 字符串」的指针。
 *
 * @return 包含则返回 true，否则返回 false。
 *         如果 sub 为「空字符串」，则一定返回 true，遵循 C 标准规定：空字符串是任何字符串的子串。
 */
bool dstr_contains_cstr(const dstr_adt *dstr, const char *sub);

/**
 * @brief 判断一个「动态字符串」是否包含指定子「动态字符串」。
 *
 * @note 本函数与查找相关函数不同：空子串按 C 标准视为任意字符串的子串，因此返回 true；
 *       而查找相关函数将空子串视为非法查找目标。
 *
 * @param dstr[in] 目标「动态字符串」的指针。
 * @param sub[in] 子「动态字符串」的指针。
 *
 * @return 包含则返回 true，否则返回 false。
 *         如果 sub 为「空字符串」，则一定返回 true，遵循 C 标准规定：空字符串是任何字符串的子串。
 */
bool dstr_contains(const dstr_adt *dstr, const dstr_adt *sub);

/**
 * @brief 判断一个「C 字符串」是否包含指定子「C 字符串」。
 *
 * @note 本函数与查找相关函数不同：空子串按 C 标准视为任意字符串的子串，因此返回 true；
 *       而查找相关函数将空子串视为非法查找目标。
 *
 * @param cstr[in] 目标「C 字符串」的指针。
 * @param sub[in] 子「C 字符串」的指针。
 *
 * @return 包含则返回 true，否则返回 false。
 *         如果 sub 为「空字符串」，则一定返回 true，遵循 C 标准规定：空字符串是任何字符串的子串。
 */
bool cstr_contains(const char *cstr, const char *sub);

/**
 * @brief 判断一个「动态字符串」是否与一个「C 字符串」相等。
 *
 * @param lhs[in] 「动态字符串」的指针。如果为空指针，则视其为「空字符串」。
 * @param rhs[in] 「C 字符串」的指针。如果为空指针，则视其为「空字符串」。
 *
 * @return 相等则返回 true，否则返回 false。两者同为「空字符串」时，返回 true。
 */
bool dstr_equals_cstr(const dstr_adt *lhs, const char *rhs);

/**
 * @brief 判断两个「动态字符串」是否相等。
 *
 * @param lhs[in] 第一个「动态字符串」的指针。如果为空指针，则视其为「空字符串」。
 * @param rhs[in] 第二个「动态字符串」的指针。如果为空指针，则视其为「空字符串」。
 *
 * @return 相等则返回 true，否则返回 false。两者同为「空字符串」时，返回 true。
 */
bool dstr_equals(const dstr_adt *lhs, const dstr_adt *rhs);

/**
 * @brief 判断两个「C 字符串」是否相等。
 *
 * @param lhs[in] 第一个「C 字符串」的指针。如果为空指针，则视其为「空字符串」。
 * @param rhs[in] 第二个「C 字符串」的指针。如果为空指针，则视其为「空字符串」。
 *
 * @return 相等则返回 true，否则返回 false。两者同为「空字符串」时，返回 true。
 */
bool cstr_equals(const char *lhs, const char *rhs);

/**
 * @brief 比较一个「动态字符串」与一个「C 字符串」。
 *
 * @param lhs[in] 「动态字符串」的指针。如果为空指针，则视其为「空字符串」。
 * @param rhs[in] 「C 字符串」的指针。如果为空指针，则视其为「空字符串」。
 *
 * @return 两者相等返回 0，前者大于后者返回正值，前者小于后者返回负值。两者同为「空字符串」时，返回 0。
 *         两者只有一者为「空字符串」时，为「空字符串」者小于非「空字符串」者。
 */
int dstr_compare_cstr(const dstr_adt *lhs, const char *rhs);

/**
 * @brief 比较两个「动态字符串」。
 *
 * @param lhs[in] 第一个「动态字符串」的指针。如果为空指针，则视其为「空字符串」。
 * @param rhs[in] 第二个「动态字符串」的指针。如果为空指针，则视其为「空字符串」。
 *
 * @return 两者相等返回 0，前者大于后者返回正值，前者小于后者返回负值。两者同为「空字符串」时，返回 0。
 *         两者只有一者为「空字符串」时，为「空字符串」者小于非「空字符串」者。
 */
int dstr_compare(const dstr_adt *lhs, const dstr_adt *rhs);

/**
 * @brief 比较两个「C 字符串」。
 *
 * @param lhs[in] 第一个「C 字符串」的指针。如果为空指针，则视其为「空字符串」。
 * @param rhs[in] 第二个「C 字符串」的指针。如果为空指针，则视其为「空字符串」。
 *
 * @return 两者相等返回 0，前者大于后者返回正值，前者小于后者返回负值。两者同为「空字符串」时，返回 0。
 *         两者只有一者为「空字符串」时，为「空字符串」者小于非「空字符串」者。
 */
int cstr_compare(const char *lhs, const char *rhs);

/* 查找、统计与替换。
 *
 * 本库所有查找、统计、替换操作均按“非重叠匹配”处理。
 * 即一旦在位置 i 找到一个匹配，下一次查找从 i + 子串长度 处继续；
 * 反向查找同理，下一次从匹配位置之前继续。
 * 因此，"aaa" 中查找 "aa" 只算出现 1 次，而不是 2 次。
 * 替换时，新插入的内容不会参与同一轮再次匹配。
 */

/**
 * @brief 查找一个「动态字符串」中指定子「C 字符串」首次出现的位置。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针或指向空「动态字符串」，则函数会直接返回 false。
 * @param sub[in] 子「C 字符串」的指针。如果为空指针或指向空「C 字符串」，则函数会直接返回 false。
 * @param out_index[out] 存储查找结果（位置索引）的 size_t 变量的指针。为空指针时不写入。
 * @param direction[in] 查找方向。
 *
 * @return 找到则返回 true，否则返回 false。
 */
bool dstr_find_cstr(const dstr_adt *dstr, const char *sub, size_t *out_index, dstr_direction_t direction);

/**
 * @brief 查找一个「动态字符串」中指定子「动态字符串」首次出现的位置。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针或指向空「动态字符串」，则函数会直接返回 false。
 * @param sub[in] 子「动态字符串」的指针。如果为空指针或指向空「动态字符串」，则函数会直接返回 false。
 * @param out_index[out] 存储查找结果（位置索引）的 size_t 变量的指针。为空指针时不写入。
 * @param direction[in] 查找方向。
 *
 * @return 找到则返回 true，否则返回 false。
 */
bool dstr_find(const dstr_adt *dstr, const dstr_adt *sub, size_t *out_index, dstr_direction_t direction);

/**
 * @brief 查找一个「C 字符串」中指定子「C 字符串」首次出现的位置。
 *
 * @param cstr[in] 目标「C 字符串」的指针。如果为空指针或指向空「C 字符串」，则函数会直接返回 false。
 * @param sub[in] 子「C 字符串」的指针。如果为空指针或指向空「C 字符串」，则函数会直接返回 false。
 * @param out_index[out] 存储查找结果（位置索引）的 size_t 变量的指针。为空指针时不写入。
 * @param direction[in] 查找方向。
 *
 * @return 找到则返回 true，否则返回 false。
 */
bool cstr_find(const char *cstr, const char *sub, size_t *out_index, dstr_direction_t direction);

/**
 * @brief 查找一个「动态字符串」中指定子「C 字符串」第 n 次出现的位置。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针或指向空「动态字符串」，则函数会直接返回 false。
 * @param sub[in] 子「C 字符串」的指针。如果为空指针或指向空「C 字符串」，则函数会直接返回 false。
 * @param out_index[out] 存储查找结果（位置索引）的 size_t 变量的指针。为空指针时不写入。
 * @param direction[in] 查找方向。
 * @param n[in] 出现的次序。从 1 开始。为 0 表示该方向的最后一次出现。
 *              具体：direction 为 DSTR_DIR_FORWARD 时，n>0 表示从前往后第 n 次，
 *              n=0 表示从前往后最后一次（等价于 DSTR_DIR_BACKWARD, n=1）；
 *              direction 为 DSTR_DIR_BACKWARD 时，n>0 表示从后往前第 n 次，
 *              n=0 表示从后往前最后一次（等价于 DSTR_DIR_FORWARD, n=1）。
 *              如果大于实际出现次数，则视为最后一次。
 *
 * @return 找到则返回 true，否则返回 false。
 */
bool dstr_find_nth_cstr(const dstr_adt *dstr, const char *sub, size_t *out_index, dstr_direction_t direction, size_t n);

/**
 * @brief 查找一个「动态字符串」中指定子「动态字符串」第 n 次出现的位置。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针或指向空「动态字符串」，则函数会直接返回 false。
 * @param sub[in] 子「动态字符串」的指针。如果为空指针或指向空「动态字符串」，则函数会直接返回 false。
 * @param out_index[out] 存储查找结果（位置索引）的 size_t 变量的指针。为空指针时不写入。
 * @param direction[in] 查找方向。
 * @param n[in] 出现的次序。从 1 开始。为 0 表示该方向的最后一次出现。
 *              具体：direction 为 DSTR_DIR_FORWARD 时，n>0 表示从前往后第 n 次，
 *              n=0 表示从前往后最后一次（等价于 DSTR_DIR_BACKWARD, n=1）；
 *              direction 为 DSTR_DIR_BACKWARD 时，n>0 表示从后往前第 n 次，
 *              n=0 表示从后往前最后一次（等价于 DSTR_DIR_FORWARD, n=1）。
 *              如果大于实际出现次数，则视为最后一次。
 *
 * @return 找到则返回 true，否则返回 false。
 */
bool dstr_find_nth(const dstr_adt *dstr, const dstr_adt *sub, size_t *out_index, dstr_direction_t direction, size_t n);

/**
 * @brief 查找一个「C 字符串」中指定子「C 字符串」第 n 次出现的位置。
 *
 * @param cstr[in] 目标「C 字符串」的指针。如果为空指针或指向空「C 字符串」，则函数会直接返回 false。
 * @param sub[in] 子「C 字符串」的指针。如果为空指针或指向空「C 字符串」，则函数会直接返回 false。
 * @param out_index[out] 存储查找结果（位置索引）的 size_t 变量的指针。为空指针时不写入。
 * @param direction[in] 查找方向。
 * @param n[in] 出现的次序。从 1 开始。为 0 表示该方向的最后一次出现。
 *              具体：direction 为 DSTR_DIR_FORWARD 时，n>0 表示从前往后第 n 次，
 *              n=0 表示从前往后最后一次（等价于 DSTR_DIR_BACKWARD, n=1）；
 *              direction 为 DSTR_DIR_BACKWARD 时，n>0 表示从后往前第 n 次，
 *              n=0 表示从后往前最后一次（等价于 DSTR_DIR_FORWARD, n=1）。
 *              如果大于实际出现次数，则视为最后一次。
 *
 * @return 找到则返回 true，否则返回 false。
 */
bool cstr_find_nth(const char *cstr, const char *sub, size_t *out_index, dstr_direction_t direction, size_t n);

/**
 * @brief 查找一个「动态字符串」中指定子「C 字符串」前 n 次出现的位置。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针或指向空「动态字符串」，则函数会直接返回 0。
 * @param sub[in] 子「C 字符串」的指针。如果为空指针或指向空「C 字符串」，则函数会直接返回 0。
 * @param out_indexes[out] 存储查找结果（位置索引）的 size_t 数组的指针。为空指针时不写入。请自行确保数组容量够大。
 * @param direction[in] 查找方向。
 * @param n[in] 查找的次数。从 1 开始。为 0 表示查找全部。如果大于实际出现次数，也会查找全部。
 *
 * @return 截止第 n 次，实际出现的次数，也表示实际向 out_indexes 数组中写入的元素个数。
 */
size_t dstr_find_indexes_cstr(const dstr_adt *dstr, const char *sub, size_t *out_indexes, dstr_direction_t direction,
                              size_t n);

/**
 * @brief 查找一个「动态字符串」中指定子「动态字符串」前 n 次出现的位置。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针或指向空「动态字符串」，则函数会直接返回 0。
 * @param sub[in] 子「动态字符串」的指针。如果为空指针或指向空「动态字符串」，则函数会直接返回 0。
 * @param out_indexes[out] 存储查找结果（位置索引）的 size_t 数组的指针。为空指针时不写入。请自行确保数组容量够大。
 * @param direction[in] 查找方向。
 * @param n[in] 查找的次数。从 1 开始。为 0 表示查找全部。如果大于实际出现次数，也会查找全部。
 *
 * @return 截止第 n 次，实际出现的次数，也表示实际向 out_indexes 数组中写入的元素个数。
 */
size_t dstr_find_indexes(const dstr_adt *dstr, const dstr_adt *sub, size_t *out_indexes, dstr_direction_t direction,
                         size_t n);

/**
 * @brief 查找一个「C 字符串」中指定子「C 字符串」前 n 次出现的位置。
 *
 * @param cstr[in] 目标「C 字符串」的指针。如果为空指针或指向空「C 字符串」，则函数会直接返回 0。
 * @param sub[in] 子「C 字符串」的指针。如果为空指针或指向空「C 字符串」，则函数会直接返回 0。
 * @param out_indexes[out] 存储查找结果（位置索引）的 size_t 数组的指针。为空指针时不写入。请自行确保数组容量够大。
 * @param direction[in] 查找方向。
 * @param n[in] 查找的次数。从 1 开始。为 0 表示查找全部。如果大于实际出现次数，也会查找全部。
 *
 * @return 截止第 n 次，实际出现的次数，也表示实际向 out_indexes 数组中写入的元素个数。
 */
size_t cstr_find_indexes(const char *cstr, const char *sub, size_t *out_indexes, dstr_direction_t direction, size_t n);

/**
 * @brief 统计一个「动态字符串」中指定子「C 字符串」出现的次数。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针或指向空「动态字符串」，则函数会直接返回 0。
 * @param sub[in] 子「C 字符串」的指针。如果为空指针或指向空「C 字符串」，则函数会直接返回 0。
 *
 * @return 出现的次数。
 */
size_t dstr_count_cstr(const dstr_adt *dstr, const char *sub);

/**
 * @brief 统计一个「动态字符串」中指定子「动态字符串」出现的次数。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针或指向空「动态字符串」，则函数会直接返回 0。
 * @param sub[in] 子「动态字符串」的指针。如果为空指针或指向空「动态字符串」，则函数会直接返回 0。
 *
 * @return 出现的次数。
 */
size_t dstr_count(const dstr_adt *dstr, const dstr_adt *sub);

/**
 * @brief 统计一个「C 字符串」中指定子「C 字符串」出现的次数。
 *
 * @param cstr[in] 目标「C 字符串」的指针。如果为空指针或指向空「C 字符串」，则函数会直接返回 0。
 * @param sub[in] 子「C 字符串」的指针。如果为空指针或指向空「C 字符串」，则函数会直接返回 0。
 *
 * @return 出现的次数。
 */
size_t cstr_count(const char *cstr, const char *sub);

/**
 * @brief 替换一个「动态字符串」中指定旧「C 字符串」为指定新「C 字符串」n 次。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针或指向空「动态字符串」，则视为不合法参数。
 * @param old_str[in] 旧「C 字符串」的指针。如果为空指针或指向空「C 字符串」，则视为不合法参数。
 *                    如果在 dstr 中一次都没有出现或出现次数不足 n 次（n 不为 0 时），则视为不合法参数。
 * @param new_str[in] 新「C 字符串」的指针。为空指针或指向空「C 字符串」时，替换为空。
 * @param direction[in] 替换方向。
 * @param n[in] 替换的次数。为 0 表示替换所有。
 *              如果大于旧「C 字符串」实际出现的次数，则视为不合法参数，将一次替换都不进行。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_replace_cstr(dstr_adt *dstr, const char *old_str, const char *new_str, dstr_direction_t direction,
                                size_t n);

/**
 * @brief 替换一个「动态字符串」中指定旧「动态字符串」为指定新「动态字符串」n 次。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针或指向空「动态字符串」，则视为不合法参数。
 * @param old_str[in] 旧「动态字符串」的指针。如果为空指针或指向空「动态字符串」，则视为不合法参数。
 *                    如果在 dstr 中一次都没有出现或出现次数不足 n 次（n 不为 0 时），则视为不合法参数。
 * @param new_str[in] 新「动态字符串」的指针。为空指针或指向空「动态字符串」时，替换为空。
 * @param direction[in] 替换方向。
 * @param n[in] 替换的次数。为 0 表示替换所有。
 *              如果大于旧「动态字符串」实际出现的次数，则视为不合法参数，将一次替换都不进行。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_replace(dstr_adt *dstr, const dstr_adt *old_str, const dstr_adt *new_str, dstr_direction_t direction,
                           size_t n);

/**
 * @brief 替换一个「动态字符串」中指定旧「C 字符串」第 n 次为指定新「C 字符串」。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针或指向空「动态字符串」，则视为不合法参数。
 * @param old_str[in] 旧「C 字符串」的指针。如果为空指针或指向空「C 字符串」，则视为不合法参数。
 *                    如果在 dstr 中一次都没有出现或出现次数不足 n 次（n 不为 0 时），则视为不合法参数。
 * @param new_str[in] 新「C 字符串」的指针。为空指针或指向空「C 字符串」时，替换为空。
 * @param direction[in] 替换方向。
 * @param n[in] 替换的次序。从 1 开始。为 0 表示该方向的最后一次出现。
 *              具体：direction 为 DSTR_DIR_FORWARD 时，n>0 表示从前往后第 n 次，
 *              n=0 表示从前往后最后一次（等价于 DSTR_DIR_BACKWARD, n=1）；
 *              direction 为 DSTR_DIR_BACKWARD 时，n>0 表示从后往前第 n 次，
 *              n=0 表示从后往前最后一次（等价于 DSTR_DIR_FORWARD, n=1）。
 *              如果大于旧「C 字符串」实际出现的次数，则视为不合法参数，将一次替换都不进行。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_replace_nth_cstr(dstr_adt *dstr, const char *old_str, const char *new_str,
                                    dstr_direction_t direction, size_t n);

/**
 * @brief 替换一个「动态字符串」中指定旧「动态字符串」第 n 次为指定新「动态字符串」。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针或指向空「动态字符串」，则视为不合法参数。
 * @param old_str[in] 旧「动态字符串」的指针。如果为空指针或指向空「动态字符串」，则视为不合法参数。
 *                    如果在 dstr 中一次都没有出现或出现次数不足 n 次（n 不为 0 时），则视为不合法参数。
 * @param new_str[in] 新「动态字符串」的指针。为空指针或指向空「动态字符串」时，替换为空。
 * @param direction[in] 替换方向。
 * @param n[in] 替换的次序。从 1 开始。为 0 表示该方向的最后一次出现。
 *              具体：direction 为 DSTR_DIR_FORWARD 时，n>0 表示从前往后第 n 次，
 *              n=0 表示从前往后最后一次（等价于 DSTR_DIR_BACKWARD, n=1）；
 *              direction 为 DSTR_DIR_BACKWARD 时，n>0 表示从后往前第 n 次，
 *              n=0 表示从后往前最后一次（等价于 DSTR_DIR_FORWARD, n=1）。
 *              如果大于旧「动态字符串」实际出现的次数，则视为不合法参数，将一次替换都不进行。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_replace_nth(dstr_adt *dstr, const dstr_adt *old_str, const dstr_adt *new_str,
                               dstr_direction_t direction, size_t n);

/* 分隔与合并。 */

/**
 * @brief 分隔一个「C 字符串」为多个「动态字符串」。
 *
 * @remark 分隔时，如果两个分隔串之间，或分隔串与首尾边界之间的子串长度为 0，
 *         将以空指针（而不是空「动态字符串」）形式存储在数组中，而不是跳过。
 *
 * @attention 返回值指向堆内存，其中又可能有指向其他堆内存的指针，
 *            因此，释放时，请先手动依次调用 dstr_destroy() 释放每个元素，然后手动调用 free() 释放数组本身。
 *
 * @param cstr[in] 目标「C 字符串」的指针。如果为空指针或指向空「C 字符串」，则函数会直接返回空指针。
 * @param separator[in] 分隔「C 字符串」的指针。如果为空指针或指向空「C 字符串」，则函数会直接返回空指针。
 * @param out_dstr_count[out] 存储分隔后的「动态字符串」的个数的 size_t 变量的指针。
 *                            如果为空指针，则函数会直接返回空指针。仅在函数成功执行时写入。
 *
 * @return 分隔后的「动态字符串」数组的指针。如果分隔失败则返回空指针。
 */
dstr_adt **dstr_split_cstr(const char *cstr, const char *separator, size_t *out_dstr_count);

/**
 * @brief 分隔一个「动态字符串」为多个「动态字符串」。
 *
 * @remark 分隔时，如果两个分隔串之间，或分隔串与首尾边界之间的子串长度为 0，
 *         将以空指针（而不是空「动态字符串」）形式存储在数组中，而不是跳过。
 *
 * @attention 返回值指向堆内存，其中又可能有指向其他堆内存的指针，
 *            因此，释放时，请先手动依次调用 dstr_destroy() 释放每个元素，然后手动调用 free() 释放数组本身。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针或指向空「动态字符串」，则函数会直接返回空指针。
 * @param separator[in] 分隔「动态字符串」的指针。如果为空指针或指向空「动态字符串」，则函数会直接返回空指针。
 * @param out_dstr_count[out] 存储分隔后的「动态字符串」的个数的 size_t 变量的指针。
 *                            如果为空指针，则函数会直接返回空指针。仅在函数成功执行时写入。
 *
 * @return 分隔后的「动态字符串」数组的指针。如果分隔失败则返回空指针。
 */
dstr_adt **dstr_split(const dstr_adt *dstr, const dstr_adt *separator, size_t *out_dstr_count);

/**
 * @brief 分隔一个「C 字符串」为多个「C 字符串」。
 *
 * @remark 分隔时，如果两个分隔串之间，或分隔串与首尾边界之间的子串长度为 0，
 *         将以空指针（而不是空「C 字符串」）形式存储在数组中，而不是跳过。
 *
 * @attention 返回值指向堆内存，其中又可能有指向其他堆内存的指针，
 *            因此，释放时，请先手动依次调用 free() 释放每个元素，然后手动调用 free() 释放数组本身。
 *
 * @param cstr[in] 目标「C 字符串」的指针。如果为空指针或指向空「C 字符串」，则函数会直接返回空指针。
 * @param separator[in] 分隔「C 字符串」的指针。如果为空指针或指向空「C 字符串」，则函数会直接返回空指针。
 * @param out_cstr_count[out] 存储分隔后的「C 字符串」的个数的 size_t 变量的指针。
 *                            如果为空指针，则函数会直接返回空指针。仅在函数成功执行时写入。
 *
 * @return 分隔后的「C 字符串」数组的指针。如果分隔失败则返回空指针。
 */
char **cstr_split(const char *cstr, const char *separator, size_t *out_cstr_count);

/**
 * @brief 合并多个「C 字符串」为一个「动态字符串」。
 *
 * @remark 合并时，其中的空指针或空「C 字符串」会以空字符串形式被合并，而不是被跳过。
 *
 * @attention 返回值指向堆内存，请手动调用 dstr_destroy() 释放。
 *
 * @param cstrs[in] 源「C 字符串」数组的指针。如果为空指针，则函数会直接返回空指针。
 * @param cstr_count[in] 源「C 字符串」的个数。如果为 0，则函数会直接返回空指针。
 * @param separator[in] 分隔「C 字符串」的指针。为空指针或指向空「C 字符串」时使用空字符串合并。
 *
 * @return 合并后的「动态字符串」的指针。如果合并失败则返回空指针。
 */
dstr_adt *dstr_join_cstr(const char *const *cstrs, size_t cstr_count, const char *separator);

/**
 * @brief 合并多个「动态字符串」为一个「动态字符串」。
 *
 * @remark 合并时，其中的空指针或空「动态字符串」会以空字符串形式被合并，而不是被跳过。
 *
 * @attention 返回值指向堆内存，请手动调用 dstr_destroy() 释放。
 *
 * @param dstrs[in] 源「动态字符串」数组的指针。如果为空指针，则函数会直接返回空指针。
 * @param dstr_count[in] 源「动态字符串」的个数。如果为 0，则函数会直接返回空指针。
 * @param separator[in] 分隔「动态字符串」的指针。为空指针或指向空「动态字符串」时使用空字符串合并。
 *
 * @return 合并后的「动态字符串」的指针。如果合并失败则返回空指针。
 */
dstr_adt *dstr_join(const dstr_adt *const *dstrs, size_t dstr_count, const dstr_adt *separator);

/**
 * @brief 合并多个「C 字符串」为一个「C 字符串」。
 *
 * @remark 合并时，其中的空指针或空「C 字符串」会以空字符串形式被合并，而不是被跳过。
 *
 * @attention 返回值指向堆内存，请手动调用 free() 释放。
 *
 * @param cstrs[in] 源「C 字符串」数组的指针。如果为空指针，则函数会直接返回空指针。
 * @param cstr_count[in] 源「C 字符串」的个数。如果为 0，则函数会直接返回空指针。
 * @param separator[in] 分隔「C 字符串」的指针。为空指针或指向空「C 字符串」时使用空字符串合并。
 *
 * @return 合并后的「C 字符串」的指针。如果合并失败则返回空指针。
 */
char *cstr_join(const char *const *cstrs, size_t cstr_count, const char *separator);

/* C++ 兼容包裹宏-结束。 */
DYNAMIC_STRING_EXTERN_C_END

#endif /* DYNAMIC_STRING_H */
