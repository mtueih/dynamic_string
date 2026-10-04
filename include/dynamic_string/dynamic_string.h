/*
 * SPDX-FileCopyrightText: 2026 mtueih
 * SPDX-License-Identifier: ISC
 */

/*======================================================================================================================
 * include/dynamic_string.h - 项目主库头文件
 *====================================================================================================================*/
#ifndef DYNAMIC_STRING_H
#define DYNAMIC_STRING_H

/*----------------------------------------------------------------------------------------------------------------------
 * 通用语义与约束
 *----------------------------------------------------------------------------------------------------------------------
 * 以下约定适用于全库，除非具体函数另有说明。
 *
 * 1. 本库是文本字符串库，而非二进制安全字符串库。
 *    底层始终是合法 C 字符串，即数据始终以 '\0' 结尾，哪怕长度为 0。这意味着 API 函数 dstr_cstr() 对于有效输入，
 *    其返回值一定不会是空指针，其也一定指向一个以 '\0' 结尾的字符串缓冲区。
 *
 * 2. 编码无关性与容量。
 *    本库并不关心字符串数据的编码，因此其长度以字节而不是字符个数为单位，且其长度不包括 '\0'。
 *    容量表示底层数据缓冲区的真实大小，同样以字节为单位，由于要存储结尾的 '\0'，因此容量始终至少比长度大 1（长度为 0
 *    时， 也至少是 1，这意味着容量值的下限保证是 1 而不是 0，且由于实现可能会做 SSO 等优化，因此实际的容量下限可能大于
 *    1）， 但不保证始终等于长度 + 1（实现可能会做几何扩容等优化）。
 *
 * 3. 空指针与空字符串。
 *    对于任何表示字符串的参数，通常都可以使用空指针来表示空字符串，这避免了为了表示空字符串而构造不必要的对象的麻烦。
 *
 * 4. 非重叠匹配。
 *    所有查找、统计与替换类的操作均按非重叠匹配处理。
 *
 * 5. 禁止内存重叠。
 *    对于任何可能涉及从一个缓冲区拷贝数据到另一个缓冲区的操作，不保证实现会做源缓冲区与目标缓冲区重叠的检查。
 *    因此涉及此类情况时，请勿让源缓冲区指针指向目标缓冲区，否则为未定义行为。
 *--------------------------------------------------------------------------------------------------------------------*/

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
 * @param cstr[in] 源「C 字符串」的指针。为「空字符串」时创建空「动态字符串」。
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
 * @param dstr[in] 源「动态字符串」的指针。为「空字符串」时创建空「动态字符串」。
 *
 * @return 所创建的「动态字符串」的指针。如果创建失败则返回空指针。
 */
dstr_adt *dstr_clone(const dstr_adt *dstr);

/**
 * @brief 提取一个「C 字符串」的子串为一个新的「动态字符串」。
 *
 * @attention 返回值指向堆内存，请手动调用 dstr_destroy() 释放。
 *
 * @param cstr[in] 源「C 字符串」的指针。为「空字符串」时创建空「动态字符串」，此时忽略 sub_start 和 sub_length。
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
 * @param dstr[in] 源「动态字符串」的指针。为「空字符串」时创建空「动态字符串」，此时忽略 sub_start 和 sub_length。
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
 * @param format[in] 格式「C 字符串」的指针。为「空字符串」时创建空「动态字符串」。
 * @param ...[in] 与 format 对应的可变参数列表。
 *
 * @return 所创建的「动态字符串」的指针。如果创建失败则返回空指针。
 */
dstr_adt *dstr_create_format(const char *format, ...);

/**
 * @brief 格式化创建一个「动态字符串」（va_list 版本）。
 *
 * @note args 可能被此函数读取并消耗，调用后不应再使用，除非重新 va_start() 或使用 va_copy() 创建副本以使用此函数。
 *
 * @attention 返回值指向堆内存，请手动调用 dstr_destroy() 释放。
 *
 * @param format[in] 格式「C 字符串」的指针。为「空字符串」时创建空「动态字符串」。
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
 * @note 此函数对于有效输入，保证其返回值一定不会是空指针，其也一定指向一个以 '\0' 结尾的字符串缓冲区。
 *
 * @warning 返回的指针指向内部缓冲区，请勿通过该指针修改其内容。
 *          该指针是临时的，可能因 dstr 的内容或容量的变化而失效，切勿依赖。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针，则函数会直接返回空指针。
 *
 * @return dstr 的内部「C 字符串」指针。对非空 dstr 保证非空。
 */
