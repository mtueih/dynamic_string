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

#define DSTR_CACHELINE_SIZE 64

// ADT 类型定义
struct dynamic_string {
	char *data;
	size_t len;
	size_t cap;
	size_t min_cap;
};

// 静态函数定义
// 计算对齐值
static size_t align_up(const size_t base, const size_t align) {
	size_t remainder;

	// 防止除 0
	if (align == 0) return base;

	// 如果 align 是 2 的幂，则使用更高效的计算方法
	if ((align & (align - 1)) == 0) {
		return (base & (align - 1)) == 0 ? base : ((base + align - 1) & ~(align - 1));
	}

	// 常规计算方法
	// 如果 base 已经是 align 的倍数，则直接返回
	remainder = base % align;
	if (remainder == 0) return base;

	return base % align == 0 ? base : (base + align - 1) / align * align;
}

// 动态调整一个「动态字符串」的容量
static int capacity_resize_dynamic(dstr_adt *const dstr, const size_t new_len) {
	char *new_cstr = NULL;
	size_t new_cap = 0; // 不低于保底值目标容量
	size_t adjusted_cap = 0; // 基于目标容量调整后的容量，为目标容量 1.5 倍
	size_t cache_line_aligned_cap = 0; // 对齐到缓存行大小后的容量

	// 安全计算 size_t 加法，防止溢出
	// new_cap = new_len + 1;
	if (!safe_size_add(new_len, 1, &new_cap)) {
		return DSTR_OVERFLOW;
	}

	// 确保容量不低于保底值
	if (new_cap < dstr->min_cap) {
		new_cap = dstr->min_cap;
	}

	// 如果目标容量与当前容量相同，则直接返回
	if (new_cap == dstr->cap) {
		return DSTR_SUCCESS;
	}

	// 如果目标容量小于当前容量，则延迟减容，仅当新容量小于当前容量的 1/4 时，才减容
	// 即，如果目标容量小于当前容量，但是又大于当前容量的 1/4，则不进行减容
	// 右移操作无溢出风险
	if (new_cap < dstr->cap && new_cap > dstr->cap >> 2) {
		return DSTR_SUCCESS;
	}

	// 如果目标容量大于当前容量，则进行容量预分配，分配 1.5 倍
	if (new_cap > dstr->cap) {
		// 尝试预分配内存
		// 安全计算 size_t 加法，防止溢出
		// adjusted_cap = new_cap + new_cap >> 1;
		safe_size_add(new_cap, new_cap >> 1, &adjusted_cap);
	}


	// 如果 adjusted_cap > new_cap，则基于 adjusted_cap 对齐
	if (adjusted_cap > new_cap) {
		// 计算与缓存行对齐的容量
		// 安全执行向上对齐，防止溢出
		safe_size_align_up(adjusted_cap, DSTR_CACHELINE_SIZE, &cache_line_aligned_cap);
	} else {
		// 否则基于 new_cap 对齐
		// 计算与缓存行对齐的容量
		// 安全执行向上对齐，防止溢出
		safe_size_align_up(new_cap, DSTR_CACHELINE_SIZE, &cache_line_aligned_cap);
	}


	// 如果 cache_line_aligned_cap > new_cap，则说明基于 adjusted_cap 或 new_cap 的对齐值计算成功
	// 此时尝试分配 cache_line_aligned_cap 容量的内存
	if (cache_line_aligned_cap > new_cap) {
		new_cstr = (char *) realloc(dstr->data, cache_line_aligned_cap);
	} else {
		// 否则说明不管是基于 adjusted_cap 对齐，
		// 还是基于 new_cap 的对齐，
		// 都失败了，此时一定回退到直接分配 new_cap 容量的内存
		new_cstr = (char *) realloc(dstr->data, new_cap);
	}

	// 如果分配成功，则跳转到收尾工作
	if (new_cstr) goto memory_allocation_successful;
	// 如果失败，有两种可能：
	// 1. 一种是分配 new_cap 容量的内存失败
	//	此时毫无悬念内存分配失败了
	// 2. 另一种可能是分配 cache_line_aligned_cap 容量的内存失败
	//	此时也有两种可能，一种是基于 adjusted_cap 的对齐值，
	//		另一种则是基于 new_cap 的对齐值
	//	如果是前者，则说明 adjusted_cap 计算成功了，而基于它的对齐值分配失败
	//	那就尝试回退到 adjusted_cap 本身，
	//	但如果 cache_line_aligned_cap 不比 adjusted_cap 大，那就没有尝试的必要了
	if (adjusted_cap > new_cap && cache_line_aligned_cap > adjusted_cap) {
		new_cstr = (char *) realloc(dstr->data, adjusted_cap);
	} else if (cache_line_aligned_cap > new_cap) {
		// 否则，后者，说明 adjusted_cap 计算失败，
		// cache_line_aligned_cap 是基于 new_cap 的，
		// 那就尝试回退到 new_cap 本身，
		// 同样，如果 cache_line_aligned_cap 不比 new_cap 大，那就没有尝试的必要了
		new_cstr = (char *) realloc(dstr->data, new_cap);
	} else {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	// 如果分配成功，则跳转到收尾工作
	if (new_cstr) goto memory_allocation_successful;



memory_allocation_successful:
	dstr->data = new_cstr;
	dstr->cap = adjusted_cap;

	if (adjusted_cap <= dstr->len) {
		dstr->len -= dstr->len + 1 - adjusted_cap;
		dstr->data[dstr->len] = '\0';
	}

	return true;
}

// 调整容量
static bool capacity_resize(dstr_adt *const dstr, const size_t new_cap) {
	char *new_cstr;

	new_cstr = (char *) realloc(dstr->data, new_cap);
	if (new_cstr == NULL) return false;

	dstr->data = new_cstr;
	dstr->cap = new_cap;

	if (new_cap <= dstr->len) {
		dstr->len -= dstr->len + 1 - new_cap;
		dstr->data[dstr->len] = '\0';
	}

	return true;
}

// API 函数定义
// 创建、销毁、清空
dstr_adt *dstr_create(const char *const cstr) {
	dstr_adt *new_dstr;
	size_t cstr_len;

	new_dstr = malloc(sizeof(struct dynamic_string));
	if (new_dstr == NULL) return NULL;

	*new_dstr = (struct dynamic_string){0};

	if (cstr != NULL && (cstr_len = strlen(cstr)) != 0) {
		if (!capacity_resize_dynamic(new_dstr, cstr_len)) {
			free(new_dstr);
			return NULL;
		}
		memcpy(new_dstr->data, cstr, cstr_len);
		new_dstr->data[new_dstr->len = cstr_len] = '\0';
	}
	return new_dstr;
}

void dstr_destroy(dstr_adt *const dstr) {
	assert(dstr != NULL);

	if (dstr->data != NULL) free(dstr->data);
	free(dstr);
}

void dstr_clear(dstr_adt *const dstr) {
	assert(dstr != NULL);

	if (dstr->data == NULL || dstr->len == 0) return;

	dstr->data[0] = '\0';
	dstr->len = 0;
}

// 属性获取与设置
char *dstr_cstr(dstr_adt *const dstr) {
	assert(dstr != NULL);

	return dstr->data;
}

const char *dstr_cstr_const(const dstr_adt *const dstr) {
	assert(dstr != NULL);

	return dstr->data;
}

size_t dstr_length(const dstr_adt *const dstr) {
	assert(dstr != NULL);

	return dstr->len;
}

size_t dstr_capacity(const dstr_adt *const dstr) {
	assert(dstr != NULL);

	return dstr->cap;
}

int dstr_set_capacity(dstr_adt *const dstr, const size_t new_capacity) {
	assert(dstr != NULL);

	if (!capacity_resize(dstr, new_capacity)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	dstr->min_cap = new_capacity;
	return DSTR_SUCCESS;
}

// 复制、追加、插入、删除
// 复制、追加、插入完整现有字符串到目标字符串
int dstr_cpy_cstr(dstr_adt *const dest, const char *const src) {
	size_t src_len;

	assert(dest != NULL && src != NULL);

	if ((src_len = strlen(src)) == 0) return DSTR_INVALID_PARAM;

	if (!capacity_resize_dynamic(dest, src_len)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	memcpy(dest->data, src, src_len);
	dest->data[dest->len = src_len] = '\0';
	return DSTR_SUCCESS;
}

int dstr_cpy(dstr_adt *const dest, const dstr_adt *const src) {
	assert(dest != NULL && src != NULL);

	if (src->len == 0) return DSTR_INVALID_PARAM;

	if (!capacity_resize_dynamic(dest, src->len)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	memcpy(dest->data, src->data, src->len);
	dest->data[dest->len = src->len] = '\0';
	return DSTR_SUCCESS;
}

int dstr_cat_cstr(dstr_adt *const dest, const char *const src) {
	size_t src_len;

	assert(dest != NULL && src != NULL);

	if ((src_len = strlen(src)) == 0) return DSTR_INVALID_PARAM;

	if (!capacity_resize_dynamic(dest, dest->len + src_len)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	memcpy(dest->data + dest->len, src, src_len);
	dest->data[dest->len += src_len] = '\0';
	return DSTR_SUCCESS;
}

int dstr_cat(dstr_adt *const dest, const dstr_adt *const src) {
	assert(dest != NULL && src != NULL);

	if (src->len == 0) return DSTR_INVALID_PARAM;

	if (!capacity_resize_dynamic(dest, dest->len + src->len)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}
	memcpy(dest->data + dest->len, src->data, src->len);
	dest->data[dest->len += src->len] = '\0';
	return DSTR_SUCCESS;
}

int dstr_insert_cstr(dstr_adt *const dest, const size_t index, const char *const src) {
	size_t src_len;

	assert(dest != NULL && src != NULL);

	if (index > dest->len) return DSTR_INVALID_PARAM;
	if ((src_len = strlen(src)) == 0) return DSTR_INVALID_PARAM;

	if (!capacity_resize_dynamic(dest, dest->len + src_len)) {
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
	assert(dest != NULL && src != NULL);

	if (index > dest->len || src->len == 0) return DSTR_INVALID_PARAM;

	if (!capacity_resize_dynamic(dest, dest->len + src->len)) {
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
	size_t src_len, sub_len;

	assert(dest != NULL && src != NULL);

	src_len = strlen(src);
	if (sub_index >= src_len || sub_index + sub_count > src_len) return DSTR_INVALID_PARAM;

	sub_len = sub_count == 0 ? src_len - sub_index : sub_count;

	if (!capacity_resize_dynamic(dest, sub_len)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	memcpy(dest->data, src + sub_index, sub_len);
	dest->data[dest->len = sub_len] = '\0';
	return DSTR_SUCCESS;
}

int dstr_cpy_sub(dstr_adt *const dest, const dstr_adt *const src, const size_t sub_index, const size_t sub_count) {
	size_t sub_len;

	assert(dest != NULL && src != NULL);

	if (sub_index >= src->len || sub_index + sub_count > src->len) return DSTR_INVALID_PARAM;

	sub_len = sub_count == 0 ? src->len - sub_index : sub_count;

	if (!capacity_resize_dynamic(dest, sub_len)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	memcpy(dest->data, src->data + sub_index, sub_len);
	dest->data[dest->len = sub_len] = '\0';
	return DSTR_SUCCESS;
}

int dstr_cat_sub_cstr(dstr_adt *const dest, const char *const src, const size_t sub_index, const size_t sub_count) {
	size_t src_len, sub_len;

	assert(dest != NULL && src != NULL);

	src_len = strlen(src);
	if (sub_index >= src_len || sub_index + sub_count > src_len) return DSTR_INVALID_PARAM;

	sub_len = sub_count == 0 ? src_len - sub_index : sub_count;

	if (!capacity_resize_dynamic(dest, dest->len + sub_len)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	memcpy(dest->data + dest->len, src + sub_index, sub_len);
	dest->data[dest->len += sub_len] = '\0';
	return DSTR_SUCCESS;
}

int dstr_cat_sub(dstr_adt *const dest, const dstr_adt *const src, const size_t sub_index, const size_t sub_count) {
	size_t sub_len;

	assert(dest != NULL && src != NULL);

	if (sub_index >= src->len || sub_index + sub_count > src->len) return DSTR_INVALID_PARAM;

	sub_len = sub_count == 0 ? src->len - sub_index : sub_count;

	if (!capacity_resize_dynamic(dest, dest->len + sub_len)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	memcpy(dest->data + dest->len, src->data + sub_index, sub_len);
	dest->data[dest->len += sub_len] = '\0';
	return DSTR_SUCCESS;
}

int dstr_insert_sub_cstr(dstr_adt *const dest, const size_t index, const char *const src, const size_t sub_index,
			 const size_t sub_count
) {
	size_t src_len, sub_len;

	assert(dest != NULL && src != NULL);

	if (index > dest->len) return DSTR_INVALID_PARAM;
	src_len = strlen(src);
	if (sub_index >= src_len || sub_index + sub_count > src_len) return DSTR_INVALID_PARAM;

	sub_len = sub_count == 0 ? src_len - sub_index : sub_count;

	if (!capacity_resize_dynamic(dest, dest->len + sub_len)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	if (index < dest->len) {
		memmove(dest->data + index + sub_len,
			dest->data + index,
			dest->len - index
		);
	}
	memcpy(dest->data + index, src + sub_index, sub_len);
	dest->data[dest->len += sub_len] = '\0';
	return DSTR_SUCCESS;
}

int dstr_insert_sub(dstr_adt *const dest, const size_t index, const dstr_adt *const src, const size_t sub_index,
		    const size_t sub_count
) {
	size_t sub_len;

	assert(dest != NULL && src != NULL);

	if (index > dest->len) return DSTR_INVALID_PARAM;
	if (sub_index >= src->len || sub_index + sub_count > src->len) return DSTR_INVALID_PARAM;

	sub_len = sub_count == 0 ? src->len - sub_index : sub_count;

	if (!capacity_resize_dynamic(dest, dest->len + sub_len)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	if (index < dest->len) {
		memmove(dest->data + index + sub_len,
			dest->data + index,
			dest->len - index
		);
	}
	memcpy(dest->data + index, src->data + sub_index, sub_len);
	dest->data[dest->len += sub_len] = '\0';
	return DSTR_SUCCESS;
}

// 删除子串
void dstr_remove(dstr_adt *const dstr, const size_t sub_index, const size_t sub_count) {
	size_t sub_len;

	assert(dstr != NULL);

	if (sub_index >= dstr->len || sub_index + sub_count > dstr->len) return;

	sub_len = sub_count == 0 ? dstr->len - sub_index : sub_count;
	if (sub_count != 0 && sub_index + sub_count < dstr->len) {
		memmove(dstr->data + sub_index,
			dstr->data + sub_index + sub_count,
			dstr->len - sub_index - sub_count
		);
	}

	dstr->data[dstr->len -= sub_len] = '\0';
	capacity_resize_dynamic(dstr, dstr->len);
}

// 删除特定内容
void dstr_trim(dstr_adt *const dstr, const char *const trim_chars) {
	char *find;
	size_t trim_chars_len;

	assert(dstr != NULL);

	if (dstr->len == 0) return;

	// 去除前导空白字符或指定字符
	for (find = dstr->data;
	     find < dstr->data + dstr->len;
	     ++find
	) {
		if (trim_chars == NULL || strlen(trim_chars) == 0) {
			if (!isspace(*find)) break;
		} else {
			if (strchr(trim_chars, *find) == NULL) break;
		}
	}

	if (find > dstr->data) {
		trim_chars_len = find - dstr->data;
		memmove(dstr->data, find, dstr->len - trim_chars_len);
		dstr->data[dstr->len -= trim_chars_len] = '\0';
	}

	// 去除尾部空白字符或指定字符
	for (find = dstr->data + dstr->len - 1;
	     find >= dstr->data;
	     --find) {
		if (trim_chars == NULL || strlen(trim_chars) == 0) {
			if (!isspace(*find)) break;
		} else {
			if (strchr(trim_chars, *find)) break;
		}
	}

	if (find < dstr->data + dstr->len - 1) {
		trim_chars_len = dstr->data + dstr->len - 1 - find;
		dstr->data[dstr->len -= trim_chars_len] = '\0';
	}

	capacity_resize_dynamic(dstr, dstr->len);
}

// 格式化写入
int dstr_printf(dstr_adt *const dstr, const char *const format, ...) {
	va_list args, temp_args;
	size_t old_cap;
	int needed_len, written_len;

	assert(dstr != NULL && format != NULL);

	va_start(args, format);

	// 计算输出长度
	va_copy(temp_args, args);
	needed_len = vsnprintf(NULL, 0, format, temp_args);
	va_end(temp_args);

	if (needed_len <= 0) {
		va_end(args);
		return DSTR_INVALID_PARAM;
	}

	// 保存旧容量
	old_cap = dstr->cap;
	if (!capacity_resize_dynamic(dstr, needed_len)) {
		return DSTR_MEMORY_ALLOC_FAILED;
	}

	written_len = vsnprintf(dstr->data, needed_len + 1, format, args);
	va_end(args);

	if (written_len < 0) {
		capacity_resize(dstr, old_cap);
		return DSTR_UNKNOWN_ERROR;
	}

	dstr->data[dstr->len = written_len] = '\0';
	return DSTR_SUCCESS;
}


// 从现有字符串生成新字符串
// 提取子串
dstr_adt *dstr_sub_cstr(const char *const cstr, const size_t sub_index, const size_t sub_count) {
	dstr_adt *new_dstr;
	size_t cstr_len, sub_len;

	assert(cstr != NULL);

	cstr_len = strlen(cstr);
	if (sub_index >= cstr_len || sub_index + sub_count > cstr_len) return NULL;

	new_dstr = malloc(sizeof(struct dynamic_string));
	if (new_dstr == NULL) return NULL;

	*new_dstr = (struct dynamic_string){0};

	sub_len = sub_count == 0 ? cstr_len - sub_index : sub_count;

	if (!capacity_resize_dynamic(new_dstr, sub_len)) {
		free(new_dstr);
		return NULL;
	}

	memcpy(new_dstr->data, cstr + sub_index, sub_len);
	new_dstr->data[new_dstr->len = sub_len] = '\0';
	return new_dstr;
}

dstr_adt *dstr_sub(const dstr_adt *const dstr, const size_t sub_index, const size_t sub_count) {
	size_t sub_len;
	dstr_adt *new_dstr;

	assert(dstr != NULL);

	if (sub_index >= dstr->len || sub_index + sub_count > dstr->len) return NULL;

	new_dstr = malloc(sizeof(struct dynamic_string));
	if (new_dstr == NULL) return NULL;

	*new_dstr = (struct dynamic_string){0};

	sub_len = sub_count == 0 ? dstr->len - sub_index : sub_count;

	if (!capacity_resize_dynamic(new_dstr, sub_len)) {
		free(new_dstr);
		return NULL;
	}

	memcpy(new_dstr->data, dstr->data + sub_index, sub_len);
	new_dstr->data[new_dstr->len = sub_len] = '\0';
	return new_dstr;
}

// 克隆
dstr_adt *dstr_clone(const dstr_adt *const dstr) {
	dstr_adt *new_dstr;

	assert(dstr != NULL);

	new_dstr = malloc(sizeof(struct dynamic_string));
	if (new_dstr == NULL) return NULL;

	*new_dstr = (struct dynamic_string){0};

	if (!capacity_resize_dynamic(new_dstr, dstr->len)) {
		free(new_dstr);
		return NULL;
	}

	memcpy(new_dstr->data, dstr->data, dstr->len);
	new_dstr->data[new_dstr->len = dstr->len] = '\0';
	return new_dstr;
}

// 查找、统计与替换
bool dstr_find_cstr(const dstr_adt *const dstr, const char *const sub, size_t *const out_index, const bool backward) {
	size_t sub_len;
	char *find;

	assert(dstr != NULL && sub != NULL);

	sub_len = strlen(sub);
	if (sub_len == 0 || sub_len > dstr->len) return false;

	if (backward) {
		for (find = dstr->data + dstr->len - sub_len;
		     find >= dstr->data;
		     --find
		) {
			if (strncmp(find, sub, sub_len) == 0) {
				if (out_index != NULL) *out_index = find - dstr->data;
				return true;
			}
		}
	} else {
		for (find = dstr->data;
		     find <= dstr->data + dstr->len - sub_len;
		     ++find
		) {
			if (strncmp(find, sub, sub_len) == 0) {
				if (out_index != NULL) *out_index = find - dstr->data;
				return true;
			}
		}
	}

	return false;
}

bool dstr_find(const dstr_adt *const dstr, const dstr_adt *const sub, size_t *const out_index, const bool backward) {
	char *find;

	assert(dstr != NULL && sub != NULL);

	if (sub->len == 0 || sub->len > dstr->len) return false;

	if (backward) {
		for (find = dstr->data + dstr->len - sub->len;
		     find >= dstr->data;
		     --find
		) {
			if (strncmp(find, sub->data, sub->len) == 0) {
				if (out_index != NULL) *out_index = find - dstr->data;
				return true;
			}
		}
	} else {
		for (find = dstr->data;
		     find <= dstr->data + dstr->len - sub->len;
		     ++find
		) {
			if (strncmp(find, sub->data, sub->len) == 0) {
				if (out_index != NULL) *out_index = find - dstr->data;
				return true;
			}
		}
	}

	return false;
}

size_t dstr_count_cstr(const dstr_adt *const dstr, const char *const sub) {
	size_t sub_len;
	char *find;
	size_t find_count;

	assert(dstr != NULL && sub != NULL);

	sub_len = strlen(sub);
	if (sub_len == 0 || sub_len > dstr->len) return 0;

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
	char *find;
	size_t find_count;

	assert(dstr != NULL && sub != NULL);

	if (sub->len == 0 || sub->len > dstr->len) return 0;

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

bool dstr_find_nth_cstr(const dstr_adt *const dstr, const char *const sub, size_t *const out_index, const size_t n,
			const bool backward
) {
	size_t sub_len;
	char *find;
	size_t find_count;

	assert(dstr != NULL && sub != NULL);

	if (n == 0) return false;
	sub_len = strlen(sub);
	if (sub_len == 0 || sub_len > dstr->len) return false;

	if (backward) {
		for (find_count = 0, find = dstr->data + dstr->len - sub_len;
		     find >= dstr->data;
		) {
			if (strncmp(find, sub, sub_len) == 0) {
				++find_count;
				if (find_count == n) {
					if (out_index != NULL) *out_index = find - dstr->data;
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
					if (out_index != NULL) *out_index = find - dstr->data;
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

bool dstr_find_nth(const dstr_adt *const dstr, const dstr_adt *const sub, size_t *const out_index, const size_t n,
		   const bool backward
) {
	char *find;
	size_t find_count;

	assert(dstr != NULL && sub != NULL);

	if (n == 0) return false;
	if (sub->len == 0 || sub->len > dstr->len) return false;

	if (backward) {
		for (find_count = 0, find = dstr->data + dstr->len - sub->len;
		     find >= dstr->data;
		) {
			if (strncmp(find, sub->data, sub->len) == 0) {
				++find_count;
				if (find_count == n) {
					if (out_index != NULL) *out_index = find - dstr->data;
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
					if (out_index != NULL) *out_index = find - dstr->data;
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


int dstr_replace_cstr(dstr_adt *const dstr, const char *const old_str, const char *const new_str, const size_t n,
		      const bool backward
) {
	// 局部变量声明
	size_t find_count;
	char *find; // 指向当前比较的字符串
	size_t old_len, new_len;
	size_t old_count;

	// 参数检查
	assert(dstr != NULL);
	assert(old_str != NULL);

	old_len = strlen(old_str);
	if (old_len == 0 || old_len > dstr->len) return DSTR_INVALID_PARAM;

	old_count = dstr_count_cstr(dstr, old_str);
	if (old_count == 0) return DSTR_INVALID_PARAM;

	new_len = new_str != NULL ? strlen(new_str) : 0;

	if (new_len > old_len) {
		if (!capacity_resize_dynamic(dstr, dstr->len + (new_len - old_len) * old_count)) {
			return DSTR_MEMORY_ALLOC_FAILED;
		}
	}
	// 替换
	if (backward) {
		for (find_count = 0, find = dstr->data + dstr->len - old_len;
		     find >= dstr->data && (n == 0 ? true : find_count < n)
		     && find_count < old_count;
		) {
			if (strncmp(find, old_str, old_len) == 0) {
				++find_count;
				// 替换
				if (old_len != new_len && find + old_len < dstr->data + dstr->len) {
					memmove(
						find + new_len,
						find + old_len,
						dstr->data + dstr->len - find - old_len
					);
				}
				if (new_len > 0) {
					memcpy(find, new_str, new_len);
				}
				dstr->len -= old_len;
				dstr->len += new_len;
				dstr->data[dstr->len] = '\0';

				find -= old_len;
			} else {
				--find;
			}
		}
	} else {
		for (find_count = 0, find = dstr->data;
		     find <= dstr->data + dstr->len - old_len && (n == 0 ? true : find_count < n)
		     && find_count < old_count;
		) {
			if (strncmp(find, old_str, old_len) == 0) {
				++find_count;
				// 替换
				if (old_len != new_len && find + old_len < dstr->data + dstr->len) {
					memmove(
						find + new_len,
						find + old_len,
						dstr->data + dstr->len - find - old_len
					);
				}
				if (new_len > 0) {
					memcpy(find, new_str, new_len);
				}
				dstr->len -= old_len;
				dstr->len += new_len;
				dstr->data[dstr->len] = '\0';

				find += new_len;
			} else {
				++find;
			}
		}
	}

	if (new_len < old_len) {
		capacity_resize_dynamic(dstr, dstr->len - (old_len - new_len) * old_count);
	}

	return DSTR_SUCCESS;
}


int dstr_replace(dstr_adt *const dstr, const dstr_adt *const old_str, const dstr_adt *const new_str,
		 const size_t n, const bool backward
) {
	// 局部变量声明
	size_t find_count;
	char *find; // 指向当前比较的字符串
	size_t old_count;
	size_t new_len;


	// 参数检查
	assert(dstr != NULL);
	assert(old_str != NULL);

	if (old_str->len == 0 || old_str->len > dstr->len) return DSTR_INVALID_PARAM;

	old_count = dstr_count(dstr, old_str);
	if (old_count == 0) return DSTR_INVALID_PARAM;

	new_len = new_str != NULL ? new_str->len : 0;

	if (new_len > old_str->len) {
		if (!capacity_resize_dynamic(dstr, dstr->len + (new_len - old_str->len) * old_count)) {
			return DSTR_MEMORY_ALLOC_FAILED;
		}
	}

	// 替换
	if (backward) {
		for (find_count = 0, find = dstr->data + dstr->len - old_str->len;
		     find >= dstr->data && (n == 0
						    ? true
						    : find_count < n
						      && find_count < old_count);
		) {
			if (strncmp(find, old_str->data, old_str->len) == 0) {
				++find_count;
				// 替换
				if (old_str->len != new_len && find + old_str->len < dstr->data + dstr->len) {
					memmove(
						find + new_len,
						find + old_str->len,
						dstr->data + dstr->len - find - old_str->len
					);
				}
				if (new_len > 0) {
					memcpy(find, new_str, new_len);
				}
				dstr->len -= old_str->len;
				dstr->len += new_len;
				dstr->data[dstr->len] = '\0';

				find -= old_str->len;
			} else {
				--find;
			}
		}
	} else {
		for (find_count = 0, find = dstr->data;
		     find <= dstr->data + dstr->len - old_str->len && (n == 0
									       ? true
									       : find_count < n
										       && find_count < old_count);
		) {
			if (strncmp(find, old_str->data, old_str->len) == 0) {
				++find_count;
				// 替换
				if (old_str->len != new_len && find + old_str->len < dstr->data + dstr->len) {
					memmove(
						find + new_len,
						find + old_str->len,
						dstr->data + dstr->len - find - old_str->len
					);
				}
				if (new_len > 0) {
					memcpy(find, new_str, new_len);
				}
				dstr->len -= old_str->len;
				dstr->len += new_len;
				dstr->data[dstr->len] = '\0';

				find += new_len;
			} else {
				++find;
			}
		}
	}

	if (new_len < old_str->len) {
		capacity_resize_dynamic(dstr, dstr->len - (old_str->len - new_len) * old_count);
	}

	return DSTR_SUCCESS;
}

// 判断与比较
bool dstr_starts_with_cstr(const dstr_adt *const dstr, const char *const prefix) {
	size_t prefix_len;

	assert(dstr != NULL && prefix != NULL);

	prefix_len = strlen(prefix);
	if (prefix_len == 0 || prefix_len > dstr->len) return false;

	return strncmp(dstr->data, prefix, prefix_len) == 0;
}


bool dstr_starts_with(const dstr_adt *const dstr, const dstr_adt *const prefix) {
	assert(dstr != NULL && prefix != NULL);

	if (prefix->len == 0 || prefix->len > dstr->len) return false;

	return strncmp(dstr->data, prefix->data, prefix->len) == 0;
}


bool dstr_ends_with_cstr(const dstr_adt *const dstr, const char *const suffix) {
	size_t suffix_len;

	assert(dstr != NULL && suffix != NULL);
	suffix_len = strlen(suffix);
	if (suffix_len == 0 || suffix_len > dstr->len) return false;

	return strncmp(dstr->data + dstr->len - suffix_len,
		       suffix, suffix_len) == 0;
}


bool dstr_ends_with(const dstr_adt *const dstr, const dstr_adt *const suffix) {
	assert(dstr != NULL && suffix != NULL);
	if (suffix->len == 0 || suffix->len > dstr->len) return false;

	return strncmp(dstr->data + dstr->len - suffix->len,
		       suffix->data, suffix->len) == 0;
}


bool dstr_contains_cstr(const dstr_adt *const dstr, const char *const sub) {
	size_t sub_len;

	assert(dstr != NULL && sub != NULL);
	sub_len = strlen(sub);
	if (sub_len == 0 || sub_len > dstr->len) return false;

	return strstr(dstr->data, sub) != NULL;
}

bool dstr_contains(const dstr_adt *const dstr, const dstr_adt *const sub) {
	assert(dstr != NULL && sub != NULL);
	if (sub->len == 0 || sub->len > dstr->len) return false;

	return strstr(dstr->data, sub->data) != NULL;
}


bool dstr_equals_cstr(const dstr_adt *const dstr, const char *const cstr) {
	size_t cstr_len;

	assert(dstr != NULL && cstr != NULL);
	cstr_len = strlen(cstr);
	if (cstr_len != dstr->len) return false;

	return strncmp(dstr->data, cstr, cstr_len) == 0;
}


bool dstr_equals(const dstr_adt *const dstr_1, const dstr_adt *const dstr_2) {
	assert(dstr_1 != NULL && dstr_2 != NULL);
	if (dstr_1->len != dstr_2->len) return false;

	return strncmp(dstr_1->data, dstr_2->data, dstr_2->len) == 0;
}

int dstr_compare_cstr(const dstr_adt *const dstr, const char *const cstr) {
	assert(dstr != NULL && cstr != NULL);

	return strcmp(dstr->data, cstr);
}

int dstr_compare(const dstr_adt *const dstr_1, const dstr_adt *const dstr_2) {
	assert(dstr_1 != NULL && dstr_2 != NULL);

	return strcmp(dstr_1->data, dstr_2->data);
}
