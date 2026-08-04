/*
 * Copyright (C) 2026 mtueih
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


/*==============================================================================
 * include/dynamic_string.h - 项目主库头文件
 *============================================================================*/
#ifndef DYNAMIC_STRING_H
#define DYNAMIC_STRING_H


/*------------------------------------------------------------------------------
 * 头文件包含
 *----------------------------------------------------------------------------*/
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>

/**
 * C23 标准已将 bool/true/false 收为内置关键字，
 * 因此按标准仅需在 C23 之前包含 stdbool.h。
 */
#if !defined(__STDC_VERSION__) || \
    (defined(__STDC_VERSION__) && __STDC_VERSION__ < 202311L)
#  include <stdbool.h>
#endif


/*------------------------------------------------------------------------------
 * ADT 类型别名声明
 *----------------------------------------------------------------------------*/

/* 「动态字符串」ADT 类型别名。 */
typedef struct dynamic_string dstr_adt;


/*------------------------------------------------------------------------------
 * 全局状态码
 *----------------------------------------------------------------------------*/

/* 全局状态码枚举类型。 */
typedef enum {
	DSTR_SUCCESS = 0,         /* 成功。 */
	DSTR_MEMORY_ALLOC_FAILED, /* 内存分配失败。 */
	DSTR_INVALID_ARGUMENT,    /* 无效参数。 */
	DSTR_UNKNOWN_ERROR,       /* 未知错误。 */
} dstr_status_t;


/*------------------------------------------------------------------------------
 * 其他类型定义
 *----------------------------------------------------------------------------*/

/**
 * 方向枚举类型。
 * 主要用于查找与替换系列函数，表示查找/替换的方向。
 */
typedef enum {
	DSTR_DIR_FORWARD, /* 从前往后。 */
	DSTR_DIR_BACKWARD /* 从后往前。 */
} dstr_direction_t;


/*------------------------------------------------------------------------------
 * 接口函数原型（声明）
 *----------------------------------------------------------------------------*/

/* 创建与销毁。 */

/**
 * @brief
 * 创建一个「动态字符串」。
 *
 * @attention
 * 返回值指向堆内存，请手动调用 dstr_destroy 释放。
 *
 * @param cstr
 * 源「C 字符串」的指针。
 * 为空指针或指向空「C 字符串」时创建空「动态字符串」。
 *
 * @return
 * 所创建的「动态字符串」的指针。
 * 如果创建失败则返回空指针。
 */
dstr_adt *dstr_create(
	const char *cstr
);

/**
 * @brief
 * 销毁一个「动态字符串」。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * 如果为空指针，则函数会直接返回。
 */
void dstr_destroy(
	dstr_adt *dstr
);

/**
 * @brief
 * 克隆一个「动态字符串」。
 *
 * @attention
 * 返回值指向堆内存，请手动调用 dstr_destroy 释放。
 *
 * @param dstr
 * 源「动态字符串」的指针。
 * 为空指针或指向空「动态字符串」时创建空「动态字符串」。
 *
 * @return
 * 所创建的「动态字符串」的指针。
 * 如果创建失败则返回空指针。
 */
dstr_adt *dstr_clone(
	const dstr_adt *dstr
);

/**
 * @brief
 * 提取一个「C 字符串」的子串。
 *
 * @attention
 * 返回值指向堆内存，请手动调用 dstr_destroy 释放。
 *
 * @param cstr
 * 源「C 字符串」的指针。
 * 为空指针或指向空「C 字符串」时创建空「动态字符串」。
 * @param sub_start
 * 子串的起始索引。
 * 如果越界，则函数会直接返回空指针。
 * @param sub_length
 * 子串的长度。
 * 为 0 表示到末尾。
 * 如果越界，则函数会直接返回空指针。
 *
 * @return
 * 所创建的「动态字符串」的指针。
 * 如果创建失败则返回空指针。
 */
dstr_adt *dstr_sub_cstr(
	const char *cstr,
	size_t sub_start,
	size_t sub_length
);

/**
 * @brief
 * 提取一个「动态字符串」的子串。
 *
 * @attention
 * 返回值指向堆内存，请手动调用 dstr_destroy 释放。
 *
 * @param dstr
 * 源「动态字符串」的指针。
 * 为空指针或指向空「动态字符串」时创建空「动态字符串」。
 * @param sub_start
 * 子串的起始索引。
 * 如果越界，则函数会直接返回空指针。
 * @param sub_length
 * 子串的长度。
 * 为 0 表示到末尾。
 * 如果越界，则函数会直接返回空指针。
 *
 * @return
 * 所创建的「动态字符串」的指针。
 * 如果创建失败则返回空指针。
 */
dstr_adt *dstr_sub(
	const dstr_adt *dstr,
	size_t sub_start,
	size_t sub_length
);