const char *dstr_cstr(const dstr_adt *dstr);

/**
 * @brief 获取一个「动态字符串」的长度。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针，则函数会直接返回 0。
 *
 * @return dstr 的长度。
 */
size_t dstr_length(const dstr_adt *dstr);

/**
 * @brief 判断一个「动态字符串」是否是空「动态字符串」。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针，则函数会直接返回 true。
 *
 * @return 如果 dstr 是空「动态字符串」则返回 true，否则返回 false。
 */
bool dstr_is_empty(const dstr_adt *dstr);

/**
 * @brief 获取一个「动态字符串」的容量。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针，则函数会直接返回 0。
 *
 * @return dstr 的容量。
 */
size_t dstr_capacity(const dstr_adt *dstr);

/**
 * @brief 设置一个「动态字符串」的容量。
 *
 * @note 由于 API 保证内部缓冲区始终是以 '\0' 结尾的合法 C 字符串，因此 API 层面约束了一个动态字符串的容量存在下限，
 *       这个下限可以不是 1，但必须大于 0。
 *       这影响了此函数的行为，当此函数成功执行后，实际容量可能等于 new_capacity，
 *       也可能等于实现所采用的容量下限（当 new_capacity 小于等于该下限时）。
 *       另外，如果 new_capacity 大于实现所采用的容量下限，那么当此函数成功执行后，new_capacity 将成为新的容量下限，
 *       在后续由于 dstr 内容长度的减小所引起的容量的自动缩小时，容量将维持不低于 new_capacity。
 *       再次调用此函数可覆盖此下限，调用 dstr_shrink_to_fit() 函数可清除此下限。
 *
 * @warning 当 new_capacity 小于等于 dstr 当前长度时，dstr 的内容会被截断，具体行为受实现的容量下限影响。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针，则视为无效参数。
 * @param new_capacity[in] 新的容量。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_set_capacity(dstr_adt *dstr, size_t new_capacity);

/**
 * @brief 调整一个「动态字符串」的容量到刚合适。
 *
 * @note 执行此函数会同时取消目标「动态字符串」由 dstr_set_capacity() 所设置的保底容量下限。
 *       容量会缩到至少能容纳 len + 1 个字节，但可能受实现最小容量下限影响。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针，则函数会直接返回。
 */
void dstr_shrink_to_fit(dstr_adt *dstr);

/* 内容编辑。 */

