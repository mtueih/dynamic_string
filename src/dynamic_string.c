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
 * include/dynamic_string.c
 *============================================================================*/


/*------------------------------------------------------------------------------
 * 头文件包含
 *----------------------------------------------------------------------------*/
#include "dynamic_string.h"

#include <ctype.h>
#include <safe_calc.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*------------------------------------------------------------------------------
 * 宏定义
 *----------------------------------------------------------------------------*/

/**
 * C23 标准引入了 nullptr 关键字，因此条件定义一个宏，
 * 在 C23 及以上标准时将宏定义为 nullptr，否则定义为 NULL。
 */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#  define DSTR_NULLPTR nullptr
#else
#  define DSTR_NULLPTR NULL
#endif

/* 取两数最大值。 */
#define DSTR_MAX(a, b) ((a) > (b) ? (a) : (b))

/* 缓存行大小。 */
#define DSTR_CACHELINE_SIZE 64


/*------------------------------------------------------------------------------
 * ADT 类型定义
 *----------------------------------------------------------------------------*/
struct dynamic_string {
	char *data;     /* 指向数据缓冲区的指针。 */
	size_t len;     /* 字符串的长度。 */
	size_t cap;     /* 「动态字符串」的容量。 */
	size_t min_cap; /* 「动态字符串」的保底容量。 */
};


/*------------------------------------------------------------------------------
 * 静态函数声明
 *----------------------------------------------------------------------------*/

/**
 * @brief 调整一个「动态字符串」的容量（基础版）。
 *        只是简单地封装了 realloc 操作。
 *        不做 new_cap 为 0 ，以及是否与原容量相等的检查。
 *        适用于确定所需容量不等于原容量，且不为 0 的情况。
 *
 * @param dstr 目标「动态字符串」的指针。
 * @param new_cap 新的容量。
 *
 * @return 调整成功返回 true，否则返回 false。
 */
static bool capacity_resize(
	dstr_adt *dstr,
	size_t new_cap
);

/**
 * @brief 调整一个「动态字符串」的容量（常规版）。
 *        在基础版的基础上增加对 new_cap 为 0 ，以及是否与原容量相等的检查。
 *        适用于不能确定所需容量是否不等于原容量、是否不为 0 的情况。
 *
 * @param dstr 目标「动态字符串」的指针。
 * @param new_cap 新的容量。
 *
 * @return 调整成功返回 true，否则返回 false。
 */
static bool capacity_resize_regular(
	dstr_adt *dstr,
	size_t new_cap
);

/**
 * @brief 调整一个「动态字符串」的容量（动态版）。
 *        在基础班的基础上增加对 new_cap 为 0 ，以及是否与原容量相等的检查。
 *        会确保容量不会低于目标「动态字符串」的容量保底值。
 *        会执行预分配、延迟减容、缓存行对齐等性能优化策略。
 *        所有由长度变化引起的容量调整都应该且只能使用此函数。
 *
 * @param dstr 目标「动态字符串」的指针。
 * @param new_cap 新的容量。
 *
 * @return 调整成功返回 true，否则返回 false。
 *         “调整成功”只保证调整后的容量不低于 new_cap，不保证预分配等策略一定生效。
 */
static bool capacity_resize_dynamic(
	dstr_adt *dstr,
	size_t new_cap
);

/**
 * @brief 从一个「C 字符串」创建一个「动态字符串」。
 *
 * @param src 源「C 字符串」的指针。
 * @param src_len 源「C 字符串」的长度。
 *                为 0 时，创建空「动态字符串」。
 * @param sub_index
 * @param sub_count
 *
 * @return 所创建的「动态字符串」的指针。
 *         如果创建失败则返回空指针。
 */
static dstr_adt *create_dstr(
	const char *src,
	size_t src_len,
	size_t sub_index,
	size_t sub_count
);

/**
 * @brief 先删除一个「动态字符串」中的指定位置处开始向后的指定个字符，
 *        然后向该位置插入一个「C 字符串」的子串。
 *
 * @param dest 目标「动态字符串」的指针。
 * @param index 目标位置的索引。
 * @param count 删除个数。
 * @param src 源「C 字符串」的指针。
 * @param src_len 源「C 字符串」的长度。
 * @param sub_index 子串的起始索引。
 * @param sub_count 字串的长度。
 *                  为 0 表示到末尾。
 *
 * @return 全局状态码。
 */
static dstr_status_t insert_str(
	dstr_adt *dest,
	size_t index,
	size_t count,
	const char *src,
	size_t src_len,
	size_t sub_index,
	size_t sub_count
);

/**
 * @brief 格式化写入字符串到「动态字符串」缓冲区的指定位置。
 *        写入前可选择性删除指定数量的字符。
 *
 * @param dstr 目标「动态字符串」的指针。
 * @param index 写入起始位置的索引。
 * @param delete_count 写入前先删除的字符数。
 * @param format 格式「C 字符串」的指针。
 * @param args 可变参数列表，类型为 va_list。
 *
 * @return 全局状态码。
 */
static dstr_status_t format_to_dstr(
	dstr_adt *dstr,
	size_t index,
	size_t delete_count,
	const char *format,
	va_list args
);

/**
 * @brief 查找一个「动态字符串」中，指定子「C 字符串」第 n 次出现的位置，
 *        并返回直到第 n 次，一共出现的次数。
 *
 * @param dstr 目标「动态字符串」的指针。
 * @param sub 子「C 字符串」的指针。
 * @param sub_len 子「C 字符串」的长度。
 * @param out_index 存储查找结果（位置索引）的 size_t 变量的指针。
 *                  为空指针时不写入。
 * @param n 出现的次序。
 *          从 1 开始。
 *          为 0 表示最后一次。
 * @param backward 是否从后向前查找。
 *
 * @return 指定子「C 字符串」截止第 n 次，一共出现的次数。
 */
static size_t find_str(
	const dstr_adt *dstr,
	const char *sub,
	size_t sub_len,
	size_t *out_index,
	size_t n,
	bool backward
);

static dstr_status_t replace_str(
	dstr_adt *dstr,
	const char *old_str,
	size_t old_str_len,
	const char *new_str,
	size_t new_str_len,
	size_t n,
	bool backward
);


/*------------------------------------------------------------------------------
 * 接口函数定义
 *----------------------------------------------------------------------------*/

/* 创建与销毁。 */

/* 创建一个「动态字符串」。 */
dstr_adt *dstr_create(
	const char *const cstr
) {
	if (cstr == DSTR_NULLPTR || cstr[0] == '\0') {
		return create_dstr(DSTR_NULLPTR, 0, 0, 0);
	}

	return create_dstr(cstr, strlen(cstr), 0, 0);
}

