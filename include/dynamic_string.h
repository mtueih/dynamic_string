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
 * include/dynamic_string.h
 *============================================================================*/
#ifndef DYNAMIC_STRING_H
#define DYNAMIC_STRING_H


/*------------------------------------------------------------------------------
 * 头文件包含
 *----------------------------------------------------------------------------*/
#include <errno.h>
#include <stddef.h>

/* C23 标准移除了 stdbool.h，因此仅在 C23 以下标准时包含此文件。 */
#if !defined(__STDC_VERSION__) || \
    (defined(__STDC_VERSION__) && __STDC_VERSION__ < 202311L)
#  include <stdbool.h>
#endif


/*------------------------------------------------------------------------------
 * ADT 类型别名声明
 *----------------------------------------------------------------------------*/
typedef struct dynamic_string dstr_adt;


/*------------------------------------------------------------------------------
 * 全局状态码
 *----------------------------------------------------------------------------*/
typedef enum {
	DSTR_SUCCESS = 0,         /* 成功。 */
	DSTR_MEMORY_ALLOC_FAILED, /* 内存分配失败。 */
	DSTR_INVALID_ARGUMENT,    /* 无效参数。 */
	DSTR_UNKNOWN_ERROR,       /* 未知错误。 */
} dstr_status_t;


/*------------------------------------------------------------------------------
 * 接口函数原型（声明）
 *----------------------------------------------------------------------------*/

/* 创建与销毁。 */

/**
 * @brief 创建一个「动态字符串」。
 *
 * @param cstr 用来初始化目标「动态字符串」的「C 字符串」的指针。
 *             为空指针或指向空「C 字符串」时创建空「动态字符串」。
 *
 * @return 所创建的「动态字符串」的指针。
 *         如果创建失败则返回空指针。
 */
dstr_adt *dstr_create(
	const char *cstr
);

/**
 * @brief 销毁一个「动态字符串」。
 *
 * @param dstr 目标「动态字符串」的指针。
 *
 * @return 参数有误时会直接返回。
 */
void dstr_destroy(
	dstr_adt *dstr
);

/**
 * @brief 克隆一个「动态字符串」。
 *
 * @param dstr 目标「动态字符串」的指针。
 *
 * @return 所创建的「动态字符串」的指针。
 *         如果创建失败则返回空指针。
 */
dstr_adt *dstr_clone(
	const dstr_adt *dstr
);

/* 属性获取与设置。 */

/**
 * @brief 获取一个「动态字符串」的内部「C 字符串」指针（非 const）。
 *
 * @param dstr 目标「动态字符串」的指针。
 *
 * @return 所获取的「C 字符串」指针。
 *         参数有误时返回空指针。
 */
char *dstr_cstr(
	dstr_adt *dstr
);

/**
 * @brief 获取一个「动态字符串」的内部「C 字符串」指针（const）。
 *
 * @param dstr 目标「动态字符串」的指针。
 *
 * @return 所获取的「C 字符串」指针。
 *         参数有误时返回空指针。
 */
const char *dstr_cstr_const(
	const dstr_adt *dstr
);

/**
 * @brief 获取一个「动态字符串」的长度。
 *
 * @param dstr 目标「动态字符串」的指针。
 *
 * @return 所获取的长度。
 *         参数有误时返回 0。
 */
size_t dstr_length(
	const dstr_adt *dstr
);

/**
 * @brief 获取一个「动态字符串」的容量。
 *
 * @param dstr 目标「动态字符串」的指针。
 *
 * @return 所获取的容量。
 *         参数有误时返回 0。
 */
size_t dstr_capacity(
	const dstr_adt *dstr
);

/**
 * @brief 设置一个「动态字符串」的容量。
 *
 * @attention 当目标容量小于当前长度时，当前内容将被截断。
 *
 * @param dstr 目标「动态字符串」的指针。
 * @param new_capacity 新的容量。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_set_capacity(
	dstr_adt *dstr,
	size_t new_capacity
);

/**
 * @brief 调整一个「动态字符串」的容量到刚合适。
 *
 * @param dstr 目标「动态字符串」的指针。
 *
 * @return 参数有误时会直接返回。
 */