/**
 * @brief 复制一个「C 字符串」到一个「动态字符串」。
 *
 * @warning src 不得指向 dest 的内部缓冲区，若需自复制，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为无效参数。
 * @param src[in] 源「C 字符串」的指针。为「空字符串」时复制空字符串（清空 dest 的内容）。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cpy_cstr(dstr_adt *dest, const char *src);

/**
 * @brief 复制一个「动态字符串」到另一个「动态字符串」。
 *
 * @warning src 不得指向 dest 的内部缓冲区，若需自复制，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为无效参数。
 * @param src[in] 源「动态字符串」的指针。为「空字符串」时复制空字符串（清空 dest 的内容）。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cpy(dstr_adt *dest, const dstr_adt *src);

/**
 * @brief 复制一个「C 字符串」的子串到一个「动态字符串」。
 *
 * @warning src 不得指向 dest 的内部缓冲区，若需自复制，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为无效参数。
 * @param src[in] 源「C 字符串」的指针。为「空字符串」时复制空字符串（清空 dest 的内容），
 *                此时忽略 sub_start 和 sub_length。
 * @param sub_start[in] 子串的起始索引。如果越界，则视为无效参数。
 * @param sub_length[in] 子串的长度。为 0 表示到末尾。如果越界，则视为无效参数。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cpy_sub_cstr(dstr_adt *dest, const char *src, size_t sub_start, size_t sub_length);

/**
 * @brief 复制一个「动态字符串」的子串到另一个「动态字符串」。
 *
 * @warning src 不得指向 dest 的内部缓冲区，若需自复制，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为无效参数。
 * @param src[in] 源「动态字符串」的指针。为「空字符串」时复制空字符串（清空 dest 的内容），
 *                此时忽略 sub_start 和 sub_length。
 * @param sub_start[in] 子串的起始索引。如果越界，则视为无效参数。
 * @param sub_length[in] 子串的长度。为 0 表示到末尾。如果越界，则视为无效参数。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cpy_sub(dstr_adt *dest, const dstr_adt *src, size_t sub_start, size_t sub_length);

/**
 * @brief 格式化复制一个字符串到一个「动态字符串」。
 *
 * @warning format 不得指向 dest 的内部缓冲区，若需自复制，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为无效参数。
 * @param format[in] 格式「C 字符串」的指针。为「空字符串」时复制空字符串（清空 dest 的内容）。
 * @param ...[in] 与 format 对应的可变参数列表。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cpy_format(dstr_adt *dest, const char *format, ...);

/**
 * @brief 格式化复制一个字符串到一个「动态字符串」（va_list 版本）。
 *
 * @warning format 不得指向 dest 的内部缓冲区，若需自复制，请先创建临时副本。
 *
 * @remark args 可能被此函数读取并消耗，调用后不应再次使用，除非重新 va_start() 或使用 va_copy()。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为无效参数。
 * @param format[in] 格式「C 字符串」的指针。为「空字符串」时复制空字符串（清空 dest 的内容）。
 * @param args[in] 已通过 va_start() 初始化的 va_list 变量，包含与 format 对应的可变参数列表信息。
 *                 该函数不会调用 va_end()，调用者需自行管理 args 的生命周期。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cpy_vformat(dstr_adt *dest, const char *format, va_list args);

/**
 * @brief 追加一个「C 字符串」到一个「动态字符串」。
 *
 * @warning src 不得指向 dest 的内部缓冲区，若需自追加，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为无效参数。
 * @param src[in] 源「C 字符串」的指针。为「空字符串」时追加空字符串（什么都不追加）。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cat_cstr(dstr_adt *dest, const char *src);

/**
 * @brief 追加一个「动态字符串」到另一个「动态字符串」。
 *
 * @warning src 不得指向 dest 的内部缓冲区，若需自追加，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为无效参数。
 * @param src[in] 源「动态字符串」的指针。为「空字符串」时追加空字符串（什么都不追加）。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cat(dstr_adt *dest, const dstr_adt *src);

/**
 * @brief 追加一个「C 字符串」的子串到一个「动态字符串」。
 *
 * @warning src 不得指向 dest 的内部缓冲区，若需自追加，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为无效参数。
 * @param src[in] 源「C 字符串」的指针。为「空字符串」时追加空字符串（什么都不追加），
 *                此时忽略 sub_start 和 sub_length。
 * @param sub_start[in] 子串的起始索引。如果越界，则视为无效参数。
 * @param sub_length[in] 子串的长度。为 0 表示到末尾。如果越界，则视为无效参数。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cat_sub_cstr(dstr_adt *dest, const char *src, size_t sub_start, size_t sub_length);

/**
 * @brief 追加一个「动态字符串」的子串到另一个「动态字符串」。
 *
 * @warning src 不得指向 dest 的内部缓冲区，若需自追加，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为无效参数。
 * @param src[in] 源「动态字符串」的指针。为「空字符串」时追加空字符串（什么都不追加），
 *                此时忽略 sub_start 和 sub_length。
 * @param sub_start[in] 子串的起始索引。如果越界，则视为无效参数。
 * @param sub_length[in] 子串的长度。为 0 表示到末尾。如果越界，则视为无效参数。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cat_sub(dstr_adt *dest, const dstr_adt *src, size_t sub_start, size_t sub_length);

/**
 * @brief 格式化追加一个字符串到一个「动态字符串」。
 *
 * @warning format 不得指向 dest 的内部缓冲区，若需自追加，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为无效参数。
 * @param format[in] 格式「C 字符串」的指针。为「空字符串」时追加空字符串（什么都不追加）。
 * @param ...[in] 与 format 对应的可变参数列表。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cat_format(dstr_adt *dest, const char *format, ...);

/**
 * @brief 格式化追加一个字符串到一个「动态字符串」（va_list 版本）。
 *
 * @warning format 不得指向 dest 的内部缓冲区，若需自追加，请先创建临时副本。
 *
 * @remark args 可能被此函数读取并消耗，调用后不应再次使用，除非重新 va_start() 或使用 va_copy()。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为无效参数。
 * @param format[in] 格式「C 字符串」的指针。为「空字符串」时追加空字符串（什么都不追加）。
 * @param args[in] 已通过 va_start() 初始化的 va_list 变量，包含与 format 对应的可变参数列表信息。
 *                 该函数不会调用 va_end()，调用者需自行管理 args 的生命周期。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cat_vformat(dstr_adt *dest, const char *format, va_list args);

/**
 * @brief 插入一个「C 字符串」到一个「动态字符串」。
 *
 * @warning src 不得指向 dest 的内部缓冲区，若需自插入，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为无效参数。
 * @param index[in] 插入位置的索引。如果越界，则视为无效参数。
 * @param src[in] 源「C 字符串」的指针。为「空字符串」时插入空字符串（什么都不插入）。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_insert_cstr(dstr_adt *dest, size_t index, const char *src);

/**
 * @brief 插入一个「动态字符串」到另一个「动态字符串」。
 *
 * @warning src 不得指向 dest 的内部缓冲区，若需自插入，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为无效参数。
 * @param index[in] 插入位置的索引。如果越界，则视为无效参数。
 * @param src[in] 源「动态字符串」的指针。为「空字符串」时插入空字符串（什么都不插入）。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_insert(dstr_adt *dest, size_t index, const dstr_adt *src);

/**
 * @brief 插入一个「C 字符串」的子串到一个「动态字符串」。
 *
 * @warning src 不得指向 dest 的内部缓冲区，若需自插入，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为无效参数。
 * @param index[in] 插入位置的索引。如果越界，则视为无效参数。
 * @param src[in] 源「C 字符串」的指针。为「空字符串」时插入空字符串（什么都不插入），
 *                此时忽略 sub_start 和 sub_length。
 * @param sub_start[in] 子串的起始索引。如果越界，则视为无效参数。
 * @param sub_length[in] 子串的长度。为 0 表示到末尾。如果越界，则视为无效参数。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_insert_sub_cstr(dstr_adt *dest, size_t index, const char *src, size_t sub_start, size_t sub_length);

/**
 * @brief 插入一个「动态字符串」的子串到另一个「动态字符串」。
 *
 * @warning src 不得指向 dest 的内部缓冲区，若需自插入，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为无效参数。
 * @param index[in] 插入位置的索引。如果越界，则视为无效参数。
 * @param src[in] 源「动态字符串」的指针。为「空字符串」时插入空字符串（什么都不插入），
 *                此时忽略 sub_start 和 sub_length。
 * @param sub_start[in] 子串的起始索引。如果越界，则视为无效参数。
 * @param sub_length[in] 子串的长度。为 0 表示到末尾。如果越界，则视为无效参数。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_insert_sub(dstr_adt *dest, size_t index, const dstr_adt *src, size_t sub_start, size_t sub_length);

/**
 * @brief 格式化插入一个字符串到一个「动态字符串」。
 *
 * @warning format 不得指向 dest 的内部缓冲区，若需自插入，请先创建临时副本。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为无效参数。
 * @param index[in] 插入位置的索引。如果越界，则视为无效参数。
 * @param format[in] 格式「C 字符串」的指针。为「空字符串」时插入空字符串（什么都不插入）。
 * @param ...[in] 与 format 对应的可变参数列表。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_insert_format(dstr_adt *dest, size_t index, const char *format, ...);

/**
 * @brief 格式化插入一个字符串到一个「动态字符串」（va_list 版本）。
 *
 * @warning format 不得指向 dest 的内部缓冲区，若需自插入，请先创建临时副本。
 *
 * @remark args 可能被此函数读取并消耗，调用后不应再次使用，除非重新 va_start() 或使用 va_copy()。
 *
 * @param dest[in] 目标「动态字符串」的指针。如果为空指针，则视为无效参数。
 * @param index[in] 插入位置的索引。如果越界，则视为无效参数。
 * @param format[in] 格式「C 字符串」的指针。为「空字符串」时插入空字符串（什么都不插入）。
 * @param args[in] 已通过 va_start() 初始化的 va_list 变量，包含与 format 对应的可变参数列表信息。
 *                 该函数不会调用 va_end()，调用者需自行管理 args 的生命周期。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_insert_vformat(dstr_adt *dest, size_t index, const char *format, va_list args);

/**
 * @brief 清空一个「动态字符串」。
 *
 * @note 此函数只会使其长度为 0，不会立即释放容量。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为空指针，则函数会直接返回。
 */