/**
 * @brief
 * 格式化创建一个「动态字符串」。
 *
 * @attention
 * 返回值指向堆内存，请手动调用 dstr_destroy 释放。
 *
 * @param format
 * 格式「C 字符串」的指针。
 * 为空指针或指向空「C 字符串」时创建空「动态字符串」。
 * @param ...
 * 与格式字符串对应的可变参数列表。
 *
 * @return
 * 所创建的「动态字符串」的指针。
 * 如果创建失败则返回空指针。
 */
dstr_adt *dstr_create_format(
	const char *format,
	...
);

/**
 * @brief
 * 格式化创建一个「动态字符串」（va_list 版本）。
 *
 * @attention
 * 返回值指向堆内存，请手动调用 dstr_destroy 释放。
 *
 * @param format
 * 格式「C 字符串」的指针。
 * 为空指针或指向空「C 字符串」时创建空「动态字符串」。
 * @param args
 * 已通过 va_start 初始化的 va_list，包含与格式字符串对应的可变参数。
 * 该函数不会调用 va_end，调用者需自行管理 va_list 的生命周期。
 *
 * @return
 * 所创建的「动态字符串」的指针。
 * 如果创建失败则返回空指针。
 */
dstr_adt *dstr_create_vformat(
	const char *format,
	va_list args
);

/* 属性获取与设置。 */

/**
 * @brief
 * 获取一个「动态字符串」的内部「C 字符串」指针。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * 如果为空指针，则函数会直接返回空指针。
 *
 * @return
 * 所获取的「C 字符串」指针。
 */
const char *dstr_cstr(
	const dstr_adt *dstr
);

/**
 * @brief
 * 获取一个「动态字符串」的长度。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * 如果为空指针，则函数会直接返回 0。
 *
 * @return
 * 所获取的长度。
 */
size_t dstr_length(
	const dstr_adt *dstr
);

/**
 * @brief
 * 判断一个「动态字符串」是否是空「动态字符串」。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * 如果为空指针，则函数会直接返回 true。
 *
 * @return
 * 如果目标「动态字符串」是空「动态字符串」则返回 true，否则返回 false。
 */
bool dstr_is_empty(
	const dstr_adt *dstr
);

/**
 * @brief
 * 获取一个「动态字符串」的容量。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * 如果为空指针，则函数会直接返回 0。
 *
 * @return
 * 所获取的容量。
 */
size_t dstr_capacity(
	const dstr_adt *dstr
);

/**
 * @brief
 * 设置一个「动态字符串」的容量。
 *
 * @attention
 * 此函数若执行成功，会同时为目标「动态字符串」设定一个最小容量下限；
 * 此后即使长度变化触发容量自动调整，实际容量也不会低于该值。
 * 调用 dstr_shrink_to_fit 函数会清除该下限。
 *
 * @warning
 * 当新的容量小于当前长度时，当前内容将被截断。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * 如果为空指针，则视为不合法参数。
 * @param new_capacity
 * 新的容量。
 *
 * @return
 * 全局状态码。
 */
dstr_status_t dstr_set_capacity(
	dstr_adt *dstr,
	size_t new_capacity
);

/**
 * @brief
 * 调整一个「动态字符串」的容量到刚合适。
 *
 * @attention
 * 执行此函数会同时取消目标「动态字符串」由 dstr_set_capacity 所设置的最小容量下限。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * 如果为空指针，则函数会直接返回。
 */
void dstr_shrink_to_fit(
	dstr_adt *dstr
);

/* 内容编辑。 */

/**
 * @brief
 * 复制一个「C 字符串」到一个「动态字符串」。
 *
 * @param dest
 * 目标「动态字符串」的指针。
 * 如果为空指针，则视为不合法参数。
 * @param src
 * 源「C 字符串」的指针。
 * 为空指针或指向空「C 字符串」时复制空字符串。
 *
 * @return
 * 全局状态码。
 */
dstr_status_t dstr_cpy_cstr(
	dstr_adt *dest,
	const char *src
);

/**
 * @brief
 * 复制一个「动态字符串」到另一个「动态字符串」。
 *
 * @param dest
 * 目标「动态字符串」的指针。
 * 如果为空指针，则视为不合法参数。
 * @param src
 * 源「动态字符串」的指针。
 * 为空指针或指向空「动态字符串」时复制空字符串。
 *
 * @return
 * 全局状态码。
 */
dstr_status_t dstr_cpy(
	dstr_adt *dest,
	const dstr_adt *src
);

/**
 * @brief
 * 复制一个「C 字符串」的子串到一个「动态字符串」。
 *
 * @param dest
 * 目标「动态字符串」的指针。
 * 如果为空指针，则视为不合法参数。
 * @param src
 * 源「C 字符串」的指针。
 * 为空指针或指向空「C 字符串」时复制空字符串。
 * @param sub_start
 * 子串的起始索引。
 * 如果越界，则视为不合法参数。
 * @param sub_length
 * 子串的长度。
 * 为 0 表示到末尾。
 * 如果越界，则视为不合法参数。
 *
 * @return
 * 全局状态码。
 */
dstr_status_t dstr_cpy_sub_cstr(
	dstr_adt *dest,
	const char *src,
	size_t sub_start,
	size_t sub_length
);