/* 销毁一个「动态字符串」。 */
void dstr_destroy(
	dstr_adt *const dstr
) {
	if (dstr == DSTR_NULLPTR) {
		return;
	}

	if (dstr->data != DSTR_NULLPTR) {
		free(dstr->data);
	}

	free(dstr);
}

/* 克隆一个「动态字符串」。 */
dstr_adt *dstr_clone(
	const dstr_adt *const dstr
) {
	if (dstr == DSTR_NULLPTR || dstr->len == 0) {
		return create_dstr(DSTR_NULLPTR, 0, 0, 0);
	}

	return create_dstr(dstr->data, dstr->len, 0, 0);
}

dstr_adt *dstr_sub_cstr(
	const char *const cstr,
	const size_t sub_index,
	const size_t sub_count
) {
	if (cstr == DSTR_NULLPTR || cstr[0] == '\0') {
		return create_dstr(DSTR_NULLPTR, 0, 0, 0);
	}

	const size_t cstr_len = strlen(cstr);
	if (sub_index >= cstr_len ||
		!safe_size_t_add(sub_index, sub_count, DSTR_NULLPTR) ||
		sub_index + sub_count > cstr_len
	) {
		return DSTR_NULLPTR;
	}

	return create_dstr(cstr, cstr_len, sub_index, sub_count);
}

dstr_adt *dstr_sub(
	const dstr_adt *const dstr,
	const size_t sub_index,
	const size_t sub_count
) {
	if (dstr == DSTR_NULLPTR || dstr->len == 0) {
		return create_dstr(DSTR_NULLPTR, 0, 0, 0);
	}

	if (sub_index >= dstr->len ||
		!safe_size_t_add(sub_index, sub_count, DSTR_NULLPTR) ||
		sub_index + sub_count > dstr->len
	) {
		return DSTR_NULLPTR;
	}

	return create_dstr(dstr->data, dstr->len, sub_index, sub_count);
}

dstr_adt *dstr_create_format(
	const char *const format,
	...
) {
	if (format == DSTR_NULLPTR || format[0] == '\0') {
		return create_dstr(DSTR_NULLPTR, 0, 0, 0);
	}

	dstr_adt *const new_dstr = create_dstr(DSTR_NULLPTR, 0, 0, 0);
	if (new_dstr == DSTR_NULLPTR) {
		return DSTR_NULLPTR;
	}

	va_list args;

	va_start(args, format);
	const dstr_status_t rc = format_to_dstr(new_dstr, 0, 0, format, args);
	va_end(args);

	if (rc != DSTR_SUCCESS) {
		free(new_dstr);
		return DSTR_NULLPTR;
	}

	return new_dstr;
}

dstr_adt *dstr_create_vformat(
	const char *const format,
	const va_list args
) {
	if (format == DSTR_NULLPTR || format[0] == '\0') {
		return create_dstr(DSTR_NULLPTR, 0, 0, 0);
	}

	dstr_adt *const new_dstr = create_dstr(DSTR_NULLPTR, 0, 0, 0);
	if (new_dstr == DSTR_NULLPTR) {
		return DSTR_NULLPTR;
	}

	const dstr_status_t rc = format_to_dstr(new_dstr, 0, 0, format, args);
	if (rc != DSTR_SUCCESS) {
		free(new_dstr);
		return DSTR_NULLPTR;
	}

	return new_dstr;
}

/* 属性获取与设置。 */

/* 获取一个「动态字符串」的内部「C 字符串」指针。 */
const char *dstr_cstr(
	const dstr_adt *const dstr
) {
	if (dstr == DSTR_NULLPTR) {
		return DSTR_NULLPTR;
	}

	return dstr->data;
}

/* 获取一个「动态字符串」的长度。 */
size_t dstr_length(
	const dstr_adt *const dstr
) {
	if (dstr == DSTR_NULLPTR) {
		return 0;
	}

	return dstr->len;
}

/* 判断一个「动态字符串」是否是空「动态字符串」。 */
bool dstr_is_empty(
	const dstr_adt *const dstr
) {
	if (dstr == DSTR_NULLPTR) {
		return true;
	}

	return (dstr->len == 0);
}

/* 获取一个「动态字符串」的容量。 */
size_t dstr_capacity(
	const dstr_adt *const dstr
) {
	if (dstr == DSTR_NULLPTR) {
		return 0;
	}

	return dstr->cap;
}

