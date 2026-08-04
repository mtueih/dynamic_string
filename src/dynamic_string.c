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
 * src/dynamic_string.c - 项目主库实现文件
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

/* 缓存行大小。 */
#define DSTR_CACHELINE_SIZE 64

/* 静态函数 replace_str 中，indexes 数组缩容绝对阈值。 */
#define INDEXES_SHRINK_BYTES 4096


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
 * @brief
 * 调整一个「动态字符串」的容量（基础版）。
 *
 * @remark
 * 只是简单地封装了 realloc 操作。
 * 不做 new_cap 为 0 ，以及是否与原容量相等的检查。
 * 适用于确定所需容量不等于原容量，且不为 0 的情况。
 *
 * @warning
 * 此函数不做 new_cap 是否为 0 的检查，new_cap 为 0 将导致
 * realloc 可能的未定义行为。
 * 如果不能确定所需容量不为 0，请使用 resize_capacity_regular()
 * 或 resize_capacity_dynamic()。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * @param new_cap
 * 新的容量。
 * 不能为 0，会导致未定义行为！
 *
 * @return
 * 如果调整成功则返回 true，否则返回 false。
 */
static bool resize_capacity(
	dstr_adt *dstr,
	size_t new_cap
);

/**
 * @brief
 * 调整一个「动态字符串」的容量（常规版）。
 *
 * @remark
 * 在基础版的基础上增加对 new_cap 为 0 ，以及是否与原容量相等的检查。
 * 适用于不能确定所需容量是否不等于原容量、是否不为 0 的情况。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * @param new_cap
 * 新的容量。
 *
 * @return
 * 如果调整成功则返回 true，否则返回 false。
 */
static bool resize_capacity_regular(
	dstr_adt *dstr,
	size_t new_cap
);

/**
 * @brief
 * 调整一个「动态字符串」的容量（动态版）。
 * 在基础版的基础上增加对 new_cap 为 0，以及是否与原容量相等的检查。
 * 会确保容量不会低于目标「动态字符串」的容量保底值。
 * 会执行预分配、延迟减容、缓存行对齐等性能优化策略。
 * 所有由长度变化引起的容量调整都应该且只能使用此函数。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * @param new_cap
 * 新的容量。
 *
 * @return
 * 如果调整成功则返回 true，否则返回 false。
 * “调整成功”只保证调整后的容量不低于 new_cap，不保证预分配等策略一定生效。
 */
static bool resize_capacity_dynamic(
	dstr_adt *dstr,
	size_t new_cap
);

/**
 * @brief
 * 从一个「C 字符串」的子串创建一个新「动态字符串」。
 *
 * @param src
 * 源「C 字符串」的指针。
 * @param src_len
 * 源「C 字符串」的长度。
 * 为 0 时，创建空「动态字符串」。
 * @param sub_start
 * 子串的起始索引。
 * @param sub_length
 * 子串的长度。
 * 为 0 表示到末尾。
 *
 * @return
 * 所创建的「动态字符串」的指针。
 * 如果创建失败则返回空指针。
 */
static dstr_adt *create_dstr(
	const char *src,
	size_t src_len,
	size_t sub_start,
	size_t sub_length
);

/**
 * @brief
 * 向一个「动态字符串」的指定位置处插入一个「C 字符串」的子串。
 * 插入前可选择性删除目标位置向后的指定数量个字符，用于覆写和删除。
 *
 * @param dest
 * 目标「动态字符串」的指针。
 * @param index
 * 插入位置的索引。
 * @param remove_length
 * 插入前先删除的字符数。
 * 0 不表示删除到末尾。
 * @param src
 * 源「C 字符串」的指针。
 * @param src_len
 * 源「C 字符串」的长度。
 * @param sub_start
 * 子串的起始索引。
 * @param sub_length
 * 子串的长度。
 * 为 0 表示到末尾。
 *
 * @return
 * 全局状态码。
 */
/* 向一个「动态字符串」的指定位置处插入一个「C 字符串」的子串。 */
static dstr_status_t insert_str(
	dstr_adt *dest,
	size_t index,
	size_t remove_length,
	const char *src,
	size_t src_len,
	size_t sub_start,
	size_t sub_length
);

/**
 * @brief
 * 向一个「动态字符串」的指定位置处格式化插入一个字符串。
 * 写入前可选择性删除目标位置向后的指定数量个字符，用于覆写和删除。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * @param index
 * 插入位置的索引。
 * @param remove_length
 * 插入前先删除的字符数。
 * 0 不表示删除到末尾。
 * @param format
 * 格式「C 字符串」的指针。
 * @param args
 * 可变参数列表，类型为 va_list。
 *
 * @return
 * 全局状态码。
 */
static dstr_status_t insert_str_format(
	dstr_adt *dstr,
	size_t index,
	size_t remove_length,
	const char *format,
	va_list args
);

/**
 * @brief
 * 查找一个「C 字符串」中，指定子「C 字符串」第 n 次出现的位置，
 * 并返回截止第 n 次，实际一共出现的次数。
 *
 * @param cstr
 * 目标「C 字符串」的指针。
 * @param cstr_len
 * 目标「C 字符串」的长度。
 * @param sub
 * 子「C 字符串」的指针。
 * @param sub_len
 * 子「C 字符串」的长度。
 * @param out_index
 * 存储查找结果（位置索引）的 size_t 变量的指针。
 * 为空指针时不写入。
 * @param out_indexes
 * 存储查找结果（位置索引）的 size_t 数组的指针。
 * 为空指针时不写入。
 * 请确保容量足够。
 * 通常需要先进行一次统计，获得确切出现次数后使用此参数，
 * 也可预备一个足够大的数组，以减少一次查找。
 * @param direction
 * 查找方向。
 * @param n
 * 出现的次序。
 * 从 1 开始。
 * 为 0 表示最后一次。
 *
 * @return
 * 指定子「C 字符串」截止第 n 次，实际一共出现的次数。
 */
static size_t find_str(
	const char *cstr,
	size_t cstr_len,
	const char *sub,
	size_t sub_len,
	size_t *out_index,
	size_t *out_indexes,
	dstr_direction_t direction,
	size_t n
);

/**
 * @brief
 * 将一个「动态字符串」中指定的旧「C 字符串」替换为指定的新「C 字符串」。
 * 可指定替换方向和替换次数。
 *
 * @param dstr
 * 目标「动态字符串」的指针。
 * @param old_str
 * 旧「C 字符串」的指针。
 * @param old_str_len
 * 旧「C 字符串」的长度。
 * @param new_str
 * 新「C 字符串」的指针。
 * 为空指针时视为替换为空字符串（即删除旧字符串）。
 * @param new_str_len
 * 新「C 字符串」的长度。
 * new_str 为空指针时该值应为 0。
 * @param direction
 * 替换方向。
 * @param n
 * 替换次数。
 * 从 1 开始。
 * 为 0 表示全部替换。
 *
 * @return
 * 全局状态码。
 */
static dstr_status_t replace_str(
	dstr_adt *dstr,
	const char *old_str,
	size_t old_str_len,
	const char *new_str,
	size_t new_str_len,
	dstr_direction_t direction,
	size_t n
);

/**
 * @brief
 * 将一个「C 字符串」按指定分隔符分割为多个「动态字符串」。
 *
 * @param cstr
 * 目标「C 字符串」的指针。
 * @param cstr_len
 * 目标「C 字符串」的长度。
 * @param separator
 * 分隔「C 字符串」的指针。
 * @param separator_len
 * 分隔「C 字符串」的长度。
 * @param out_dstr_count
 * 存储分割后「动态字符串」个数的 size_t 变量的指针。
 *
 * @return
 * 分割后「动态字符串」指针数组的指针。
 * 如果分割失败则返回空指针。
 * 注意：数组中的某些元素可能为空指针，表示该部分为空字符串。
 */
static dstr_adt **split_str(
	const char *cstr,
	size_t cstr_len,
	const char *separator,
	size_t separator_len,
	size_t *out_dstr_count
);

/**
 * @brief
 * 将多个「C 字符串」或「动态字符串」按指定分隔符连接合并为一个「动态字符串」。
 *
 * @remark
 * cstrs 和 dstrs 互斥，每次调用仅其中一个有效，另一个应为空指针。
 *
 * @param cstrs
 * 「C 字符串」常量指针数组的指针。
 * 为空指针时表示使用 dstrs。
 * @param dstrs
 * 「动态字符串」常量指针数组的指针。
 * 为空指针时表示使用 cstrs。
 * @param str_count
 * 字符串的数量。
 * @param separator
 * 分隔「C 字符串」的指针。
 * 为空指针时表示不使用分隔符。
 * @param separator_len
 * 分隔「C 字符串」的长度。
 * separator 为空指针时该值应为 0。
 *
 * @return
 * 合并后「动态字符串」的指针。
 * 如果合并失败则返回空指针。
 */