void dstr_shrink_to_fit(
	dstr_adt *dstr
);

/* 内容编辑。 */

/**
 * @brief 复制一个「C 字符串」到一个「动态字符串」。
 *
 * @param dest 目标「动态字符串」的指针。
 * @param src 源「C 字符串」的指针。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cpy_cstr(
	dstr_adt *dest,
	const char *src
);

/**
 * @brief 复制一个「动态字符串」到另一个「动态字符串」。
 *
 * @param dest 目标「动态字符串」的指针。
 * @param src 源「动态字符串」的指针。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cpy(
	dstr_adt *dest,
	const dstr_adt *src
);

/**
 * @brief 复制一个「C 字符串」的子串到一个「动态字符串」。
 *
 * @param dest 目标「动态字符串」的指针。
 * @param src 源「C 字符串」的指针。
 * @param sub_index 子串的起始索引。
 * @param sub_count 子串的长度。
 *                  为 0 表示到末尾。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cpy_sub_cstr(
	dstr_adt *dest,
	const char *src,
	size_t sub_index,
	size_t sub_count
);

/**
 * @brief 复制一个「动态字符串」的子串到另一个「动态字符串」。
 *
 * @param dest 目标「动态字符串」的指针。
 * @param src 源「动态字符串」的指针。
 * @param sub_index 子串的起始索引。
 * @param sub_count 子串的长度。
 *                  为 0 表示到末尾。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cpy_sub(
	dstr_adt *dest,
	const dstr_adt *src,
	size_t sub_index,
	size_t sub_count
);

/**
 * @brief 追加一个「C 字符串」到一个「动态字符串」。
 *
 * @param dest 目标「动态字符串」的指针。
 * @param src 源「C 字符串」的指针。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cat_cstr(
	dstr_adt *dest,
	const char *src
);

/**
 * @brief 追加一个「动态字符串」到另一个「动态字符串」。
 *
 * @param dest 目标「动态字符串」的指针。
 * @param src 源「动态字符串」的指针。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cat(
	dstr_adt *dest,
	const dstr_adt *src
);

/**
 * @brief 追加一个「C 字符串」的子串到一个「动态字符串」。
 *
 * @param dest 目标「动态字符串」的指针。
 * @param src 源「C 字符串」的指针。
 * @param sub_index 子串的起始索引。
 * @param sub_count 子串的长度。
 *                  为 0 表示到末尾。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cat_sub_cstr(
	dstr_adt *dest,
	const char *src,
	size_t sub_index,
	size_t sub_count
);

/**
 * @brief 追加一个「动态字符串」的子串到另一个「动态字符串」。
 *
 * @param dest 目标「动态字符串」的指针。
 * @param src 源「动态字符串」的指针。
 * @param sub_index 子串的起始索引。
 * @param sub_count 子串的长度。
 *                  为 0 表示到末尾。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_cat_sub(
	dstr_adt *dest,
	const dstr_adt *src,
	size_t sub_index,
	size_t sub_count
);

/**
 * @brief 插入一个「C 字符串」到一个「动态字符串」。
 *
 * @param dest 目标「动态字符串」的指针。
 * @param index 插入位置的索引。
 * @param src 源「C 字符串」的指针。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_insert_cstr(
	dstr_adt *dest,
	size_t index,
	const char *src
);

/**
 * @brief 插入一个「动态字符串」到另一个「动态字符串」。
 *
 * @param dest 目标「动态字符串」的指针。
 * @param index 插入位置的索引。
 * @param src 源「动态字符串」的指针。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_insert(
	dstr_adt *dest,
	size_t index,
	const dstr_adt *src
);

/**
 * @brief 插入一个「C 字符串」的子串到一个「动态字符串」。
 *
 * @param dest 目标「动态字符串」的指针。
 * @param index 插入位置的索引。
 * @param src 源「C 字符串」的指针。
 * @param sub_index 子串的起始索引。
 * @param sub_count 子串的长度。
 *                  为 0 表示到末尾。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_insert_sub_cstr(
	dstr_adt *dest,
	size_t index,
	const char *src,
	size_t sub_index,
	size_t sub_count
);

/**
 * @brief 插入一个「动态字符串」的子串到另一个「动态字符串」。
 *
 * @param dest 目标「动态字符串」的指针。
 * @param index 插入位置的索引。
 * @param src 源「动态字符串」的指针。
 * @param sub_index 子串的起始索引。
 * @param sub_count 子串的长度。
 *                  为 0 表示到末尾。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_insert_sub(
	dstr_adt *dest,
	size_t index,
	const dstr_adt *src,
	size_t sub_index,
	size_t sub_count
);

/**
 * @brief 清空一个「动态字符串」。
 *        使其长度为 0，不会立即释放内存。
 *
 * @param dstr 目标「动态字符串」的指针。
 *
 * @return 参数有误时会直接返回。
 */