/**
 * @brief
 * 复制一个「动态字符串」的子串到另一个「动态字符串」。
 *
 * @param dest
 * 目标「动态字符串」的指针。
 * 如果为空指针，则视为不合法参数。
 * @param src
 * 源「动态字符串」的指针。
 * 为空指针或指向空「动态字符串」时复制空字符串。
 * @param sub_start
 * 子串的起始索引。
 * 如果越界，则视为不合法参数。
 * @param sub_length
 * 子串的长度。
 * 为 0 表示到末尾。
 * 如果越界，则视为不合法参数。
 *
 * @return
 * 全局状态码。
 */
dstr_status_t dstr_cpy_sub(
	dstr_adt *dest,
	const dstr_adt *src,
	size_t sub_start,
	size_t sub_length
);

/**
 * @brief
 * 格式化复制一个字符串到一个「动态字符串」。
 *
 * @param dest
 * 目标「动态字符串」的指针。
 * 如果为空指针，则视为不合法参数。
 * @param format
 * 格式「C 字符串」的指针。
 * 为空指针或指向空「C 字符串」时复制空字符串。
 * @param ...
 * 与格式字符串对应的可变参数列表。
 *
 * @return
 * 全局状态码。
 */
dstr_status_t dstr_cpy_format(
	dstr_adt *dest,
	const char *format,
	...
);

/**
 * @brief
 * 格式化复制一个字符串到一个「动态字符串」（va_list 版本）。
 *
 * @param dest
 * 目标「动态字符串」的指针。
 * 如果为空指针，则视为不合法参数。
 * @param format
 * 格式「C 字符串」的指针。
 * 为空指针或指向空「C 字符串」时复制空字符串。
 * @param args
 * 已通过 va_start 初始化的 va_list，包含与格式字符串对应的可变参数。
 * 该函数不会调用 va_end，调用者需自行管理 va_list 的生命周期。
 *
 * @return
 * 全局状态码。
 */
dstr_status_t dstr_cpy_vformat(
	dstr_adt *dest,
	const char *format,
	va_list args
);

/**
 * @brief
 * 追加一个「C 字符串」到一个「动态字符串」。
 *
 * @param dest
 * 目标「动态字符串」的指针。
 * 如果为空指针，则视为不合法参数。
 * @param src
 * 源「C 字符串」的指针。
 * 为空指针或指向空「C 字符串」时追加空字符串。
 *
 * @return
 * 全局状态码。
 */
dstr_status_t dstr_cat_cstr(
	dstr_adt *dest,
	const char *src
);

/**
 * @brief
 * 追加一个「动态字符串」到另一个「动态字符串」。
 *
 * @param dest
 * 目标「动态字符串」的指针。
 * 如果为空指针，则视为不合法参数。
 * @param src
 * 源「动态字符串」的指针。
 * 为空指针或指向空「动态字符串」时追加空字符串。
 *
 * @return
 * 全局状态码。
 */
dstr_status_t dstr_cat(
	dstr_adt *dest,
	const dstr_adt *src
);

/**
 * @brief
 * 追加一个「C 字符串」的子串到一个「动态字符串」。
 *
 * @param dest
 * 目标「动态字符串」的指针。
 * 如果为空指针，则视为不合法参数。
 * @param src
 * 源「C 字符串」的指针。
 * 为空指针或指向空「C 字符串」时追加空字符串。
 * @param sub_start
 * 子串的起始索引。
 * 如果越界，则视为不合法参数。
 * @param sub_length
 * 子串的长度。
 * 为 0 表示到末尾。
 * 如果越界，则视为不合法参数。
 *
 * @return
 * 全局状态码。
 */
dstr_status_t dstr_cat_sub_cstr(
	dstr_adt *dest,
	const char *src,
	size_t sub_start,
	size_t sub_length
);

/**
 * @brief
 * 追加一个「动态字符串」的子串到另一个「动态字符串」。
 *
 * @param dest
 * 目标「动态字符串」的指针。
 * 如果为空指针，则视为不合法参数。
 * @param src
 * 源「动态字符串」的指针。
 * 为空指针或指向空「动态字符串」时追加空字符串。
 * @param sub_start
 * 子串的起始索引。
 * 如果越界，则视为不合法参数。
 * @param sub_length
 * 子串的长度。
 * 为 0 表示到末尾。
 * 如果越界，则视为不合法参数。
 *
 * @return
 * 全局状态码。
 */
dstr_status_t dstr_cat_sub(
	dstr_adt *dest,
	const dstr_adt *src,
	size_t sub_start,
	size_t sub_length
);

/**
 * @brief
 * 格式化追加一个字符串到一个「动态字符串」。
 *
 * @param dest
 * 目标「动态字符串」的指针。
 * 如果为空指针，则视为不合法参数。
 * @param format
 * 格式「C 字符串」的指针。
 * 为空指针或指向空「C 字符串」时追加空字符串。
 * @param ...
 * 与格式字符串对应的可变参数列表。
 *
 * @return
 * 全局状态码。
 */