static dstr_adt *join_str(
	const char *const *cstrs,
	const dstr_adt *const *dstrs,
	size_t str_count,
	const char *separator,
	size_t separator_len
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
	if (dstr == DSTR_NULLPTR) { return; }

	free(dstr->data);
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

/* 提取一个「C 字符串」的子串。 */
dstr_adt *dstr_sub_cstr(
	const char *const cstr,
	const size_t sub_start,
	const size_t sub_length
) {
	if (cstr == DSTR_NULLPTR || cstr[0] == '\0') {
		return create_dstr(DSTR_NULLPTR, 0, 0, 0);
	}

	const size_t cstr_len = strlen(cstr);
	if (sub_start >= cstr_len ||
		!safe_size_t_add(sub_start, sub_length, DSTR_NULLPTR) ||
		sub_start + sub_length > cstr_len
	) { return DSTR_NULLPTR; }

	return create_dstr(cstr, cstr_len, sub_start, sub_length);
}

/* 提取一个「动态字符串」的子串。 */
dstr_adt *dstr_sub(
	const dstr_adt *const dstr,
	const size_t sub_start,
	const size_t sub_length
) {
	if (dstr == DSTR_NULLPTR || dstr->len == 0) {
		return create_dstr(DSTR_NULLPTR, 0, 0, 0);
	}

	if (sub_start >= dstr->len ||
		!safe_size_t_add(sub_start, sub_length, DSTR_NULLPTR) ||
		sub_start + sub_length > dstr->len
	) { return DSTR_NULLPTR; }

	return create_dstr(dstr->data, dstr->len, sub_start, sub_length);
}

/* 格式化创建一个「动态字符串」。 */
dstr_adt *dstr_create_format(
	const char *const format,
	...
) {
	if (format == DSTR_NULLPTR || format[0] == '\0') {
		return create_dstr(DSTR_NULLPTR, 0, 0, 0);
	}

	dstr_adt *const new_dstr = create_dstr(DSTR_NULLPTR, 0, 0, 0);
	if (new_dstr == DSTR_NULLPTR) { return DSTR_NULLPTR; }

	va_list args;

	va_start(args, format);
	const dstr_status_t rc = insert_str_format(new_dstr, 0, 0, format, args);
	va_end(args);

	if (rc != DSTR_SUCCESS) {
		free(new_dstr);
		return DSTR_NULLPTR;
	}

	return new_dstr;
}

/* 格式化创建一个「动态字符串」（va_list 版本）。 */
dstr_adt *dstr_create_vformat(
	const char *const format,
	va_list args
) {
	if (format == DSTR_NULLPTR || format[0] == '\0') {
		return create_dstr(DSTR_NULLPTR, 0, 0, 0);
	}

	dstr_adt *const new_dstr = create_dstr(DSTR_NULLPTR, 0, 0, 0);
	if (new_dstr == DSTR_NULLPTR) { return DSTR_NULLPTR; }

	const dstr_status_t rc = insert_str_format(new_dstr, 0, 0, format, args);
	if (rc != DSTR_SUCCESS) {
		free(new_dstr);
		return DSTR_NULLPTR;
	}

	return new_dstr;
}

/* 通过接管一个堆内存「C 字符串」所有权的方式创建一个「动态字符串」（移动语义）。 */
dstr_adt *dstr_create_move(
	char *const data,
	const size_t length
) {
	dstr_adt *const new_dstr = malloc(sizeof(dstr_adt));
	if (new_dstr == DSTR_NULLPTR) { return DSTR_NULLPTR; }

	if (data == DSTR_NULLPTR) {
		new_dstr->data = DSTR_NULLPTR;
		new_dstr->min_cap = new_dstr->cap = new_dstr->len = 0;
	} else {
		new_dstr->data = data;

		new_dstr->len = (data[length] == '\0') ? length : strlen(data);
		new_dstr->cap = new_dstr->len + 1;

		new_dstr->min_cap = 0;
	}

	return new_dstr;
}

/* 属性获取与设置。 */

/* 获取一个「动态字符串」的内部「C 字符串」指针。 */
const char *dstr_cstr(
	const dstr_adt *const dstr
) {
	if (dstr == DSTR_NULLPTR) { return DSTR_NULLPTR; }

	return dstr->data;
}

/* 获取一个「动态字符串」的长度。 */
size_t dstr_length(
	const dstr_adt *const dstr
) {
	if (dstr == DSTR_NULLPTR) { return 0; }

	return dstr->len;
}

/* 判断一个「动态字符串」是否是空「动态字符串」。 */
bool dstr_is_empty(
	const dstr_adt *const dstr
) {
	if (dstr == DSTR_NULLPTR) { return true; }

	return (dstr->len == 0);
}

/* 获取一个「动态字符串」的容量。 */
size_t dstr_capacity(
	const dstr_adt *const dstr
) {
	if (dstr == DSTR_NULLPTR) { return 0; }

	return dstr->cap;
}

/* 设置一个「动态字符串」的容量。 */
dstr_status_t dstr_set_capacity(
	dstr_adt *const dstr,
	const size_t new_capacity
) {
	if (dstr == DSTR_NULLPTR) { return DSTR_INVALID_ARGUMENT; }

	if (!resize_capacity_regular(dstr, new_capacity)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	dstr->min_cap = new_capacity;

	if (new_capacity <= dstr->len) {
		if (new_capacity > 0) {
			dstr->len = new_capacity - 1;
			dstr->data[dstr->len] = '\0';
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
	if (dstr == DSTR_NULLPTR) { return; }

	dstr->min_cap = 0;

	resize_capacity_regular(dstr, (dstr->len > 0) ? (dstr->len + 1) : 0);
}

/* 内容编辑。 */

/* 复制一个「C 字符串」到一个「动态字符串」。 */
dstr_status_t dstr_cpy_cstr(
	dstr_adt *const dest,
	const char *const src
) {
	if (dest == DSTR_NULLPTR) { return DSTR_INVALID_ARGUMENT; }

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
	if (dest == DSTR_NULLPTR) { return DSTR_INVALID_ARGUMENT; }

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
	const size_t sub_start,
	const size_t sub_length
) {
	if (dest == DSTR_NULLPTR) { return DSTR_INVALID_ARGUMENT; }

	/* src 为空指针或指向空字符串，均视为复制空字符串。 */
	if (src == DSTR_NULLPTR || src[0] == '\0') {
		return insert_str(dest, 0, dest->len,DSTR_NULLPTR, 0, 0, 0);
	}

	/* 越界检查。 */
	const size_t src_len = strlen(src);
	if (sub_start >= src_len ||
		!safe_size_t_add(sub_start, sub_length, DSTR_NULLPTR) ||
		sub_start + sub_length > src_len
	) { return DSTR_INVALID_ARGUMENT; }

	return insert_str(dest, 0, dest->len, src, src_len, sub_start, sub_length);
}

/* 复制一个「动态字符串」的子串到另一个「动态字符串」。 */
dstr_status_t dstr_cpy_sub(
	dstr_adt *const dest,
	const dstr_adt *const src,
	const size_t sub_start,
	const size_t sub_length
) {
	if (dest == DSTR_NULLPTR) { return DSTR_INVALID_ARGUMENT; }

	/* src 为空指针或指向空字符串，均视为复制空字符串。 */
	if (src == DSTR_NULLPTR || src->len == 0) {
		return insert_str(dest, 0, dest->len,DSTR_NULLPTR, 0, 0, 0);
	}

	/* 越界检查。 */
	if (sub_start >= src->len ||
		!safe_size_t_add(sub_start, sub_length, DSTR_NULLPTR) ||
		sub_start + sub_length > src->len
	) { return DSTR_INVALID_ARGUMENT; }

	return insert_str(dest, 0, dest->len, src->data, src->len, sub_start, sub_length);
}

/* 格式化复制一个字符串到一个「动态字符串」。 */
dstr_status_t dstr_cpy_format(
	dstr_adt *const dest,
	const char *const format,
	...
) {
	if (dest == DSTR_NULLPTR) { return DSTR_INVALID_ARGUMENT; }

	/* format 为空指针或指向空字符串，均视为复制空字符串。 */
	if (format == DSTR_NULLPTR || format[0] == '\0') {
		return insert_str(dest, 0, dest->len, DSTR_NULLPTR, 0, 0, 0);
	}

	va_list args;

	va_start(args, format);
	const dstr_status_t result = insert_str_format(dest, 0, dest->len, format, args);
	va_end(args);

	return result;
}

/* 格式化复制一个字符串到一个「动态字符串」（va_list 版本）。 */
dstr_status_t dstr_cpy_vformat(
	dstr_adt *const dest,
	const char *const format,
	va_list args
) {
	if (dest == DSTR_NULLPTR) { return DSTR_INVALID_ARGUMENT; }

	/* format 为空指针或指向空字符串，均视为复制空字符串。 */
	if (format == DSTR_NULLPTR || format[0] == '\0') {
		return insert_str(dest, 0, dest->len, DSTR_NULLPTR, 0, 0, 0);
	}

	return insert_str_format(dest, 0, dest->len, format, args);
}

/* 追加一个「C 字符串」到一个「动态字符串」。 */
dstr_status_t dstr_cat_cstr(
	dstr_adt *const dest,
	const char *const src
) {
	if (dest == DSTR_NULLPTR) { return DSTR_INVALID_ARGUMENT; }

	/* src 为空指针或指向空字符串，均视为追加空字符串。 */
	if (src == DSTR_NULLPTR || src[0] == '\0') { return DSTR_SUCCESS; }

	return insert_str(dest, dest->len, 0, src, strlen(src), 0, 0);
}

/* 追加一个「动态字符串」到另一个「动态字符串」。 */
dstr_status_t dstr_cat(
	dstr_adt *const dest,
	const dstr_adt *const src
) {
	if (dest == DSTR_NULLPTR) { return DSTR_INVALID_ARGUMENT; }

	/* src 为空指针或指向空字符串，均视为追加空字符串。 */
	if (src == DSTR_NULLPTR || src->len == 0) { return DSTR_SUCCESS; }

	return insert_str(dest, dest->len, 0, src->data, src->len, 0, 0);
}

/* 追加一个「C 字符串」的子串到一个「动态字符串」。 */
dstr_status_t dstr_cat_sub_cstr(
	dstr_adt *const dest,
	const char *const src,
	const size_t sub_start,
	const size_t sub_length
) {
	if (dest == DSTR_NULLPTR) { return DSTR_INVALID_ARGUMENT; }

	/* src 为空指针或指向空字符串，均视为追加空字符串。 */
	if (src == DSTR_NULLPTR || src[0] == '\0') { return DSTR_SUCCESS; }

	/* 越界检查。 */
	const size_t src_len = strlen(src);
	if (sub_start >= src_len ||
		!safe_size_t_add(sub_start, sub_length, DSTR_NULLPTR) ||
		sub_start + sub_length > src_len
	) { return DSTR_INVALID_ARGUMENT; }

	return insert_str(dest, dest->len, 0, src, src_len, sub_start, sub_length);
}

/* 追加一个「动态字符串」的子串到另一个「动态字符串」。 */
dstr_status_t dstr_cat_sub(
	dstr_adt *const dest,
	const dstr_adt *const src,
	const size_t sub_start,
	const size_t sub_length
) {
	if (dest == DSTR_NULLPTR) { return DSTR_INVALID_ARGUMENT; }

	/* src 为空指针或指向空字符串，均视为追加空字符串。 */
	if (src == DSTR_NULLPTR || src->len == 0) { return DSTR_SUCCESS; }

	/* 越界检查。 */
	if (sub_start >= src->len ||
		!safe_size_t_add(sub_start, sub_length, DSTR_NULLPTR) ||
		sub_start + sub_length > src->len
	) { return DSTR_INVALID_ARGUMENT; }

	return insert_str(dest, dest->len, 0, src->data, src->len, sub_start, sub_length);
}

/* 格式化追加一个字符串到一个「动态字符串」。 */
dstr_status_t dstr_cat_format(
	dstr_adt *const dest,
	const char *const format,
	...
) {
	if (dest == DSTR_NULLPTR) { return DSTR_INVALID_ARGUMENT; }

	/* format 为空指针或指向空字符串，均视为追加空字符串。 */
	if (format == DSTR_NULLPTR || format[0] == '\0') { return DSTR_SUCCESS; }

	va_list args;

	va_start(args, format);
	const dstr_status_t result = insert_str_format(dest, dest->len, 0, format, args);
	va_end(args);

	return result;
}

/* 格式化追加一个字符串到一个「动态字符串」（va_list 版本）。 */
dstr_status_t dstr_cat_vformat(
	dstr_adt *const dest,
	const char *const format,
	va_list args
) {
	if (dest == DSTR_NULLPTR) { return DSTR_INVALID_ARGUMENT; }

	/* format 为空指针或指向空字符串，均视为追加空字符串。 */
	if (format == DSTR_NULLPTR || format[0] == '\0') { return DSTR_SUCCESS; }

	return insert_str_format(dest, dest->len, 0, format, args);
}

/* 插入一个「C 字符串」到一个「动态字符串」。 */
dstr_status_t dstr_insert_cstr(
	dstr_adt *const dest,
	const size_t index,
	const char *const src
) {
	if (dest == DSTR_NULLPTR || index > dest->len) { return DSTR_INVALID_ARGUMENT; }

	/* src 为空指针或指向空字符串，均视为插入空字符串。 */
	if (src == DSTR_NULLPTR || src[0] == '\0') { return DSTR_SUCCESS; }

	return insert_str(dest, index, 0, src, strlen(src), 0, 0);
}

/* 插入一个「动态字符串」到另一个「动态字符串」。 */
dstr_status_t dstr_insert(
	dstr_adt *const dest,
	const size_t index,
	const dstr_adt *const src
) {
	if (dest == DSTR_NULLPTR || index > dest->len) { return DSTR_INVALID_ARGUMENT; }

	/* src 为空指针或指向空字符串，均视为插入空字符串。 */
	if (src == DSTR_NULLPTR || src->len == 0) { return DSTR_SUCCESS; }

	return insert_str(dest, index, 0, src->data, src->len, 0, 0);
}

/* 插入一个「C 字符串」的子串到一个「动态字符串」。 */
dstr_status_t dstr_insert_sub_cstr(
	dstr_adt *const dest,
	const size_t index,
	const char *const src,
	const size_t sub_start,
	const size_t sub_length
) {
	if (dest == DSTR_NULLPTR || index > dest->len) { return DSTR_INVALID_ARGUMENT; }

	/* src 为空指针或指向空字符串，均视为插入空字符串。 */
	if (src == DSTR_NULLPTR || src[0] == '\0') { return DSTR_SUCCESS; }

	/* 越界检查。 */
	const size_t src_len = strlen(src);
	if (sub_start >= src_len ||
		!safe_size_t_add(sub_start, sub_length, DSTR_NULLPTR) ||
		sub_start + sub_length > src_len
	) { return DSTR_INVALID_ARGUMENT; }

	return insert_str(dest, index, 0, src, src_len, sub_start, sub_length);
}

/* 插入一个「动态字符串」的子串到另一个「动态字符串」。 */
dstr_status_t dstr_insert_sub(
	dstr_adt *const dest,
	const size_t index,
	const dstr_adt *const src,
	const size_t sub_start,
	const size_t sub_length
) {
	if (dest == DSTR_NULLPTR || index > dest->len) { return DSTR_INVALID_ARGUMENT; }

	/* src 为空指针或指向空字符串，均视为插入空字符串。 */
	if (src == DSTR_NULLPTR || src->len == 0) { return DSTR_SUCCESS; }

	/* 越界检查。 */
	if (sub_start >= src->len ||
		!safe_size_t_add(sub_start, sub_length, DSTR_NULLPTR) ||
		sub_start + sub_length > src->len
	) { return DSTR_INVALID_ARGUMENT; }

	return insert_str(dest, index, 0, src->data, src->len, sub_start, sub_length);
}

/* 格式化插入一个字符串到一个「动态字符串」。 */
dstr_status_t dstr_insert_format(
	dstr_adt *const dest,
	const size_t index,
	const char *const format,
	...
) {
	if (dest == DSTR_NULLPTR || index > dest->len) { return DSTR_INVALID_ARGUMENT; }

	/* format 为空指针或指向空字符串，均视为插入空字符串。 */
	if (format == DSTR_NULLPTR || format[0] == '\0') { return DSTR_SUCCESS; }

	va_list args;

	va_start(args, format);
	const dstr_status_t result = insert_str_format(dest, index, 0, format, args);
	va_end(args);

	return result;
}

/* 格式化插入一个字符串到一个「动态字符串」（va_list 版本）。 */
dstr_status_t dstr_insert_vformat(
	dstr_adt *const dest,
	const size_t index,
	const char *const format,
	va_list args
) {
	if (dest == DSTR_NULLPTR || index > dest->len) { return DSTR_INVALID_ARGUMENT; }

	/* format 为空指针或指向空字符串，均视为插入空字符串。 */
	if (format == DSTR_NULLPTR || format[0] == '\0') { return DSTR_SUCCESS; }

	return insert_str_format(dest, index, 0, format, args);
}

/* 清空一个「动态字符串」。 */
void dstr_clear(
	dstr_adt *const dstr
) {
	if (dstr == DSTR_NULLPTR) { return; }

	dstr->len = 0;

	if (dstr->data != DSTR_NULLPTR) {
		dstr->data[0] = '\0';
	}
}

/* 删除一个「动态字符串」的子串。 */
void dstr_remove(
	dstr_adt *const dstr,
	const size_t sub_start,
	const size_t sub_length
) {
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		sub_start >= dstr->len ||
		!safe_size_t_add(sub_start, sub_length, DSTR_NULLPTR) ||
		sub_start + sub_length > dstr->len
	) { return; }

	const size_t remove_length = (sub_length > 0) ? sub_length : (dstr->len - sub_start);
	insert_str(dstr, sub_start, remove_length, DSTR_NULLPTR, 0, 0, 0);
}

/* 删除一个「动态字符串」首尾的空白字符或指定字符。 */
void dstr_trim(
	dstr_adt *const dstr,
	const char *const trim_chars
) {
	if (dstr == DSTR_NULLPTR || dstr->len == 0) { return; }

	/* trim_chars 为空指针或指向空字符串时，均视为没有指定字符。 */
	const bool is_specified_trim_chars = (trim_chars != DSTR_NULLPTR && trim_chars[0] != '\0');

	/* 用于迭代和区间定位。 */
	const char *p = dstr->data;
	const char *q = p + dstr->len;

	/* 定位剩余区间。 */
	if (is_specified_trim_chars) {
		while (p < q && strchr(trim_chars, *p) != DSTR_NULLPTR) {
			++p;
		}

		while (q > p && strchr(trim_chars, *(q - 1)) != DSTR_NULLPTR) {
			--q;
		}
	} else {
		while (isspace((unsigned char)*p)) {
			++p;
		}

		while (q > p && isspace((unsigned char)*(q - 1))) {
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

	resize_capacity_dynamic(dstr, (dstr->len > 0) ? (dstr->len + 1) : 0);

	if (dstr->data != DSTR_NULLPTR) {
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
	) { return false; }

	const size_t prefix_len = strlen(prefix);
	if (prefix_len > dstr->len) { return false; }

	return (memcmp(dstr->data, prefix, prefix_len) == 0);
}

/* 判断一个「动态字符串」是否以指定「动态字符串」前缀开头。 */
bool dstr_starts_with(
	const dstr_adt *const dstr,
	const dstr_adt *const prefix
) {
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		prefix == DSTR_NULLPTR || prefix->len == 0
	) { return false; }

	if (prefix->len > dstr->len) { return false; }

	return (memcmp(dstr->data, prefix->data, prefix->len) == 0);
}

/* 判断一个「动态字符串」是否以指定「C 字符串」后缀结尾。 */
bool dstr_ends_with_cstr(
	const dstr_adt *const dstr,
	const char *const suffix
) {
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		suffix == DSTR_NULLPTR || suffix[0] == '\0'
	) { return false; }

	const size_t suffix_len = strlen(suffix);
	if (suffix_len > dstr->len) { return false; }

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
	) { return false; }

	if (suffix->len > dstr->len) { return false; }

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
	) { return false; }

	const size_t sub_len = strlen(sub);
	if (sub_len > dstr->len) { return false; }

	return (strstr(dstr->data, sub) != DSTR_NULLPTR);
}

/* 判断一个「动态字符串」是否包含指定子「动态字符串」。 */
bool dstr_contains(
	const dstr_adt *const dstr,
	const dstr_adt *const sub
) {
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		sub == DSTR_NULLPTR || sub->len == 0
	) { return false; }

	if (sub->len > dstr->len) { return false; }

	return (strstr(dstr->data, sub->data) != DSTR_NULLPTR);
}