void dstr_clear(
	dstr_adt *dstr
);

/**
 * @brief 删除一个「动态字符串」的子串。
 *
 * @param dstr 目标「动态字符串」的指针。
 * @param index 子串的起始索引。
 * @param count 子串的长度。
 *              为 0 表示到末尾。
 *
 * @return 参数有误时会直接返回。
 */
void dstr_remove(
	dstr_adt *dstr,
	size_t index,
	size_t count
);

/**
 * @brief 删除一个「动态字符串」首尾的空白字符或指定字符。
 *
 * @param dstr 目标「动态字符串」的指针。
 * @param trim_chars 指定字符集合的「C 字符串」的指针。
 *                   为空指针或指向空「C 字符串」时删除空白字符。
 *
 * @return 参数有误时会直接返回。
 */
void dstr_trim(
	dstr_adt *dstr,
	const char *trim_chars
);

/**
 * @brief 格式化写入字符串到一个「动态字符串」。
 *
 * @param dstr 目标「动态字符串」的指针。
 * @param format 格式「C 字符串」的指针。
 * @param ... 可变参数列表。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_printf(
	dstr_adt *dstr,
	const char *format,
	...
);

/* 关系判断与比较。 */

/**
 * @brief 判断一个「动态字符串」是否以指定「C 字符串」前缀开头。
 *
 * @param dstr 目标「动态字符串」的指针。
 * @param prefix 前缀「C 字符串」的指针。
 *
 * @return 如果目标「动态字符串」以指定「C 字符串」前缀开头则返回 true，否则返回 false。
 *         参数有误时返回 false。
 */
bool dstr_starts_with_cstr(
	const dstr_adt *dstr,
	const char *prefix
);

/**
 * @brief 判断一个「动态字符串」是否以指定「动态字符串」前缀开头。
 *
 * @param dstr 目标「动态字符串」的指针。
 * @param prefix 前缀「动态字符串」的指针。
 *
 * @return 如果目标「动态字符串」以指定「动态字符串」前缀开头则返回 true，否则返回 false。
 *         参数有误时返回 false。
 */
bool dstr_starts_with(
	const dstr_adt *dstr,
	const dstr_adt *prefix
);

/**
 * @brief 判断一个「动态字符串」是否以指定「C 字符串」后缀结尾。
 *
 * @param dstr 目标「动态字符串」的指针。
 * @param suffix 后缀「C 字符串」的指针。
 *
 * @return 如果目标「动态字符串」以指定「C 字符串」后缀结尾则返回 true，否则返回 false。
 *         参数有误时返回 false。
 */
bool dstr_ends_with_cstr(
	const dstr_adt *dstr,
	const char *suffix
);

