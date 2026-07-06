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

#include <assert.h>
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

static dstr_adt *create_dstr(
	const char *src,
	size_t src_len
);

static dstr_status_t insert_str(
	dstr_adt *dest,
	size_t index,
	size_t count,
	const char *src,
	size_t src_len,
	size_t sub_index,
	size_t sub_count
);


/*------------------------------------------------------------------------------
 * 接口函数定义
 *----------------------------------------------------------------------------*/

/* 创建与销毁。 */

/* 创建一个「动态字符串」。 */
dstr_adt *dstr_create(
	const char *const cstr
) {
	if (cstr != DSTR_NULLPTR && cstr[0] != '\0') {
		const size_t cstr_len = strlen(cstr);

		if (!safe_size_t_add(cstr_len, 1, DSTR_NULLPTR)) {
			return DSTR_NULLPTR;
		}

		return create_dstr(cstr, cstr_len);
	}

	return create_dstr(DSTR_NULLPTR, 0);
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
	if (dstr == DSTR_NULLPTR) {
		return DSTR_NULLPTR;
	}

	return create_dstr(dstr->data, dstr->len);
}

/* 属性获取与设置。 */

/* 获取一个「动态字符串」的内部「C 字符串」指针（非 const）。 */
char *dstr_cstr(
	dstr_adt *const dstr
) {
	if (dstr == DSTR_NULLPTR) {
		return DSTR_NULLPTR;
	}

	return dstr->data;
}

/* 获取一个「动态字符串」的内部「C 字符串」指针（const）。 */
const char *dstr_cstr_const(
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

	capacity_resize_regular(dstr, (dstr->len > 0) ? (dstr->len + 1) : 0);
}

/* 内容编辑。 */

/* 复制一个「C 字符串」到一个「动态字符串」。 */
dstr_status_t dstr_cpy_cstr(
	dstr_adt *const dest,
	const char *const src
) {
	if (dest == DSTR_NULLPTR || src == DSTR_NULLPTR) {
		return DSTR_INVALID_ARGUMENT;
	}

	const size_t src_len = strlen(src);

	return insert_str(dest, 0, dest->len, src, src_len, 0, 0);
}

