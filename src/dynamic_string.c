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
 *        在基础班的基础上增加对 new_cap 为 0 ，以及是否与原容量相等的检查。
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

/* 格式化写入字符串到一个「动态字符串」。 */
dstr_status_t dstr_printf(
	dstr_adt *const dstr,
	const char *const format,
	...
) {
	if (dstr == DSTR_NULLPTR) {
		return DSTR_INVALID_ARGUMENT;
	}

	/* format 为空指针或指向空字符串，均视为写入空字符串。 */
	if (format == DSTR_NULLPTR || format[0] == '\0') {
		dstr->len = 0;
		capacity_resize_dynamic(dstr, 0);

		if (dstr->cap > 0) {
			dstr->data[0] = '\0';
		}
		return DSTR_SUCCESS;
	}

	va_list args, temp_args; /* 变参列表。 */
	size_t new_len;

	/* 变参列表初始化。 */
	va_start(args, format);

	/* 计算所需长度。 */
	va_copy(temp_args, args);
	const int temp_len = vsnprintf(DSTR_NULLPTR, 0, format, temp_args);
	va_end(temp_args);

	if (temp_len < 0) {
		va_end(args);
		return DSTR_INVALID_ARGUMENT;
	}

	new_len = temp_len;


	/* 所需长度大于当前长度，尝试扩容。 */
	if
	(
		!
		safe_size_t_add(new_len, 1, DSTR_NULLPTR)
	) {
		va_end(args);
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	if
	(new_len
		>
		dstr
		->
		len
	) {
		if (!capacity_resize_dynamic(dstr, new_len + 1)) {
			va_end(args);
			return DSTR_MEMORY_ALLOC_FAILED;
		}
	}

	/* 执行写入。 */
	if
	(new_len
		>
		0
	) {
		vsnprintf(dstr->data, new_len + 1, format, args);
	}
	va_end(args);

	if
	(new_len < dstr->len) {
		capacity_resize_dynamic(dstr, (new_len > 0) ? (new_len + 1) : 0);
	}

	dstr
		->
		len = new_len;

	return
		DSTR_SUCCESS;
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

dstr_status_t dstr_cat_printf(
	dstr_adt *const dstr,
	const char *const format,
	...
) {
	if (dstr == DSTR_NULLPTR) {
		return DSTR_INVALID_ARGUMENT;
	}

	va_list args; /* 变参列表。 */
	size_t output_len;

	/* 变参列表初始化。 */
	va_start(args, format);

	/* format 为空指针或指向空字符串，均视为追加空字符串。 */
	if (format == DSTR_NULLPTR || format[0] == '\0') {
		output_len = 0;
	} else {
		va_list temp_args;

		va_copy(temp_args, args);
		const int output_len_temp = vsnprintf(
			DSTR_NULLPTR,
			0,
			format,
			temp_args
		);
		va_end(temp_args);

		if (output_len_temp < 0) {
			va_end(args);
			return DSTR_INVALID_ARGUMENT;
		}

		output_len = output_len_temp;
	}

	/* 所需长度大于当前长度，尝试扩容。 */
	if (output_len > 0) {
		if (!safe_size_t_add(output_len, 1, DSTR_NULLPTR) ||
			!capacity_resize_dynamic(dstr, output_len)
		) {
			return DSTR_MEMORY_ALLOC_FAILED;
		}
	}

	/* 执行写入。 */
	if (output_len > 0) {
		vsnprintf(dstr->data, output_len + 1, format, args);
	}
	va_end(args);

	if (output_len < dstr->len) {
		capacity_resize_dynamic(dstr, (output_len > 0) ? (output_len + 1) : 0);
	}
	dstr->len = output_len;

	return DSTR_SUCCESS;
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

dstr_status_t dstr_insert_printf(
	dstr_adt *const dstr,
	const size_t index,
	const char *const format,
	...
) {
	if (dstr == DSTR_NULLPTR || index > dstr->len ||
		format == DSTR_NULLPTR
	) {
		return DSTR_INVALID_ARGUMENT;
	}

	va_list args, temp_args; /* 变参列表。 */
	size_t needed_cap;       /* 容纳输出字符串所需容量。 */

	/* 变参列表初始化。 */
	va_start(args, format);

	/* 计算输出长度。 */
	va_copy(temp_args, args);
	const int output_len = vsnprintf(DSTR_NULLPTR, 0, format, temp_args);
	va_end(temp_args);

	if (output_len < 0) {
		va_end(args);
		return DSTR_INVALID_ARGUMENT;
	}

	if (!safe_size_t_add(output_len, 1, &needed_cap)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}
	/* 所需长度大于当前长度，尝试扩容。 */
	if (output_len > dstr->len) {
		if (!capacity_resize_dynamic(dstr, needed_cap)) {
			return DSTR_MEMORY_ALLOC_FAILED;
		}
	}

	/* 执行写入。 */
	vsnprintf(dstr->data, needed_cap, format, args);
	va_end(args);

	if (output_len < dstr->len) {
		capacity_resize_dynamic(dstr, (output_len > 0) ? needed_cap : 0);
	}
	dstr->len = output_len;

	return DSTR_SUCCESS;
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
		prefix == DSTR_NULLPTR ||prefix[0] == '\0'
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
		prefix == DSTR_NULLPTR ||prefix->len == 0
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
	if (dstr == DSTR_NULLPTR || old_str == DSTR_NULLPTR ||
		old_str[0] == '\0'
	) {
		return DSTR_INVALID_ARGUMENT;
	}

	/* 计算 old_str 长度。 */
	const size_t old_str_len = strlen(old_str);
	if (old_str_len > dstr->len) {
		return DSTR_INVALID_ARGUMENT;
	}

	return replace_str(
		dstr,
		old_str,
		old_str_len,
		new_str,
		/* 计算 new_str 字符串长度。空指针视为 0。 */
		(new_str != DSTR_NULLPTR) ? strlen(new_str) : 0,
		n,
		backward
	);
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
	if (dstr == DSTR_NULLPTR || old_str == DSTR_NULLPTR ||
		old_str->len == 0
	) {
		return DSTR_INVALID_ARGUMENT;
	}

	if (old_str->len > dstr->len) {
		return DSTR_INVALID_ARGUMENT;
	}

	return replace_str(
		dstr,
		old_str->data,
		old_str->len,
		(new_str != DSTR_NULLPTR) ? new_str->data : DSTR_NULLPTR,
		/* 计算 new_str 字符串长度。空指针视为 0。 */
		(new_str != DSTR_NULLPTR) ? new_str->len : 0,
		n,
		backward
	);
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

	if (backward) {
		p = dstr->data + dstr->len - sub_len;

		while (p >= dstr->data) {
			if (memcmp(p, sub, sub_len) == 0) {
				++find_count;
				find = p;

				if (find_count == n) {
					break;
				}

				p -= sub_len;
			} else {
				--p;
			}
		}
	} else {
		p = dstr->data;

		while (p < dstr->data + dstr->len) {
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
	/* 进行一次查询，并记录出现过的位置。 */
	/* 最多可能匹配 dstr->len 次，分配空间记录索引。 */
	size_t *matches = (size_t*)malloc(dstr->len * sizeof(size_t));
	if (matches == DSTR_NULLPTR) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	size_t find_count = 0;

	if (backward) {
		/* 从后向前搜索，找到的匹配顺序为逆序（最后匹配在前）。 */
		const char *p = dstr->data + dstr->len - old_str_len;

		while (p >= dstr->data) {
			if (memcmp(p, old_str, old_str_len) == 0) {
				matches[find_count++] = p - dstr->data;

				if (find_count == n) break;

				/* 跳过已匹配部分，避免重叠（可根据需要调整） */
				if (p >= dstr->data + old_str_len) {
					p -= old_str_len;
				} else {
					break;
				}

				p -= old_str_len;
			} else {
				if (p == dstr->data) break;
				--p;
			}
		}
	} else {
		/* 从前向后搜索，匹配顺序为正序（第一个匹配在前）。 */
		const char *p = dstr->data;
		const char *const end = dstr->data + dstr->len - old_str_len;

		while (p <= end) {
			if (memcmp(p, old_str, old_str_len) == 0) {
				matches[find_count++] = p - dstr->data;

				if (find_count == n) break;

				p += old_str_len;
			} else {
				++p;
			}
		}
	}

	/* 如果实际出现次数为 0，或不足 n 次（n 不为 0 时），则直接返回，一次替换都不进行。 */
	if (find_count == 0 || (n > 0 && find_count < n)) {
		free(matches);
		return DSTR_INVALID_ARGUMENT;
	}

	/* 扩容缓冲区。 */
	if (new_str_len > old_str_len) {
		/**
		 * 安全计算 size_t 乘法：
		 * (old_str_len - new_str_len) * find_count
		 * 防止溢出。
		 */
		size_t add_len;
		if (!safe_size_t_mul(old_str_len - new_str_len, find_count, &add_len)) {
			free(matches);
			return DSTR_MEMORY_ALLOC_FAILED;
		}

		/**
		 * 安全计算 size_t 加法：
		 * (dstr->len + 1) + add_len
		 * 防止溢出。
		 */
		size_t new_len;
		if (!safe_size_t_add(dstr->len + 1, add_len, &new_len)) {
			free(matches);
			return DSTR_MEMORY_ALLOC_FAILED;
		}

		/* 扩容。 */
		if (!capacity_resize_dynamic(dstr, new_len)) {
			free(matches);
			return DSTR_MEMORY_ALLOC_FAILED;
		}
	}

	/* 执行替换。 */
	if (backward) {
		const bool is_longer = new_str_len > old_str_len;
		const size_t offset = is_longer
			? new_str_len - old_str_len
			: old_str_len - new_str_len;

		for (size_t i = 0; i < find_count; ++i) {
			if (offset != 0) {
				size_t move_start_index = matches[0] + old_str_len;

				/* 移动原有数据。 */
				if (i != 0 || move_start_index < dstr->len) {
					size_t count = find_count - 1 - i;
					size_t move_end_index = is_longer
						? move_start_index + offset * count
						: move_start_index - offset * count;
				}
			}
		}
	} else {}

	free(matches);
	return DSTR_SUCCESS;
}

// #include <stdbool.h>
// #include <stdlib.h>
// #include <string.h>
//
// /* 假设的动态字符串结构体 */
// typedef struct {
// 	char *data;   /* 字符串缓冲区，以 '\0' 结尾 */
// 	size_t len;   /* 字符串长度，不含 '\0' */
// 	size_t alloc; /* 缓冲区总容量 */
// } dstr_adt;
//
// /* 函数返回状态码 */
// typedef enum {
// 	DSTR_STATUS_OK = 0,
// 	DSTR_STATUS_NOMEM,
// 	DSTR_STATUS_NOT_FOUND,
// 	DSTR_STATUS_INVALID_ARG
// } dstr_status_t;
//
// /*
//  * 函数：replace_str
//  * 功能：在动态字符串 dstr 中替换子串
//  * 参数：
//  *   dstr         - 动态字符串对象
//  *   old_str      - 待替换的子串
//  *   old_str_len  - 待替换子串长度
//  *   new_str      - 替换后的子串
//  *   new_str_len  - 替换后子串长度
//  *   n            - 最大替换次数，0 表示替换所有
//  *   backward     - true: 从后向前替换最后 n 次; false: 从前向后替换前 n 次
//  * 返回：
//  *   DSTR_STATUS_OK 成功，DSTR_STATUS_NOT_FOUND 匹配次数不足，DSTR_STATUS_NOMEM 内存不足
//  */
// static dstr_status_t replace_str(
// 	dstr_adt *dstr,
// 	const char *old_str,
// 	size_t old_str_len,
// 	const char *new_str,
// 	size_t new_str_len,
// 	size_t n,
// 	bool backward
// ) {
// 	/* 参数有效性检查 */
// 	if (dstr == NULL || old_str == NULL || new_str == NULL)
// 		return DSTR_STATUS_INVALID_ARG;
// 	if (old_str_len == 0)
// 		return DSTR_STATUS_INVALID_ARG; /* 不允许替换空字符串 */
//
// 	size_t txt_len = dstr->len;
// 	if (txt_len == 0)
// 		return DSTR_STATUS_OK; /* 空串无需替换 */
//
// 	/* ---------------- 1. KMP 搜索所有匹配位置 ---------------- */
// 	/* 构建 LPS 数组 */
// 	size_t *lps = (size_t*)malloc(old_str_len * sizeof(size_t));
// 	if (lps == NULL)
// 		return DSTR_STATUS_NOMEM;
//
// 	lps[0] = 0;
// 	for (size_t i = 1, len = 0; i < old_str_len;) {
// 		if (old_str[i] == old_str[len]) {
// 			lps[i++] = ++len;
// 		} else if (len != 0) {
// 			len = lps[len - 1];
// 		} else {
// 			lps[i++] = 0;
// 		}
// 	}
//
// 	/* 动态数组存放匹配位置（索引） */
// 	size_t *matches = NULL;
// 	size_t matches_cnt = 0;
// 	size_t matches_cap = 0;
//
// 	/* 是否需要在收集到 n 个匹配后提前停止（仅当 n>0 且非 backward） */
// 	bool early_stop = (n > 0) && !backward;
//
// 	size_t i = 0; /* 遍历 dstr->data 的索引 */
// 	size_t j = 0; /* 遍历 old_str 的索引 */
// 	while (i < txt_len) {
// 		if (dstr->data[i] == old_str[j]) {
// 			i++;
// 			j++;
// 		}
// 		if (j == old_str_len) {
// 			/* 找到一个匹配，位置为 i - j */
// 			size_t pos = i - j;
//
// 			/* 扩容位置数组 */
// 			if (matches_cnt >= matches_cap) {
// 				size_t new_cap = (matches_cap == 0) ? 16 : matches_cap * 2;
// 				size_t *tmp = (size_t*)realloc(matches, new_cap * sizeof(size_t));
// 				if (tmp == NULL) {
// 					free(lps);
// 					free(matches);
// 					return DSTR_STATUS_NOMEM;
// 				}
// 				matches = tmp;
// 				matches_cap = new_cap;
// 			}
// 			matches[matches_cnt++] = pos;
//
// 			/* 提前停止：只需要前 n 个匹配且已收集足够 */
// 			if (early_stop && matches_cnt == n)
// 				break;
//
// 			/* KMP 跳转 */
// 			j = lps[j - 1];
// 		} else if (i < txt_len && dstr->data[i] != old_str[j]) {
// 			if (j != 0) {
// 				j = lps[j - 1];
// 			} else {
// 				i++;
// 			}
// 		}
// 	}
// 	free(lps);
// 	lps = NULL;
//
// 	/* ---------------- 2. 判断匹配数量是否满足要求 ---------------- */
// 	size_t total_matches = matches_cnt;
//
// 	/* 如果提前停止，我们没有扫描全部，但此时 total_matches == n 且 !backward，符合要求 */
// 	if (n > 0 && total_matches < n) {
// 		/* 匹配次数不足，一次也不替换 */
// 		free(matches);
// 		return DSTR_STATUS_NOT_FOUND;
// 	}
//
// 	/* 确定最终要替换的匹配子集 */
// 	size_t selected_count;
// 	size_t *selected_matches; /* 指向 matches 数组中的起始位置 */
// 	if (n == 0) {
// 		/* 替换所有匹配 */
// 		selected_count = total_matches;
// 		selected_matches = matches;
// 	} else {
// 		selected_count = n;
// 		if (backward) {
// 			/* 替换最后 n 个匹配 */
// 			selected_matches = matches + (total_matches - n);
// 		} else {
// 			/* 替换前 n 个匹配 */
// 			selected_matches = matches;
// 		}
// 	}
//
// 	/* 计算替换后的新长度 */
// 	size_t old_len = dstr->len;
// 	ssize_t delta = (ssize_t)new_str_len - (ssize_t)old_str_len;
// 	size_t new_len = old_len + (size_t)(selected_count * delta);
//
// 	/* ---------------- 3. 尝试分配临时缓冲区 ---------------- */
// 	char *temp = (char*)malloc(old_len);
// 	if (temp != NULL) {
// 		/* 算法一：使用临时缓冲区，从前向后写 */
// 		memcpy(temp, dstr->data, old_len);
//
// 		/* 扩容原 dstr 缓冲区 */
// 		if (new_len + 1 > dstr->alloc) {
// 			char *new_data = (char*)realloc(dstr->data, new_len + 1);
// 			if (new_data == NULL) {
// 				free(temp);
// 				free(matches);
// 				return DSTR_STATUS_NOMEM;
// 			}
// 			dstr->data = new_data;
// 			dstr->alloc = new_len + 1;
// 		}
//
// 		size_t r = 0; /* 读指针：临时缓冲区 */
// 		size_t w = 0; /* 写指针：dstr->data */
//
// 		for (size_t k = 0; k < selected_count; k++) {
// 			size_t pos = selected_matches[k];
//
// 			/* 复制匹配前的普通字符 */
// 			size_t normal_len = pos - r;
// 			if (normal_len > 0) {
// 				memcpy(dstr->data + w, temp + r, normal_len);
// 				w += normal_len;
// 				r += normal_len;
// 			}
//
// 			/* 写入新字符串 */
// 			if (new_str_len > 0) {
// 				memcpy(dstr->data + w, new_str, new_str_len);
// 				w += new_str_len;
// 			}
//
// 			/* 跳过原字符串中的 old_str */
// 			r += old_str_len;
// 		}
//
// 		/* 复制剩余尾部 */
// 		size_t tail_len = old_len - r;
// 		if (tail_len > 0) {
// 			memcpy(dstr->data + w, temp + r, tail_len);
// 			w += tail_len;
// 		}
//
// 		dstr->data[w] = '\0';
// 		dstr->len = new_len;
//
// 		free(temp);
// 		free(matches);
// 		return DSTR_STATUS_OK;
// 	}
//
// 	/* ---------------- 4. 临时缓冲区分配失败：原地修改 ---------------- */
// 	/* 先确保 dstr 缓冲区足够 */
// 	if (new_len + 1 > dstr->alloc) {
// 		char *new_data = (char*)realloc(dstr->data, new_len + 1);
// 		if (new_data == NULL) {
// 			free(matches);
// 			return DSTR_STATUS_NOMEM;
// 		}
// 		dstr->data = new_data;
// 		dstr->alloc = new_len + 1;
// 	}
//
// 	/* 原地修改：从后向前分段处理，始终安全 */
// 	size_t src_end = old_len; /* 源数据尾后指针 */
// 	size_t dst_end = new_len; /* 目标尾后指针 */
//
// 	for (size_t k = selected_count; k > 0; k--) {
// 		size_t pos = selected_matches[k - 1];
//
// 		/* 1. 复制当前匹配之后的后缀 */
// 		size_t suffix_start = pos + old_str_len;
// 		size_t suffix_len = src_end - suffix_start;
// 		if (suffix_len > 0) {
// 			memmove(
// 				dstr->data + dst_end - suffix_len,
// 				dstr->data + suffix_start,
// 				suffix_len
// 			);
// 		}
// 		dst_end -= suffix_len;
//
// 		/* 2. 写入新子串 */
// 		if (new_str_len > 0) {
// 			memcpy(
// 				dstr->data + dst_end - new_str_len,
// 				new_str,
// 				new_str_len
// 			);
// 		}
// 		dst_end -= new_str_len;
//
// 		/* 3. 更新源尾后指针 */
// 		src_end = pos;
// 	}
// 	/* 此时 dst_end 必然等于 src_end（即前缀长度），前缀已在正确位置，无需移动 */
//
// 	dstr->data[new_len] = '\0';
// 	dstr->len = new_len;
//
// 	free(matches);
// 	return DSTR_STATUS_OK;
// }