/* 判断一个「动态字符串」是否与一个「C 字符串」相等。 */
bool dstr_equals_cstr(
	const dstr_adt *const lhs,
	const char *const rhs
) {
	const int str_1_valid = (lhs != DSTR_NULLPTR && lhs->len > 0) ? 1 : 0;
	const int str_2_valid = (rhs != DSTR_NULLPTR && rhs[0] != '\0') ? 1 : 0;

	if (str_1_valid + str_2_valid < 2) {
		return (str_1_valid == str_2_valid);
	}

	const size_t rhs_len = strlen(rhs);
	if (lhs->len != rhs_len) { return false; }

	return (memcmp(lhs->data, rhs, rhs_len) == 0);
}

/* 判断两个「动态字符串」是否相等。 */
bool dstr_equals(
	const dstr_adt *const lhs,
	const dstr_adt *const rhs
) {
	const int str_1_valid = (lhs != DSTR_NULLPTR && lhs->len > 0) ? 1 : 0;
	const int str_2_valid = (rhs != DSTR_NULLPTR && rhs->len > 0) ? 1 : 0;

	if (str_1_valid + str_2_valid < 2) {
		return (str_1_valid == str_2_valid);
	}

	if (lhs->len != rhs->len) { return false; }

	return (memcmp(lhs->data, rhs->data, rhs->len) == 0);
}