/* 复制一个「动态字符串」到另一个「动态字符串」。 */
dstr_status_t dstr_cpy(
	dstr_adt *const dest,
	const dstr_adt *const src
) {
	if (dest == DSTR_NULLPTR || src == DSTR_NULLPTR) {
		return DSTR_INVALID_ARGUMENT;
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
	if (dest == DSTR_NULLPTR || src == DSTR_NULLPTR) {
		return DSTR_INVALID_ARGUMENT;
	}

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
	if (dest == DSTR_NULLPTR || src == DSTR_NULLPTR) {
		return DSTR_INVALID_ARGUMENT;
	}

	if (sub_index >= src->len ||
		!safe_size_t_add(sub_index, sub_count, DSTR_NULLPTR) ||
		sub_index + sub_count > src->len
	) {
		return DSTR_INVALID_ARGUMENT;
	}

	return insert_str(dest, 0, dest->len, src->data, src->len, sub_index, sub_count);
}

/* 追加一个「C 字符串」到一个「动态字符串」。 */
dstr_status_t dstr_cat_cstr(
	dstr_adt *const dest,
	const char *const src
) {
	if (dest == DSTR_NULLPTR || src == DSTR_NULLPTR) {
		return DSTR_INVALID_ARGUMENT;
	}

	const size_t src_len = strlen(src);

	if (src_len == 0) {
		return DSTR_SUCCESS;
	}

	return insert_str(dest, dest->len, 0, src, src_len, 0, 0);
}

/* 追加一个「动态字符串」到另一个「动态字符串」。 */
dstr_status_t dstr_cat(
	dstr_adt *const dest,
	const dstr_adt *const src
) {
	if (dest == DSTR_NULLPTR || src == DSTR_NULLPTR) {
		return DSTR_INVALID_ARGUMENT;
	}

	if (src->len == 0) {
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
	if (dest == DSTR_NULLPTR || src == DSTR_NULLPTR) {
		return DSTR_INVALID_ARGUMENT;
	}

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
	if (dest == DSTR_NULLPTR || src == DSTR_NULLPTR) {
		return DSTR_INVALID_ARGUMENT;
	}

	if (sub_index >= src->len ||
		!safe_size_t_add(sub_index, sub_count, DSTR_NULLPTR) ||
		sub_index + sub_count > src->len
	) {
		return DSTR_INVALID_ARGUMENT;
	}

	return insert_str(dest, dest->len, 0, src->data, src->len, sub_index, sub_count);
}

/* 插入一个「C 字符串」到一个「动态字符串」。 */
dstr_status_t dstr_insert_cstr(
	dstr_adt *const dest,
	const size_t index,
	const char *const src
) {
	if (dest == DSTR_NULLPTR || src == DSTR_NULLPTR ||
		index > dest->len
	) {
		return DSTR_INVALID_ARGUMENT;
	}

	const size_t src_len = strlen(src);

	if (src_len == 0) {
		return DSTR_SUCCESS;
	}

	return insert_str(dest, index, 0, src, src_len, 0, 0);
}

/* 插入一个「动态字符串」到另一个「动态字符串」。 */
dstr_status_t dstr_insert(
	dstr_adt *const dest,
	const size_t index,
	const dstr_adt *const src
) {
	if (dest == DSTR_NULLPTR || src == DSTR_NULLPTR ||
		index > dest->len
	) {
		return DSTR_INVALID_ARGUMENT;
	}

	if (src->len == 0) {
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
	if (dest == DSTR_NULLPTR || src == DSTR_NULLPTR ||
		index > dest->len
	) {
		return DSTR_INVALID_ARGUMENT;
	}

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
	if (dest == DSTR_NULLPTR || src == DSTR_NULLPTR ||
		index > dest->len
	) {
		return DSTR_INVALID_ARGUMENT;
	}

	if (sub_index >= src->len ||
		!safe_size_t_add(sub_index, sub_count, DSTR_NULLPTR) ||
		sub_index + sub_count > src->len
	) {
		return DSTR_INVALID_ARGUMENT;
	}

	return insert_str(dest, index, 0, src->data, src->len, sub_index, sub_count);
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
	if (dstr == DSTR_NULLPTR ||
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
	if (dstr == DSTR_NULLPTR) {
		return;
	}
	if (dstr->len == 0) {
		return;
	}

	const bool is_specified_trim_chars = (trim_chars != DSTR_NULLPTR && trim_chars[0] != '\0');
	const char *p = dstr->data;
	const char *q = dstr->data + dstr->len;

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

/* 格式化写入字符串到一个「动态字符串」。 */
dstr_status_t dstr_printf(
	dstr_adt *const dstr,
	const char *const format,
	...
) {
	if (dstr == DSTR_NULLPTR || format == DSTR_NULLPTR) {
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
	if (output_len > 0) {
		vsnprintf(dstr->data, needed_cap, format, args);
	}
	va_end(args);

	if (output_len < dstr->len) {
		capacity_resize_dynamic(dstr, (output_len > 0) ? needed_cap : 0);
	}
	dstr->len = output_len;

	return DSTR_SUCCESS;
}

/* 关系判断与比较。 */

/* 判断一个「动态字符串」是否以指定「C 字符串」前缀开头。 */
bool dstr_starts_with_cstr(
	const dstr_adt *const dstr,
	const char *const prefix
) {
	if (dstr == DSTR_NULLPTR || prefix == DSTR_NULLPTR) {
		return false;
	}

	const size_t prefix_len = strlen(prefix);

	if (prefix_len == 0 || prefix_len > dstr->len) {
		return false;
	}

	return (strncmp(dstr->data, prefix, prefix_len) == 0);
}

/* 判断一个「动态字符串」是否以指定「动态字符串」前缀开头。 */
bool dstr_starts_with(
	const dstr_adt *const dstr,
	const dstr_adt *const prefix
) {
	if (dstr == DSTR_NULLPTR || prefix == DSTR_NULLPTR) {
		return false;
	}

	if (prefix->len == 0 || prefix->len > dstr->len) {
		return false;
	}

	return (strncmp(dstr->data, prefix->data, prefix->len) == 0);
}

/* 判断一个「动态字符串」是否以指定「C 字符串」后缀结尾。 */
bool dstr_ends_with_cstr(
	const dstr_adt *const dstr,
	const char *const suffix
) {
	if (dstr == DSTR_NULLPTR || suffix == DSTR_NULLPTR) {
		return false;
	}

	const size_t suffix_len = strlen(suffix);

	if (suffix_len == 0 || suffix_len > dstr->len) {
		return false;
	}

	return (strncmp(
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
	if (dstr == DSTR_NULLPTR || suffix == DSTR_NULLPTR) {
		return false;
	}

	if (suffix->len == 0 || suffix->len > dstr->len) {
		return false;
	}

	return (strncmp(
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
	if (dstr == DSTR_NULLPTR || sub == DSTR_NULLPTR) {
		return false;
	}

	const size_t sub_len = strlen(sub);

	if (sub_len == 0 || sub_len > dstr->len) {
		return false;
	}

	return (strstr(dstr->data, sub) != DSTR_NULLPTR);
}

/* 判断一个「动态字符串」是否包含指定子「动态字符串」。 */
bool dstr_contains(
	const dstr_adt *const dstr,
	const dstr_adt *const sub
) {
	if (dstr == DSTR_NULLPTR || sub == DSTR_NULLPTR) {
		return false;
	}

	if (sub->len == 0 || sub->len > dstr->len) {
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
	const int str_2_valid = (cstr != DSTR_NULLPTR) ? 1 : 0;

	if (str_1_valid + str_2_valid < 2) {
		return ((str_1_valid ^ str_2_valid) == 0);
	}

	const size_t cstr_len = strlen(cstr);
	if (dstr->len != cstr_len) {
		return false;
	}

	return (strncmp(dstr->data, cstr, cstr_len) == 0);
}

/* 判断两个「动态字符串」是否相等。 */
bool dstr_equals(
	const dstr_adt *const dstr_1,
	const dstr_adt *const dstr_2
) {
	const int str_1_valid = (dstr_1 != DSTR_NULLPTR && dstr_1->len > 0) ? 1 : 0;
	const int str_2_valid = (dstr_2 != DSTR_NULLPTR && dstr_2->len > 0) ? 1 : 0;

	if (str_1_valid + str_2_valid < 2) {
		return ((str_1_valid ^ str_2_valid) == 0);
	}

	if (dstr_1->len != dstr_2->len) {
		return false;
	}

	return (strncmp(dstr_1->data, dstr_2->data, dstr_1->len) == 0);
}

/* 比较一个「动态字符串」与一个「C 字符串」。 */
int dstr_compare_cstr(
	const dstr_adt *const dstr,
	const char *const cstr
) {
	const int str_1_valid = (dstr != DSTR_NULLPTR && dstr->len > 0) ? 1 : 0;
	const int str_2_valid = (cstr != DSTR_NULLPTR) ? 1 : 0;

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
	if (dstr == DSTR_NULLPTR || sub == DSTR_NULLPTR) {
		return false;
	}

	const size_t sub_len = strlen(sub);
	if (sub_len == 0 || sub_len > dstr->len) {
		return false;
	}

	const char *p;

	if (backward) {
		p = dstr->data + dstr->len - sub_len;

		while (p >= dstr->data) {
			if (strncmp(p, sub, sub_len) == 0) {
				if (out_index != DSTR_NULLPTR) {
					*out_index = p - dstr->data;
				}

				return true;
			}

			--p;
		}
	} else {
		p = dstr->data;

		while (p < dstr->data + dstr->len) {
			if (strncmp(p, sub, sub_len) == 0) {
				if (out_index != DSTR_NULLPTR) {
					*out_index = p - dstr->data;
				}

				return true;
			}

			++p;
		}
	}

	return false;
}

/* 查找一个「动态字符串」中指定子「动态字符串」首次出现的位置。 */
bool dstr_find(
	const dstr_adt *const dstr,
	const dstr_adt *const sub,
	size_t *const out_index,
	const bool backward
) {
	if (dstr == DSTR_NULLPTR || sub == DSTR_NULLPTR) {
		return false;
	}

	if (sub->len == 0 || sub->len > dstr->len) {
		return false;
	}

	const char *p;

	if (backward) {
		p = dstr->data + dstr->len - sub->len;

		while (p >= dstr->data) {
			if (strncmp(p, sub->data, sub->len) == 0) {
				if (out_index != DSTR_NULLPTR) {
					*out_index = p - dstr->data;
				}

				return true;
			}

			--p;
		}
	} else {
		p = dstr->data;

		while (p < dstr->data + dstr->len) {
			if (strncmp(p, sub->data, sub->len) == 0) {
				if (out_index != DSTR_NULLPTR) {
					*out_index = p - dstr->data;
				}

				return true;
			}

			++p;
		}
	}

	return false;
}

/* 查找一个「动态字符串」中指定子「C 字符串」第 n 次出现的位置。 */
bool dstr_find_nth_cstr(
	const dstr_adt *const dstr,
	const char *const sub,
	size_t *const out_index,
	const size_t n,
	const bool backward
) {
	if (dstr == DSTR_NULLPTR || sub == DSTR_NULLPTR) {
		return false;
	}

	const size_t sub_len = strlen(sub);
	if (sub_len == 0 || sub_len > dstr->len) {
		return false;
	}

	const char *p;
	const char *find = DSTR_NULLPTR;
	size_t find_count = 0;

	if (backward) {
		p = dstr->data + dstr->len - sub_len;

		while (p >= dstr->data) {
			if (strncmp(p, sub, sub_len) == 0) {
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
			if (strncmp(p, sub, sub_len) == 0) {
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

	if (find == DSTR_NULLPTR) {
		return false;
	}

	if (out_index != DSTR_NULLPTR) {
		*out_index = find - dstr->data;
	}

	return true;
}

/* 查找一个「动态字符串」中指定子「动态字符串」第 n 次出现的位置。 */
bool dstr_find_nth(
	const dstr_adt *const dstr,
	const dstr_adt *const sub,
	size_t *const out_index,
	const size_t n,
	const bool backward
) {
	if (dstr == DSTR_NULLPTR || sub == DSTR_NULLPTR) {
		return false;
	}

	if (sub->len == 0 || sub->len > dstr->len) {
		return false;
	}

	const char *p;
	const char *find = DSTR_NULLPTR;
	size_t find_count = 0;

	if (backward) {
		p = dstr->data + dstr->len - sub->len;

		while (p >= dstr->data) {
			if (strncmp(p, sub->data, sub->len) == 0) {
				++find_count;
				find = p;

				if (find_count == n) {
					break;
				}

				p -= sub->len;
			} else {
				--p;
			}
		}
	} else {
		p = dstr->data;

		while (p < dstr->data + dstr->len) {
			if (strncmp(p, sub->data, sub->len) == 0) {
				++find_count;
				find = p;

				if (find_count == n) {
					break;
				}

				p += sub->len;
			} else {
				++p;
			}

		}
	}

	if (find == DSTR_NULLPTR) {
		return false;
	}

	if (out_index != DSTR_NULLPTR) {
		*out_index = find - dstr->data;
	}

	return true;
}

/* 统计一个「动态字符串」中指定子「C 字符串」出现的次数。 */
size_t dstr_count_cstr(
	const dstr_adt *const dstr,
	const char *const sub,
	const bool backward
) {
	if (dstr == DSTR_NULLPTR || sub == DSTR_NULLPTR) {
		return 0;
	}

	const size_t sub_len = strlen(sub);
	if (sub_len == 0 || sub_len > dstr->len) {
		return 0;
	}

	const char *p;
	size_t find_count = 0;

	if (backward) {
		p = dstr->data + dstr->len - sub_len;

		while (p >= dstr->data) {
			if (strncmp(p, sub, sub_len) == 0) {
				++find_count;
				p -= sub_len;
			} else {
				--p;
			}
		}
	} else {
		p = dstr->data;

		while (p < dstr->data + dstr->len) {
			if (strncmp(p, sub, sub_len) == 0) {
				++find_count;
				p += sub_len;
			} else {
				++p;
			}

		}
	}

	return find_count;
}

/* 统计一个「动态字符串」中指定子「动态字符串」出现的次数。 */
size_t dstr_count(
	const dstr_adt *const dstr,
	const dstr_adt *const sub,
	const bool backward
) {
	if (dstr == DSTR_NULLPTR || sub == DSTR_NULLPTR) {
		return 0;
	}

	if (sub->len == 0 || sub->len > dstr->len) {
		return 0;
	}

	const char *p;
	size_t find_count = 0;

	if (backward) {
		p = dstr->data + dstr->len - sub->len;

		while (p >= dstr->data) {
			if (strncmp(p, sub->data, sub->len) == 0) {
				++find_count;
				p -= sub->len;
			} else {
				--p;
			}
		}
	} else {
		p = dstr->data;

		while (p < dstr->data + dstr->len) {
			if (strncmp(p, sub->data, sub->len) == 0) {
				++find_count;
				p += sub->len;
			} else {
				++p;
			}

		}
	}

	return find_count;
}

/* 替换一个「动态字符串」中指定旧「C 字符串」为指定新「C 字符串」n 次。 */
dstr_status_t dstr_replace_cstr(
	dstr_adt *const dstr,
	const char *const old_str,
	const char *const new_str,
	const size_t n,
	const bool backward
) {}

/* 替换一个「动态字符串」中指定旧「动态字符串」为指定新「动态字符串」n 次。 */
dstr_status_t dstr_replace(
	dstr_adt *const dstr,
	const dstr_adt *const old_str,
	const dstr_adt *const new_str,
	const size_t n,
	const bool backward
) {}


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
	const size_t src_len
) {
	dstr_adt *new_dstr = malloc(sizeof(dstr_adt));
	if (new_dstr == DSTR_NULLPTR) {
		return DSTR_NULLPTR;
	}

	new_dstr->data = DSTR_NULLPTR;

	if (src_len > 0) {
		if (!capacity_resize(new_dstr, src_len + 1)) {
			free(new_dstr);
			return DSTR_NULLPTR;
		}

		memcpy(new_dstr->data, src, src_len);
		new_dstr->data[new_dstr->len = src_len] = '\0';
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