/* 设置一个「动态字符串」的容量。 */
dstr_status_t dstr_set_capacity(
	dstr_adt *const dstr,
	const size_t new_capacity
) {
	if (dstr == DSTR_NULLPTR) {
		return DSTR_INVALID_ARGUMENT;
	}

	if (!capacity_resize_regular(dstr, new_capacity)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	dstr->min_cap = new_capacity;

	if (new_capacity <= dstr->len) {
		if (new_capacity > 0) {
			dstr->data[dstr->len = new_capacity - 1] = '\0';
		} else {
			dstr->len = 0;
		}
	}

	return DSTR_SUCCESS;
}

/* 调整一个「动态字符串」的容量到刚合适。 */
void dstr_shrink_to_fit(
	dstr_adt *const dstr
) {
	if (dstr == DSTR_NULLPTR) {
		return;
	}

	dstr->min_cap = 0;

	capacity_resize_regular(
		dstr,
		(dstr->len > 0) ? (dstr->len + 1) : 0
	);
}

/* 内容编辑。 */

/* 复制一个「C 字符串」到一个「动态字符串」。 */
dstr_status_t dstr_cpy_cstr(
	dstr_adt *const dest,
	const char *const src
) {
	if (dest == DSTR_NULLPTR) {
		return DSTR_INVALID_ARGUMENT;
	}

	/* src 为空指针或指向空字符串，均视为复制空字符串。 */
	if (src == DSTR_NULLPTR || src[0] == '\0') {
		return insert_str(dest, 0, dest->len,DSTR_NULLPTR, 0, 0, 0);
	}

	return insert_str(dest, 0, dest->len, src, strlen(src), 0, 0);
}

/* 复制一个「动态字符串」到另一个「动态字符串」。 */
dstr_status_t dstr_cpy(
	dstr_adt *const dest,
	const dstr_adt *const src
) {
	if (dest == DSTR_NULLPTR) {
		return DSTR_INVALID_ARGUMENT;
	}

	/* src 为空指针或指向空字符串，均视为复制空字符串。 */
	if (src == DSTR_NULLPTR || src->len == 0) {
		return insert_str(dest, 0, dest->len,DSTR_NULLPTR, 0, 0, 0);
	}

	return insert_str(dest, 0, dest->len, src->data, src->len, 0, 0);
}

/* 复制一个「C 字符串」的子串到一个「动态字符串」。 */
dstr_status_t dstr_cpy_sub_cstr(
	dstr_adt *const dest,
	const char *const src,
	const size_t sub_index,
	const size_t sub_count
) {
	if (dest == DSTR_NULLPTR) {
		return DSTR_INVALID_ARGUMENT;
	}

	/* src 为空指针或指向空字符串，均视为复制空字符串。 */
	if (src == DSTR_NULLPTR || src[0] == '\0') {
		return insert_str(dest, 0, dest->len,DSTR_NULLPTR, 0, 0, 0);
	}

	/* 越界检查。 */
	const size_t src_len = strlen(src);
	if (sub_index >= src_len ||
		!safe_size_t_add(sub_index, sub_count, DSTR_NULLPTR) ||
		sub_index + sub_count > src_len
	) {
		return DSTR_INVALID_ARGUMENT;
	}

	return insert_str(dest, 0, dest->len, src, src_len, sub_index, sub_count);
}

/* 复制一个「动态字符串」的子串到另一个「动态字符串」。 */
dstr_status_t dstr_cpy_sub(
	dstr_adt *const dest,
	const dstr_adt *const src,
	const size_t sub_index,
	const size_t sub_count
) {
	if (dest == DSTR_NULLPTR) {
		return DSTR_INVALID_ARGUMENT;
	}

	/* src 为空指针或指向空字符串，均视为复制空字符串。 */
	if (src == DSTR_NULLPTR || src->len == 0) {
		return insert_str(dest, 0, dest->len,DSTR_NULLPTR, 0, 0, 0);
	}

	/* 越界检查。 */
	if (sub_index >= src->len ||
		!safe_size_t_add(sub_index, sub_count, DSTR_NULLPTR) ||
		sub_index + sub_count > src->len
	) {
		return DSTR_INVALID_ARGUMENT;
	}

	return insert_str(dest, 0, dest->len, src->data, src->len, sub_index, sub_count);
}

dstr_status_t dstr_cpy_format(
	dstr_adt *const dstr,
	const char *const format,
	...
) {
	if (dstr == DSTR_NULLPTR) {
		return DSTR_INVALID_ARGUMENT;
	}

	/* format 为空指针或指向空字符串，均视为复制空字符串。 */
	if (format == DSTR_NULLPTR || format[0] == '\0') {
		return insert_str(dstr, 0, dstr->len, DSTR_NULLPTR, 0, 0, 0);
	}

	va_list args;

	va_start(args, format);
	const dstr_status_t result = format_to_dstr(dstr, 0, dstr->len, format, args);
	va_end(args);

	return result;
}

dstr_status_t dstr_cpy_vformat(
	dstr_adt *const dstr,
	const char *const format,
	const va_list args
) {
	if (dstr == DSTR_NULLPTR) {
		return DSTR_INVALID_ARGUMENT;
	}

	/* format 为空指针或指向空字符串，均视为复制空字符串。 */
	if (format == DSTR_NULLPTR || format[0] == '\0') {
		return insert_str(dstr, 0, dstr->len, DSTR_NULLPTR, 0, 0, 0);
	}

	return format_to_dstr(dstr, 0, dstr->len, format, args);
}

/* 追加一个「C 字符串」到一个「动态字符串」。 */
dstr_status_t dstr_cat_cstr(
	dstr_adt *const dest,
	const char *const src
) {
	if (dest == DSTR_NULLPTR) {
		return DSTR_INVALID_ARGUMENT;
	}

	/* src 为空指针或指向空字符串，均视为追加空字符串。 */
	if (src == DSTR_NULLPTR || src[0] == '\0') {
		return DSTR_SUCCESS;
	}

	return insert_str(dest, dest->len, 0, src, strlen(src), 0, 0);
}

/* 追加一个「动态字符串」到另一个「动态字符串」。 */
dstr_status_t dstr_cat(
	dstr_adt *const dest,
	const dstr_adt *const src
) {
	if (dest == DSTR_NULLPTR) {
		return DSTR_INVALID_ARGUMENT;
	}

	/* src 为空指针或指向空字符串，均视为追加空字符串。 */
	if (src == DSTR_NULLPTR || src->len == 0) {
		return DSTR_SUCCESS;
	}

	return insert_str(dest, dest->len, 0, src->data, src->len, 0, 0);
}

/* 追加一个「C 字符串」的子串到一个「动态字符串」。 */
dstr_status_t dstr_cat_sub_cstr(
	dstr_adt *const dest,
	const char *const src,
	const size_t sub_index,
	const size_t sub_count
) {
	if (dest == DSTR_NULLPTR) {
		return DSTR_INVALID_ARGUMENT;
	}

	/* src 为空指针或指向空字符串，均视为追加空字符串。 */
	if (src == DSTR_NULLPTR || src[0] == '\0') {
		return DSTR_SUCCESS;
	}

	/* 越界检查。 */
	const size_t src_len = strlen(src);
	if (sub_index >= src_len ||
		!safe_size_t_add(sub_index, sub_count, DSTR_NULLPTR) ||
		sub_index + sub_count > src_len
	) {
		return DSTR_INVALID_ARGUMENT;
	}

	return insert_str(dest, dest->len, 0, src, src_len, sub_index, sub_count);
}

/* 追加一个「动态字符串」的子串到另一个「动态字符串」。 */
dstr_status_t dstr_cat_sub(
	dstr_adt *const dest,
	const dstr_adt *const src,
	const size_t sub_index,
	const size_t sub_count
) {
	if (dest == DSTR_NULLPTR) {
		return DSTR_INVALID_ARGUMENT;
	}

	/* src 为空指针或指向空字符串，均视为追加空字符串。 */
	if (src == DSTR_NULLPTR || src->len == 0) {
		return DSTR_SUCCESS;
	}

	/* 越界检查。 */
	if (sub_index >= src->len ||
		!safe_size_t_add(sub_index, sub_count, DSTR_NULLPTR) ||
		sub_index + sub_count > src->len
	) {
		return DSTR_INVALID_ARGUMENT;
	}

	return insert_str(dest, dest->len, 0, src->data, src->len, sub_index, sub_count);
}

dstr_status_t dstr_cat_format(
	dstr_adt *const dstr,
	const char *const format,
	...
) {
	if (dstr == DSTR_NULLPTR) {
		return DSTR_INVALID_ARGUMENT;
	}

	/* format 为空指针或指向空字符串，均视为追加空字符串。 */
	if (format == DSTR_NULLPTR || format[0] == '\0') {
		return DSTR_SUCCESS;
	}

	va_list args;

	va_start(args, format);
	const dstr_status_t result = format_to_dstr(dstr, dstr->len, 0, format, args);
	va_end(args);

	return result;
}

dstr_status_t dstr_cat_vformat(
	dstr_adt *const dstr,
	const char *const format,
	const va_list args
) {
	if (dstr == DSTR_NULLPTR) {
		return DSTR_INVALID_ARGUMENT;
	}

	/* format 为空指针或指向空字符串，均视为追加空字符串。 */
	if (format == DSTR_NULLPTR || format[0] == '\0') {
		return DSTR_SUCCESS;
	}

	return format_to_dstr(dstr, dstr->len, 0, format, args);
}

/* 插入一个「C 字符串」到一个「动态字符串」。 */
dstr_status_t dstr_insert_cstr(
	dstr_adt *const dest,
	const size_t index,
	const char *const src
) {
	if (dest == DSTR_NULLPTR || index > dest->len) {
		return DSTR_INVALID_ARGUMENT;
	}

	/* src 为空指针或指向空字符串，均视为插入空字符串。 */
	if (src == DSTR_NULLPTR || src[0] == '\0') {
		return DSTR_SUCCESS;
	}

	return insert_str(dest, index, 0, src, strlen(src), 0, 0);
}

/* 插入一个「动态字符串」到另一个「动态字符串」。 */
dstr_status_t dstr_insert(
	dstr_adt *const dest,
	const size_t index,
	const dstr_adt *const src
) {
	if (dest == DSTR_NULLPTR || index > dest->len) {
		return DSTR_INVALID_ARGUMENT;
	}

	/* src 为空指针或指向空字符串，均视为插入空字符串。 */
	if (src == DSTR_NULLPTR || src->len == 0) {
		return DSTR_SUCCESS;
	}

	return insert_str(dest, index, 0, src->data, src->len, 0, 0);
}

/* 插入一个「C 字符串」的子串到一个「动态字符串」。 */
dstr_status_t dstr_insert_sub_cstr(
	dstr_adt *const dest,
	const size_t index,
	const char *const src,
	const size_t sub_index,
	const size_t sub_count
) {
	if (dest == DSTR_NULLPTR || index > dest->len) {
		return DSTR_INVALID_ARGUMENT;
	}

	/* src 为空指针或指向空字符串，均视为插入空字符串。 */
	if (src == DSTR_NULLPTR || src[0] == '\0') {
		return DSTR_SUCCESS;
	}

	/* 越界检查。 */
	const size_t src_len = strlen(src);
	if (sub_index >= src_len ||
		!safe_size_t_add(sub_index, sub_count, DSTR_NULLPTR) ||
		sub_index + sub_count > src_len
	) {
		return DSTR_INVALID_ARGUMENT;
	}

	return insert_str(dest, index, 0, src, src_len, sub_index, sub_count);
}

/* 插入一个「动态字符串」的子串到另一个「动态字符串」。 */
dstr_status_t dstr_insert_sub(
	dstr_adt *const dest,
	const size_t index,
	const dstr_adt *const src,
	const size_t sub_index,
	const size_t sub_count
) {
	if (dest == DSTR_NULLPTR || index > dest->len) {
		return DSTR_INVALID_ARGUMENT;
	}

	/* src 为空指针或指向空字符串，均视为插入空字符串。 */
	if (src == DSTR_NULLPTR || src->len == 0) {
		return DSTR_SUCCESS;
	}

	/* 越界检查。 */
	if (sub_index >= src->len ||
		!safe_size_t_add(sub_index, sub_count, DSTR_NULLPTR) ||
		sub_index + sub_count > src->len
	) {
		return DSTR_INVALID_ARGUMENT;
	}

	return insert_str(dest, index, 0, src->data, src->len, sub_index, sub_count);
}

dstr_status_t dstr_insert_format(
	dstr_adt *const dstr,
	const size_t index,
	const char *const format,
	...
) {
	if (dstr == DSTR_NULLPTR || index > dstr->len) {
		return DSTR_INVALID_ARGUMENT;
	}

	/* format 为空指针或指向空字符串，均视为插入空字符串。 */
	if (format == DSTR_NULLPTR || format[0] == '\0') {
		return DSTR_SUCCESS;
	}

	va_list args;

	va_start(args, format);
	const dstr_status_t result = format_to_dstr(dstr, index, 0, format, args);
	va_end(args);

	return result;
}

dstr_status_t dstr_insert_vformat(
	dstr_adt *const dstr,
	const size_t index,
	const char *const format,
	const va_list args
) {
	if (dstr == DSTR_NULLPTR || index > dstr->len) {
		return DSTR_INVALID_ARGUMENT;
	}

	/* format 为空指针或指向空字符串，均视为插入空字符串。 */
	if (format == DSTR_NULLPTR || format[0] == '\0') {
		return DSTR_SUCCESS;
	}

	return format_to_dstr(dstr, index, 0, format, args);
}

/* 清空一个「动态字符串」。 */
void dstr_clear(
	dstr_adt *const dstr
) {
	if (dstr == DSTR_NULLPTR) {
		return;
	}

	dstr->len = 0;
}

/* 删除一个「动态字符串」的子串。 */
void dstr_remove(
	dstr_adt *const dstr,
	const size_t index,
	const size_t count
) {
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		index >= dstr->len ||
		!safe_size_t_add(index, count, DSTR_NULLPTR) ||
		index + count > dstr->len
	) {
		return;
	}

	const size_t remove_count = (count > 0) ? count : (dstr->len - index);

	insert_str(dstr, index, remove_count, DSTR_NULLPTR, 0, 0, 0);
}

