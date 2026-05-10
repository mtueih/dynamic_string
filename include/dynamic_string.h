//
// Created by mtueih on 2026/2/25.
//

#ifndef DYNAMIC_STRING_H
#define DYNAMIC_STRING_H

#include <attrs.h>
#include <errno.h>
#include <stddef.h>

#if !defined(__STDC_VERSION__) || (defined(__STDC_VERSION__) && __STDC_VERSION__ < 202311L)
#include <stdbool.h>
#endif

// ADT 类型别名声明。
typedef struct dynamic_string dstr_adt;

// 状态码。
enum {
	// 成功。
	DSTR_SUCCESS = 0,
	// 内存分配失败。
	DSTR_MEMORY_ALLOC_FAILED = ENOMEM,
	// 无效参数。
	DSTR_INVALID_PARAM = EINVAL,
	// 计算溢出。
	DSTR_OVERFLOW = ERANGE,
	// 未知错误。
	DSTR_UNKNOWN_ERROR,
};

// API 函数原型（声明）。
// 创建、销毁、清空。
/**
 * 创建一个「动态字符串」。
 * @param cstr 用来初始化目标「动态字符串」的「C 字符串」的指针，为「空指针」或「指向空字符串」时创建空的「动态字符串」。
 * @return 所创建的「动态字符串」的指针，如果创建失败则返回「空指针」。
 */
ATTRS_NODISCARD_SIMPLE
dstr_adt *dstr_create(
	const char *cstr
);

/**
 * 销毁一个「动态字符串」。
 * @param dstr 目标「动态字符串」的指针。
 */
void dstr_destroy(
	dstr_adt *dstr
) ATTRS_NONNULL(1);

/**
 * 清空一个「动态字符串」（使其长度为 0，不会立即释放内存）。
 * @param dstr 目标「动态字符串」的指针。
 */
void dstr_clear(
	dstr_adt *dstr
) ATTRS_NONNULL(1);

// 属性获取与设置。
/**
 * 获取一个「动态字符串」的内部「C 字符串」指针（非 const）。
 * @param dstr 目标「动态字符串」的指针。
 * @return 所获取的「C 字符串」指针。
 */
char *dstr_cstr(
	dstr_adt *dstr
) ATTRS_NONNULL(1);

/**
 * 获取一个「动态字符串」的内部「C 字符串」指针（const）。
 * @param dstr 目标「动态字符串」的指针。
 * @return 所获取的「C 字符串」指针。
 */
const char *dstr_cstr_const(
	const dstr_adt *dstr
) ATTRS_NONNULL(1);

/**
 * 获取一个「动态字符串」的长度。
 * @param dstr 目标「动态字符串」的指针。
 * @return 所获取的长度。
 */
size_t dstr_length(
	const dstr_adt *dstr
) ATTRS_NONNULL(1);

/**
 * 获取一个「动态字符串」的容量。
 * @param dstr 目标「动态字符串」的指针。
 * @return 所获取的容量。
 */
size_t dstr_capacity(
	const dstr_adt *dstr
) ATTRS_NONNULL(1);

/**
 * 设置一个「动态字符串」的容量。
 * @warning 当「目标容量」小于「当前长度」时，「当前内容」将被截断。
 * @param dstr 目标「动态字符串」的指针。
 * @param new_capacity 目标容量。
 * @return 全局状态码。
 */
int dstr_set_capacity(
	dstr_adt *dstr,
	size_t new_capacity
) ATTRS_NONNULL(1);

// 复制、追加、插入、删除。
// 复制、追加、插入完整现有字符串。
/**
 * 复制一个「C 字符串」到一个「动态字符串」。
 * @param dest 目标「动态字符串」的指针。
 * @param src 源「C 字符串」的指针。
 * @return 全局状态码。
 */
int dstr_cpy_cstr(
	dstr_adt *dest,
	const char *src
) ATTRS_NONNULL(1, 2);

/**
 * 复制一个「动态字符串」到另一个「动态字符串」。
 * @param dest 目标「动态字符串」的指针。
 * @param src 源「动态字符串」的指针。
 * @return 全局状态码。
 */
int dstr_cpy(
	dstr_adt *dest,
	const dstr_adt *src
) ATTRS_NONNULL(1, 2);