/* 比较一个「动态字符串」与一个「C 字符串」。 */
int dstr_compare_cstr(
	const dstr_adt *const lhs,
	const char *const rhs
) {
	const int str_1_valid = (lhs != DSTR_NULLPTR && lhs->len > 0) ? 1 : 0;
	const int str_2_valid = (rhs != DSTR_NULLPTR && rhs[0] != '\0') ? 1 : 0;

	if (str_1_valid + str_2_valid < 2) {
		return str_1_valid - str_2_valid;
	}

	return strcmp(lhs->data, rhs);
}

/* 比较两个「动态字符串」。 */
int dstr_compare(
	const dstr_adt *const lhs,
	const dstr_adt *const rhs
) {
	const int str_1_valid = (lhs != DSTR_NULLPTR && lhs->len > 0) ? 1 : 0;
	const int str_2_valid = (rhs != DSTR_NULLPTR && rhs->len > 0) ? 1 : 0;

	if (str_1_valid + str_2_valid < 2) {
		return str_1_valid - str_2_valid;
	}

	return strcmp(lhs->data, rhs->data);
}

/* 查找、统计与替换。 */

/* 查找一个「动态字符串」中指定子「C 字符串」首次出现的位置。 */
bool dstr_find_cstr(
	const dstr_adt *const dstr,
	const char *const sub,
	size_t *const out_index,
	const dstr_direction_t direction
) {
	/* 参数合法性检查。 */
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		sub == DSTR_NULLPTR || sub[0] == '\0'
	) { return false; }

	return (find_str(
		dstr->data, dstr->len,
		sub, strlen(sub),
		out_index, DSTR_NULLPTR,
		direction, 1
	) > 0);
}

/* 查找一个「动态字符串」中指定子「动态字符串」首次出现的位置。 */
bool dstr_find(
	const dstr_adt *const dstr,
	const dstr_adt *const sub,
	size_t *const out_index,
	const dstr_direction_t direction
) {
	/* 参数合法性检查。 */
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		sub == DSTR_NULLPTR || sub->len == 0
	) { return false; }

	return (find_str(
		dstr->data, dstr->len,
		sub->data, sub->len,
		out_index, DSTR_NULLPTR,
		direction, 1
	) > 0);
}

/* 查找一个「动态字符串」中指定子「C 字符串」第 n 次出现的位置。 */
bool dstr_find_nth_cstr(
	const dstr_adt *const dstr,
	const char *const sub,
	size_t *const out_index,
	const dstr_direction_t direction,
	const size_t n
) {
	/* 参数合法性检查。 */
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		sub == DSTR_NULLPTR || sub[0] == '\0'
	) { return false; }

	return (find_str(
		dstr->data, dstr->len,
		sub, strlen(sub),
		out_index, DSTR_NULLPTR,
		direction, n
	) > 0);
}

/* 查找一个「动态字符串」中指定子「动态字符串」第 n 次出现的位置。 */
bool dstr_find_nth(
	const dstr_adt *const dstr,
	const dstr_adt *const sub,
	size_t *const out_index,
	const dstr_direction_t direction,
	const size_t n
) {
	/* 参数合法性检查。 */
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		sub == DSTR_NULLPTR || sub->len == 0
	) { return false; }

	return (find_str(
		dstr->data, dstr->len,
		sub->data, sub->len,
		out_index, DSTR_NULLPTR,
		direction, n
	) > 0);
}

/* 查找一个「动态字符串」中指定子「C 字符串」前 n 次出现的位置。 */
size_t dstr_find_indexes_cstr(
	const dstr_adt *const dstr,
	const char *const sub,
	size_t *const out_indexes,
	const dstr_direction_t direction,
	const size_t n
) {
	/* 参数合法性检查。 */
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		sub == DSTR_NULLPTR || sub[0] == '\0'
	) { return 0; }

	return find_str(
		dstr->data, dstr->len,
		sub, strlen(sub),
		DSTR_NULLPTR, out_indexes,
		direction, n
	);
}

/* 查找一个「动态字符串」中指定子「动态字符串」前 n 次出现的位置。 */
size_t dstr_find_indexes(
	const dstr_adt *const dstr,
	const dstr_adt *const sub,
	size_t *const out_indexes,
	const dstr_direction_t direction,
	const size_t n
) {
	/* 参数合法性检查。 */
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		sub == DSTR_NULLPTR || sub->len == 0
	) { return 0; }

	return find_str(
		dstr->data, dstr->len,
		sub->data, sub->len,
		DSTR_NULLPTR, out_indexes,
		direction, n
	);
}

/* 统计一个「动态字符串」中指定子「C 字符串」出现的次数。 */
size_t dstr_count_cstr(
	const dstr_adt *const dstr,
	const char *const sub
) {
	/* 参数合法性检查。 */
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		sub == DSTR_NULLPTR || sub[0] == '\0'
	) { return 0; }

	return find_str(
		dstr->data, dstr->len,
		sub, strlen(sub),
		DSTR_NULLPTR, DSTR_NULLPTR,
		DSTR_DIR_FORWARD, 0
	);
}

/* 统计一个「动态字符串」中指定子「动态字符串」出现的次数。 */
size_t dstr_count(
	const dstr_adt *const dstr,
	const dstr_adt *const sub
) {
	/* 参数合法性检查。 */
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		sub == DSTR_NULLPTR || sub->len == 0
	) { return 0; }

	return find_str(
		dstr->data, dstr->len,
		sub->data, sub->len,
		DSTR_NULLPTR, DSTR_NULLPTR,
		DSTR_DIR_FORWARD, 0
	);
}

/* 替换一个「动态字符串」中指定旧「C 字符串」为指定新「C 字符串」n 次。 */
dstr_status_t dstr_replace_cstr(
	dstr_adt *const dstr,
	const char *const old_str,
	const char *const new_str,
	const dstr_direction_t direction,
	const size_t n
) {
	/* 参数合法性检查。 */
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		old_str == DSTR_NULLPTR || old_str[0] == '\0'
	) { return DSTR_INVALID_ARGUMENT; }

	if (new_str == DSTR_NULLPTR || new_str[0] == '\0') {
		return replace_str(
			dstr,
			old_str, strlen(old_str),
			DSTR_NULLPTR, 0,
			direction, n
		);
	}

	return replace_str(
		dstr,
		old_str, strlen(old_str),
		new_str, strlen(new_str),
		direction, n
	);
}