/* 删除一个「动态字符串」首尾的空白字符或指定字符。 */
void dstr_trim(
	dstr_adt *const dstr,
	const char *const trim_chars
) {
	if (dstr == DSTR_NULLPTR || dstr->len == 0) {
		return;
	}

	/* trim_chars 为空指针或指向空字符串时，均视为没有指定字符。 */
	const bool is_specified_trim_chars = (trim_chars != DSTR_NULLPTR && trim_chars[0] != '\0');
	/* 用于迭代和区间定位。 */
	const char *p = dstr->data;
	const char *q = p + dstr->len;

	/* 定位剩余区间。 */
	if (is_specified_trim_chars) {
		while (strchr(trim_chars, *p) != DSTR_NULLPTR) {
			++p;
		}
		while (q > p && strchr(trim_chars, *(q - 1)) != DSTR_NULLPTR) {
			--q;
		}
	} else {
		while (isspace(*p)) {
			++p;
		}
		while (q > p && isspace(*(q - 1))) {
			--q;
		}
	}

	if (p < q) {
		dstr->len = q - p;
		if (p > dstr->data) {
			memmove(dstr->data, p, dstr->len);
		}
	} else {
		dstr->len = 0;
	}

	capacity_resize_dynamic(dstr, (dstr->len > 0) ? (dstr->len + 1) : 0);
	if (dstr->cap > 0) {
		dstr->data[dstr->len] = '\0';
	}
}