void dstr_clear(dstr_adt *dstr);

/**
 * @brief 删除一个「动态字符串」的子串。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为「空字符串」，则函数会直接返回，
 *                 此时忽略 sub_start 和 sub_length。
 * @param sub_start[in] 子串的起始索引。如果越界，则函数会直接返回。
 * @param sub_length[in] 子串的长度。为 0 表示到末尾。如果越界，则函数会直接返回。
 */
void dstr_remove(dstr_adt *dstr, size_t sub_start, size_t sub_length);

/**
 * @brief 删除一个「动态字符串」首尾的空白字符或指定字符。
 *
 * @note 空白字符判定使用 C 标准库函数 isspace()。本库不修改 locale，因此判定结果取决于调用时的 locale。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为「空字符串」，则函数会直接返回。
 * @param trim_chars[in] 包含要删除的字符的「C 字符串」的指针。为「空字符串」时删除空白字符。
 */
void dstr_trim(dstr_adt *dstr, const char *trim_chars);

/* 关系判断与比较。
 *
 * 此组函数对空字符串的处理方式较特殊，遵循字符串理论中的相关定理。
 */

/**
 * @brief 判断一个「动态字符串」是否以指定「C 字符串」前缀开头。
 *
 * @param dstr[in] 目标「动态字符串」的指针。
 * @param prefix[in] 前缀「C 字符串」的指针。
 *
 * @return 如果目标「动态字符串」以指定「C 字符串」前缀开头则返回 true，否则返回 false。
 *         如果 prefix 为「空字符串」，则一定返回 true（视「空字符串」是任何字符串的前缀）。
 */
