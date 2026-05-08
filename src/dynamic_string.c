//
// Created by mtueih on 2026/2/25.
//

#include "dynamic_string.h"
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <safe_calc.h>

#define DYNAMIC_STRING_CACHELINE_SIZE 64

// 如果 C 标准大于 C23，则 将 NULL 定义为 nullptr
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#define DYNAMIC_STRING_NULL_PTR nullptr
#else
#define DYNAMIC_STRING_NULL_PTR NULL
#endif


// ADT 类型定义
struct dynamic_string {
	char *data;
	size_t len;
	size_t cap;
	size_t min_cap;
};

// 静态函数定义
// 释放动态字符串的缓冲区内存（不包括 adt 本身）
static void local_free_dstr(dstr_adt *const dstr) {
	if (dstr->data != DYNAMIC_STRING_NULL_PTR) {
		free(dstr->data);
		dstr->data = DYNAMIC_STRING_NULL_PTR;
	}
	dstr->cap = 0;
	dstr->len = 0;
}

// 调整容量
/**
 * 调整一个动态字符串的容量，
 * @warning 只进行实际 realloc、更新 adt 成员变量 data、cap、len（如果容量不够则会截断）的值；
 *	请确保 new_cap 不等于 0，否则将导致 relloc 的未定义行为，以及可能的溢出，以及后续的越界访问
 * @param new_cap 请确保 new_cap 不等于 0，否则将导致 relloc 的未定义行为，以及可能的溢出，以及后续的越界访问
 */
static bool local_capacity_resize(dstr_adt *const dstr, const size_t new_cap) {
	char *new_cstr = realloc(dstr->data, new_cap);

	if (new_cstr == DYNAMIC_STRING_NULL_PTR) return false;

	dstr->data = new_cstr;
	dstr->cap = new_cap;

	if (new_cap <= dstr->len) {
		dstr->len = new_cap - 1;
		dstr->data[dstr->len] = '\0';
	}

	return true;
}

// 实际调整一个动字符串的容量
static bool local_capacity_resize_really(dstr_adt *const dstr, const size_t new_cap) {
	if (new_cap == 0) {
		local_free_dstr(dstr);
		return true;
	}

	return local_capacity_resize(dstr, new_cap);
}

// 动态调整一个「动态字符串」的容量
/**
 * 动态调整一个「动态字符串」的容量
 * @param new_len 所需的字符串长度，无须手动+1
 */
static bool local_capacity_resize_dynamic(dstr_adt *const dstr, const size_t new_len) {
	size_t new_cap = 0; // 不低于保底值目标容量
	size_t adjusted_cap = 0; // 基于目标容量调整后的容量，为目标容量 1.5 倍
	size_t cache_line_aligned_cap = 0; // 对齐到缓存行大小后的容量

	// 安全计算 size_t 加法，防止溢出
	// new_cap = new_len + 1;
	if (!safe_size_add(new_len, 1, &new_cap)) {
		return false;
	}

	// 确保容量不低于保底值
	if (new_cap < dstr->min_cap) {
		new_cap = dstr->min_cap;
	}

	// 如果目标容量为 1，即所需长度为 0，且 dstr->min_cap 不为 1，则不保留 '\0'，释放内存
	if (new_cap == 1 && dstr->min_cap == 0) {
		local_free_dstr(dstr);

		return true;
	}

	// 如果目标容量与当前容量相同，则直接返回
	if (new_cap == dstr->cap) {
		return true;
	}

	// 如果目标容量小于当前容量，则延迟减容，仅当新容量小于当前容量的 1/4 时，才减容
	// 即，如果目标容量小于当前容量，但是又大于当前容量的 1/4，则不进行减容
	// 右移操作无溢出风险
	if (new_cap < dstr->cap && new_cap > dstr->cap >> 2) {
		return true;
	}

	// 如果目标容量大于当前容量，则进行容量预分配，分配 1.5 倍
	if (new_cap > dstr->cap) {
		// 尝试预分配内存
		// 安全计算 size_t 加法（adjusted_cap = new_cap + new_cap >> 1），防止溢出
		if (safe_size_add(new_cap, new_cap >> 1, &adjusted_cap)) {
			// 尝试对齐到缓存行大小
			// 安全计算 size_t 向上对齐（adjusted_cap ⬆ DYNAMIC_STRING_CACHELINE_SIZE），防止溢出
			if (safe_size_align_up(adjusted_cap, DYNAMIC_STRING_CACHELINE_SIZE, &cache_line_aligned_cap)) {
				if (local_capacity_resize(dstr, cache_line_aligned_cap)) {
					return true;
				}
			}
			// 对齐溢出，或分配对齐后的容量失败，
			// 如果是对齐溢出，或对齐后的容量大于 adjusted_cap，则尝试分配 adjusted_cap
			if (cache_line_aligned_cap != adjusted_cap) {
				if (local_capacity_resize(dstr, adjusted_cap)) {
					return true;
				}
			}
		}
		// 预分配溢出，或尝试分配 缓存行大小、 adjusted_cap 都失败，
		// 如果是预分配溢出，或预分配容量大于 new_cap，则尝试分配 new_cap
		if (adjusted_cap != new_cap) {
			if (local_capacity_resize(dstr, new_cap)) {
				return true;
			}
		}
		return false;
	}

	// 目标容量小于当前容量的 1/4，减容
	// 尝试对齐到缓存行大小
	if (safe_size_align_up(new_cap, DYNAMIC_STRING_CACHELINE_SIZE, &cache_line_aligned_cap)) {
		if (local_capacity_resize(dstr, cache_line_aligned_cap)) {
			return true;
		}
	}
	// 对齐溢出，或分配对齐后的容量失败，
	// 如果是对齐溢出，或对齐后的容量大于原容量，则尝试分配原容量
	if (cache_line_aligned_cap != new_cap) {
		if (local_capacity_resize(dstr, new_cap)) {
			return true;
		}
	}
	return false;
}

// 在堆上创建 adt 并初始化
static struct dynamic_string *local_create_dstr_and_init(void) {
	dstr_adt *new_dstr;

	new_dstr = malloc(sizeof(struct dynamic_string));
	if (new_dstr == DYNAMIC_STRING_NULL_PTR) return DYNAMIC_STRING_NULL_PTR;