/* 关系判断与比较。 */

/* 判断一个「动态字符串」是否以指定「C 字符串」前缀开头。 */
bool dstr_starts_with_cstr(
	const dstr_adt *const dstr,
	const char *const prefix
) {
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		prefix == DSTR_NULLPTR || prefix[0] == '\0'
	) {
		return false;
	}

	const size_t prefix_len = strlen(prefix);
	if (prefix_len > dstr->len) {
		return false;
	}

	return (memcmp(dstr->data, prefix, prefix_len) == 0);
}

/* 判断一个「动态字符串」是否以指定「动态字符串」前缀开头。 */
bool dstr_starts_with(
	const dstr_adt *const dstr,
	const dstr_adt *const prefix
) {
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		prefix == DSTR_NULLPTR || prefix->len == 0
	) {
		return false;
	}

	if (prefix->len > dstr->len) {
		return false;
	}

	return (memcmp(dstr->data, prefix->data, prefix->len) == 0);
}

/* 判断一个「动态字符串」是否以指定「C 字符串」后缀结尾。 */
bool dstr_ends_with_cstr(
	const dstr_adt *const dstr,
	const char *const suffix
) {
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		suffix == DSTR_NULLPTR || suffix[0] == '\0'
	) {
		return false;
	}

	const size_t suffix_len = strlen(suffix);
	if (suffix_len > dstr->len) {
		return false;
	}

	return (memcmp(
		dstr->data + dstr->len - suffix_len,
		suffix,
		suffix_len
	) == 0);
}

/* 判断一个「动态字符串」是否以指定「动态字符串」后缀结尾。 */
bool dstr_ends_with(
	const dstr_adt *const dstr,
	const dstr_adt *const suffix
) {
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		suffix == DSTR_NULLPTR || suffix->len == 0
	) {
		return false;
	}

	if (suffix->len > dstr->len) {
		return false;
	}

	return (memcmp(
		dstr->data + dstr->len - suffix->len,
		suffix->data,
		suffix->len
	) == 0);
}

/* 判断一个「动态字符串」是否包含指定子「C 字符串」。 */
bool dstr_contains_cstr(
	const dstr_adt *const dstr,
	const char *const sub
) {
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		sub == DSTR_NULLPTR || sub[0] == '\0'
	) {
		return false;
	}

	const size_t sub_len = strlen(sub);
	if (sub_len > dstr->len) {
		return false;
	}

	return (strstr(dstr->data, sub) != DSTR_NULLPTR);
}

/* 判断一个「动态字符串」是否包含指定子「动态字符串」。 */
bool dstr_contains(
	const dstr_adt *const dstr,
	const dstr_adt *const sub
) {
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		sub == DSTR_NULLPTR || sub->len == 0
	) {
		return false;
	}

	if (sub->len > dstr->len) {
		return false;
	}

	return (strstr(dstr->data, sub->data) != DSTR_NULLPTR);
}

/* 判断一个「动态字符串」是否与一个「C 字符串」相等。 */
bool dstr_equals_cstr(
	const dstr_adt *const dstr,
	const char *const cstr
) {
	const int str_1_valid = (dstr != DSTR_NULLPTR && dstr->len > 0) ? 1 : 0;
	const int str_2_valid = (cstr != DSTR_NULLPTR && cstr[0] != '\0') ? 1 : 0;

	if (str_1_valid + str_2_valid < 2) {
		return (str_1_valid == str_2_valid);
	}

	const size_t cstr_len = strlen(cstr);
	if (dstr->len != cstr_len) {
		return false;
	}

	return (memcmp(dstr->data, cstr, cstr_len) == 0);
}

/* 判断两个「动态字符串」是否相等。 */
bool dstr_equals(
	const dstr_adt *const dstr_1,
	const dstr_adt *const dstr_2
) {
	const int str_1_valid = (dstr_1 != DSTR_NULLPTR && dstr_1->len > 0) ? 1 : 0;
	const int str_2_valid = (dstr_2 != DSTR_NULLPTR && dstr_2->len > 0) ? 1 : 0;

	if (str_1_valid + str_2_valid < 2) {
		return (str_1_valid == str_2_valid);
	}

	if (dstr_1->len != dstr_2->len) {
		return false;
	}

	return (memcmp(dstr_1->data, dstr_2->data, dstr_1->len) == 0);
}

/* 比较一个「动态字符串」与一个「C 字符串」。 */
int dstr_compare_cstr(
	const dstr_adt *const dstr,
	const char *const cstr
) {
	const int str_1_valid = (dstr != DSTR_NULLPTR && dstr->len > 0) ? 1 : 0;
	const int str_2_valid = (cstr != DSTR_NULLPTR && cstr[0] != '\0') ? 1 : 0;

	if (str_1_valid + str_2_valid < 2) {
		return str_1_valid - str_2_valid;
	}

	return strcmp(dstr->data, cstr);
}

/* 比较两个「动态字符串」。 */
int dstr_compare(
	const dstr_adt *const dstr_1,
	const dstr_adt *const dstr_2
) {
	const int str_1_valid = (dstr_1 != DSTR_NULLPTR && dstr_1->len > 0) ? 1 : 0;
	const int str_2_valid = (dstr_2 != DSTR_NULLPTR && dstr_2->len > 0) ? 1 : 0;

	if (str_1_valid + str_2_valid < 2) {
		return str_1_valid - str_2_valid;
	}

	return strcmp(dstr_1->data, dstr_2->data);
}

/* 查找、统计与替换。 */

/* 查找一个「动态字符串」中指定子「C 字符串」首次出现的位置。 */
bool dstr_find_cstr(
	const dstr_adt *const dstr,
	const char *const sub,
	size_t *const out_index,
	const bool backward
) {
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		sub == DSTR_NULLPTR || sub[0] == '\0'
	) {
		return false;
	}

	const size_t sub_len = strlen(sub);
	if (sub_len > dstr->len) {
		return false;
	}

	return (find_str(dstr, sub, sub_len, out_index, 1, backward) > 0);
}