/**
 * 追加一个「C 字符串」到一个「动态字符串」。
 * @param dest 目标「动态字符串」的指针。
 * @param src 源「C 字符串」的指针。
 * @return 全局状态码。
 */
int dstr_cat_cstr(
	dstr_adt *dest,
	const char *src
) ATTRS_NONNULL(1, 2);

/**
 * 追加一个「动态字符串」到另一个「动态字符串」。
 * @param dest 目标「动态字符串」的指针。
 * @param src 源「动态字符串」的指针。
 * @return 全局状态码。
 */
int dstr_cat(
	dstr_adt *dest,
	const dstr_adt *src
) ATTRS_NONNULL(1, 2);

/**
 * 插入一个「C 字符串」到一个「动态字符串」。
 * @param dest 目标「动态字符串」的指针。
 * @param index 插入位置的索引。
 * @param src 源「C 字符串」的指针。
 * @return 全局状态码。
 */
int dstr_insert_cstr(
	dstr_adt *dest,
	size_t index,
	const char *src
) ATTRS_NONNULL(1, 3);

/**
 * 插入一个「动态字符串」到另一个「动态字符串」。
 * @param dest 目标「动态字符串」的指针。
 * @param index 插入位置的索引。
 * @param src 源「动态字符串」的指针。
 * @return 全局状态码。
 */
int dstr_insert(
	dstr_adt *dest,
	size_t index,
	const dstr_adt *src
) ATTRS_NONNULL(1, 3);

// 复制、追加、插入现有字符串的子串。

/**
 * 复制一个「C 字符串」的子串到一个「动态字符串」。
 * @param dest 目标「动态字符串」的指针。
 * @param src 源「C 字符串」的指针。
 * @param sub_index 子串的起始索引。
 * @param sub_count 子串的长度，为 0 表示到末尾。
 * @return 全局状态码。
 */
int dstr_cpy_sub_cstr(
	dstr_adt *dest,
	const char *src,
	size_t sub_index,
	size_t sub_count
) ATTRS_NONNULL(1, 2);

/**
 * 复制一个「动态字符串」的子串到另一个「动态字符串」。
 * @param dest 目标「动态字符串」的指针。
 * @param src 源「动态字符串」的指针。
 * @param sub_index 子串的起始索引。
 * @param sub_count 子串的长度，为 0 表示到末尾。
 * @return 全局状态码。
 */
int dstr_cpy_sub(
	dstr_adt *dest,
	const dstr_adt *src,
	size_t sub_index,
	size_t sub_count
) ATTRS_NONNULL(1, 2);

/**
 * 追加一个「C 字符串」的子串到一个「动态字符串」。
 * @param dest 目标「动态字符串」的指针。
 * @param src 源「C 字符串」的指针。
 * @param sub_index 子串的起始索引。
 * @param sub_count 子串的长度，为 0 表示到末尾。
 * @return 全局状态码。
 */
int dstr_cat_sub_cstr(
	dstr_adt *dest,
	const char *src,
	size_t sub_index,
	size_t sub_count
) ATTRS_NONNULL(1, 2);

/**
 * 追加一个「动态字符串」的子串到另一个「动态字符串」。
 * @param dest 目标「动态字符串」的指针。
 * @param src 源「动态字符串」的指针。
 * @param sub_index 子串的起始索引。
 * @param sub_count 子串的长度，为 0 表示到末尾。
 * @return 全局状态码。
 */
int dstr_cat_sub(
	dstr_adt *dest,
	const dstr_adt *src,
	size_t sub_index,
	size_t sub_count
) ATTRS_NONNULL(1, 2);

/**
 * 插入一个「C 字符串」的子串到一个「动态字符串」。
 * @param dest 目标「动态字符串」的指针。
 * @param index 插入位置的索引。
 * @param src 源「C 字符串」的指针。
 * @param sub_index 子串的起始索引。
 * @param sub_count 子串的长度，为 0 表示到末尾。
 * @return 全局状态码。
 */
int dstr_insert_sub_cstr(
	dstr_adt *dest,
	size_t index,
	const char *src,
	size_t sub_index,
	size_t sub_count
) ATTRS_NONNULL(1, 3);