/* 替换一个「动态字符串」中指定旧「动态字符串」为指定新「动态字符串」n 次。 */
dstr_status_t dstr_replace(
	dstr_adt *const dstr,
	const dstr_adt *const old_str,
	const dstr_adt *const new_str,
	const dstr_direction_t direction,
	const size_t n
) {
	/* 参数合法性检查。 */
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		old_str == DSTR_NULLPTR || old_str->len == 0
	) { return DSTR_INVALID_ARGUMENT; }

	if (new_str == DSTR_NULLPTR || new_str->len == 0) {
		return replace_str(
			dstr,
			old_str->data, old_str->len,
			DSTR_NULLPTR, 0,
			direction, n
		);
	}

	return replace_str(
		dstr,
		old_str->data, old_str->len,
		new_str->data, new_str->len,
		direction, n
	);
}

/* 分隔与合并。 */

/* 分隔一个「C 字符串」为多个「动态字符串」。 */
dstr_adt **dstr_split_cstr(
	const char *const cstr,
	const char *const separator,
	size_t *const out_dstr_count
) {
	/* 参数合法性检查。 */
	if (cstr == DSTR_NULLPTR || cstr[0] == '\0' ||
		separator == DSTR_NULLPTR || separator[0] == '\0' ||
		out_dstr_count == DSTR_NULLPTR
	) { return DSTR_NULLPTR; }

	return split_str(
		cstr, strlen(cstr),
		separator, strlen(separator),
		out_dstr_count
	);
}

/* 分隔一个「动态字符串」为多个「动态字符串」。 */
dstr_adt **dstr_split(
	const dstr_adt *const dstr,
	const dstr_adt *const separator,
	size_t *const out_dstr_count
) {
	/* 参数合法性检查。 */
	if (dstr == DSTR_NULLPTR || dstr->len == 0 ||
		separator == DSTR_NULLPTR || separator->len == 0 ||
		out_dstr_count == DSTR_NULLPTR
	) { return DSTR_NULLPTR; }

	return split_str(
		dstr->data, dstr->len,
		separator->data, separator->len,
		out_dstr_count
	);
}

/* 合并多个「C 字符串」为一个「动态字符串」。 */
dstr_adt *dstr_join_cstr(
	const char *const *const cstrs,
	const size_t cstr_count,
	const char *const separator
) {
	/* 参数合法性检查。 */
	if (cstrs == DSTR_NULLPTR || cstr_count == 0) { return DSTR_NULLPTR; }

	if (separator == DSTR_NULLPTR || separator[0] == '\0') {
		return join_str(
			cstrs, DSTR_NULLPTR,
			cstr_count,
			DSTR_NULLPTR, 0
		);
	}

	return join_str(
		cstrs, DSTR_NULLPTR,
		cstr_count,
		separator, strlen(separator)
	);
}

/* 合并多个「动态字符串」为一个「动态字符串」。 */
dstr_adt *dstr_join(
	const dstr_adt *const *const dstrs,
	const size_t dstr_count,
	const dstr_adt *const separator
) {
	/* 参数合法性检查。 */
	if (dstrs == DSTR_NULLPTR || dstr_count == 0) { return DSTR_NULLPTR; }

	if (separator == DSTR_NULLPTR || separator->len == 0) {
		return join_str(
			DSTR_NULLPTR, dstrs,
			dstr_count,
			DSTR_NULLPTR, 0
		);
	}

	return join_str(
		DSTR_NULLPTR, dstrs,
		dstr_count,
		separator->data, separator->len
	);
}


/*------------------------------------------------------------------------------
 * 静态函数定义
 *----------------------------------------------------------------------------*/

/* 调整一个「动态字符串」的容量（基础版）。 */
static bool resize_capacity(
	dstr_adt *const dstr,
	const size_t new_cap
) {
	char *const new_data = realloc(dstr->data, new_cap);
	if (new_data == DSTR_NULLPTR) { return false; }

	dstr->data = new_data;
	dstr->cap = new_cap;

	return true;
}

/* 调整一个「动态字符串」的容量（常规版）。 */
static bool resize_capacity_regular(
	dstr_adt *const dstr,
	const size_t new_cap
) {
	if (new_cap == dstr->cap) { return true; }

	if (new_cap == 0) {
		free(dstr->data);
		dstr->data = DSTR_NULLPTR;
		dstr->cap = 0;

		return true;
	}

	return resize_capacity(dstr, new_cap);
}

/* 调整一个「动态字符串」的容量（动态版）。 */
static bool resize_capacity_dynamic(
	dstr_adt *const dstr,
	const size_t new_cap
) {
	/* 对齐到缓存行大小的容量值。 */
	size_t aligned_cap = 0;

	/**
	 * 目标容量。
	 * 确保不低于 dstr 的容量保底值。
	 * 因此取所需容量 new_cap 和 dstr 的容量保底值 dstr->min_cap 的最大值。
	 */
	const size_t target_cap = (new_cap > dstr->min_cap) ? new_cap : dstr->min_cap;

	/* 目标容量与当前容量相等，无需调整，直接返回 true。 */
	if (target_cap == dstr->cap) { return true; }

	/**
	 * 如果目标容量小于当前容量，则延迟减容/几何缩容。
	 * 仅当目标容量小于等于当前容量的 1/4 时，才实际减容。
	 * 因此，当目标容量小于当前容量，且大于当前容量的 1/4 时，直接返回 true。
	 */
	if (target_cap < dstr->cap && target_cap > (dstr->cap >> 2)) { return true; }

	/* 目标容量为 0，单独释放。 */
	if (target_cap == 0) {
		free(dstr->data);
		dstr->data = DSTR_NULLPTR;
		dstr->cap = 0;

		return true;
	}

	/* 如果目标容量大于当前容量，则执行容量预分配、缓存行对齐。 */
	if (target_cap > dstr->cap) {
		/* 预分配后的容量值。 */
		size_t adjusted_cap = 0;

		/* 尝试预分配内存。 */
		if (safe_size_t_add(target_cap, target_cap >> 1, &adjusted_cap)) {
			/* 尝试对齐到缓存行大小。 */
			if (safe_size_t_align_up(adjusted_cap,DSTR_CACHELINE_SIZE, &aligned_cap)) {
				/* 尝试调整 aligned_cap。 */
				if (resize_capacity(dstr, aligned_cap)) { return true; }
			}

			/**
			 * 对齐溢出，或调整 aligned_cap 失败。
			 * 此时，仅在 aligned_cap 与 adjusted_cap 不相等时，尝试调整 adjusted_cap。
			 */
			if (aligned_cap != adjusted_cap) {
				if (resize_capacity(dstr, adjusted_cap)) { return true; }
			}
		}

		/**
		 * 预分配溢出，或调整 aligned_cap、adjusted_cap 都失败。
		 * 此时，仅在 adjusted_cap 与 target_cap 不相等时，尝试调整 target_cap。
		 */
		if (adjusted_cap != target_cap) {
			if (resize_capacity(dstr, target_cap)) { return true; }
		}

		/* 上述尝试都失败，则返回 false。 */
		return false;
	}

	/* 目标容量小于等于当前容量的 1/4，执行减容。 */

	/* 尝试对齐到缓存行大小。 */
	if (safe_size_t_align_up(target_cap, DSTR_CACHELINE_SIZE, &aligned_cap)) {
		/**
		 * aligned_cap 有可能与当前容量相等。
		 * 如果相等，则直接返回 true。
		 */
		if (aligned_cap == dstr->cap) { return true; }

		/* 尝试调整 aligned_cap。 */
		if (resize_capacity(dstr, aligned_cap)) { return true; }
	}

	/**
	 * 对齐溢出，或调整 aligned_cap 失败。
	 * 此时，仅在 aligned_cap 与 target_cap 不相等时，尝试调整 target_cap。
	 */
	if (aligned_cap != target_cap) {
		if (resize_capacity(dstr, target_cap)) { return true; }
	}

	/* 上述尝试都失败，则返回 false。 */
	return false;
}

/* 从一个「C 字符串」的子串创建一个新「动态字符串」。 */
static dstr_adt *create_dstr(
	const char *const src,
	const size_t src_len,
	const size_t sub_start,
	const size_t sub_length
) {
	dstr_adt *const new_dstr = malloc(sizeof(dstr_adt));
	if (new_dstr == DSTR_NULLPTR) { return DSTR_NULLPTR; }

	/* 初始化成员变量 data 为空指针。capacity_resize 操作需要。 */
	new_dstr->data = DSTR_NULLPTR;

	/* capacity_resize 操作如果成功，会更新成员变量 data、cap。 */
	if (src_len > 0) {
		const size_t copy_len = (sub_length > 0) ? sub_length : (src_len - sub_start);

		if (!safe_size_t_add(copy_len, 1, DSTR_NULLPTR) ||
			!resize_capacity(new_dstr, copy_len + 1)
		) {
			free(new_dstr);
			return DSTR_NULLPTR;
		}

		memcpy(new_dstr->data, src + sub_start, copy_len);

		/* 更新成员变量 len 并在结尾补 '\0'。 */
		new_dstr->len = copy_len;
		new_dstr->data[copy_len] = '\0';
	} else {
		/* 在此分支中，手动初始化成员变量 cap、len。 */
		new_dstr->cap = 0;
		new_dstr->len = 0;
	}

	new_dstr->min_cap = 0;
	return new_dstr;
}