/* 查找一个「动态字符串」中指定子「动态字符串」首次出现的位置。 */
bool dstr_find(
	const dstr_adt *const dstr,
	const dstr_adt *const sub,
	size_t *const out_index,
	const bool backward
) {
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		sub == DSTR_NULLPTR || sub->len == 0
	) {
		return false;
	}

	if (sub->len > dstr->len) {
		return false;
	}

	return (find_str(dstr, sub->data, sub->len, out_index, 1, backward) > 0);
}

/* 查找一个「动态字符串」中指定子「C 字符串」第 n 次出现的位置。 */
bool dstr_find_nth_cstr(
	const dstr_adt *const dstr,
	const char *const sub,
	size_t *const out_index,
	const size_t n,
	const bool backward
) {
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		sub == DSTR_NULLPTR || sub[0] == '\0'
	) {
		return false;
	}

	const size_t sub_len = strlen(sub);
	if (sub_len > dstr->len) {
		return false;
	}

	return (find_str(dstr, sub, sub_len, out_index, n, backward) > 0);
}

/* 查找一个「动态字符串」中指定子「动态字符串」第 n 次出现的位置。 */
bool dstr_find_nth(
	const dstr_adt *const dstr,
	const dstr_adt *const sub,
	size_t *const out_index,
	const size_t n,
	const bool backward
) {
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		sub == DSTR_NULLPTR || sub->len == 0
	) {
		return false;
	}

	if (sub->len > dstr->len) {
		return false;
	}

	return (find_str(dstr, sub->data, sub->len, out_index, n, backward) > 0);
}

/* 统计一个「动态字符串」中指定子「C 字符串」出现的次数。 */
size_t dstr_count_cstr(
	const dstr_adt *const dstr,
	const char *const sub
) {
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		sub == DSTR_NULLPTR || sub[0] == '\0'
	) {
		return 0;
	}

	const size_t sub_len = strlen(sub);
	if (sub_len > dstr->len) {
		return 0;
	}

	return find_str(dstr, sub, sub_len, DSTR_NULLPTR, 0, false);
}

/* 统计一个「动态字符串」中指定子「动态字符串」出现的次数。 */
size_t dstr_count(
	const dstr_adt *const dstr,
	const dstr_adt *const sub
) {
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		sub == DSTR_NULLPTR || sub->len == 0
	) {
		return 0;
	}

	if (sub->len > dstr->len) {
		return 0;
	}

	return find_str(dstr, sub->data, sub->len, DSTR_NULLPTR, 0, false);
}

/* 替换一个「动态字符串」中指定旧「C 字符串」为指定新「C 字符串」n 次。 */
dstr_status_t dstr_replace_cstr(
	dstr_adt *const dstr,
	const char *const old_str,
	const char *const new_str,
	const size_t n,
	const bool backward
) {
	/* 参数检查。 */
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		old_str == DSTR_NULLPTR || old_str[0] == '\0'
	) {
		return DSTR_INVALID_ARGUMENT;
	}

	/* 计算 old_str 长度。 */
	const size_t old_str_len = strlen(old_str);
	if (old_str_len > dstr->len) {
		return DSTR_INVALID_ARGUMENT;
	}

	if (new_str == DSTR_NULLPTR || new_str[0] == '\0') {
		replace_str(dstr, old_str, old_str_len, DSTR_NULLPTR, 0, n, backward);
	}

	return replace_str(dstr, old_str, old_str_len, new_str, strlen(new_str), n, backward);
}

/* 替换一个「动态字符串」中指定旧「动态字符串」为指定新「动态字符串」n 次。 */
dstr_status_t dstr_replace(
	dstr_adt *const dstr,
	const dstr_adt *const old_str,
	const dstr_adt *const new_str,
	const size_t n,
	const bool backward
) {
	/* 参数检查。 */
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		old_str == DSTR_NULLPTR || old_str->len == 0
	) {
		return DSTR_INVALID_ARGUMENT;
	}

	if (old_str->len > dstr->len) {
		return DSTR_INVALID_ARGUMENT;
	}

	if (new_str == DSTR_NULLPTR || new_str->len == 0) {
		replace_str(dstr, old_str->data, old_str->len, DSTR_NULLPTR, 0, n, backward);
	}

	return replace_str(dstr, old_str->data, old_str->len, new_str->data, new_str->len, n, backward);
}


/*------------------------------------------------------------------------------
 * 静态函数定义
 *----------------------------------------------------------------------------*/

/* 调整一个「动态字符串」的容量（基础版）。 */
static bool capacity_resize(
	dstr_adt *const dstr,
	const size_t new_cap
) {
	char *new_data = realloc(dstr->data, new_cap);
	if (new_data == DSTR_NULLPTR) {
		return false;
	}

	dstr->data = new_data;
	dstr->cap = new_cap;

	return true;
}

/* 调整一个「动态字符串」的容量（常规版）。 */
static bool capacity_resize_regular(
	dstr_adt *const dstr,
	const size_t new_cap
) {
	if (new_cap == dstr->cap) {
		return true;
	}

	if (new_cap == 0) {
		free(dstr->data);
		dstr->data = DSTR_NULLPTR;
		dstr->cap = 0;

		return true;
	}

	return capacity_resize(dstr, new_cap);
}