bool dstr_starts_with_cstr(const dstr_adt *dstr, const char *prefix);

/**
 * @brief 判断一个「动态字符串」是否以指定「动态字符串」前缀开头。
 *
 * @param dstr[in] 目标「动态字符串」的指针。
 * @param prefix[in] 前缀「动态字符串」的指针。
 *
 * @return 如果目标「动态字符串」以指定「动态字符串」前缀开头则返回 true，否则返回 false。
 *         如果 prefix 为「空字符串」，则一定返回 true（视「空字符串」是任何字符串的前缀）。
 */
bool dstr_starts_with(const dstr_adt *dstr, const dstr_adt *prefix);

/**
 * @brief 判断一个「C 字符串」是否以指定「C 字符串」前缀开头。
 *
 * @param cstr[in] 目标「C 字符串」的指针。
 * @param prefix[in] 前缀「C 字符串」的指针。
 *
 * @return 如果目标「C 字符串」以指定「C 字符串」前缀开头则返回 true，否则返回 false。
 *         如果 prefix 为「空字符串」，则一定返回 true（视「空字符串」是任何字符串的前缀）。
 */
bool cstr_starts_with(const char *cstr, const char *prefix);

/**
 * @brief 判断一个「动态字符串」是否以指定「C 字符串」后缀结尾。
 *
 * @param dstr[in] 目标「动态字符串」的指针。
 * @param suffix[in] 后缀「C 字符串」的指针。
 *
 * @return 如果目标「动态字符串」以指定「C 字符串」后缀结尾则返回 true，否则返回 false。
 *         如果 suffix 为「空字符串」，则一定返回 true（视「空字符串」是任何字符串的后缀）。
 */
bool dstr_ends_with_cstr(const dstr_adt *dstr, const char *suffix);

/**
 * @brief 判断一个「动态字符串」是否以指定「动态字符串」后缀结尾。
 *
 * @param dstr[in] 目标「动态字符串」的指针。
 * @param suffix[in] 后缀「动态字符串」的指针。
 *
 * @return 如果目标「动态字符串」以指定「动态字符串」后缀结尾则返回 true，否则返回 false。
 *         如果 suffix 为「空字符串」，则一定返回 true（视「空字符串」是任何字符串的后缀）。
 */
bool dstr_ends_with(const dstr_adt *dstr, const dstr_adt *suffix);

/**
 * @brief 判断一个「C 字符串」是否以指定「C 字符串」后缀结尾。
 *
 * @param cstr[in] 目标「C 字符串」的指针。
 * @param suffix[in] 后缀「C 字符串」的指针。
 *
 * @return 如果目标「C 字符串」以指定「C 字符串」后缀结尾则返回 true，否则返回 false。
 *         如果 suffix 为「空字符串」，则一定返回 true（视「空字符串」是任何字符串的后缀）。
 */
bool cstr_ends_with(const char *cstr, const char *suffix);

/**
 * @brief 判断一个「动态字符串」是否包含指定子「C 字符串」。
 *
 * @param dstr[in] 目标「动态字符串」的指针。
 * @param sub[in] 子「C 字符串」的指针。
 *
 * @return 包含则返回 true，否则返回 false。
 *         如果 sub 为「空字符串」，则一定返回 true（视「空字符串」是任何字符串的子串）。
 */
bool dstr_contains_cstr(const dstr_adt *dstr, const char *sub);

/**
 * @brief 判断一个「动态字符串」是否包含指定子「动态字符串」。
 *
 * @param dstr[in] 目标「动态字符串」的指针。
 * @param sub[in] 子「动态字符串」的指针。
 *
 * @return 包含则返回 true，否则返回 false。
 *         如果 sub 为「空字符串」，则一定返回 true（视「空字符串」是任何字符串的子串）。
 */