/* 向一个「动态字符串」的指定位置处插入一个「C 字符串」的子串。 */
static dstr_status_t insert_str(
	dstr_adt *const dest,
	const size_t index,
	const size_t remove_length,
	const char *const src,
	const size_t src_len,
	const size_t sub_start,
	const size_t sub_length
) {
	const size_t copy_len = (src_len > 0)
		? ((sub_length > 0) ? (sub_length) : (src_len - sub_start))
		: 0;
	const size_t new_len = dest->len - remove_length + copy_len;

	/* 当新长度大于当前长度时尝试扩容。 */
	if (new_len > dest->len) {
		if (!safe_size_t_add(new_len, 1, DSTR_NULLPTR) ||
			!resize_capacity_dynamic(dest, new_len + 1)
		) { return DSTR_MEMORY_ALLOC_FAILED; }
	}

	/* 当存在需要移动的尾部数据时，执行移动。 */
	const size_t tail_len = dest->len - index - remove_length;
	if (tail_len > 0 && remove_length != copy_len) {
		memmove(
			dest->data + index + copy_len,
			dest->data + index + remove_length,
			tail_len
		);
	}

	/* 当存在需要拷贝的数据时，执行拷贝。 */
	if (copy_len > 0) {
		memcpy(dest->data + index, src + sub_start, copy_len);
	}

	/* 如果新长度小于当前长度，则在操作执行完后，尝试缩容。 */
	if (new_len < dest->len) {
		/* 当长度为 0 时，所需容量为 0；当长度不为 0 时，所需容量为长度 + 1。 */
		resize_capacity_dynamic(dest, (new_len > 0) ? (new_len + 1) : 0);
	}

	/* 如果长度有变化，则更新长度。 */
	if (new_len != dest->len) {
		dest->len = new_len;

		/**
		 * 当新的长度为 0 时，容量有可能为 0，dest->data 为空指针。
		 * 为避免空指针解引用，仅当容量不为 0 时，于 dest->data[new_len] 处写入 '\0'。
		 */
		if (dest->data != DSTR_NULLPTR) {
			dest->data[new_len] = '\0';
		}
	}

	return DSTR_SUCCESS;
}

/* 向一个「动态字符串」的指定位置处格式化插入一个字符串。 */
static dstr_status_t insert_str_format(
	dstr_adt *const dstr,
	const size_t index,
	const size_t remove_length,
	const char *const format,
	va_list args
) {
	va_list temp_args;

	va_copy(temp_args, args);
	const int temp_len = vsnprintf(DSTR_NULLPTR, 0, format, temp_args);
	va_end(temp_args);

	if (temp_len < 0) { return DSTR_INVALID_ARGUMENT; }

	const size_t format_len = temp_len;
	const size_t new_len = dstr->len - remove_length + format_len;

	/* 当新长度大于当前长度时尝试扩容。 */
	if (new_len > dstr->len) {
		if (!safe_size_t_add(new_len, 1, DSTR_NULLPTR) ||
			!resize_capacity_dynamic(dstr, new_len + 1)
		) { return DSTR_MEMORY_ALLOC_FAILED; }
	}

	/* 当存在需要移动的尾部数据时，执行移动。 */
	const size_t tail_len = dstr->len - index - remove_length;
	if (tail_len > 0 && remove_length != format_len) {
		memmove(
			dstr->data + index + format_len,
			dstr->data + index + remove_length,
			tail_len
		);
	}

	/* 当存在需要格式化的数据时，格式化写入到 dstr->data。 */
	if (format_len > 0) {
		/**
	 	 * vsnprintf 写入后会在末尾强制写入 '\0'。
	 	 * 对于插入操作（tail_len > 0），该 '\0' 会覆盖被搬移到 format_len 末尾的第一个尾部字符。
	 	 * 因此，在写入前暂存该字符，写入后恢复。
	 	 */
		char saved_char;
		if (tail_len > 0) {
			saved_char = dstr->data[index + format_len];
		}

		vsnprintf(dstr->data + index, format_len + 1, format, args);

		/* 恢复被 '\0' 覆盖的字符。 */
		if (tail_len > 0) {
			dstr->data[index + format_len] = saved_char;
		}
	}

	/* 如果新长度小于当前长度，则在操作执行完后尝试缩容。 */
	if (new_len < dstr->len) {
		resize_capacity_dynamic(dstr, (new_len > 0) ? (new_len + 1) : 0);
	}

	/* 如果长度有变化，则更新长度。 */
	if (new_len != dstr->len) {
		dstr->len = new_len;

		if (dstr->data != DSTR_NULLPTR) {
			dstr->data[new_len] = '\0';
		}
	}

	return DSTR_SUCCESS;
}

/* 查找一个「C 字符串」中，指定子「C 字符串」第 n 次出现的位置。 */
static size_t find_str(
	const char *const cstr,
	const size_t cstr_len,
	const char *const sub,
	const size_t sub_len,
	size_t *const out_index,
	size_t *const out_indexes,
	const dstr_direction_t direction,
	const size_t n
) {
	/* 如果 sub 长度大于 cstr 长度，则直接返回 0，避免越界访问。 */
	if (sub_len > cstr_len) { return 0; }

	/* 用于迭代的指针变量。 */
	const char *p;
	/* 用于存储当前出现位置的指针。 */
	const char *find = DSTR_NULLPTR;
	/* 用于统计出现的次数。 */
	size_t find_count = 0;

	/* 指针常量，指向查找区间的后一个位置。用于迭代边界。 */
	const char *const end = cstr + cstr_len - sub_len + 1;

	/**
	 * 分块执行正向与逆向查找。
	 * 避免每次循环中都存在条件判断。
	 * 增加分支开销。
	 */
	if (direction == DSTR_DIR_BACKWARD) {
		p = end;

		while (p > cstr) {
			const char *const cur = p - 1;

			if (memcmp(cur, sub, sub_len) == 0) {
				find = cur;

				if (out_indexes != DSTR_NULLPTR) {
					out_indexes[find_count] = cur - cstr;
				}

				if (++find_count == n) { break; }

				if ((p - cstr) > sub_len) {
					p -= sub_len;
				} else { break; }
			} else {
				--p;
			}
		}
	} else {
		p = cstr;

		while (p < end) {
			if (memcmp(p, sub, sub_len) == 0) {
				find = p;

				if (out_indexes != DSTR_NULLPTR) {
					out_indexes[find_count] = find - cstr;
				}

				if (++find_count == n) { break; }

				p += sub_len;
			} else {
				++p;
			}
		}
	}

	if (out_index != DSTR_NULLPTR && find != DSTR_NULLPTR) {
		*out_index = find - cstr;
	}

	return find_count;
}