dstr_status_t dstr_cat_format(
	dstr_adt *dest,
	const char *format,
	...
);

/**
 * @brief
 * 格式化追加一个字符串到一个「动态字符串」（va_list 版本）。
 *
 * @param dest
 * 目标「动态字符串」的指针。
 * 如果为空指针，则视为不合法参数。
 * @param format
 * 格式「C 字符串」的指针。
 * 为空指针或指向空「C 字符串」时追加空字符串。
 * @param args
 * 已通过 va_start 初始化的 va_list，包含与格式字符串对应的可变参数。
 * 该函数不会调用 va_end，调用者需自行管理 va_list 的生命周期。
 *
 * @return
 * 全局状态码。
 */
dstr_status_t dstr_cat_vformat(
	dstr_adt *dest,
	const char *format,
	va_list args
);

/**
 * @brief
 * 插入一个「C 字符串」到一个「动态字符串」。
 *
 * @param dest
 * 目标「动态字符串」的指针。
 * 如果为空指针，则视为不合法参数。
 * @param index
 * 插入位置的索引。
 * 如果越界，则视为不合法参数。
 * @param src
 * 源「C 字符串」的指针。
 * 为空指针或指向空「C 字符串」时插入空字符串。
 *
 * @return
 * 全局状态码。
 */
dstr_status_t dstr_insert_cstr(
	dstr_adt *dest,
	size_t index,
	const char *src
);

/**
 * @brief
 * 插入一个「动态字符串」到另一个「动态字符串」。
 *
 * @param dest
 * 目标「动态字符串」的指针。
 * 如果为空指针，则视为不合法参数。
 * @param index
 * 插入位置的索引。
 * 如果越界，则视为不合法参数。
 * @param src
 * 源「动态字符串」的指针。
 * 为空指针或指向空「动态字符串」时插入空字符串。
 *
 * @return
 * 全局状态码。
 */
dstr_status_t dstr_insert(
	dstr_adt *dest,
	size_t index,
	const dstr_adt *src
);

/**
 * @brief
 * 插入一个「C 字符串」的子串到一个「动态字符串」。
 *
 * @param dest
 * 目标「动态字符串」的指针。
 * 如果为空指针，则视为不合法参数。
 * @param index
 * 插入位置的索引。
 * 如果越界，则视为不合法参数。
 * @param src
 * 源「C 字符串」的指针。
 * 为空指针或指向空「C 字符串」时插入空字符串。
 * @param sub_start
 * 子串的起始索引。
 * 如果越界，则视为不合法参数。
 * @param sub_length
 * 子串的长度。
 * 为 0 表示到末尾。
 * 如果越界，则视为不合法参数。
 *
 * @return
 * 全局状态码。
 */
dstr_status_t dstr_insert_sub_cstr(
	dstr_adt *dest,
	size_t index,
	const char *src,
	size_t sub_start,
	size_t sub_length
);

/**
 * @brief
 * 插入一个「动态字符串」的子串到另一个「动态字符串」。
 *
 * @param dest
 * 目标「动态字符串」的指针。
 * 如果为空指针，则视为不合法参数。
 * @param index
 * 插入位置的索引。
 * 如果越界，则视为不合法参数。
 * @param src
 * 源「动态字符串」的指针。
 * 为空指针或指向空「动态字符串」时插入空字符串。
 * @param sub_start
 * 子串的起始索引。
 * 如果越界，则视为不合法参数。
 * @param sub_length
 * 子串的长度。
 * 为 0 表示到末尾。
 * 如果越界，则视为不合法参数。
 *
 * @return
 * 全局状态码。
 */
dstr_status_t dstr_insert_sub(
	dstr_adt *dest,
	size_t index,
	const dstr_adt *src,
	size_t sub_start,
	size_t sub_length
);

/**
 * @brief
 * 格式化插入一个字符串到一个「动态字符串」。
 *
 * @param dest
 * 目标「动态字符串」的指针。
 * 如果为空指针，则视为不合法参数。
 * @param index
 * 插入位置的索引。
 * 如果越界，则视为不合法参数。
 * @param format
 * 格式「C 字符串」的指针。
 * 为空指针或指向空「C 字符串」时插入空字符串。
 * @param ...
 * 与格式字符串对应的可变参数列表。
 *
 * @return
 * 全局状态码。
 */
dstr_status_t dstr_insert_format(
	dstr_adt *dest,
	size_t index,
	const char *format,
	...
);

/**
 * @brief
 * 格式化插入一个字符串到一个「动态字符串」（va_list 版本）。
 *
 * @param dest
 * 目标「动态字符串」的指针。
 * 如果为空指针，则视为不合法参数。
 * @param index
 * 插入位置的索引。
 * 如果越界，则视为不合法参数。
 * @param format
 * 格式「C 字符串」的指针。
 * 为空指针或指向空「C 字符串」时插入空字符串。
 * @param args
 * 已通过 va_start 初始化的 va_list，包含与格式字符串对应的可变参数。
 * 该函数不会调用 va_end，调用者需自行管理 va_list 的生命周期。
 *
 * @return
 * 全局状态码。
 */