/* 调整一个「动态字符串」的容量（动态版）。 */
static bool capacity_resize_dynamic(
	dstr_adt *const dstr,
	const size_t new_cap
) {
	size_t aligned_cap = 0; /* 对齐到缓存行大小的容量值。 */

	/**
	 * 目标容量。
	 * 确保不低于 dstr 的容量保底值。
	 * 因此取所需容量 new_cap 和 dstr 的容量保底值 dstr->min_cap 的最大值。
	 */
	const size_t target_cap = DSTR_MAX(new_cap, dstr->min_cap);

	/* 目标容量与当前容量相等，无需调整，直接返回 true。 */
	if (target_cap == dstr->cap) {
		return true;
	}

	/**
	 * 如果目标容量小于当前容量，则延迟减容。
	 * 仅当目标容量小于当前容量的 1/4 时，才减容。
	 * 因此，当目标容量小于当前容量，且大于当前容量的 1/4 时，直接返回 true。
	 */
	if (target_cap < dstr->cap && target_cap > (dstr->cap >> 2)) {
		return true;
	}

	/* 目标容量为 0，单独释放。 */
	if (target_cap == 0) {
		free(dstr->data);
		dstr->data = DSTR_NULLPTR;
		dstr->cap = 0;

		return true;
	}

	/* 如果目标容量大于当前容量，则执行容量预分配、缓存行对齐。 */
	if (target_cap > dstr->cap) {
		size_t adjusted_cap = 0; /* 预分配后的容量值。 */

		/* 尝试预分配内存。 */
		if (safe_size_t_add(target_cap, target_cap >> 1, &adjusted_cap)) {
			/* 尝试对齐到缓存行大小。 */
			if (safe_size_t_align_up(adjusted_cap,DSTR_CACHELINE_SIZE, &aligned_cap)) {
				/* 尝试调整 aligned_cap。 */
				if (capacity_resize(dstr, aligned_cap)) {
					return true;
				}
			}

			/**
			 * 对齐溢出，或调整 aligned_cap 失败。
			 * 此时，仅在 aligned_cap 与 adjusted_cap 不相等时，尝试调整 adjusted_cap。
			 */
			if (aligned_cap != adjusted_cap) {
				if (capacity_resize(dstr, adjusted_cap)) {
					return true;
				}
			}
		}

		/**
		 * 预分配溢出，或调整 aligned_cap、adjusted_cap 都失败。
		 * 此时，仅在 adjusted_cap 与 target_cap 不相等时，尝试调整 target_cap。
		 */
		if (adjusted_cap != target_cap) {
			if (capacity_resize(dstr, target_cap)) {
				return true;
			}
		}

		/* 上述尝试都失败，则返回 false。 */
		return false;
	}

	/* 目标容量小于当前容量的 1/4，执行减容。 */

	/* 尝试对齐到缓存行大小。 */
	if (safe_size_t_align_up(target_cap, DSTR_CACHELINE_SIZE, &aligned_cap)) {
		/**
		 * aligned_cap 有可能与当前容量相等。
		 * 如果相等，则直接返回 true。
		 */
		if (aligned_cap == dstr->cap) {
			return true;
		}

		/* 尝试调整 aligned_cap。 */
		if (capacity_resize(dstr, aligned_cap)) {
			return true;
		}
	}

	/**
	 * 对齐溢出，或调整 aligned_cap 失败。
	 * 此时，仅在 aligned_cap 与 target_cap 不相等时，尝试调整 target_cap。
	 */
	if (aligned_cap != target_cap) {
		if (capacity_resize(dstr, target_cap)) {
			return true;
		}
	}

	/* 上述尝试都失败，则返回 false。 */
	return false;
}

static dstr_adt *create_dstr(
	const char *const src,
	const size_t src_len,
	const size_t sub_index,
	const size_t sub_count
) {
	dstr_adt *new_dstr = malloc(sizeof(dstr_adt));
	if (new_dstr == DSTR_NULLPTR) {
		return DSTR_NULLPTR;
	}

	new_dstr->data = DSTR_NULLPTR;

	if (src_len > 0) {
		const size_t copy_len = (sub_count > 0)
			? sub_count
			: src_len - sub_index;

		if (!safe_size_t_add(copy_len, 1, DSTR_NULLPTR) ||
			!capacity_resize(new_dstr, copy_len + 1)
		) {
			free(new_dstr);
			return DSTR_NULLPTR;
		}

		memcpy(new_dstr->data, src + sub_index, copy_len);
		new_dstr->data[new_dstr->len = copy_len] = '\0';
	} else {
		new_dstr->cap = 0;
		new_dstr->len = 0;
	}

	new_dstr->min_cap = 0;

	return new_dstr;
}

static dstr_status_t insert_str(
	dstr_adt *const dest,
	const size_t index,
	const size_t count,
	const char *const src,
	const size_t src_len,
	const size_t sub_index,
	const size_t sub_count
) {
	const size_t copy_len = (src_len > 0)
		? ((sub_count > 0) ? (sub_count) : (src_len - sub_index))
		: 0;
	const size_t new_len = dest->len - count + copy_len;

	/* 当新长度大于当前长度时尝试扩容。 */
	if (new_len > dest->len) {
		if (!safe_size_t_add(new_len, 1, DSTR_NULLPTR) ||
			!capacity_resize_dynamic(dest, new_len + 1)
		) {
			return DSTR_MEMORY_ALLOC_FAILED;
		}
	}

	/* 当存在需要移动的尾部数据时，执行移动。 */
	const size_t tail_len = dest->len - index - count;
	if (tail_len > 0 && count != copy_len) {
		memmove(
			dest->data + index + copy_len,
			dest->data + index + count,
			tail_len
		);
	}

	/* 当存在需要拷贝的数据时，执行拷贝。 */
	if (copy_len > 0) {
		memcpy(
			dest->data + index,
			src + sub_index,
			copy_len
		);
	}

	/* 如果新长度小于当前长度，则在操作执行完后，尝试缩容。 */
	if (new_len < dest->len) {
		/* 当长度为 0 时，所需容量为 0；当长度不为 0 时，所需容量为长度 + 1。 */
		capacity_resize_dynamic(dest, (new_len > 0) ? (new_len + 1) : 0);
	}

	/* 如果长度有变化，则更新长度。 */
	if (new_len != dest->len) {
		dest->len = new_len;

		/**
		 * 当新的长度为 0 时，容量有可能为 0，dest->data 为空指针。
		 * 为避免空指针解引用，仅当容量不为 0 时，于 dest->data[new_len] 处写入 '\0'。
		 */
		if (dest->cap > 0) {
			dest->data[new_len] = '\0';
		}
	}

	return DSTR_SUCCESS;
}

static dstr_status_t format_to_dstr(
	dstr_adt *const dstr,
	const size_t index,
	const size_t delete_count,
	const char *const format,
	va_list args
) {
	va_list temp_args;

	va_copy(temp_args, args);
	const int temp_len = vsnprintf(DSTR_NULLPTR, 0, format, temp_args);
	va_end(temp_args);

	if (temp_len < 0) {
		return DSTR_INVALID_ARGUMENT;
	}

	const size_t format_len = temp_len;
	const size_t new_len = dstr->len - delete_count + format_len;

	/* 当新长度大于当前长度时尝试扩容。 */
	if (new_len > dstr->len) {
		if (!safe_size_t_add(new_len, 1, DSTR_NULLPTR) ||
			!capacity_resize_dynamic(dstr, new_len + 1)
		) {
			return DSTR_MEMORY_ALLOC_FAILED;
		}
	}

	/* 当存在需要移动的尾部数据时，执行移动。 */
	const size_t tail_len = dstr->len - index - delete_count;
	if (tail_len > 0 && delete_count != format_len) {
		memmove(
			dstr->data + index + format_len,
			dstr->data + index + delete_count,
			tail_len
		);
	}

	/**
	 * vsnprintf 写入后会在末尾强制写入 '\0'。
	 * 对于插入操作（tail_len > 0），该 '\0' 会覆盖被搬移到 format_len 末尾的第一个尾部字符。
	 * 因此，在写入前暂存该字符，写入后恢复。
	 */
	char saved_char = '\0';
	if (format_len > 0 && tail_len > 0) {
		saved_char = dstr->data[index + format_len];
	}

	/* 当存在需要格式化的数据时，格式化写入到 dstr->data。 */
	if (format_len > 0) {
		vsnprintf(dstr->data + index, format_len + 1, format, args);

		/* 恢复被 '\0' 覆盖的字符。 */
		if (tail_len > 0) {
			dstr->data[index + format_len] = saved_char;
		}
	}

	/* 如果新长度小于当前长度，则在操作执行完后尝试缩容。 */
	if (new_len < dstr->len) {
		capacity_resize_dynamic(dstr, (new_len > 0) ? (new_len + 1) : 0);
	}

	/* 如果长度有变化，则更新长度。 */
	if (new_len != dstr->len) {
		dstr->len = new_len;

		if (dstr->cap > 0) {
			dstr->data[new_len] = '\0';
		}
	}

	return DSTR_SUCCESS;
}