/* 将一个「动态字符串」中指定的旧「C 字符串」替换为指定的新「C 字符串」。 */
static dstr_status_t replace_str(
	dstr_adt *const dstr,
	const char *const old_str,
	const size_t old_str_len,
	const char *const new_str,
	const size_t new_str_len,
	const dstr_direction_t direction,
	const size_t n
) {
	/* old_str 在 dstr 中最多可能出现的次数。 */
	const size_t max_possible = dstr->len / old_str_len;
	/* 提前返回：不可能有匹配（max_possible == 0）或 n 超出理论上界。 */
	if (max_possible == 0 || n > max_possible) { return DSTR_INVALID_ARGUMENT; }

	/* 指向“用于存储 old_str 在 dstr 中出现的位置的动态数组”的指针。 */
	size_t *indexes = DSTR_NULLPTR;
	/* old_str 在 dstr 中出现的实际次数。 */
	size_t old_str_count = 0;

	/**
	 * 最优路径下（用空间换时间），indexes 数组的元素个数。
	 * 当 n 不为 0，即用户指定了替换的次数时，为 n 个，否则为 max_possible;
	 */
	const size_t indexes_count = (n > 0) ? n : max_possible;
	/* 尝试分配 indexes_count 个元素的动态数组。 */
	/* 【indexes_count * sizeof(size_t)】不会溢出。 */
	const bool indexes_count_ok = safe_size_t_mul(indexes_count, sizeof(size_t), DSTR_NULLPTR);
	if (indexes_count_ok) {
		indexes = malloc(indexes_count * sizeof(size_t));
	}

	/**
	 * 如果分配失败，回退到次优路径，先统计一次实际出现的次数（old_str_count），
	 * 然后分配 old_str_count 个元素的动态数组。
	 */
	if (indexes == DSTR_NULLPTR) {
		old_str_count = find_str(
			dstr->data, dstr->len,
			old_str, old_str_len,
			DSTR_NULLPTR, DSTR_NULLPTR,
			direction, n
		);

		/* 如果 old_str 实际出现次数为 0 次或不足 n 次，视为参数不合法并返回。 */
		if (old_str_count == 0 || old_str_count < n) { return DSTR_INVALID_ARGUMENT; }

		/**
		 * 尝试分配 old_str_count 个元素的动态数组。
		 * 如果【indexes_count * sizeof(size_t)】不会溢出，
		 * 那【old_str_count * sizeof(size_t)】也一定不会溢出，
		 * 因为 old_str_count <= indexes_count。
		 * 如果 old_str_count == indexes_count，那就没有尝试的必要了。
		 */
		if (old_str_count < indexes_count && (indexes_count_ok ||
			safe_size_t_mul(old_str_count, sizeof(size_t), DSTR_NULLPTR)
		)) {
			indexes = malloc(old_str_count * sizeof(size_t));
		}
	}

	/* 此时，如果 indexes 不是空指针，则进行二次/一次查找以记录出现位置。*/
	if (indexes != DSTR_NULLPTR) {
		/* 二次查找（次优路径下）或一次查找（最优路径下），记录出现的位置。 */
		const size_t temp_count = find_str(
			dstr->data, dstr->len,
			old_str, old_str_len,
			DSTR_NULLPTR, indexes,
			direction, n
		);

		/* 如果 old_str_count 为 0，则说明走的是最优路径。 */
		if (old_str_count == 0) {
			/* 此时，由于是第一次查找，需要对统计次数做判断。 */
			if (temp_count == 0 || temp_count < n) {
				free(indexes);
				return DSTR_INVALID_ARGUMENT;
			}

			/* 此时，分配的数组大小有可能远大于需要，因此需要按需缩容以释放多余空间。 */
			/**
			 * 缩容策略：双重阈值，绝对阈值与相对阈值。
			 * 绝对阈值：多余内存大小大于 INDEXES_SHRINK_BYTES 时，
			 * 即多余元素个数大于 INDEXES_SHRINK_BYTES / sizeof(size_t) 时，才缩容；
			 * 相对阈值：利用率小于 50%，即，实际元素个数小于 indexes_count 的一半时，才缩容。
			 * 双重阈值：以上两条件同时满足时才缩容。
			 */
			if (temp_count < (indexes_count >> 1) &&
				indexes_count - temp_count > INDEXES_SHRINK_BYTES / sizeof(size_t)
			) {
				size_t *const temp_data = realloc(indexes, temp_count * sizeof(size_t));

				if (temp_data != DSTR_NULLPTR) {
					indexes = temp_data;
				}
			}

			/* 更新 old_str_count 的值。 */
			old_str_count = temp_count;
		}
	}

	/* new_str 与 old_str 长度比较情况。 */
	const int new_cmp_old = (new_str_len > old_str_len)
		? 1
		: ((new_str_len < old_str_len) ? -1 : 0);
	/* new_str_len 与 old_str_len 差的绝对值。 */
	const size_t len_diff = (new_cmp_old > 0)
		? (new_str_len - old_str_len)
		: (old_str_len - new_str_len);

	/* 替换完成后新的字符串长度。 */
	size_t new_len;

	/* 扩容缓冲区。 */
	if (new_cmp_old > 0) {
		/* 新增的长度。 */
		size_t increased_length;

		if (
			/* 安全计算 size_t 乘法（len_diff * old_str_count），防止溢出。 */
			!safe_size_t_mul(len_diff, old_str_count, &increased_length) ||
			/* 安全计算 size_t 加法（dstr->len + increased_length），防止溢出。 */
			!safe_size_t_add(dstr->len, increased_length, &new_len) ||
			/* 安全计算 size_t 加法（new_len + 1），防止溢出。 */
			!safe_size_t_add(new_len, 1, DSTR_NULLPTR) ||
			/* 调整容量。 */
			!resize_capacity_dynamic(dstr, new_len + 1)
		) {
			free(indexes);
			return DSTR_MEMORY_ALLOC_FAILED;
		}
	}

	/* 此时，如果 indexes 是空指针，则放弃空间换时间，回退到最坏路径。*/
	if (indexes == DSTR_NULLPTR) { goto worst_case; }

	/**
	 * 执行替换。
	 * 当 new_str_len > old_str_len 时，需要先搬移后面的数据，防止覆盖，
	 * 即遍历时要从大索引到小索引，而 find_str 写入索引的大小顺序取决于查找方向 direction，
	 * 当从前往后遍历时，索引按从小到大顺序写入，反之从大到小的顺序写入；
	 * 因此，当 new_str_len > old_str_len 时，如果索引是按从大到小顺序写入的，
	 * 即 (direction == DSTR_DIR_BACKWARD)，那么就要正向遍历数组，
	 * 由此总结规律，当（new_str_len > old_str_len）与（direction == DSTR_DIR_BACKWARD）同为真时，
	 * 正向遍历数组，否则逆向遍历。
	 */
	if ((new_cmp_old > 0) == (direction == DSTR_DIR_BACKWARD)) {
		for (size_t i = 0; i < old_str_count; ++i) {
			/* 如果 new_str 与 old_str 长度不等，则需移动数据。 */
			if (new_cmp_old != 0) {
				/* 被搬移数据的起始索引。 */
				const size_t move_start = indexes[i] + old_str_len;
				/**
				 * 被搬移数据的结束索引。
				 * 如果此块被搬移数据，是最后一块（从前往后），
				 * 那么值应该为 dstr->len，否则就是后一次 old_str 出现位置。
				 */
				const size_t move_end = (direction == DSTR_DIR_BACKWARD)
					? ((i == 0) ? dstr->len : indexes[i - 1])
					: ((i == (old_str_count - 1)) ? dstr->len : indexes[i + 1]);
				/* 被搬移数据的长度。 */
				const size_t move_len = move_end - move_start;

				if (move_len > 0) {
					const size_t move_step = len_diff * (i + 1);

					char *const move_src = dstr->data + move_start;
					char *const move_dest = (new_cmp_old > 0)
						? (move_src + move_step)
						: (move_src - move_step);

					memmove(move_dest, move_src, move_len);
				}
			}

			/* 拷贝新数据。 */
			if (new_str_len > 0) {
				const size_t copy_target = indexes[i] - len_diff * i;

				memcpy(
					dstr->data + copy_target,
					new_str,
					new_str_len
				);
			}
		}
	} else {
		for (size_t i = old_str_count; i > 0; --i) {
			const size_t cur = i - 1;

			/* 如果 new_str 与 old_str 长度不等，则需移动数据。 */
			if (new_cmp_old != 0) {
				const size_t move_start = indexes[cur] + old_str_len;
				const size_t move_end = (direction == DSTR_DIR_BACKWARD)
					? ((cur == 0) ? dstr->len : indexes[cur - 1])
					: ((i == old_str_count) ? dstr->len : indexes[i]);
				const size_t move_len = move_end - move_start;

				if (move_len > 0) {
					const size_t move_step = len_diff * i;

					char *const move_src = dstr->data + move_start;
					char *const move_dest = (new_cmp_old > 0)
						? (move_src + move_step)
						: (move_src - move_step);

					memmove(move_dest, move_src, move_len);
				}
			}

			/* 拷贝新数据。 */
			if (new_str_len > 0) {
				const size_t copy_target = indexes[cur] - len_diff * cur;

				memcpy(
					dstr->data + copy_target,
					new_str,
					new_str_len
				);
			}
		}
	}

	free(indexes);

	/* 长度有变化时，更新长度。 */
	if (new_cmp_old != 0) {
		/* 之前仅在 new_cmp_old > 0 分支计算了 new_len，因此此处进行计算。 */
		if (new_cmp_old < 0) {
			new_len = dstr->len - len_diff * old_str_count;
		}

		dstr->len = new_len;
	}

	goto end;

worst_case: {
		/**
		 * 最坏路径：所有分配均失败，使用边查找边替换的算法（最笨算法），完全的时间换空间算法。
		 * 边查找边替换：每找到一个匹配就立即执行 memmove + memcpy。
		 */

		/* 用于迭代。 */
		char *p;
		/* 迭代中的边界指针，在边查找边替换的逻辑中，每次迭代都有可能改变其值。 */
		char *end = dstr->data + dstr->len - old_str_len + 1;
		/* 已替换的次数。用于在等于 n 时跳出循环。 */
		size_t replaced_count = 0;

		if (direction == DSTR_DIR_BACKWARD) {
			p = end;

			while (p > dstr->data) {
				/* 指向当前待比较数据指针。 */
				char *const cur = p - 1;

				if (memcmp(cur, old_str, old_str_len) == 0) {
					/* 移动尾部数据。 */
					if (new_cmp_old != 0) {
						const char *const move_src = cur + old_str_len;

						if (move_src < end) {
							memmove(
								cur + new_str_len,
								move_src,
								dstr->data + dstr->len - move_src
							);
						}
					}

					/* 拷贝新数据。 */
					if (new_str_len > 0) {
						memcpy(cur, new_str, new_str_len);
					}

					/* 更新 dstr->len。 */
					if (new_cmp_old < 0) {
						dstr->len -= len_diff;
					} else {
						dstr->len += len_diff;
					}

					/* 如果替换足够次数则跳出循环。 */
					if (++replaced_count == n) { break; }

					/* 如果 (p - dstr->data) > old_str_len 则继续迭代，否则跳出。 */
					if ((p - dstr->data) > old_str_len) {
						p -= old_str_len;
					} else { break; }
				} else {
					--p;
				}
			}
		} else {
			p = dstr->data;

			while (p < end) {
				if (memcmp(p, old_str, old_str_len) == 0) {
					/* 移动尾部数据。 */
					if (new_cmp_old != 0) {
						const char *const move_src = p + old_str_len;

						if (move_src < end) {
							memmove(
								p + new_str_len,
								move_src,
								dstr->data + dstr->len - move_src
							);
						}
					}

					/* 拷贝新数据。 */
					if (new_str_len > 0) {
						memcpy(p, new_str, new_str_len);
					}

					/* 更新 dstr->len。 */
					if (new_cmp_old < 0) {
						dstr->len -= len_diff;
					} else {
						dstr->len += len_diff;
					}

					/* 如果替换足够次数则跳出循环。 */
					if (++replaced_count == n) { break; }

					/* 更新 end 指针。 */
					if (new_cmp_old < 0) {
						end -= len_diff;
					} else {
						end += len_diff;
					}

					p += new_str_len;
				} else {
					++p;
				}
			}
		}
	}

end: /* 收尾工作。 */
	/* 长度变短，尝试缩容。 */
	if (new_cmp_old < 0) {
		resize_capacity_dynamic(dstr, (dstr->len > 0) ? (dstr->len + 1) : 0);
	}

	if (dstr->data != DSTR_NULLPTR) {
		dstr->data[dstr->len] = '\0';
	}

	return DSTR_SUCCESS;
}