bool dstr_contains(const dstr_adt *dstr, const dstr_adt *sub);

/**
 * @brief 判断一个「C 字符串」是否包含指定子「C 字符串」。
 *
 * @param cstr[in] 目标「C 字符串」的指针。
 * @param sub[in] 子「C 字符串」的指针。
 *
 * @return 包含则返回 true，否则返回 false。
 *         如果 sub 为「空字符串」，则一定返回 true（视「空字符串」是任何字符串的子串）。
 */
bool cstr_contains(const char *cstr, const char *sub);

/**
 * @brief 判断一个「动态字符串」是否与一个「C 字符串」相等。
 *
 * @param lhs[in] 「动态字符串」的指针。
 * @param rhs[in] 「C 字符串」的指针。
 *
 * @return 相等则返回 true，否则返回 false。两者同为「空字符串」时，返回 true。
 */
bool dstr_equals_cstr(const dstr_adt *lhs, const char *rhs);

/**
 * @brief 判断两个「动态字符串」是否相等。
 *
 * @param lhs[in] 第一个「动态字符串」的指针。
 * @param rhs[in] 第二个「动态字符串」的指针。
 *
 * @return 相等则返回 true，否则返回 false。两者同为「空字符串」时，返回 true。
 */
bool dstr_equals(const dstr_adt *lhs, const dstr_adt *rhs);

/**
 * @brief 判断两个「C 字符串」是否相等。
 *
 * @param lhs[in] 第一个「C 字符串」的指针。
 * @param rhs[in] 第二个「C 字符串」的指针。
 *
 * @return 相等则返回 true，否则返回 false。两者同为「空字符串」时，返回 true。
 */
bool cstr_equals(const char *lhs, const char *rhs);

/**
 * @brief 比较一个「动态字符串」与一个「C 字符串」。
 *
 * @param lhs[in] 「动态字符串」的指针。
 * @param rhs[in] 「C 字符串」的指针。
 *
 * @return 两者相等返回 0，前者大于后者返回正值，前者小于后者返回负值。两者同为「空字符串」时，返回 0。
 *         两者只有一者为「空字符串」时，为「空字符串」者小于非「空字符串」者。
 */
int dstr_compare_cstr(const dstr_adt *lhs, const char *rhs);

/**
 * @brief 比较两个「动态字符串」。
 *
 * @param lhs[in] 第一个「动态字符串」的指针。
 * @param rhs[in] 第二个「动态字符串」的指针。
 *
 * @return 两者相等返回 0，前者大于后者返回正值，前者小于后者返回负值。两者同为「空字符串」时，返回 0。
 *         两者只有一者为「空字符串」时，为「空字符串」者小于非「空字符串」者。
 */
int dstr_compare(const dstr_adt *lhs, const dstr_adt *rhs);

/**
 * @brief 比较两个「C 字符串」。
 *
 * @param lhs[in] 第一个「C 字符串」的指针。
 * @param rhs[in] 第二个「C 字符串」的指针。
 *
 * @return 两者相等返回 0，前者大于后者返回正值，前者小于后者返回负值。两者同为「空字符串」时，返回 0。
 *         两者只有一者为「空字符串」时，为「空字符串」者小于非「空字符串」者。
 */
int cstr_compare(const char *lhs, const char *rhs);

/* 查找、统计与替换。
 *
 * 此组函数均按“非重叠匹配”处理。
 */

/**
 * @brief 查找一个「动态字符串」中指定子「C 字符串」首次出现的位置。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为「空字符串」，则函数会直接返回 false。
 * @param sub[in] 子「C 字符串」的指针。如果为「空字符串」，则函数会直接返回 false。
 * @param out_index[out] 存储查找结果（位置索引）的 size_t 变量的指针。为空指针时不写入。
 * @param direction[in] 查找方向。
 *
 * @return 找到则返回 true，否则返回 false。
 */
bool dstr_find_cstr(const dstr_adt *dstr, const char *sub, size_t *out_index, dstr_direction_t direction);

/**
 * @brief 查找一个「动态字符串」中指定子「动态字符串」首次出现的位置。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为「空字符串」，则函数会直接返回 false。
 * @param sub[in] 子「动态字符串」的指针。如果为「空字符串」，则函数会直接返回 false。
 * @param out_index[out] 存储查找结果（位置索引）的 size_t 变量的指针。为空指针时不写入。
 * @param direction[in] 查找方向。
 *
 * @return 找到则返回 true，否则返回 false。
 */