dstr_status_t dstr_insert_vformat(
	dstr_adt *dest,
	size_t index,
	const char *format,
	va_list args
);

/**
 * @brief
 * 清空一个「动态字符串」。
 *
 * @remark
 * 使其长度为 0，不会立即释放容量。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * 如果为空指针，则函数会直接返回。
 */
void dstr_clear(
	dstr_adt *dstr
);

/**
 * @brief
 * 删除一个「动态字符串」的子串。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * 如果为空指针或指向空「动态字符串」，则函数会直接返回。
 * @param sub_start
 * 子串的起始索引。
 * 如果越界，则函数会直接返回。
 * @param sub_length
 * 子串的长度。
 * 为 0 表示到末尾。
 * 如果越界，则函数会直接返回。
 */
void dstr_remove(
	dstr_adt *dstr,
	size_t sub_start,
	size_t sub_length
);

/**
 * @brief
 * 删除一个「动态字符串」首尾的空白字符或指定字符。
 *
 * @remark
 * 空白字符判定使用 C 标准库函数 isspace()。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * 如果为空指针或指向空「动态字符串」，则函数会直接返回。
 * @param trim_chars
 * 指定字符集合的「C 字符串」的指针。
 * 为空指针或指向空「C 字符串」时删除空白字符。
 */
void dstr_trim(
	dstr_adt *dstr,
	const char *trim_chars
);

/* 关系判断与比较。 */

/**
 * @brief
 * 判断一个「动态字符串」是否以指定「C 字符串」前缀开头。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * 如果为空指针或指向空「动态字符串」，则函数会直接返回 false。
 * @param prefix
 * 前缀「C 字符串」的指针。
 * 如果为空指针或指向空「C 字符串」，则函数会直接返回 false。
 *
 * @return
 * 如果目标「动态字符串」以指定「C 字符串」前缀开头则返回 true，否则返回 false。
 */
bool dstr_starts_with_cstr(
	const dstr_adt *dstr,
	const char *prefix
);

/**
 * @brief
 * 判断一个「动态字符串」是否以指定「动态字符串」前缀开头。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * 如果为空指针或指向空「动态字符串」，则函数会直接返回 false。
 * @param prefix
 * 前缀「动态字符串」的指针。
 * 如果为空指针或指向空「动态字符串」，则函数会直接返回 false。
 *
 * @return
 * 如果目标「动态字符串」以指定「动态字符串」前缀开头则返回 true，否则返回 false。
 */
bool dstr_starts_with(
	const dstr_adt *dstr,
	const dstr_adt *prefix
);

/**
 * @brief
 * 判断一个「动态字符串」是否以指定「C 字符串」后缀结尾。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * 如果为空指针或指向空「动态字符串」，则函数会直接返回 false。
 * @param suffix
 * 后缀「C 字符串」的指针。
 * 如果为空指针或指向空「C 字符串」，则函数会直接返回 false。
 *
 * @return
 * 如果目标「动态字符串」以指定「C 字符串」后缀结尾则返回 true，否则返回 false。
 */
bool dstr_ends_with_cstr(
	const dstr_adt *dstr,
	const char *suffix
);

/**
 * @brief
 * 判断一个「动态字符串」是否以指定「动态字符串」后缀结尾。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * 如果为空指针或指向空「动态字符串」，则函数会直接返回 false。
 * @param suffix
 * 后缀「动态字符串」的指针。
 * 如果为空指针或指向空「动态字符串」，则函数会直接返回 false。
 *
 * @return
 * 如果目标「动态字符串」以指定「动态字符串」后缀结尾则返回 true，否则返回 false。
 */
bool dstr_ends_with(
	const dstr_adt *dstr,
	const dstr_adt *suffix
);

/**
 * @brief
 * 判断一个「动态字符串」是否包含指定子「C 字符串」。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * 如果为空指针或指向空「动态字符串」，则函数会直接返回 false。
 * @param sub
 * 子「C 字符串」的指针。
 * 如果为空指针或指向空「C 字符串」，则函数会直接返回 false。
 *
 * @return
 * 包含则返回 true，否则返回 false。
 */
bool dstr_contains_cstr(
	const dstr_adt *dstr,
	const char *sub
);

/**
 * @brief
 * 判断一个「动态字符串」是否包含指定子「动态字符串」。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * 如果为空指针或指向空「动态字符串」，则函数会直接返回 false。
 * @param sub
 * 子「动态字符串」的指针。
 * 如果为空指针或指向空「动态字符串」，则函数会直接返回 false。
 *
 * @return
 * 包含则返回 true，否则返回 false。
 */
bool dstr_contains(
	const dstr_adt *dstr,
	const dstr_adt *sub
);

/**
 * @brief
 * 判断一个「动态字符串」是否与一个「C 字符串」相等。
 *
 * @param lhs
 * 「动态字符串」的指针。
 * 如果为空指针，则视其为「空字符串」。
 * @param rhs
 * 「C 字符串」的指针。
 * 如果为空指针，则视其为「空字符串」。
 *
 * @return
 * 相等则返回 true，否则返回 false。
 * 两者同为「空字符串」时，返回 true。
 */