/**
 * @brief 判断一个「动态字符串」是否以指定「动态字符串」后缀结尾。
 *
 * @param dstr 目标「动态字符串」的指针。
 * @param suffix 后缀「动态字符串」的指针。
 *
 * @return 如果目标「动态字符串」以指定「动态字符串」后缀结尾则返回 true，否则返回 false。
 *         参数有误时返回 false。
 */
bool dstr_ends_with(
	const dstr_adt *dstr,
	const dstr_adt *suffix
);

/**
 * @brief 判断一个「动态字符串」是否包含指定子「C 字符串」。
 *
 * @param dstr 目标「动态字符串」的指针。
 * @param sub 子「C 字符串」的指针。
 *
 * @return 包含则返回 true，否则返回 false。
 *         参数有误时返回 false。
 */
bool dstr_contains_cstr(
	const dstr_adt *dstr,
	const char *sub
);

/**
 * @brief 判断一个「动态字符串」是否包含指定子「动态字符串」。
 *
 * @param dstr 目标「动态字符串」的指针。
 * @param sub 子「动态字符串」的指针。
 *
 * @return 包含则返回 true，否则返回 false。
 *         参数有误时返回 false。
 */
bool dstr_contains(
	const dstr_adt *dstr,
	const dstr_adt *sub
);

/**
 * @brief 判断一个「动态字符串」是否与一个「C 字符串」相等。
 *
 * @param dstr 「动态字符串」的指针。
 * @param cstr 「C 字符串」的指针。
 *
 * @return 相等则返回 true，否则返回 false。
 *         参数有误时返回 false。
 */
bool dstr_equals_cstr(
	const dstr_adt *dstr,
	const char *cstr
);

/**
 * @brief 判断两个「动态字符串」是否相等。
 *
 * @param dstr_1 第一个「动态字符串」的指针。
 * @param dstr_2 第二个「动态字符串」的指针。
 *
 * @return 相等则返回 true，否则返回 false。
 *         参数有误时返回 false。
 */
bool dstr_equals(
	const dstr_adt *dstr_1,
	const dstr_adt *dstr_2
);

/**
 * @brief 比较一个「动态字符串」与一个「C 字符串」。
 *
 * @param dstr 「动态字符串」的指针。
 * @param cstr 「C 字符串」的指针。
 *
 * @return 两者相等返回 0，前者大于后者返回正值，前者小于后者返回负值。
 *         参数有误时返回非 0 值。
 */
int dstr_compare_cstr(
	const dstr_adt *dstr,
	const char *cstr
);

/**
 * @brief 比较两个「动态字符串」。
 *
 * @param dstr_1 第一个「动态字符串」的指针。
 * @param dstr_2 第二个「动态字符串」的指针。
 *
 * @return 两者相等返回 0，前者大于后者返回正值，前者小于后者返回负值。
 *         参数有误时返回非 0 值。
 */
int dstr_compare(
	const dstr_adt *dstr_1,
	const dstr_adt *dstr_2
);

/* 查找、统计与替换。 */

/**
 * @brief 查找一个「动态字符串」中指定子「C 字符串」首次出现的位置。
 *
 * @param dstr 目标「动态字符串」的指针。
 * @param sub 子「C 字符串」的指针。
 * @param out_index 存储查找结果（位置索引）的 size_t 变量的指针。
 *                  为空指针时不写入。
 * @param backward 是否从后向前查找。
 *
 * @return 找到则返回 true，否则返回 false。
 *         参数有误时返回 false。
 */
bool dstr_find_cstr(
	const dstr_adt *dstr,
	const char *sub,
	size_t *out_index,
	bool backward
);

/**
 * @brief 查找一个「动态字符串」中指定子「动态字符串」首次出现的位置。
 *
 * @param dstr 目标「动态字符串」的指针。
 * @param sub 子「动态字符串」的指针。
 * @param out_index 存储查找结果（位置索引）的 size_t 变量的指针。
 *                  为空指针时不写入。
 * @param backward 是否从后向前查找。
 *
 * @return 找到则返回 true，否则返回 false。
 *         参数有误时返回 false。
 */