bool dstr_find(const dstr_adt *dstr, const dstr_adt *sub, size_t *out_index, dstr_direction_t direction);

/**
 * @brief 查找一个「C 字符串」中指定子「C 字符串」首次出现的位置。
 *
 * @param cstr[in] 目标「C 字符串」的指针。如果为「空字符串」，则函数会直接返回 false。
 * @param sub[in] 子「C 字符串」的指针。如果为「空字符串」，则函数会直接返回 false。
 * @param out_index[out] 存储查找结果（位置索引）的 size_t 变量的指针。为空指针时不写入。
 * @param direction[in] 查找方向。
 *
 * @return 找到则返回 true，否则返回 false。
 */
bool cstr_find(const char *cstr, const char *sub, size_t *out_index, dstr_direction_t direction);

/**
 * @brief 查找一个「动态字符串」中指定子「C 字符串」第 n 次出现的位置。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为「空字符串」，则函数会直接返回 false。
 * @param sub[in] 子「C 字符串」的指针。如果为「空字符串」，则函数会直接返回 false。
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
 * @param dstr[in] 目标「动态字符串」的指针。如果为「空字符串」，则函数会直接返回 false。
 * @param sub[in] 子「动态字符串」的指针。如果为「空字符串」，则函数会直接返回 false。
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
 * @param cstr[in] 目标「C 字符串」的指针。如果为「空字符串」，则函数会直接返回 false。
 * @param sub[in] 子「C 字符串」的指针。如果为「空字符串」，则函数会直接返回 false。
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
 * @param dstr[in] 目标「动态字符串」的指针。如果为「空字符串」，则函数会直接返回 0。
 * @param sub[in] 子「C 字符串」的指针。如果为「空字符串」，则函数会直接返回 0。
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
 * @param dstr[in] 目标「动态字符串」的指针。如果为「空字符串」，则函数会直接返回 0。
 * @param sub[in] 子「动态字符串」的指针。如果为「空字符串」，则函数会直接返回 0。
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
 * @param cstr[in] 目标「C 字符串」的指针。如果为「空字符串」，则函数会直接返回 0。
 * @param sub[in] 子「C 字符串」的指针。如果为「空字符串」，则函数会直接返回 0。
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
 * @param dstr[in] 目标「动态字符串」的指针。如果为「空字符串」，则函数会直接返回 0。
 * @param sub[in] 子「C 字符串」的指针。如果为「空字符串」，则函数会直接返回 0。
 *
 * @return 出现的次数。
 */
size_t dstr_count_cstr(const dstr_adt *dstr, const char *sub);

/**
 * @brief 统计一个「动态字符串」中指定子「动态字符串」出现的次数。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为「空字符串」，则函数会直接返回 0。
 * @param sub[in] 子「动态字符串」的指针。如果为「空字符串」，则函数会直接返回 0。
 *
 * @return 出现的次数。
 */
size_t dstr_count(const dstr_adt *dstr, const dstr_adt *sub);

/**
 * @brief 统计一个「C 字符串」中指定子「C 字符串」出现的次数。
 *
 * @param cstr[in] 目标「C 字符串」的指针。如果为「空字符串」，则函数会直接返回 0。
 * @param sub[in] 子「C 字符串」的指针。如果为「空字符串」，则函数会直接返回 0。
 *
 * @return 出现的次数。
 */
size_t cstr_count(const char *cstr, const char *sub);