bool dstr_equals_cstr(
	const dstr_adt *lhs,
	const char *rhs
);

/**
 * @brief
 * 判断两个「动态字符串」是否相等。
 *
 * @param lhs
 * 第一个「动态字符串」的指针。
 * 如果为空指针，则视其为「空字符串」。
 * @param rhs
 * 第二个「动态字符串」的指针。
 * 如果为空指针，则视其为「空字符串」。
 *
 * @return
 * 相等则返回 true，否则返回 false。
 * 两者同为「空字符串」时，返回 true。
 */
bool dstr_equals(
	const dstr_adt *lhs,
	const dstr_adt *rhs
);

/**
 * @brief
 * 比较一个「动态字符串」与一个「C 字符串」。
 *
 * @param lhs
 * 「动态字符串」的指针。
 * 如果为空指针，则视其为「空字符串」。
 * @param rhs
 * 「C 字符串」的指针。
 * 如果为空指针，则视其为「空字符串」。
 *
 * @return
 * 两者相等返回 0，前者大于后者返回正值，前者小于后者返回负值。
 * 两者同为「空字符串」时，返回 0。
 * 两者只有一者为「空字符串」时，为「空字符串」者小于非「空字符串」者。
 */
int dstr_compare_cstr(
	const dstr_adt *lhs,
	const char *rhs
);

/**
 * @brief
 * 比较两个「动态字符串」。
 *
 * @param lhs
 * 第一个「动态字符串」的指针。
 * 如果为空指针，则视其为「空字符串」。
 * @param rhs
 * 第二个「动态字符串」的指针。
 * 如果为空指针，则视其为「空字符串」。
 *
 * @return
 * 两者相等返回 0，前者大于后者返回正值，前者小于后者返回负值。
 * 两者同为「空字符串」时，返回 0。
 * 两者只有一者为「空字符串」时，为「空字符串」者小于非「空字符串」者。
 */
int dstr_compare(
	const dstr_adt *lhs,
	const dstr_adt *rhs
);

/* 查找、统计与替换。 */

/**
 * @brief
 * 查找一个「动态字符串」中指定子「C 字符串」首次出现的位置。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * 如果为空指针或指向空「动态字符串」，则函数会直接返回 false。
 * @param sub
 * 子「C 字符串」的指针。
 * 如果为空指针或指向空「C 字符串」，则函数会直接返回 false。
 * @param out_index
 * 存储查找结果（位置索引）的 size_t 变量的指针。
 * 为空指针时不写入。
 * @param direction
 * 查找方向。
 *
 * @return
 * 找到则返回 true，否则返回 false。
 */
bool dstr_find_cstr(
	const dstr_adt *dstr,
	const char *sub,
	size_t *out_index,
	dstr_direction_t direction
);

/**
 * @brief
 * 查找一个「动态字符串」中指定子「动态字符串」首次出现的位置。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * 如果为空指针或指向空「动态字符串」，则函数会直接返回 false。
 * @param sub
 * 子「动态字符串」的指针。
 * 如果为空指针或指向空「动态字符串」，则函数会直接返回 false。
 * @param out_index
 * 存储查找结果（位置索引）的 size_t 变量的指针。
 * 为空指针时不写入。
 * @param direction
 * 查找方向。
 *
 * @return
 * 找到则返回 true，否则返回 false。
 */
bool dstr_find(
	const dstr_adt *dstr,
	const dstr_adt *sub,
	size_t *out_index,
	dstr_direction_t direction
);

/**
 * @brief
 * 查找一个「动态字符串」中指定子「C 字符串」第 n 次出现的位置。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * 如果为空指针或指向空「动态字符串」，则函数会直接返回 false。
 * @param sub
 * 子「C 字符串」的指针。
 * 如果为空指针或指向空「C 字符串」，则函数会直接返回 false。
 * @param out_index
 * 存储查找结果（位置索引）的 size_t 变量的指针。
 * 为空指针时不写入。
 * @param direction
 * 查找方向。
 * @param n
 * 出现的次序。
 * 从 1 开始。
 * 为 0 表示最后一次。
 * 如果大于实际出现次数，则视为最后一次。
 *
 * @return
 * 找到则返回 true，否则返回 false。
 */
bool dstr_find_nth_cstr(
	const dstr_adt *dstr,
	const char *sub,
	size_t *out_index,
	dstr_direction_t direction,
	size_t n
);

/**
 * @brief
 * 查找一个「动态字符串」中指定子「动态字符串」第 n 次出现的位置。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * 如果为空指针或指向空「动态字符串」，则函数会直接返回 false。
 * @param sub
 * 子「动态字符串」的指针。
 * 如果为空指针或指向空「动态字符串」，则函数会直接返回 false。
 * @param out_index
 * 存储查找结果（位置索引）的 size_t 变量的指针。
 * 为空指针时不写入。
 * @param direction
 * 查找方向。
 * @param n
 * 出现的次序。
 * 从 1 开始。
 * 为 0 表示最后一次。
 * 如果大于实际出现次数，则视为最后一次。
 *
 * @return
 * 找到则返回 true，否则返回 false。
 */