/**
 * 插入一个「动态字符串」的子串到另一个「动态字符串」。
 * @param dest 目标「动态字符串」的指针。
 * @param index 插入位置的索引。
 * @param src 源「动态字符串」的指针。
 * @param sub_index 子串的起始索引。
 * @param sub_count 子串的长度，为 0 表示到末尾。
 * @return 全局状态码。
 */
int dstr_insert_sub(
	dstr_adt *dest,
	size_t index,
	const dstr_adt *src,
	size_t sub_index,
	size_t sub_count
) ATTRS_NONNULL(1, 3);

// 删除子串。
/**
 * 删除一个「动态字符串」的子串。
 * @param dstr 目标「动态字符串」的指针。
 * @param index 子串的起始索引。
 * @param count 子串的长度，为 0 表示到末尾。
 */
void dstr_remove(
	dstr_adt *dstr,
	size_t index,
	size_t count
) ATTRS_NONNULL(1);

// 删除特定内容。
/**
 * 删除一个「动态字符串」首尾的「空白字符」或「指定字符」。
 * @param dstr 目标「动态字符串」的指针。
 * @param trim_chars 指定字符集合的「C 字符串」的指针，为「空指针」或「指向空字符串」表示删除空白字符。
 */
void dstr_trim(
	dstr_adt *dstr,
	const char *trim_chars
) ATTRS_NONNULL(1);

// 格式化写入。
/**
 * 格式化写入字符串到一个「动态字符串」。
 * @param dstr 目标「动态字符串」的指针。
 * @param format 「格式字符串」的指针。
 * @param ... 可变参数列表。
 * @return 全局状态码。
 */
int dstr_printf(
	dstr_adt *dstr,
	const char *format,
	...
) ATTRS_FORMAT(printf, 2, 3) ATTRS_NONNULL(1, 2);

// 从现有字符串生成新「动态字符串」。
// 提取子串。
/**
 * 从一个「C 字符串」提取子串并创建新的「动态字符串」。
 * @param cstr 源「C 字符串」的指针。
 * @param sub_index 子串的起始索引。
 * @param sub_count 子串的长度，为 0 表示到末尾。
 * @return 所创建的「动态字符串」的指针，如果创建失败则返回「空指针」。
 */
ATTRS_NODISCARD_SIMPLE
dstr_adt *dstr_sub_cstr(
	const char *cstr,
	size_t sub_index,
	size_t sub_count
) ATTRS_NONNULL(1);

/**
 * 从一个「动态字符串」提取子串并创建新的「动态字符串」。
 * @param dstr 源「动态字符串」的指针。
 * @param sub_index 子串的起始索引。
 * @param sub_count 子串的长度，为 0 表示到末尾。
 * @return 所创建的「动态字符串」的指针，如果创建失败则返回「空指针」。
 */
ATTRS_NODISCARD_SIMPLE
dstr_adt *dstr_sub(
	const dstr_adt *dstr,
	size_t sub_index,
	size_t sub_count
) ATTRS_NONNULL(1);

// 克隆。
/**
 * 克隆一个「动态字符串」。
 * @param dstr 源「动态字符串」的指针。
 * @return 所创建的「动态字符串」的指针，如果创建失败则返回「空指针」。
 */
ATTRS_NODISCARD_SIMPLE
dstr_adt *dstr_clone(
	const dstr_adt *dstr
) ATTRS_NONNULL(1);

// 查找、统计与替换。
/**
 * 查找一个「C 字符串」在「动态字符串」中的位置。
 * @param dstr 目标「动态字符串」的指针。
 * @param sub 要查找的子「C 字符串」的指针。
 * @param out_index 存储查找结果（位置索引）的指针，不为「空指针」时写入。
 * @param backward 是否从后向前查找。
 * @return 如果找到则返回 true，否则返回 false。
 */
bool dstr_find_cstr(
	const dstr_adt *dstr,
	const char *sub,
	size_t *out_index,
	bool backward
) ATTRS_NONNULL(1, 2);