/**
 * @brief 替换一个「动态字符串」中指定旧「C 字符串」为指定新「C 字符串」n 次。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为「空字符串」，则视为无效参数。
 * @param old_str[in] 旧「C 字符串」的指针。如果为「空字符串」，则视为无效参数。
 *                    如果在 dstr 中一次都没有出现或出现次数不足 n 次（n 不为 0 时），则视为无效参数。
 * @param new_str[in] 新「C 字符串」的指针。为「空字符串」时，替换为空。
 * @param direction[in] 替换方向。
 * @param n[in] 替换的次数。为 0 表示替换所有。
 *              如果大于旧「C 字符串」实际出现的次数，则视为无效参数，将一次替换都不进行。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_replace_cstr(dstr_adt *dstr, const char *old_str, const char *new_str, dstr_direction_t direction,
                                size_t n);

/**
 * @brief 替换一个「动态字符串」中指定旧「动态字符串」为指定新「动态字符串」n 次。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为「空字符串」，则视为无效参数。
 * @param old_str[in] 旧「动态字符串」的指针。如果为「空字符串」，则视为无效参数。
 *                    如果在 dstr 中一次都没有出现或出现次数不足 n 次（n 不为 0 时），则视为无效参数。
 * @param new_str[in] 新「动态字符串」的指针。为「空字符串」时，替换为空。
 * @param direction[in] 替换方向。
 * @param n[in] 替换的次数。为 0 表示替换所有。
 *              如果大于旧「动态字符串」实际出现的次数，则视为无效参数，将一次替换都不进行。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_replace(dstr_adt *dstr, const dstr_adt *old_str, const dstr_adt *new_str, dstr_direction_t direction,
                           size_t n);

/**
 * @brief 替换一个「动态字符串」中指定旧「C 字符串」第 n 次为指定新「C 字符串」。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为「空字符串」，则视为无效参数。
 * @param old_str[in] 旧「C 字符串」的指针。如果为「空字符串」，则视为无效参数。
 *                    如果在 dstr 中一次都没有出现或出现次数不足 n 次（n 不为 0 时），则视为无效参数。
 * @param new_str[in] 新「C 字符串」的指针。为「空字符串」时，替换为空。
 * @param direction[in] 替换方向。
 * @param n[in] 替换的次序。从 1 开始。为 0 表示该方向的最后一次出现。
 *              具体：direction 为 DSTR_DIR_FORWARD 时，n>0 表示从前往后第 n 次，
 *              n=0 表示从前往后最后一次（等价于 DSTR_DIR_BACKWARD, n=1）；
 *              direction 为 DSTR_DIR_BACKWARD 时，n>0 表示从后往前第 n 次，
 *              n=0 表示从后往前最后一次（等价于 DSTR_DIR_FORWARD, n=1）。
 *              如果大于旧「C 字符串」实际出现的次数，则视为无效参数，将一次替换都不进行。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_replace_nth_cstr(dstr_adt *dstr, const char *old_str, const char *new_str,
                                    dstr_direction_t direction, size_t n);

/**
 * @brief 替换一个「动态字符串」中指定旧「动态字符串」第 n 次为指定新「动态字符串」。
 *
 * @param dstr[in] 目标「动态字符串」的指针。如果为「空字符串」，则视为无效参数。
 * @param old_str[in] 旧「动态字符串」的指针。如果为「空字符串」，则视为无效参数。
 *                    如果在 dstr 中一次都没有出现或出现次数不足 n 次（n 不为 0 时），则视为无效参数。
 * @param new_str[in] 新「动态字符串」的指针。为「空字符串」时，替换为空。
 * @param direction[in] 替换方向。
 * @param n[in] 替换的次序。从 1 开始。为 0 表示该方向的最后一次出现。
 *              具体：direction 为 DSTR_DIR_FORWARD 时，n>0 表示从前往后第 n 次，
 *              n=0 表示从前往后最后一次（等价于 DSTR_DIR_BACKWARD, n=1）；
 *              direction 为 DSTR_DIR_BACKWARD 时，n>0 表示从后往前第 n 次，
 *              n=0 表示从后往前最后一次（等价于 DSTR_DIR_FORWARD, n=1）。
 *              如果大于旧「动态字符串」实际出现的次数，则视为无效参数，将一次替换都不进行。
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
 * @param cstr[in] 目标「C 字符串」的指针。如果为「空字符串」，则函数会直接返回空指针。
 * @param separator[in] 分隔「C 字符串」的指针。如果为「空字符串」，则函数会直接返回空指针。
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
 * @param dstr[in] 目标「动态字符串」的指针。如果为「空字符串」，则函数会直接返回空指针。
 * @param separator[in] 分隔「动态字符串」的指针。如果为「空字符串」，则函数会直接返回空指针。
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
 * @param cstr[in] 目标「C 字符串」的指针。如果为「空字符串」，则函数会直接返回空指针。
 * @param separator[in] 分隔「C 字符串」的指针。如果为「空字符串」，则函数会直接返回空指针。
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
 * @param separator[in] 分隔「C 字符串」的指针。为「空字符串」时使用空字符串合并。
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
 * @param separator[in] 分隔「动态字符串」的指针。为「空字符串」时使用空字符串合并。
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
 * @param separator[in] 分隔「C 字符串」的指针。为「空字符串」时使用空字符串合并。
 *
 * @return 合并后的「C 字符串」的指针。如果合并失败则返回空指针。
 */
char *cstr_join(const char *const *cstrs, size_t cstr_count, const char *separator);

/* C++ 兼容包裹宏-结束。 */
DYNAMIC_STRING_EXTERN_C_END

#endif /* DYNAMIC_STRING_H */