bool dstr_find_nth(
	const dstr_adt *dstr,
	const dstr_adt *sub,
	size_t *out_index,
	dstr_direction_t direction,
	size_t n
);

/**
 * @brief
 * 查找一个「动态字符串」中指定子「C 字符串」前 n 次出现的位置。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * 如果为空指针或指向空「动态字符串」，则函数会直接返回 0。
 * @param sub
 * 子「C 字符串」的指针。
 * 如果为空指针或指向空「C 字符串」，则函数会直接返回 0。
 * @param out_indexes
 * 存储查找结果（位置索引）的 size_t 数组的指针。
 * 为空指针时不写入。
 * 请自行确保数组容量够大。
 * @param direction
 * 查找方向。
 * @param n
 * 查找的次数。
 * 从 1 开始。
 * 为 0 表示查找全部。
 * 如果大于实际出现次数，也会查找全部。
 *
 * @return
 * 截止第 n 次，实际出现的次数，
 * 也表示实际向 out_indexes 数组中写入的元素个数。
 */
size_t dstr_find_indexes_cstr(
	const dstr_adt *dstr,
	const char *sub,
	size_t *out_indexes,
	dstr_direction_t direction,
	size_t n
);

/**
 * @brief
 * 查找一个「动态字符串」中指定子「动态字符串」前 n 次出现的位置。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * 如果为空指针或指向空「动态字符串」，则函数会直接返回 0。
 * @param sub
 * 子「动态字符串」的指针。
 * 如果为空指针或指向空「动态字符串」，则函数会直接返回 0。
 * @param out_indexes
 * 存储查找结果（位置索引）的 size_t 数组的指针。
 * 为空指针时不写入。
 * 请自行确保数组容量够大。
 * @param direction
 * 查找方向。
 * @param n
 * 查找的次数。
 * 从 1 开始。
 * 为 0 表示查找全部。
 * 如果大于实际出现次数，也会查找全部。
 *
 * @return
 * 截止第 n 次，实际出现的次数，
 * 也表示实际向 out_indexes 数组中写入的元素个数。
 */
size_t dstr_find_indexes(
	const dstr_adt *dstr,
	const dstr_adt *sub,
	size_t *out_indexes,
	dstr_direction_t direction,
	size_t n
);

/**
 * @brief
 * 统计一个「动态字符串」中指定子「C 字符串」出现的次数。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * 如果为空指针或指向空「动态字符串」，则函数会直接返回 0。
 * @param sub
 * 子「C 字符串」的指针。
 * 如果为空指针或指向空「C 字符串」，则函数会直接返回 0。
 *
 * @return
 * 出现的次数。
 */
size_t dstr_count_cstr(
	const dstr_adt *dstr,
	const char *sub
);

/**
 * @brief
 * 统计一个「动态字符串」中指定子「动态字符串」出现的次数。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * 如果为空指针或指向空「动态字符串」，则函数会直接返回 0。
 * @param sub
 * 子「动态字符串」的指针。
 * 如果为空指针或指向空「动态字符串」，则函数会直接返回 0。
 *
 * @return
 * 出现的次数。
 */
size_t dstr_count(
	const dstr_adt *dstr,
	const dstr_adt *sub
);

/**
 * @brief
 * 替换一个「动态字符串」中指定旧「C 字符串」为指定新「C 字符串」n 次。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * 如果为空指针或指向空「动态字符串」，则视为不合法参数。
 * @param old_str
 * 旧「C 字符串」的指针。
 * 如果为空指针或指向空「C 字符串」，则视为不合法参数。
 * 如果在 dstr 中一次都没有出现或出现次数不足 n 次（n 不为 0 时），则视为不合法参数。
 * @param new_str
 * 新「C 字符串」的指针。
 * 为空指针或指向空「C 字符串」时，替换为空。
 * @param direction
 * 替换方向。
 * @param n
 * 替换的次数。
 * 为 0 表示替换所有。
 * 如果大于旧「C 字符串」实际出现的次数，则视为不合法参数，将一次替换都不进行。
 *
 * @return
 * 全局状态码。
 */
dstr_status_t dstr_replace_cstr(
	dstr_adt *dstr,
	const char *old_str,
	const char *new_str,
	dstr_direction_t direction,
	size_t n
);

/**
 * @brief
 * 替换一个「动态字符串」中指定旧「动态字符串」为指定新「动态字符串」n 次。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * 如果为空指针或指向空「动态字符串」，则视为不合法参数。
 * @param old_str
 * 旧「动态字符串」的指针。
 * 如果为空指针或指向空「动态字符串」，则视为不合法参数。
 * 如果在 dstr 中一次都没有出现或出现次数不足 n 次（n 不为 0 时），则视为不合法参数。
 * @param new_str
 * 新「动态字符串」的指针。
 * 为空指针或指向空「动态字符串」时，替换为空。
 * @param direction
 * 替换方向。
 * @param n
 * 替换的次数。
 * 为 0 表示替换所有。
 * 如果大于旧「动态字符串」实际出现的次数，则视为不合法参数，将一次替换都不进行。
 *
 * @return
 * 全局状态码。
 */