	// 初始化
	new_dstr->data = DYNAMIC_STRING_NULL_PTR;
	new_dstr->cap = 0;
	new_dstr->len = 0;
	new_dstr->min_cap = 0;

	return new_dstr;
}


// API 函数定义
// 创建、销毁、清空
dstr_adt *dstr_create(const char *const cstr) {
	size_t cstr_len;

	dstr_adt *new_dstr = local_create_dstr_and_init();
	if (new_dstr == DYNAMIC_STRING_NULL_PTR) return DYNAMIC_STRING_NULL_PTR;

	if (cstr != DYNAMIC_STRING_NULL_PTR && (cstr_len = strlen(cstr)) != 0) {
		if (!local_capacity_resize_dynamic(new_dstr, cstr_len)) {
			free(new_dstr);
			return DYNAMIC_STRING_NULL_PTR;
		}

		memcpy(new_dstr->data, cstr, cstr_len);
		new_dstr->data[new_dstr->len = cstr_len] = '\0';
	}

	return new_dstr;
}

void dstr_destroy(dstr_adt *const dstr) {
	// 断言，开发阶段参数检查
	assert(dstr != DYNAMIC_STRING_NULL_PTR);

	if (dstr->data != DYNAMIC_STRING_NULL_PTR) free(dstr->data);
	free(dstr);
}

void dstr_clear(dstr_adt *const dstr) {
	// 断言，开发阶段参数检查
	assert(dstr != DYNAMIC_STRING_NULL_PTR);

	if (dstr->data == DYNAMIC_STRING_NULL_PTR || dstr->len == 0) return;

	dstr->data[dstr->len = 0] = '\0';
}

// 属性获取与设置
char *dstr_cstr(dstr_adt *const dstr) {
	// 断言，开发阶段参数检查
	assert(dstr != DYNAMIC_STRING_NULL_PTR);

	return dstr->data;
}

const char *dstr_cstr_const(const dstr_adt *const dstr) {
	// 断言，开发阶段参数检查
	assert(dstr != DYNAMIC_STRING_NULL_PTR);

	return dstr->data;
}

size_t dstr_length(const dstr_adt *const dstr) {
	// 断言，开发阶段参数检查
	assert(dstr != DYNAMIC_STRING_NULL_PTR);

	return dstr->len;
}

size_t dstr_capacity(const dstr_adt *const dstr) {
	// 断言，开发阶段参数检查
	assert(dstr != DYNAMIC_STRING_NULL_PTR);

	return dstr->cap;
}