/**
 * 查找一个「动态字符串」在另一个「动态字符串」中的位置。
 * @param dstr 目标「动态字符串」的指针。
 * @param sub 要查找的子「动态字符串」的指针。
 * @param out_index 存储查找结果（位置索引）的指针，不为「空指针」时写入。
 * @param backward 是否从后向前查找。
 * @return 如果找到则返回 true，否则返回 false。
 */
bool dstr_find(
	const dstr_adt *dstr,
	const dstr_adt *sub,
	size_t *out_index,
	bool backward
) ATTRS_NONNULL(1, 2);

/**
 * 统计一个「C 字符串」在「动态字符串」中出现的次数。
 * @param dstr 目标「动态字符串」的指针。
 * @param sub 要统计的子「C 字符串」的指针。
 * @param backward 是否从后向前统计。
 * @return 子「C 字符串」在目标「动态字符串」中出现的次数。
 */
size_t dstr_count_cstr(
	const dstr_adt *dstr,
	const char *sub,
	bool backward
) ATTRS_NONNULL(1, 2);

/**
 * 统计一个「动态字符串」在另一个「动态字符串」中出现的次数。
 * @param dstr 目标「动态字符串」的指针。
 * @param sub 要统计的子「动态字符串」的指针。
 * @param backward 是否从后向前统计。
 * @return 子「动态字符串」在目标「动态字符串」中出现的次数。
 */
size_t dstr_count(
	const dstr_adt *dstr,
	const dstr_adt *sub,
	bool backward
) ATTRS_NONNULL(1, 2);

/**
 * 查找一个「C 字符串」在「动态字符串」中第 n 次出现的位置。
 * @param dstr 目标「动态字符串」的指针。
 * @param sub 要查找的子「C 字符串」的指针。
 * @param out_index 存储查找结果（位置索引）的指针，不为「空指针」时写入。
 * @param n 要查找的出现的次序，从 1 开始。
 * @param backward 是否从后向前查找。
 * @return 如果找到则返回 true，否则返回 false。
 */
bool dstr_find_nth_cstr(
	const dstr_adt *dstr,
	const char *sub,
	size_t *out_index,
	size_t n,
	bool backward
) ATTRS_NONNULL(1, 2);

/**
 * 查找一个「动态字符串」在另一个「动态字符串」中第 n 次出现的位置。
 * @param dstr 目标「动态字符串」的指针。
 * @param sub 要查找的子「动态字符串」的指针。
 * @param out_index 存储查找结果（位置索引）的指针，不为「空指针」时写入。
 * @param n 要查找的出现的次序，从 1 开始。
 * @param backward 是否从后向前查找。
 * @return 如果找到则返回 true，否则返回 false。
 */
bool dstr_find_nth(
	const dstr_adt *dstr,
	const dstr_adt *sub,
	size_t *out_index,
	size_t n,
	bool backward
) ATTRS_NONNULL(1, 2);

/**
 * 替换一个「动态字符串」中的「C 字符串」。
 * @param dstr 目标「动态字符串」的指针。
 * @param old_str 被替换的子「C 字符串」的指针。
 * @param new_str 替换后的子「C 字符串」的指针，为「空指针」或「指向空字符串」表示替换为空。
 * @param n 替换的次数，为 0 时替换所有。
 * @param backward 是否从后向前替换。
 * @return 全局状态码。
 */
int dstr_replace_cstr(
	dstr_adt *dstr,
	const char *old_str,
	const char *new_str,
	size_t n,
	bool backward
) ATTRS_NONNULL(1, 2);

/**
 * 替换一个「动态字符串」中的另一个「动态字符串」。
 * @param dstr 目标「动态字符串」的指针。
 * @param old_str 被替换的子「动态字符串」的指针。
 * @param new_str 替换后的子「动态字符串」的指针，为「空指针」或「指向空字符串」表示替换为空。
 * @param n 替换的次数，为 0 时替换所有。
 * @param backward 是否从后向前替换。
 * @return 全局状态码。
 */
int dstr_replace(
	dstr_adt *dstr,
	const dstr_adt *old_str,
	const dstr_adt *new_str,
	size_t n,
	bool backward
) ATTRS_NONNULL(1, 2);