dstr_status_t dstr_replace(
	dstr_adt *dstr,
	const dstr_adt *old_str,
	const dstr_adt *new_str,
	dstr_direction_t direction,
	size_t n
);

/* 分隔与合并。 */

/**
 * @brief
 * 分隔一个「C 字符串」为多个「动态字符串」。
 *
 * @remark
 * 分隔时，如果两个分隔符之间，或分隔符与首尾边界之间的子串长度为 0，
 * 将以空指针（而不是空「动态字符串」）形式存储在数组中，而不是跳过。
 *
 * @attention
 * 返回值指向堆内存，其中又可能有指向其他堆内存的指针，
 * 因此，释放时，请先手动依次调用 dstr_destroy 释放每个元素，
 * 然后手动调用 free 释放数组本身。
 *
 * @param cstr
 * 目标「C 字符串」的指针。
 * 如果为空指针或指向空「C 字符串」，则函数会直接返回空指针。
 * @param separator
 * 分隔「C 字符串」的指针。
 * 如果为空指针或指向空「C 字符串」，则函数会直接返回空指针。
 * @param out_dstr_count
 * 存储分隔后的「动态字符串」的个数的 size_t 变量的指针。
 * 如果为空指针，则函数会直接返回空指针。
 *
 * @return
 * 分隔后的「动态字符串」数组的指针。
 * 如果分隔失败则返回空指针。
 */
dstr_adt **dstr_split_cstr(
	const char *cstr,
	const char *separator,
	size_t *out_dstr_count
);

/**
 * @brief
 * 分隔一个「动态字符串」为多个「动态字符串」。
 *
 * @remark
 * 分隔时，如果两个分隔符之间，或分隔符与首尾边界之间的子串长度为 0，
 * 将以空指针（而不是空「动态字符串」）形式存储在数组中，而不是跳过。
 *
 * @attention
 * 返回值指向堆内存，其中又可能有指向其他堆内存的指针，
 * 因此，释放时，请先手动依次调用 dstr_destroy 释放每个元素，
 * 然后手动调用 free 释放数组本身。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * 如果为空指针或指向空「动态字符串」，则函数会直接返回空指针。
 * @param separator
 * 分隔「动态字符串」的指针。
 * 如果为空指针或指向空「动态字符串」，则函数会直接返回空指针。
 * @param out_dstr_count
 * 存储分隔后的「动态字符串」的个数的 size_t 变量的指针。
 * 如果为空指针，则函数会直接返回空指针。
 *
 * @return
 * 分隔后的「动态字符串」数组的指针。
 * 如果分隔失败则返回空指针。
 */
dstr_adt **dstr_split(
	const dstr_adt *dstr,
	const dstr_adt *separator,
	size_t *out_dstr_count
);

/**
 * @brief
 * 合并多个「C 字符串」为一个「动态字符串」。
 *
 * @remark
 * 合并时，其中的空指针或空「C 字符串」会以空字符串形式被合并，而不是被跳过。
 *
 * @attention
 * 返回值指向堆内存，请手动调用 dstr_destroy 释放。
 *
 * @param cstrs
 * 源「C 字符串」数组的指针。
 * 如果为空指针，则函数会直接返回空指针。
 * @param cstr_count
 * 源「C 字符串」的个数。
 * 如果为 0，则函数会直接返回空指针。
 * @param separator
 * 分隔「C 字符串」的指针。
 * 为空指针或指向空「C 字符串」时使用空字符串合并。
 *
 * @return
 * 合并后的「动态字符串」的指针。
 * 如果合并失败则返回空指针。
 */
dstr_adt *dstr_join_cstr(
	const char *const *cstrs,
	size_t cstr_count,
	const char *separator
);

/**
 * @brief
 * 合并多个「动态字符串」为一个「动态字符串」。
 *
 * @remark
 * 合并时，其中的空指针或空「动态字符串」会以空字符串形式被合并，而不是被跳过。
 *
 * @attention
 * 返回值指向堆内存，请手动调用 dstr_destroy 释放。
 *
 * @param dstrs
 * 源「动态字符串」数组的指针。
 * 如果为空指针，则函数会直接返回空指针。
 * @param dstr_count
 * 源「动态字符串」的个数。
 * 如果为 0，则函数会直接返回空指针。
 * @param separator
 * 分隔「动态字符串」的指针。
 * 为空指针或指向空「动态字符串」时使用空字符串合并。
 *
 * @return
 * 合并后的「动态字符串」的指针。
 * 如果合并失败则返回空指针。
 */
dstr_adt *dstr_join(
	const dstr_adt *const *dstrs,
	size_t dstr_count,
	const dstr_adt *separator
);


#endif /* DYNAMIC_STRING_H */