bool dstr_find(
	const dstr_adt *dstr,
	const dstr_adt *sub,
	size_t *out_index,
	bool backward
);

/**
 * @brief 查找一个「动态字符串」中指定子「C 字符串」第 n 次出现的位置。
 *
 * @param dstr 目标「动态字符串」的指针。
 * @param sub 子「C 字符串」的指针。
 * @param out_index 存储查找结果（位置索引）的 size_t 变量的指针。
 *                  为空指针时不写入。
 * @param n 出现的次序。
 *          从 1 开始。
 *          为 0 表示最后一次。
 * @param backward 是否从后向前查找。
 *
 * @return 找到则返回 true，否则返回 false。
 *         参数有误时返回 false。
 */
bool dstr_find_nth_cstr(
	const dstr_adt *dstr,
	const char *sub,
	size_t *out_index,
	size_t n,
	bool backward
);

/**
 * @brief 查找一个「动态字符串」中指定子「动态字符串」第 n 次出现的位置。
 *
 * @param dstr 目标「动态字符串」的指针。
 * @param sub 子「动态字符串」的指针。
 * @param out_index 存储查找结果（位置索引）的 size_t 变量的指针。
 *                  为空指针时不写入。
 * @param n 出现的次序。
 *          从 1 开始。
 *          为 0 表示最后一次。
 * @param backward 是否从后向前查找。
 *
 * @return 找到则返回 true，否则返回 false。
 *         参数有误时返回 false。
 */
bool dstr_find_nth(
	const dstr_adt *dstr,
	const dstr_adt *sub,
	size_t *out_index,
	size_t n,
	bool backward
);

/**
 * @brief 统计一个「动态字符串」中指定子「C 字符串」出现的次数。
 *
 * @param dstr 目标「动态字符串」的指针。
 * @param sub 子「C 字符串」的指针。
 * @param backward 是否从后向前查找。
 *
 * @return 出现的次数。
 *         参数有误时返回 0。
 */
size_t dstr_count_cstr(
	const dstr_adt *dstr,
	const char *sub,
	bool backward
);

/**
 * @brief 统计一个「动态字符串」中指定子「动态字符串」出现的次数。
 *
 * @param dstr 目标「动态字符串」的指针。
 * @param sub 子「动态字符串」的指针。
 * @param backward 是否从后向前查找。
 *
 * @return 出现的次数。
 *         参数有误时返回 0。
 */
size_t dstr_count(
	const dstr_adt *dstr,
	const dstr_adt *sub,
	bool backward
);

/**
 * @brief 替换一个「动态字符串」中指定旧「C 字符串」为指定新「C 字符串」n 次。
 *
 * @param dstr 目标「动态字符串」的指针。
 * @param old_str 旧「C 字符串」的指针。
 * @param new_str 新「C 字符串」的指针。
 * @param n 替换的次数。
 *          为 0 表示替换所有。
 * @param backward 是否从后向前替换。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_replace_cstr(
	dstr_adt *dstr,
	const char *old_str,
	const char *new_str,
	size_t n,
	bool backward
);

/**
 * @brief 替换一个「动态字符串」中指定旧「动态字符串」为指定新「动态字符串」n 次。
 *
 * @param dstr 目标「动态字符串」的指针。
 * @param old_str 旧「动态字符串」的指针。
 * @param new_str 新「动态字符串」的指针。
 * @param n 替换的次数。
 *          为 0 表示替换所有。
 * @param backward 是否从后向前替换。
 *
 * @return 全局状态码。
 */
dstr_status_t dstr_replace(
	dstr_adt *dstr,
	const dstr_adt *old_str,
	const dstr_adt *new_str,
	size_t n,
	bool backward
);


#endif /* DYNAMIC_STRING_H */