int dstr_set_capacity(dstr_adt *const dstr, const size_t new_capacity) {
	// 断言，开发阶段参数检查
	assert(dstr != DYNAMIC_STRING_NULL_PTR);

	if (!local_capacity_resize_really(dstr, new_capacity)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	dstr->min_cap = new_capacity;
	return DSTR_SUCCESS;
}

// 复制、追加、插入、删除
// 复制、追加、插入完整现有字符串到目标字符串
int dstr_cpy_cstr(dstr_adt *const dest, const char *const src) {
	size_t src_len;

	// 断言，开发阶段参数检查
	assert(dest != DYNAMIC_STRING_NULL_PTR && src != DYNAMIC_STRING_NULL_PTR);

	// 参数检查
	if ((src_len = strlen(src)) == 0) return DSTR_INVALID_PARAM;

	// 容量调整
	if (!local_capacity_resize_dynamic(dest, src_len)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	// 字符串插入/拷贝
	memcpy(dest->data, src, src_len);
	dest->data[dest->len = src_len] = '\0';
	return DSTR_SUCCESS;
}

int dstr_cpy(dstr_adt *const dest, const dstr_adt *const src) {
	// 断言，开发阶段参数检查
	assert(dest != DYNAMIC_STRING_NULL_PTR && src != DYNAMIC_STRING_NULL_PTR);

	if (src->len == 0) return DSTR_INVALID_PARAM;

	if (!local_capacity_resize_dynamic(dest, src->len)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	memcpy(dest->data, src->data, src->len);
	dest->data[dest->len = src->len] = '\0';
	return DSTR_SUCCESS;
}

int dstr_cat_cstr(dstr_adt *const dest, const char *const src) {
	size_t src_len;

	// 断言，开发阶段参数检查
	assert(dest != DYNAMIC_STRING_NULL_PTR && src != DYNAMIC_STRING_NULL_PTR);

	if ((src_len = strlen(src)) == 0) return DSTR_INVALID_PARAM;

	if (!local_capacity_resize_dynamic(dest, dest->len + src_len)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	memcpy(dest->data + dest->len, src, src_len);
	dest->data[dest->len += src_len] = '\0';
	return DSTR_SUCCESS;
}

int dstr_cat(dstr_adt *const dest, const dstr_adt *const src) {
	// 断言，开发阶段参数检查
	assert(dest != DYNAMIC_STRING_NULL_PTR && src != DYNAMIC_STRING_NULL_PTR);

	if (src->len == 0) return DSTR_INVALID_PARAM;

	if (!local_capacity_resize_dynamic(dest, dest->len + src->len)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}
	memcpy(dest->data + dest->len, src->data, src->len);
	dest->data[dest->len += src->len] = '\0';
	return DSTR_SUCCESS;
}

int dstr_insert_cstr(dstr_adt *const dest, const size_t index, const char *const src) {
	size_t src_len;

	// 断言，开发阶段参数检查
	assert(dest != DYNAMIC_STRING_NULL_PTR && src != DYNAMIC_STRING_NULL_PTR);

	if (index > dest->len) return DSTR_INVALID_PARAM;
	if ((src_len = strlen(src)) == 0) return DSTR_INVALID_PARAM;

	if (!local_capacity_resize_dynamic(dest, dest->len + src_len)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	if (index < dest->len) {
		memmove(dest->data + index + src_len,
			dest->data + index,
			dest->len - index
		);
	}
	memcpy(dest->data + index, src, src_len);
	dest->data[dest->len += src_len] = '\0';
	return DSTR_SUCCESS;
}

int dstr_insert(dstr_adt *const dest, const size_t index, const dstr_adt *const src) {
	// 断言，开发阶段参数检查
	assert(dest != DYNAMIC_STRING_NULL_PTR && src != DYNAMIC_STRING_NULL_PTR);

	if (index > dest->len || src->len == 0) return DSTR_INVALID_PARAM;

	if (!local_capacity_resize_dynamic(dest, dest->len + src->len)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	if (index < dest->len) {
		memmove(dest->data + index + src->len,
			dest->data + index,
			dest->len - index
		);
	}
	memcpy(dest->data + index, src->data, src->len);
	dest->data[dest->len += src->len] = '\0';
	return DSTR_SUCCESS;
}

// 复制、追加、插入现有字符串的子串到目标字符串
int dstr_cpy_sub_cstr(dstr_adt *const dest, const char *const src, const size_t sub_index, const size_t sub_count) {
	// 断言，开发阶段参数检查
	assert(dest != DYNAMIC_STRING_NULL_PTR && src != DYNAMIC_STRING_NULL_PTR);

	const size_t src_len = strlen(src);
	if (sub_index >= src_len || sub_index + sub_count > src_len) return DSTR_INVALID_PARAM;

	const size_t sub_len = sub_count == 0 ? src_len - sub_index : sub_count;

	if (!local_capacity_resize_dynamic(dest, sub_len)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	memcpy(dest->data, src + sub_index, sub_len);
	dest->data[dest->len = sub_len] = '\0';
	return DSTR_SUCCESS;
}

int dstr_cpy_sub(dstr_adt *const dest, const dstr_adt *const src, const size_t sub_index, const size_t sub_count) {
	// 断言，开发阶段参数检查
	assert(dest != DYNAMIC_STRING_NULL_PTR && src != DYNAMIC_STRING_NULL_PTR);

	// 检查子串范围是否越界
	if (sub_index >= src->len || sub_index + sub_count > src->len) return DSTR_INVALID_PARAM;

	// 计算子串长度，sub_count 为 0 视为到末尾
	const size_t sub_len = sub_count == 0 ? src->len - sub_index : sub_count;

	// 扩容
	if (!local_capacity_resize_dynamic(dest, sub_len)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	// 复制子串
	memcpy(dest->data, src->data + sub_index, sub_len);
	dest->data[dest->len = sub_len] = '\0';
	return DSTR_SUCCESS;
}

int dstr_cat_sub_cstr(dstr_adt *const dest, const char *const src, const size_t sub_index, const size_t sub_count) {
	// 断言，开发阶段参数检查
	assert(dest != DYNAMIC_STRING_NULL_PTR && src != DYNAMIC_STRING_NULL_PTR);

	// 获取源字符串长度
	const size_t src_len = strlen(src);
	// 检查子串范围是否越界
	if (sub_index >= src_len || sub_index + sub_count > src_len) return DSTR_INVALID_PARAM;

	// 计算子串长度，sub_count 为 0 视为到末尾
	const size_t sub_len = sub_count == 0 ? src_len - sub_index : sub_count;

	// 扩容
	if (!local_capacity_resize_dynamic(dest, dest->len + sub_len)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	// 复制子串
	memcpy(dest->data + dest->len, src + sub_index, sub_len);
	dest->data[dest->len += sub_len] = '\0';
	return DSTR_SUCCESS;
}

int dstr_cat_sub(dstr_adt *const dest, const dstr_adt *const src, const size_t sub_index, const size_t sub_count) {
	// 断言，开发阶段参数检查
	assert(dest != DYNAMIC_STRING_NULL_PTR && src != DYNAMIC_STRING_NULL_PTR);

	// 检查子串范围是否越界
	if (sub_index >= src->len || sub_index + sub_count > src->len) return DSTR_INVALID_PARAM;

	// 计算子串长度，sub_count 为 0 视为到末尾
	const size_t sub_len = sub_count == 0 ? src->len - sub_index : sub_count;

	// 扩容
	if (!local_capacity_resize_dynamic(dest, dest->len + sub_len)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	// 复制子串
	memcpy(dest->data + dest->len, src->data + sub_index, sub_len);
	dest->data[dest->len += sub_len] = '\0';
	return DSTR_SUCCESS;
}

int dstr_insert_sub_cstr(
	dstr_adt *const dest, const size_t index, const char *const src, const size_t sub_index,
	const size_t sub_count
) {
	// 断言，开发阶段参数检查
	assert(dest != DYNAMIC_STRING_NULL_PTR && src != DYNAMIC_STRING_NULL_PTR);

	// 检查索引是否越界
	if (index > dest->len) return DSTR_INVALID_PARAM;
	// 获取源字符串长度
	const size_t src_len = strlen(src);
	// 检查子串范围是否越界
	if (sub_index >= src_len || sub_index + sub_count > src_len) return DSTR_INVALID_PARAM;

	// 计算子串长度，sub_count 为 0 视为到末尾
	const size_t sub_len = sub_count == 0 ? src_len - sub_index : sub_count;

	// 扩容
	if (!local_capacity_resize_dynamic(dest, dest->len + sub_len)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	// 如果子串不位于末尾，则需要移动后面的字符串
	if (index < dest->len) {
		memmove(dest->data + index + sub_len,
			dest->data + index,
			dest->len - index
		);
	}
	// 复制子串
	memcpy(dest->data + index, src + sub_index, sub_len);
	dest->data[dest->len += sub_len] = '\0';
	return DSTR_SUCCESS;
}

int dstr_insert_sub(
	dstr_adt *const dest, const size_t index, const dstr_adt *const src, const size_t sub_index,
	const size_t sub_count
) {
	// 断言，开发阶段参数检查
	assert(dest != DYNAMIC_STRING_NULL_PTR && src != DYNAMIC_STRING_NULL_PTR);

	// 检查索引是否越界
	if (index > dest->len) return DSTR_INVALID_PARAM;
	// 检查子串范围是否越界
	if (sub_index >= src->len || sub_index + sub_count > src->len) return DSTR_INVALID_PARAM;

	// 计算子串长度，sub_count 为 0 视为到末尾
	size_t sub_len = sub_count == 0 ? src->len - sub_index : sub_count;

	// 扩容
	if (!local_capacity_resize_dynamic(dest, dest->len + sub_len)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	// 如果子串不位于末尾，则需要移动后面的字符串
	if (index < dest->len) {
		memmove(dest->data + index + sub_len,
			dest->data + index,
			dest->len - index
		);
	}

	// 复制子串
	memcpy(dest->data + index, src->data + sub_index, sub_len);
	dest->data[dest->len += sub_len] = '\0';
	return DSTR_SUCCESS;
}

// 删除子串
void dstr_remove(dstr_adt *const dstr, const size_t sub_index, const size_t sub_count) {
	// 断言，开发阶段参数检查
	assert(dstr != DYNAMIC_STRING_NULL_PTR);

	// 检查子串范围是否越界
	if (sub_index >= dstr->len || sub_index + sub_count > dstr->len) return;

	// 计算子串长度，sub_count 为 0 视为到末尾
	const size_t sub_len = sub_count == 0 ? dstr->len - sub_index : sub_count;
	// 如果子串不位于末尾，则需要移动后面的字符串
	if (sub_count != 0 && sub_index + sub_count < dstr->len) {
		memmove(dstr->data + sub_index,
			dstr->data + sub_index + sub_count,
			dstr->len - sub_index - sub_count
		);
	}

	dstr->data[dstr->len -= sub_len] = '\0';
	local_capacity_resize_dynamic(dstr, dstr->len);
}

// 删除特定内容
void dstr_trim(dstr_adt *const dstr, const char *const trim_chars) {
	char *find;
	size_t trim_chars_len;

	// 断言，开发阶段参数检查
	assert(dstr != DYNAMIC_STRING_NULL_PTR);

	if (dstr->len == 0) return;

	// 判断 trim_chars 是否有效
	const bool is_trim_chars_valid = trim_chars != DYNAMIC_STRING_NULL_PTR && strlen(trim_chars) != 0;
	// 去除前导空白字符或指定字符
	if (is_trim_chars_valid) {
		for (find = dstr->data;
		     find < dstr->data + dstr->len;
		     ++find
		) {
			if (strchr(trim_chars, *find) == DYNAMIC_STRING_NULL_PTR) break;
		}
	} else {
		for (find = dstr->data;
		     find < dstr->data + dstr->len;
		     ++find
		) {
			if (!isspace(*find)) break;
		}
	}

	if (find > dstr->data) {
		trim_chars_len = find - dstr->data;
		memmove(dstr->data, find, dstr->len - trim_chars_len);
		dstr->data[dstr->len -= trim_chars_len] = '\0';
	}

	// 去除尾部空白字符或指定字符
	if (is_trim_chars_valid) {
		for (find = dstr->data + dstr->len - 1;
		     find >= dstr->data;
		     --find) {
			if (strchr(trim_chars, *find) == DYNAMIC_STRING_NULL_PTR) break;
		}
	} else {
		for (find = dstr->data + dstr->len - 1;
		     find >= dstr->data;
		     --find) {
			if (!isspace(*find)) break;
		}
	}

	if (find < dstr->data + dstr->len - 1) {
		trim_chars_len = dstr->data + dstr->len - 1 - find;
		dstr->data[dstr->len -= trim_chars_len] = '\0';
	}

	local_capacity_resize_dynamic(dstr, dstr->len);
}

// 格式化写入
int dstr_printf(dstr_adt *const dstr, const char *const format, ...) {
	va_list args, temp_args;

	// 断言，开发阶段参数检查
	assert(dstr != DYNAMIC_STRING_NULL_PTR && format != DYNAMIC_STRING_NULL_PTR);

	// 变参列表初始化
	va_start(args, format);

	// 计算输出长度
	va_copy(temp_args, args);
	const int needed_len = vsnprintf(DYNAMIC_STRING_NULL_PTR, 0, format, temp_args);
	va_end(temp_args);

	if (needed_len <= 0) {
		va_end(args);
		return DSTR_INVALID_PARAM;
	}

	// 保存原来的容量，用于恢复
	const size_t old_cap = dstr->cap;
	if (!local_capacity_resize_dynamic(dstr, needed_len)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	// 执行写入
	const int written_len = vsnprintf(dstr->data, needed_len + 1, format, args);
	va_end(args);

	// 写入失败，恢复原容量
	if (written_len < 0) {
		local_capacity_resize_really(dstr, old_cap);
		return DSTR_UNKNOWN_ERROR;
	}

	dstr->data[dstr->len = written_len] = '\0';
	return DSTR_SUCCESS;
}


// 从现有字符串生成新字符串
// 提取子串
dstr_adt *dstr_sub_cstr(const char *const cstr, const size_t sub_index, const size_t sub_count) {
	// 断言，开发阶段参数检查
	assert(cstr != DYNAMIC_STRING_NULL_PTR);

	// 获取 cstr 的长度
	const size_t cstr_len = strlen(cstr);
	// 检查子串范围是否越界
	if (sub_index >= cstr_len || sub_index + sub_count > cstr_len) return DYNAMIC_STRING_NULL_PTR;

	// 在堆上创建并初始化一个新「动态字符串」
	dstr_adt *new_dstr = local_create_dstr_and_init();
	if (new_dstr == DYNAMIC_STRING_NULL_PTR) return DYNAMIC_STRING_NULL_PTR;

	// 计算子串长度，sub_count 为 0 视为到末尾
	const size_t sub_len = sub_count == 0 ? cstr_len - sub_index : sub_count;

	// 调整「动态字符串」容量
	if (!local_capacity_resize_dynamic(new_dstr, sub_len)) {
		free(new_dstr);
		return DYNAMIC_STRING_NULL_PTR;
	}

	// 拷贝子串
	memcpy(new_dstr->data, cstr + sub_index, sub_len);
	new_dstr->data[new_dstr->len = sub_len] = '\0';
	return new_dstr;
}

dstr_adt *dstr_sub(const dstr_adt *const dstr, const size_t sub_index, const size_t sub_count) {
	// 断言，开发阶段参数检查
	assert(dstr != DYNAMIC_STRING_NULL_PTR);

	// 检查子串范围是否越界
	if (sub_index >= dstr->len || sub_index + sub_count > dstr->len) return DYNAMIC_STRING_NULL_PTR;

	// 在堆上创建并初始化一个新「动态字符串」
	dstr_adt *new_dstr = local_create_dstr_and_init();
	if (new_dstr == DYNAMIC_STRING_NULL_PTR) return DYNAMIC_STRING_NULL_PTR;

	// 计算子串长度，sub_count 为 0 视为到末尾
	const size_t sub_len = sub_count == 0 ? dstr->len - sub_index : sub_count;

	// 调整「动态字符串」容量
	if (!local_capacity_resize_dynamic(new_dstr, sub_len)) {
		free(new_dstr);
		return DYNAMIC_STRING_NULL_PTR;
	}

	// 拷贝子串
	memcpy(new_dstr->data, dstr->data + sub_index, sub_len);
	new_dstr->data[new_dstr->len = sub_len] = '\0';
	return new_dstr;
}

// 克隆
dstr_adt *dstr_clone(const dstr_adt *const dstr) {
	// 断言，开发阶段参数检查
	assert(dstr != DYNAMIC_STRING_NULL_PTR);

	// 如果 dstr 长度为 0，则不进行克隆
	if (dstr->len == 0) return DYNAMIC_STRING_NULL_PTR;

	// 在堆上创建并初始化一个新「动态字符串」
	dstr_adt *new_dstr = local_create_dstr_and_init();
	if (new_dstr == DYNAMIC_STRING_NULL_PTR) return DYNAMIC_STRING_NULL_PTR;

	// 调整「动态字符串」容量
	if (!local_capacity_resize_dynamic(new_dstr, dstr->len)) {
		free(new_dstr);
		return DYNAMIC_STRING_NULL_PTR;
	}

	// 拷贝数据
	memcpy(new_dstr->data, dstr->data, dstr->len);
	new_dstr->data[new_dstr->len = dstr->len] = '\0';
	return new_dstr;
}

// 查找、统计与替换
bool dstr_find_cstr(const dstr_adt *const dstr, const char *const sub, size_t *const out_index, const bool backward) {
	// 局部变量声明
	char *find;

	// 断言，开发阶段参数检查
	assert(dstr != DYNAMIC_STRING_NULL_PTR && sub != DYNAMIC_STRING_NULL_PTR);

	// 获取 sub 长度
	const size_t sub_len = strlen(sub);
	// 如果 sub 长度为 0 或者 sub 长度大于 dstr 长度，则 dstr 一定不包含 sub
	if (sub_len == 0 || sub_len > dstr->len) return false;

	// 执行查找
	if (backward) {
		for (find = dstr->data + dstr->len - sub_len;
		     find >= dstr->data;
		     --find
		) {
			if (strncmp(find, sub, sub_len) == 0) {
				if (out_index != DYNAMIC_STRING_NULL_PTR) *out_index = find - dstr->data;
				return true;
			}
		}
	} else {
		for (find = dstr->data;
		     find <= dstr->data + dstr->len - sub_len;
		     ++find
		) {
			if (strncmp(find, sub, sub_len) == 0) {
				if (out_index != DYNAMIC_STRING_NULL_PTR) *out_index = find - dstr->data;
				return true;
			}
		}
	}

	return false;
}

bool dstr_find(const dstr_adt *const dstr, const dstr_adt *const sub, size_t *const out_index, const bool backward) {
	// 局部变量声明
	char *find;

	// 断言，开发阶段参数检查
	assert(dstr != DYNAMIC_STRING_NULL_PTR && sub != DYNAMIC_STRING_NULL_PTR);

	// 如果 sub 长度为 0 或者 sub 长度大于 dstr 长度，则 dstr 一定不包含 sub
	if (sub->len == 0 || sub->len > dstr->len) return false;

	// 执行查找
	if (backward) {
		for (find = dstr->data + dstr->len - sub->len;
		     find >= dstr->data;
		     --find
		) {
			if (strncmp(find, sub->data, sub->len) == 0) {
				if (out_index != DYNAMIC_STRING_NULL_PTR) *out_index = find - dstr->data;
				return true;
			}
		}
	} else {
		for (find = dstr->data;
		     find <= dstr->data + dstr->len - sub->len;
		     ++find
		) {
			if (strncmp(find, sub->data, sub->len) == 0) {
				if (out_index != DYNAMIC_STRING_NULL_PTR) *out_index = find - dstr->data;
				return true;
			}
		}
	}

	return false;
}

size_t dstr_count_cstr(const dstr_adt *const dstr, const char *const sub) {
	// 局部变量声明
	char *find;
	size_t find_count;

	// 断言，开发阶段参数检查
	assert(dstr != DYNAMIC_STRING_NULL_PTR && sub != DYNAMIC_STRING_NULL_PTR);

	// 获取 sub 长度
	const size_t sub_len = strlen(sub);
	// 如果 sub 长度为 0 或者 sub 长度大于 dstr 长度，则 dstr 一定不包含 sub
	if (sub_len == 0 || sub_len > dstr->len) return 0;

	// 执行查找统计
	for (find_count = 0, find = dstr->data;
	     find <= dstr->data + dstr->len - sub_len;
	) {
		if (strncmp(find, sub, sub_len) == 0) {
			++find_count;
			find += sub_len;
		} else {
			++find;
		}
	}

	return find_count;
}

size_t dstr_count(const dstr_adt *const dstr, const dstr_adt *const sub) {
	// 局部变量声明
	char *find;
	size_t find_count;

	// 断言，开发阶段参数检查
	assert(dstr != DYNAMIC_STRING_NULL_PTR && sub != DYNAMIC_STRING_NULL_PTR);

	// 如果 sub 长度为 0 或者 sub 长度大于 dstr 长度，则 dstr 一定不包含 sub
	if (sub->len == 0 || sub->len > dstr->len) return 0;

	// 执行查找统计
	for (find_count = 0, find = dstr->data;
	     find <= dstr->data + dstr->len - sub->len;
	) {
		if (strncmp(find, sub->data, sub->len) == 0) {
			++find_count;
			find += sub->len;
		} else {
			++find;
		}
	}

	return find_count;
}

bool dstr_find_nth_cstr(
	const dstr_adt *const dstr, const char *const sub, size_t *const out_index, const size_t n,
	const bool backward
) {
	// 局部变量声明
	char *find;
	size_t find_count;

	// 断言，开发阶段参数检查
	assert(dstr != DYNAMIC_STRING_NULL_PTR && sub != DYNAMIC_STRING_NULL_PTR);

	// n 从 1 开始，n 为 0 视为参数无效
	if (n == 0) return false;

	// 获取 sub 长度
	const size_t sub_len = strlen(sub);
	// 如果 sub 长度为 0 或者 sub 长度大于 dstr 长度，则 dstr 一定不包含 sub
	if (sub_len == 0 || sub_len > dstr->len) return false;

	// 执行查找
	if (backward) {
		for (find_count = 0, find = dstr->data + dstr->len - sub_len;
		     find >= dstr->data;
		) {
			if (strncmp(find, sub, sub_len) == 0) {
				++find_count;
				if (find_count == n) {
					if (out_index != DYNAMIC_STRING_NULL_PTR) *out_index = find - dstr->data;
					return true;
				}
				find -= sub_len;
			} else {
				--find;
			}
		}
	} else {
		for (find_count = 0, find = dstr->data;
		     find <= dstr->data + dstr->len - sub_len;
		) {
			if (strncmp(find, sub, sub_len) == 0) {
				++find_count;
				if (find_count == n) {
					if (out_index != DYNAMIC_STRING_NULL_PTR) *out_index = find - dstr->data;
					return true;
				}
				find += sub_len;
			} else {
				++find;
			}
		}
	}


	return false;
}

bool dstr_find_nth(
	const dstr_adt *const dstr, const dstr_adt *const sub, size_t *const out_index, const size_t n,
	const bool backward
) {
	// 局部变量声明
	char *find;
	size_t find_count;

	// 断言，开发阶段参数检查
	assert(dstr != DYNAMIC_STRING_NULL_PTR && sub != DYNAMIC_STRING_NULL_PTR);

	// n 从 1 开始，n 为 0 视为参数无效
	if (n == 0) return false;
	// 如果 sub 长度为 0 或者 sub 长度大于 dstr 长度，则 dstr 一定不包含 sub
	if (sub->len == 0 || sub->len > dstr->len) return false;

	// 执行查找
	if (backward) {
		for (find_count = 0, find = dstr->data + dstr->len - sub->len;
		     find >= dstr->data;
		) {
			if (strncmp(find, sub->data, sub->len) == 0) {
				++find_count;
				if (find_count == n) {
					if (out_index != DYNAMIC_STRING_NULL_PTR) *out_index = find - dstr->data;
					return true;
				}
				find -= sub->len;
			} else {
				--find;
			}
		}
	} else {
		for (find_count = 0, find = dstr->data;
		     find <= dstr->data + dstr->len - sub->len;
		) {
			if (strncmp(find, sub->data, sub->len) == 0) {
				++find_count;
				if (find_count == n) {
					if (out_index != DYNAMIC_STRING_NULL_PTR) *out_index = find - dstr->data;
					return true;
				}
				find += sub->len;
			} else {
				++find;
			}
		}
	}

	return false;
}


int dstr_replace_cstr(
	dstr_adt *const dstr, const char *const old_str, const char *const new_str, const size_t n,
	const bool backward
) {
	// 局部变量声明
	size_t find_count;
	char *find; // 指向当前比较的字符串

	// 断言，开发阶段参数检查
	assert(dstr != DYNAMIC_STRING_NULL_PTR && old_str != DYNAMIC_STRING_NULL_PTR);

	// 获取 old_str 长度
	const size_t old_str_len = strlen(old_str);

	// 如果 old_str 长度为 0 或者长度大于 dstr 长度，则说明 dstr 一定不包含 old_str，视为参数无效
	if (old_str_len == 0 || old_str_len > dstr->len) return DSTR_INVALID_PARAM;

	// 获取 old_str 在 dstr 中出现的次数
	const size_t old_count = dstr_count_cstr(dstr, old_str);
	// 如果 old_str 在 dstr 中没有出现，则视为参数无效
	if (old_count == 0) return DSTR_INVALID_PARAM;

	// 计算 new_str 长度，如果 new_str 为 NULL 则视长度为 0
	const size_t new_str_len = new_str != DYNAMIC_STRING_NULL_PTR ? strlen(new_str) : 0;

	// 如果 new_str 长度大于 old_str，则需要扩容
	if (new_str_len > old_str_len) {
		// 所需的长度：原来的长度 + (new_str 长度 - old_str 长度) * old_str 在 dstr 中出现的次数
		// 安全计算 size_t 乘法（(new_len - old_str_len) * old_count），防止溢出
		size_t added_length;
		if (!safe_size_mul(new_str_len - old_str_len, old_count, &added_length)) {
			return DSTR_OVERFLOW;
		}
		// 安全计算 size_t 加法（dstr->len + added_length），防止溢出
		size_t needed_length;
		if (!safe_size_add(dstr->len, added_length, &needed_length)) {
			return DSTR_OVERFLOW;
		}
		// 执行扩容
		if (!local_capacity_resize_dynamic(dstr, needed_length)) {
			return DSTR_MEMORY_ALLOC_FAILED;
		}
	}

	// 执行替换
	if (backward) {
		for (find_count = 0, find = dstr->data + dstr->len - old_str_len;
		     find >= dstr->data && find_count < old_count
		     && (n == 0 ? true : find_count < n);
		) {
			if (strncmp(find, old_str, old_str_len) == 0) {
				++find_count;
				// 替换
				// 如果 old_str 长度与 new_str 不同，且当前 old_str 不位于末尾，则需要移动字符串
				if (old_str_len != new_str_len && find + old_str_len < dstr->data + dstr->len) {
					memmove(
						find + new_str_len,
						find + old_str_len,
						dstr->data + dstr->len - find - old_str_len
					);
				}
				// 如果 new_str 长度不为 0，则需要拷贝
				if (new_str_len > 0) {
					memcpy(find, new_str, new_str_len);
				}
				// 更新长度
				dstr->len -= old_str_len;
				dstr->len += new_str_len;
				dstr->data[dstr->len] = '\0';

				find -= old_str_len;
			} else {
				--find;
			}
		}
	} else {
		for (find_count = 0, find = dstr->data;
		     find <= dstr->data + dstr->len - old_str_len && find_count < old_count
		     && (n == 0 ? true : find_count < n);
		) {
			if (strncmp(find, old_str, old_str_len) == 0) {
				++find_count;
				// 替换
				// 如果 old_str 长度与 new_str 不同，且当前 old_str 不位于末尾，则需要移动字符串
				if (old_str_len != new_str_len && find + old_str_len < dstr->data + dstr->len) {
					memmove(
						find + new_str_len,
						find + old_str_len,
						dstr->data + dstr->len - find - old_str_len
					);
				}
				// 如果 new_str 长度不为 0，则需要拷贝
				if (new_str_len > 0) {
					memcpy(find, new_str, new_str_len);
				}
				// 更新长度
				dstr->len -= old_str_len;
				dstr->len += new_str_len;
				dstr->data[dstr->len] = '\0';

				find += new_str_len;
			} else {
				++find;
			}
		}
	}

	if (new_str_len < old_str_len) {
		local_capacity_resize_dynamic(dstr, dstr->len - (old_str_len - new_str_len) * old_count);
	}

	return DSTR_SUCCESS;
}


int dstr_replace(
	dstr_adt *const dstr, const dstr_adt *const old_str, const dstr_adt *const new_str,
	const size_t n, const bool backward
) {
	// 局部变量声明
	size_t find_count;
	char *find; // 指向当前比较的字符串


	// 断言，开发阶段参数检查
	assert(dstr != DYNAMIC_STRING_NULL_PTR && old_str != DYNAMIC_STRING_NULL_PTR);

	// 如果 old_str 长度为 0 或者长度大于 dstr 长度，则说明 dstr 一定不包含 old_str，视为参数无效
	if (old_str->len == 0 || old_str->len > dstr->len) return DSTR_INVALID_PARAM;

	// 获取 old_str 在 dstr 中出现的次数
	const size_t old_count = dstr_count(dstr, old_str);
	// 如果 old_str 在 dstr 中没有出现，则视为参数无效
	if (old_count == 0) return DSTR_INVALID_PARAM;

	// 计算 new_str 长度，如果 new_str 为 NULL 则视长度为 0
	const size_t new_str_len = new_str != DYNAMIC_STRING_NULL_PTR ? new_str->len : 0;

	// 如果 new_str 长度大于 old_str，则需要扩容
	if (new_str_len > old_str->len) {
		// 所需的长度：原来的长度 + (new_str 长度 - old_str 长度) * old_str 在 dstr 中出现的次数
		// 安全计算 size_t 乘法（(new_len - old_str->len) * old_count），防止溢出
		size_t added_length;
		if (!safe_size_mul(new_str_len - old_str->len, old_count, &added_length)) {
			return DSTR_OVERFLOW;
		}
		// 安全计算 size_t 加法（dstr->len + added_length），防止溢出
		size_t needed_length;
		if (!safe_size_add(dstr->len, added_length, &needed_length)) {
			return DSTR_OVERFLOW;
		}
		// 执行扩容
		if (!local_capacity_resize_dynamic(dstr, needed_length)) {
			return DSTR_MEMORY_ALLOC_FAILED;
		}
	}

	// 执行替换
	if (backward) {
		for (find_count = 0, find = dstr->data + dstr->len - old_str->len;
		     find >= dstr->data && find_count < old_count
		     && (n == 0 ? true : find_count < n);
		) {
			if (strncmp(find, old_str->data, old_str->len) == 0) {
				++find_count;
				// 替换
				// 如果 old_str 长度与 new_str 不同，且当前 old_str 不位于末尾，则需要移动字符串
				if (old_str->len != new_str_len && find + old_str->len < dstr->data + dstr->len) {
					memmove(
						find + new_str_len,
						find + old_str->len,
						dstr->data + dstr->len - find - old_str->len
					);
				}
				// 如果 new_str 长度不为 0，则需要拷贝
				if (new_str_len > 0) {
					memcpy(find, new_str->data, new_str_len);
				}
				// 更新长度
				dstr->len -= old_str->len;
				dstr->len += new_str_len;
				dstr->data[dstr->len] = '\0';

				find -= old_str->len;
			} else {
				--find;
			}
		}
	} else {
		for (find_count = 0, find = dstr->data;
		     find <= dstr->data + dstr->len - old_str->len && find_count < old_count
		     && (n == 0 ? true : find_count < n);
		) {
			if (strncmp(find, old_str->data, old_str->len) == 0) {
				++find_count;
				// 替换
				// 如果 old_str 长度与 new_str 不同，且当前 old_str 不位于末尾，则需要移动字符串
				if (old_str->len != new_str_len && find + old_str->len < dstr->data + dstr->len) {
					memmove(
						find + new_str_len,
						find + old_str->len,
						dstr->data + dstr->len - find - old_str->len
					);
				}
				// 如果 new_str 长度不为 0，则需要拷贝
				if (new_str_len > 0) {
					memcpy(find, new_str->data, new_str_len);
				}
				// 更新长度
				dstr->len -= old_str->len;
				dstr->len += new_str_len;
				dstr->data[dstr->len] = '\0';

				find += new_str_len;
			} else {
				++find;
			}
		}
	}

	// 缩容
	if (new_str_len < old_str->len) {
		// 如果能执行到此处，则以下 size_t 计算一定不会溢出
		local_capacity_resize_dynamic(dstr, dstr->len - (old_str->len - new_str_len) * old_count);
	}

	return DSTR_SUCCESS;
}

// 判断与比较
bool dstr_starts_with_cstr(const dstr_adt *const dstr, const char *const prefix) {
	// 断言，开发阶段参数检查
	assert(dstr != DYNAMIC_STRING_NULL_PTR && prefix != DYNAMIC_STRING_NULL_PTR);

	// 获取 prefix 长度
	const size_t prefix_len = strlen(prefix);

	// 如果 prefix 长度为 0，或者 prefix 长度大于 dstr 长度，则一定不以 prefix 开头
	if (prefix_len == 0 || prefix_len > dstr->len) return false;

	// 进行字符串比较，以判断是否以 prefix 开头
	return strncmp(dstr->data, prefix, prefix_len) == 0;
}


bool dstr_starts_with(const dstr_adt *const dstr, const dstr_adt *const prefix) {
	// 断言，开发阶段参数检查
	assert(dstr != DYNAMIC_STRING_NULL_PTR && prefix != DYNAMIC_STRING_NULL_PTR);

	// 如果 prefix 长度为 0，或者 prefix 长度大于 dstr 长度，则一定不以 prefix 开头
	if (prefix->len == 0 || prefix->len > dstr->len) return false;

	// 进行字符串比较，以判断是否以 prefix 开头
	return strncmp(dstr->data, prefix->data, prefix->len) == 0;
}


bool dstr_ends_with_cstr(const dstr_adt *const dstr, const char *const suffix) {
	// 断言，开发阶段参数检查
	assert(dstr != DYNAMIC_STRING_NULL_PTR && suffix != DYNAMIC_STRING_NULL_PTR);

	// 获取 suffix 长度
	const size_t suffix_len = strlen(suffix);

	// 如果 suffix 长度为 0，或者 suffix 长度大于 dstr 长度，则一定不以 suffix 结尾
	if (suffix_len == 0 || suffix_len > dstr->len) return false;

	// 进行字符串比较，以判断是否以 suffix 结尾
	return strncmp(dstr->data + dstr->len - suffix_len,
		       suffix, suffix_len) == 0;
}


bool dstr_ends_with(const dstr_adt *const dstr, const dstr_adt *const suffix) {
	// 断言，开发阶段参数检查
	assert(dstr != DYNAMIC_STRING_NULL_PTR && suffix != DYNAMIC_STRING_NULL_PTR);

	// 如果 suffix 长度为 0，或者 suffix 长度大于 dstr 长度，则一定不以 suffix 结尾
	if (suffix->len == 0 || suffix->len > dstr->len) return false;

	// 进行字符串比较，以判断是否以 suffix 结尾
	return strncmp(dstr->data + dstr->len - suffix->len,
		       suffix->data, suffix->len) == 0;
}

bool dstr_contains_cstr(const dstr_adt *const dstr, const char *const sub) {
	// 断言，开发阶段参数检查
	assert(dstr != DYNAMIC_STRING_NULL_PTR && sub != DYNAMIC_STRING_NULL_PTR);

	// 获取 sub 串长度
	const size_t sub_len = strlen(sub);

	// 如果 sub 长度为 0，或者 dstr 长度小于 sub 长度，则一定不包含
	if (sub_len == 0 || sub_len > dstr->len) return false;

	// 进行字符串查找，以判断是否包含
	return strstr(dstr->data, sub) != DYNAMIC_STRING_NULL_PTR;
}

bool dstr_contains(const dstr_adt *const dstr, const dstr_adt *const sub) {
	// 断言，开发阶段参数检查
	assert(dstr != DYNAMIC_STRING_NULL_PTR && sub != DYNAMIC_STRING_NULL_PTR);

	// 如果 sub 长度为 0，或者 dstr 长度小于 sub 长度，则一定不包含
	if (sub->len == 0 || sub->len > dstr->len) return false;

	// 进行字符串查找，以判断是否包含
	return strstr(dstr->data, sub->data) != DYNAMIC_STRING_NULL_PTR;
}

bool dstr_equals_cstr(const dstr_adt *const dstr, const char *const cstr) {
	// 断言，开发阶段参数检查
	assert(dstr != DYNAMIC_STRING_NULL_PTR && cstr != DYNAMIC_STRING_NULL_PTR);

	// 获取 cstr 字符串长度
	const size_t cstr_len = strlen(cstr);

	// 长度不同则一定不同
	if (cstr_len != dstr->len) return false;

	// 检测空指针，避免 strncmp 未定义行为
	// 如果 dstr->data 为空，视为不相等
	if (dstr->data == DYNAMIC_STRING_NULL_PTR) {
		return false;
	}

	// 否则进行比较，以判断是否相等
	return strncmp(dstr->data, cstr, cstr_len) == 0;
}

bool dstr_equals(const dstr_adt *const dstr_1, const dstr_adt *const dstr_2) {
	// 断言，开发阶段参数检查
	assert(dstr_1 != DYNAMIC_STRING_NULL_PTR && dstr_2 != DYNAMIC_STRING_NULL_PTR);

	// 长度不同则一定不同
	if (dstr_1->len != dstr_2->len) return false;

	// 检测空指针，避免 strncmp 未定义行为
	// 如果都为空，视为相等
	if (dstr_1->data == DYNAMIC_STRING_NULL_PTR && dstr_2->data == DYNAMIC_STRING_NULL_PTR) {
		return true;
	}
	// 如果有一个为空，另一个不为空，则视为不相等
	if (dstr_1->data == DYNAMIC_STRING_NULL_PTR || dstr_2->data == DYNAMIC_STRING_NULL_PTR) {
		return false;
	}

	// 都不为空，则进行比较，以判断是否相等
	return strncmp(dstr_1->data, dstr_2->data, dstr_2->len) == 0;
}

int dstr_compare_cstr(const dstr_adt *const dstr, const char *const cstr) {
	// 断言，开发阶段参数检查
	assert(dstr != DYNAMIC_STRING_NULL_PTR && cstr != DYNAMIC_STRING_NULL_PTR);

	// 检测空指针，避免 strcmp 未定义行为
	// 如果 dstr->data 为空，视为小于 cstr
	if (dstr->data == DYNAMIC_STRING_NULL_PTR) {
		return -1;
	}

	// 否则进行比较
	return strcmp(dstr->data, cstr);
}

int dstr_compare(const dstr_adt *const dstr_1, const dstr_adt *const dstr_2) {
	// 断言，开发阶段参数检查
	assert(dstr_1 != DYNAMIC_STRING_NULL_PTR && dstr_2 != DYNAMIC_STRING_NULL_PTR);

	// 检测空指针，避免 strcmp 未定义行为
	// 如果都为空，视为相等
	if (dstr_1->data == DYNAMIC_STRING_NULL_PTR && dstr_2->data == DYNAMIC_STRING_NULL_PTR) {
		return 0;
	}
	// 否则视为不为空指针的大于为空指针的
	// dstr_1->data 为空，视为小于 dstr_2
	if (dstr_1->data == DYNAMIC_STRING_NULL_PTR) {
		return -1;
	}
	// dstr_2->data 为空，视为小于 dstr_1
	if (dstr_2->data == DYNAMIC_STRING_NULL_PTR) {
		return 1;
	}

	// 否则进行比较
	return strcmp(dstr_1->data, dstr_2->data);
}