// 判断与比较。
/**
 * 判断一个「动态字符串」是否以指定前缀开头。
 * @param dstr 目标「动态字符串」的指针。
 * @param prefix 前缀「C 字符串」的指针。
 * @return 如果目标「动态字符串」以指定前缀开头则返回 true，否则返回 false。
 */
bool dstr_starts_with_cstr(
	const dstr_adt *dstr,
	const char *prefix
) ATTRS_NONNULL(1, 2);

/**
 * 判断一个「动态字符串」是否以指定前缀开头。
 * @param dstr 目标「动态字符串」的指针。
 * @param prefix 前缀「动态字符串」的指针。
 * @return 如果目标「动态字符串」以指定前缀开头则返回 true，否则返回 false。
 */
bool dstr_starts_with(
	const dstr_adt *dstr,
	const dstr_adt *prefix
) ATTRS_NONNULL(1, 2);

/**
 * 判断一个「动态字符串」是否以指定后缀结尾。
 * @param dstr 目标「动态字符串」的指针。
 * @param suffix 后缀「C 字符串」的指针。
 * @return 如果目标「动态字符串」以指定后缀结尾则返回 true，否则返回 false。
 */
bool dstr_ends_with_cstr(
	const dstr_adt *dstr,
	const char *suffix
) ATTRS_NONNULL(1, 2);

/**
 * 判断一个「动态字符串」是否以指定后缀结尾。
 * @param dstr 目标「动态字符串」的指针。
 * @param suffix 后缀「动态字符串」的指针。
 * @return 如果目标「动态字符串」以指定后缀结尾则返回 true，否则返回 false。
 */
bool dstr_ends_with(
	const dstr_adt *dstr,
	const dstr_adt *suffix
) ATTRS_NONNULL(1, 2);

/**
 * 判断一个「动态字符串」是否包含指定子串。
 * @param dstr 目标「动态字符串」的指针。
 * @param sub 子「C 字符串」的指针。
 * @return 如果目标「动态字符串」包含指定子串则返回 true，否则返回 false。
 */
bool dstr_contains_cstr(
	const dstr_adt *dstr,
	const char *sub
) ATTRS_NONNULL(1, 2);

/**
 * 判断一个「动态字符串」是否包含指定子串。
 * @param dstr 目标「动态字符串」的指针。
 * @param sub 子「动态字符串」的指针。
 * @return 如果目标「动态字符串」包含指定子串则返回 true，否则返回 false。
 */
bool dstr_contains(
	const dstr_adt *dstr,
	const dstr_adt *sub
) ATTRS_NONNULL(1, 2);

/**
 * 判断一个「动态字符串」是否等于一个「C 字符串」。
 * @param dstr 「动态字符串」的指针。
 * @param cstr 「C 字符串」的指针。
 * @return 等于则返回 true，否则返回 false。
 */
bool dstr_equals_cstr(
	const dstr_adt *dstr,
	const char *cstr
) ATTRS_NONNULL(1, 2);

/**
 * 判断两个「动态字符串」是否相等。
 * @param dstr_1 第一个「动态字符串」的指针。
 * @param dstr_2 第二个「动态字符串」的指针。
 * @return 相等则返回 true，否则返回 false。
 */
bool dstr_equals(
	const dstr_adt *dstr_1,
	const dstr_adt *dstr_2
) ATTRS_NONNULL(1, 2);

/**
 * 比较一个「动态字符串」与一个「C 字符串」。
 * @param dstr 「动态字符串」的指针。
 * @param cstr 「C 字符串」的指针。
 * @return 相等则返回 0，dstr 小于 cstr 则返回负数，dstr 大于 cstr 则返回正数。
 */
int dstr_compare_cstr(
	const dstr_adt *dstr,
	const char *cstr
) ATTRS_NONNULL(1, 2);

/**
 * 比较两个「动态字符串」。
 * @param dstr_1 第一个「动态字符串」的指针。
 * @param dstr_2 第二个「动态字符串」的指针。
 * @return 相等则返回 0，dstr_1 小于 dstr_2 则返回负数，dstr_1 大于 dstr_2 则返回正数。
 */
int dstr_compare(
	const dstr_adt *dstr_1,
	const dstr_adt *dstr_2
) ATTRS_NONNULL(1, 2);

#endif // DYNAMIC_STRING_H