/* 将一个「C 字符串」按指定分隔符分割为多个「动态字符串」。 */
static dstr_adt **split_str(
	const char *const cstr,
	const size_t cstr_len,
	const char *const separator,
	const size_t separator_len,
	size_t *const out_dstr_count
) {
	/* 第一遍查找，统计 separator 在 cstr 中，出现的次数。 */
	const size_t separator_count = find_str(
		cstr, cstr_len,
		separator, separator_len,
		DSTR_NULLPTR, DSTR_NULLPTR,
		DSTR_DIR_FORWARD, 0
	);

	/* 如果 separator 在 cstr 中一次都没有出现，则直接返回空指针。  */
	if (separator_count == 0) { return DSTR_NULLPTR; }

	/* dstrs 数组元素个数（分隔后的子串个数）。 */
	const size_t dstr_count = separator_count + 1;

	/* 动态分配 dstr_adt * 数组（比 separator_count 多 1）。 */
	dstr_adt **dstrs;
	if (!safe_size_t_mul(dstr_count, sizeof(dstr_adt*), DSTR_NULLPTR) ||
		(dstrs = malloc(dstr_count * sizeof(dstr_adt*))) == DSTR_NULLPTR
	) { return DSTR_NULLPTR; }

	/* 执行一次边查找边创建。 */
	/* 指针变量，用于迭代。 */
	const char *p = cstr;
	/* 指针常量，指向查找区间的后一个位置。用于迭代边界。 */
	const char *const end = p + cstr_len - separator_len + 1;
	/* 找到的次数。 */
	size_t find_count = 0;
	/* 记录上一次找到的位置指针。 */
	const char *last = p;

	while (p < end) {
		if (memcmp(p, separator, separator_len) == 0) {
create_sub:
			/* 以当前区间子串创建动态字符串。 */
			const char *const sub_start = (find_count > 0)
				? last + separator_len
				: cstr;
			const char *const sub_end = (find_count == separator_count)
				? cstr + cstr_len
				: p;
			const size_t sub_len = sub_end - sub_start;

			if (sub_len > 0) {
				dstrs[find_count] = create_dstr(sub_start, sub_len, 0, 0);

				if (dstrs[find_count] == DSTR_NULLPTR) {
					/* 创建失败，释放之前已创建的「动态字符串」。 */
					for (size_t j = 0; j < find_count; ++j) {
						if (dstrs[j] != DSTR_NULLPTR) {
							free(dstrs[j]->data);
							free(dstrs[j]);
						}
					}
					free(dstrs);
					return DSTR_NULLPTR;
				}
			} else {
				dstrs[find_count] = DSTR_NULLPTR;
			}

			++find_count;
			last = p;

			if (find_count == separator_count) {
				goto create_sub;
			}

			if (find_count > separator_count) {
				break;
			}

			p += separator_len;
		} else {
			++p;
		}
	}

	*out_dstr_count = dstr_count;
	return dstrs;
}

/* 将多个「C 字符串」或「动态字符串」按指定分隔符连接合并为一个「动态字符串」。 */
static dstr_adt *join_str(
	const char *const *const cstrs,
	const dstr_adt *const *const dstrs,
	const size_t str_count,
	const char *const separator,
	const size_t separator_len
) {
	/* 合并后的字符串的长度。 */
	size_t target_len;

	/* 创建空动态字符串，失败则直接返回空指针。 */
	dstr_adt *const result = create_dstr(DSTR_NULLPTR, 0, 0, 0);
	if (result == DSTR_NULLPTR) { return DSTR_NULLPTR; }

	/**
	 * 计算 separator 多次出现的总长度。
	 * 如果 separator_len 为 0，则直接为 0，减少不必要的计算。
	 */
	if (separator_len == 0) {
		target_len = 0;
	} else {
		/* 计算 separator 会出现的次数。 */
		const size_t separator_count = str_count - 1;

		/* 安全计算 size_t 乘法（separator_len * separator_count），防止溢出。 */
		if (!safe_size_t_mul(separator_len, separator_count, &target_len)) {
			free(result);
			return DSTR_NULLPTR;
		}
	}

	/* 判断输入是 cstrs 还是 dstrs。 */
	const bool is_cstrs = (cstrs != DSTR_NULLPTR);

	/**
	 * 如果是 cstrs，动态分配一个 size_t 数组，用于缓存每个 C 字符串的长度。
	 * 如果分配失败，则不进行缓存，后续重新计算，不视为致命错误。
	 */
	size_t *cstr_lens = DSTR_NULLPTR;
	if (is_cstrs && safe_size_t_mul(str_count, sizeof(size_t), DSTR_NULLPTR)) {
		cstr_lens = malloc(str_count * sizeof(size_t));
	}

	/**
	 * 第一遍遍历：累加每个字符串的长度到 target_len，
	 * 以计算合并后的字符串的长度。
	 * 注意：指针本身可能为空（视为长度为 0）；
	 * 不为空时长度也可能为 0；
	 * 对于 cstrs，通过 cstrs[i][0] == '\0' 判断，以减少不必要的 strlen 调用。
	 */
	if (is_cstrs) {
		for (size_t i = 0; i < str_count; ++i) {
			const size_t cstr_len = (cstrs[i] != DSTR_NULLPTR && cstrs[i][0] != '\0')
				? strlen(cstrs[i])
				: 0;

			/* 如果长度缓存数组分配成功，记录长度。 */
			if (cstr_lens != DSTR_NULLPTR) {
				cstr_lens[i] = cstr_len;
			}

			/* 累加到 target_len，检测溢出。 */
			if (!safe_size_t_add(target_len, cstr_len, &target_len)) {
				free(cstr_lens);
				free(result);
				return DSTR_NULLPTR;
			}
		}
	} else {
		for (size_t i = 0; i < str_count; ++i) {
			if (!safe_size_t_add(
				target_len,
				(dstrs[i] != DSTR_NULLPTR) ? dstrs[i]->len : 0,
				&target_len
			)) {
				free(result);
				return DSTR_NULLPTR;
			}
		}
	}

	/* 如果 target_len 为 0，则释放资源后直接返回 result。 */
	if (target_len == 0) {
		free(cstr_lens);
		return result;
	}

	/* 扩容。 */
	if (!safe_size_t_add(target_len, 1, DSTR_NULLPTR) ||
		!resize_capacity(result, target_len + 1)
	) {
		free(cstr_lens);
		free(result);
		return DSTR_NULLPTR;
	}

	/**
	 * 第二遍遍历：执行拷贝。
	 * 如果是 cstrs 且长度缓存数组分配成功，则复用它；
	 * 否则重新计算每个 C 字符串的长度。
	 */
	char *dest = result->data;

	if (is_cstrs) {
		for (size_t i = 0; i < str_count; ++i) {
			/* 如果不是第一个元素，先拷贝 separator。 */
			if (i > 0 && separator_len > 0) {
				memcpy(dest, separator, separator_len);
				dest += separator_len;
			}

			/* 从缓存获取或重新计算长度。 */
			const size_t cstr_len = (cstr_lens != DSTR_NULLPTR)
				? cstr_lens[i]
				: ((cstrs[i] != DSTR_NULLPTR && cstrs[i][0] != '\0')
					? strlen(cstrs[i])
					: 0);

			/* 拷贝 cstr。 */
			if (cstr_len > 0) {
				memcpy(dest, cstrs[i], cstr_len);
				dest += cstr_len;
			}
		}
	} else {
		for (size_t i = 0; i < str_count; ++i) {
			/* 如果不是第一个元素，先拷贝 separator。 */
			if (i > 0 && separator_len > 0) {
				memcpy(dest, separator, separator_len);
				dest += separator_len;
			}

			/* 拷贝 dstr。 */
			if (dstrs[i] != DSTR_NULLPTR && dstrs[i]->len > 0) {
				memcpy(dest, dstrs[i]->data, dstrs[i]->len);
				dest += dstrs[i]->len;
			}
		}
	}

	/* 设置终止符和长度。 */
	*dest = '\0';
	result->len = target_len;

	free(cstr_lens);
	return result;
}