static size_t find_str(
	const dstr_adt *const dstr,
	const char *const sub,
	const size_t sub_len,
	size_t *const out_index,
	const size_t n,
	const bool backward
) {
	const char *p;
	const char *find = DSTR_NULLPTR;
	size_t find_count = 0;
	const char *const end = dstr->data + dstr->len - sub_len + 1;

	if (backward) {
		p = end;

		while (p > dstr->data) {
			if (memcmp(p - 1, sub, sub_len) == 0) {
				++find_count;
				find = p - 1;

				if (find_count == n) {
					break;
				}

				if ((p - dstr->data) > sub_len) {
					p -= sub_len;
				} else {
					break;
				}
			} else {
				--p;
			}
		}
	} else {
		p = dstr->data;

		while (p < end) {
			if (memcmp(p, sub, sub_len) == 0) {
				++find_count;
				find = p;

				if (find_count == n) {
					break;
				}

				p += sub_len;
			} else {
				++p;
			}
		}
	}


	if (out_index != DSTR_NULLPTR && find != DSTR_NULLPTR) {
		*out_index = find - dstr->data;
	}

	return find_count;
}

static dstr_status_t replace_str(
	dstr_adt *const dstr,
	const char *const old_str,
	const size_t old_str_len,
	const char *const new_str,
	const size_t new_str_len,
	const size_t n,
	const bool backward
) {
	/* 先执行一次统计，以获取 old_str 出现的次数。 */
	const size_t old_str_count = find_str(dstr, old_str, old_str_len, DSTR_NULLPTR, n, backward);
	if (old_str_count == 0 || old_str_count < n) {
		return DSTR_INVALID_ARGUMENT;
	}

	/* 动态分配 size_t 数组，用来记录每次出现的位置。 */
	size_t indexes_size, *indexes;
	if (!safe_size_t_mul(old_str_count, sizeof(size_t), &indexes_size) ||
		(indexes = malloc(indexes_size)) == DSTR_NULLPTR
	) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	size_t length_difference = 0; /* new_str_len 与old_str_len差绝对值。 */
	size_t new_length, required_capacity;

	if (new_str_len > old_str_len) { /* 扩容缓冲区。 */
		length_difference = new_str_len - old_str_len;

		size_t increased_length;

		if (!safe_size_t_mul(length_difference, old_str_count, &increased_length) ||
			!safe_size_t_add(dstr->len, increased_length, &new_length) ||
			!safe_size_t_add(new_length, 1, &required_capacity) ||
			!capacity_resize_dynamic(dstr, required_capacity)
		) {
			free(indexes);
			return DSTR_MEMORY_ALLOC_FAILED;
		}
	} else if (new_str_len < old_str_len) {
		length_difference = old_str_len - new_str_len;

		const size_t reduced_length = length_difference * old_str_count;

		new_length = dstr->len - reduced_length;
		required_capacity = new_length + 1;
	}

	/* 进行一次查找，记录每次出现的位置。 */
	size_t i = 0;
	const char *p;
	const char *const end = dstr->data + dstr->len - old_str_len + 1;

	if (backward) {
		p = end;

		while (p > dstr->data) {
			if (memcmp(p - 1, old_str, old_str_len) == 0) {
				indexes[old_str_count - ++i] = p - 1 - dstr->data;

				if (i == old_str_count) {
					break;
				}

				p -= old_str_len;
			} else {
				--p;
			}
		}
	} else {
		p = dstr->data;

		while (p < end) {
			if (memcmp(p, old_str, old_str_len) == 0) {
				indexes[i++] = p - dstr->data;

				if (i == old_str_count) {
					break;
				}

				p += old_str_len;
			} else {
				++p;
			}
		}
	}

	/* 执行替换。 */
	if (new_str_len > old_str_len) {
		for (i = old_str_count; i > 0; --i) {
			/* 计算当前 old_str 出现位置后面需要移动的数据长度。 */
			const size_t move_start = indexes[i - 1] + old_str_len;
			const size_t move_end = (i == old_str_count) ? dstr->len : indexes[i];
			const size_t move_length = move_end - move_start;

			/* 移动原有数据。 */
			if (move_length > 0) {
				const size_t move_step = length_difference * i;
				memmove(
					dstr->data + move_start + move_step,
					dstr->data + move_start,
					move_length
				);
			}

			/* 拷贝新数据。 */
			if (new_str_len > 0) {
				const size_t copy_target = indexes[i - 1] + length_difference * (i - 1);
				memcpy(
					dstr->data + copy_target,
					new_str,
					new_str_len
				);
			}
		}
	} else {
		for (i = 0; i < old_str_count; ++i) {
			if (new_str_len < old_str_len) {
				/* 计算当前 old_str 出现位置后面需要移动的数据长度。 */
				const size_t move_start = indexes[i] + old_str_len;
				const size_t move_end = (i == old_str_count - 1) ? dstr->len : indexes[i + 1];
				const size_t move_length = move_end - move_start;

				/* 移动原有数据。 */
				if (move_length > 0) {
					const size_t move_step = length_difference * (i + 1);
					memmove(
						dstr->data + move_start - move_step,
						dstr->data + move_start,
						move_length
					);
				}
			}

			/* 拷贝新数据。 */
			if (new_str_len > 0) {
				const size_t copy_target = indexes[i] - length_difference * i;
				memcpy(
					dstr->data + copy_target,
					new_str,
					new_str_len
				);
			}
		}
	}

	free(indexes);

	if (new_str_len != old_str_len) {
		dstr->len = new_length;

		if (new_str_len < old_str_len) {
			capacity_resize_dynamic(dstr, required_capacity);
		}

		if (dstr->cap > 0) {
			dstr->data[new_length] = '\0';
		}
	}

	return DSTR_SUCCESS;
}
