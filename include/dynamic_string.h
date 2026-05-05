//
// Created by mtueih on 2026/2/25.
//

#ifndef DYNAMIC_STRING_H
#define DYNAMIC_STRING_H

#include <attrs.h>
#include <errno.h>
#if defined(__STDC_VERSION__) && __STDC_VERSION__ < 202311L
	#include <stdbool.h>
#endif
#include <stddef.h>

// ADT 类型别名声明
typedef struct dynamic_string dstr_adt;

// 状态码
enum {
	// 成功
	DSTR_SUCCESS = 0,
	// 内存分配失败
	DSTR_MEMORY_ALLOC_FAILED = ENOMEM,
	// 无效参数
	DSTR_INVALID_PARAM = EINVAL,
	// 计算溢出
	DSTR_OVERFLOW = ERANGE,
	// 未知错误
	DSTR_UNKNOWN_ERROR,
};

// API 函数原型（声明）
// 创建、销毁、清空
/**
 * 创建一个动态字符串。
 * @param cstr 初始 C 字符串，为 NULL 时创建空字符串。
 * @return 创建的动态字符串，失败时返回 NULL。
 */
ATTRS_NODISCARD_SIMPLE
dstr_adt *dstr_create(
	const char *cstr
);

/**
 * 销毁一个动态字符串。
 * @param dstr 要销毁的动态字符串。
 */
void dstr_destroy(
	dstr_adt *dstr
) ATTRS_NONNULL(1);

/**
 * 清空一个动态字符串（使其长度为 0，不会立即释放内存）。
 * @param dstr 要清空的动态字符串。
 */
void dstr_clear(
	dstr_adt *dstr
) ATTRS_NONNULL(1);

// 属性获取与设置
/**
 * 获取一个动态字符串的 C 字符串表示（非 const）。
 * @param dstr 要获取 C 字符串表示的动态字符串。
 * @return 动态字符串的 C 字符串表示。
 */
char *dstr_cstr(
	dstr_adt *dstr
) ATTRS_NONNULL(1);

/**
 * 获取一个动态字符串的常量 C 字符串表示（const）。
 * @param dstr 要获取常量 C 字符串表示的动态字符串。
 * @return 动态字符串的常量 C 字符串表示。
 */
const char *dstr_cstr_const(
	const dstr_adt *dstr
) ATTRS_NONNULL(1);

/**
 * 获取一个动态字符串的长度。
 * @param dstr 要获取长度的动态字符串。
 * @return 动态字符串的长度。
 */
size_t dstr_length(
	const dstr_adt *dstr
) ATTRS_NONNULL(1);

/**
 * 获取一个动态字符串的容量。
 * @param dstr 要获取容量的动态字符串。
 * @return 动态字符串的容量。
 */
size_t dstr_capacity(
	const dstr_adt *dstr
) ATTRS_NONNULL(1);

// 设置容量
/**
 * 设置一个动态字符串的容量。
 * @warning 当 new_capacity 小于当前字符串的长度，则动态字符串将缩小。
 * @param dstr 要设置容量的动态字符串。
 * @param new_capacity 新的容量。
 * @return 设置成功时返回 0，失败时返回非 0 值。
 */
int dstr_set_capacity(
	dstr_adt *dstr,
	size_t new_capacity
) ATTRS_NONNULL(1);

// 复制、追加、插入、删除
// 复制、追加、插入完整现有字符串到目标字符串
/**
 * 复制一个 C 字符串到一个动态字符串。
 * @param dest 目标动态字符串。
 * @param src 源 C 字符串。
 * @return 复制成功时返回 0，失败时返回非 0 值。
 */
int dstr_cpy_cstr(
	dstr_adt *dest,
	const char *src
) ATTRS_NONNULL(1, 2);

/**
 * 复制一个动态字符串到另一个动态字符串。
 * @param dest 目标动态字符串。
 * @param src 源动态字符串。
 * @return 复制成功时返回 0，失败时返回非 0 值。
 */
int dstr_cpy(
	dstr_adt *dest,
	const dstr_adt *src
) ATTRS_NONNULL(1, 2);

/**
 * 追加一个 C 字符串到一个动态字符串。
 * @param dest 目标动态字符串。
 * @param src 源 C 字符串。
 */
int dstr_cat_cstr(
	dstr_adt *dest,
	const char *src
) ATTRS_NONNULL(1, 2);

/**
 * 追加一个动态字符串到另一个动态字符串。
 * @param dest 目标动态字符串。
 * @param src 源动态字符串。
 */
int dstr_cat(
	dstr_adt *dest,
	const dstr_adt *src
) ATTRS_NONNULL(1, 2);

/**
 * 插入一个 C 字符串到一个动态字符串。
 * @param dest 目标动态字符串。
 * @param index 插入位置的索引。
 * @param src 源 C 字符串。
 */
int dstr_insert_cstr(
	dstr_adt *dest,
	size_t index,
	const char *src
) ATTRS_NONNULL(1, 3);

/**
 * 插入一个动态字符串到另一个动态字符串。
 * @param dest 目标动态字符串。
 * @param index 插入位置的索引。
 * @param src 源动态字符串。
 */
int dstr_insert(
	dstr_adt *dest,
	size_t index,
	const dstr_adt *src
) ATTRS_NONNULL(1, 3);

// 复制、追加、插入现有字符串的子串到目标字符串

/**
 * 复制一个 C 字符串的子串到一个动态字符串。
 * @param dest 目标动态字符串。
 * @param src 源 C 字符串。
 * @param sub_index 子串的起始索引。
 * @param sub_count 子串的长度，为 0 时复制到字符串末尾。
 */
int dstr_cpy_sub_cstr(
	dstr_adt *dest,
	const char *src,
	size_t sub_index,
	size_t sub_count
) ATTRS_NONNULL(1, 2);

/**
 * 复制一个动态字符串的子串到另一个动态字符串。
 * @param dest 目标动态字符串。
 * @param src 源动态字符串。
 * @param sub_index 子串的起始索引。
 * @param sub_count 子串的长度，为 0 时复制到字符串末尾。
 */
int dstr_cpy_sub(
	dstr_adt *dest,
	const dstr_adt *src,
	size_t sub_index,
	size_t sub_count
) ATTRS_NONNULL(1, 2);

/**
 * 追加一个 C 字符串的子串到一个动态字符串。
 * @param dest 目标动态字符串。
 * @param src 源 C 字符串。
 * @param sub_index 子串的起始索引。
 * @param sub_count 子串的长度，为 0 时复制到字符串末尾。
 */
int dstr_cat_sub_cstr(
	dstr_adt *dest,
	const char *src,
	size_t sub_index,
	size_t sub_count
) ATTRS_NONNULL(1, 2);

/**
 * 追加一个动态字符串的子串到另一个动态字符串。
 * @param dest 目标动态字符串。
 * @param src 源动态字符串。
 * @param sub_index 子串的起始索引。
 * @param sub_count 子串的长度，为 0 时复制到字符串末尾。
 */
int dstr_cat_sub(
	dstr_adt *dest,
	const dstr_adt *src,
	size_t sub_index,
	size_t sub_count
) ATTRS_NONNULL(1, 2);

/**
 * 插入一个 C 字符串的子串到一个动态字符串。
 * @param dest 目标动态字符串。
 * @param index 插入位置的索引。
 * @param src 源 C 字符串。
 * @param sub_index 子串的起始索引。
 * @param sub_count 子串的长度，为 0 时复制到字符串末尾。
 */
int dstr_insert_sub_cstr(
	dstr_adt *dest,
	size_t index,
	const char *src,
	size_t sub_index,
	size_t sub_count
) ATTRS_NONNULL(1, 3);

/**
 * 插入一个动态字符串的子串到另一个动态字符串。
 * @param dest 目标动态字符串。
 * @param index 插入位置的索引。
 * @param src 源动态字符串。
 * @param sub_index 子串的起始索引。
 * @param sub_count 子串的长度，为 0 时复制到字符串末尾。
 */
int dstr_insert_sub(
	dstr_adt *dest,
	size_t index,
	const dstr_adt *src,
	size_t sub_index,
	size_t sub_count
) ATTRS_NONNULL(1, 3);

// 删除子串
/**
 * 删除一个动态字符串的子串。
 * @param dstr 目标动态字符串。
 * @param index 子串的起始索引。
 * @param count 子串的长度，为 0 时删除到字符串末尾。
 */
void dstr_remove(
	dstr_adt *dstr,
	size_t index,
	size_t count
) ATTRS_NONNULL(1);

// 删除特定内容
/**
 * 去除一个动态字符串首尾的指定字符。
 * @param dstr 目标动态字符串。
 * @param trim_chars 为 NULL 时去除空白字符。
 */
void dstr_trim(
	dstr_adt *dstr,
	const char *trim_chars
) ATTRS_NONNULL(1);

// 格式化写入
/**
 * 格式化写入一个动态字符串。
 * @param dstr 目标动态字符串。
 * @param format 格式化字符串。
 * @param ... 可变参数列表。
 */
int dstr_printf(
	dstr_adt *dstr,
	const char *format,
	...
) ATTRS_FORMAT(printf, 2, 3) ATTRS_NONNULL(1, 2);

// 从现有字符串生成新字符串
// 提取子串
/**
 * 从一个 C 字符串提取子串。
 * @param cstr 源 C 字符串。
 * @param sub_index 子串的起始索引。
 * @param sub_count 子串的长度，为 0 时复制到字符串末尾。
 * @return 提取的子串。
 */
ATTRS_NODISCARD_SIMPLE
dstr_adt *dstr_sub_cstr(
	const char *cstr,
	size_t sub_index,
	size_t sub_count
) ATTRS_NONNULL(1);

/**
 * 从一个动态字符串提取子串。
 * @param dstr 源动态字符串。
 * @param sub_index 子串的起始索引。
 * @param sub_count 子串的长度，为 0 时复制到字符串末尾。
 * @return 提取的子串。
 */
ATTRS_NODISCARD_SIMPLE
dstr_adt *dstr_sub(
	const dstr_adt *dstr,
	size_t sub_index,
	size_t sub_count
) ATTRS_NONNULL(1);

// 克隆
/**
 * 克隆一个动态字符串。
 * @param dstr 源动态字符串。
 * @return 克隆的动态字符串。
 */
ATTRS_NODISCARD_SIMPLE
dstr_adt *dstr_clone(
	const dstr_adt *dstr
) ATTRS_NONNULL(1);

// 查找、统计与替换
/**
 * 查找一个 C 字符串。
 * @param dstr 目标动态字符串。
 * @param sub 要查找的子串。
 * @param out_index 输出参数，不为 NULL 时将找到的子串的起始索引写入其指向的内存。
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
 * 查找一个动态字符串。
 * @param dstr 目标动态字符串。
 * @param sub 要查找的子串。
 * @param out_index 输出参数，不为 NULL 时将找到的子串的起始索引写入其指向的内存。
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
 * 统计一个 C 字符串出现的次数。
 * @param dstr 目标动态字符串。
 * @param sub 要统计的子串。
 * @return 子串在目标动态字符串中出现的次数。
 */
size_t dstr_count_cstr(
	const dstr_adt *dstr,
	const char *sub
) ATTRS_NONNULL(1, 2);

/**
 * 统计一个动态字符串出现的次数。
 * @param dstr 目标动态字符串。
 * @param sub 要统计的子串。
 * @return 子串在目标动态字符串中出现的次数。
 */
size_t dstr_count(
	const dstr_adt *dstr,
	const dstr_adt *sub
) ATTRS_NONNULL(1, 2);

/**
 * 查找一个 C 字符串的第 n 次出现。
 * @param dstr 目标动态字符串。
 * @param sub 要查找的子串。
 * @param out_index 输出参数，不为 NULL 时将找到的子串的起始索引写入其指向的内存。
 * @param n 要查找的第 n 次出现，n 从 1 开始，为 0 表示最后一次。
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
 * 查找一个动态字符串的第 n 次出现。
 * @param dstr 目标动态字符串。
 * @param sub 要查找的子串。
 * @param out_index 输出参数，不为 NULL 时将找到的子串的起始索引写入其指向的内存。
 * @param n 要查找的第 n 次出现，n 从 1 开始，为 0 表示最后一次。
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
 * 替换一个 C 字符串。
 * @param dstr 目标动态字符串。
 * @param old_str 要替换的子串。
 * @param new_str 替换后的子串，为 NULL 或 "" 表示替换为空。
 * @param n 替换的次数，为 0 时替换所有。
 * @param backward 是否从后向前替换。
 */
int dstr_replace_cstr(
	dstr_adt *dstr,
	const char *old_str,
	const char *new_str,
	size_t n,
	bool backward
) ATTRS_NONNULL(1, 2);

/**
 * 替换一个动态字符串。
 * @param dstr 目标动态字符串。
 * @param old_str 要替换的子串。
 * @param new_str 替换后的子串，为 NULL 或 "" 表示替换为空。
 * @param n 替换的次数，为 0 时替换所有。
 * @param backward 是否从后向前替换。
 */
int dstr_replace(
	dstr_adt *dstr,
	const dstr_adt *old_str,
	const dstr_adt *new_str,
	size_t n,
	bool backward
) ATTRS_NONNULL(1, 2);

// 判断与比较
/**
 * 判断一个动态字符串是否以指定前缀开头。
 * @param dstr 目标动态字符串。
 * @param prefix 指定的前缀。
 * @return 如果目标动态字符串以指定前缀开头则返回 true，否则返回 false。
 */
bool dstr_starts_with_cstr(
	const dstr_adt *dstr,
	const char *prefix
) ATTRS_NONNULL(1, 2);

/**
 * 判断一个动态字符串是否以指定前缀开头。
 * @param dstr 目标动态字符串。
 * @param prefix 指定的前缀。
 * @return 如果目标动态字符串以指定前缀开头则返回 true，否则返回 false。
 */
bool dstr_starts_with(
	const dstr_adt *dstr,
	const dstr_adt *prefix
) ATTRS_NONNULL(1, 2);

/**
 * 判断一个动态字符串是否以指定后缀结尾。
 * @param dstr 目标动态字符串。
 * @param suffix 指定的后缀。
 * @return 如果目标动态字符串以指定后缀结尾则返回 true，否则返回 false。
 */
bool dstr_ends_with_cstr(
	const dstr_adt *dstr,
	const char *suffix
) ATTRS_NONNULL(1, 2);

/**
 * 判断一个动态字符串是否以指定后缀结尾。
 * @param dstr 目标动态字符串。
 * @param suffix 指定的后缀。
 * @return 如果目标动态字符串以指定后缀结尾则返回 true，否则返回 false。
 */
bool dstr_ends_with(
	const dstr_adt *dstr,
	const dstr_adt *suffix
) ATTRS_NONNULL(1, 2);

/**
 * 判断一个动态字符串是否包含指定子串。
 * @param dstr 目标动态字符串。
 * @param sub 指定的子串。
 * @return 如果目标动态字符串包含指定子串则返回 true，否则返回 false。
 */
bool dstr_contains_cstr(
	const dstr_adt *dstr,
	const char *sub
) ATTRS_NONNULL(1, 2);

/**
 * 判断一个动态字符串是否包含指定子串。
 * @param dstr 目标动态字符串。
 * @param sub 指定的子串。
 * @return 如果目标动态字符串包含指定子串则返回 true，否则返回 false。
 */
bool dstr_contains(
	const dstr_adt *dstr,
	const dstr_adt *sub
) ATTRS_NONNULL(1, 2);

/**
 * 判断一个动态字符串是否等于指定 C 字符串。
 * @param dstr 目标动态字符串。
 * @param cstr 指定的 C 字符串。
 * @return 如果目标动态字符串等于指定 C 字符串则返回 true，否则返回 false。
 */
bool dstr_equals_cstr(
	const dstr_adt *dstr,
	const char *cstr
) ATTRS_NONNULL(1, 2);

/**
 * 判断两个动态字符串是否相等。
 * @param dstr_1 第一个动态字符串。
 * @param dstr_2 第二个动态字符串。
 * @return 如果两个动态字符串相等则返回 true，否则返回 false。
 */
bool dstr_equals(
	const dstr_adt *dstr_1,
	const dstr_adt *dstr_2
) ATTRS_NONNULL(1, 2);

/**
 * 比较一个动态字符串与指定 C 字符串。
 * @param dstr 目标动态字符串。
 * @param cstr 指定的 C 字符串。
 * @return 如果两个字符串相等则返回 0，如果 dstr 小于 cstr 则返回负数，如果 dstr 大于 cstr 则返回正数。
 */
int dstr_compare_cstr(
	const dstr_adt *dstr,
	const char *cstr
) ATTRS_NONNULL(1, 2);

/**
 * 比较两个动态字符串。
 * @param dstr_1 第一个动态字符串。
 * @param dstr_2 第二个动态字符串。
 * @return 如果两个动态字符串相等则返回 0，如果 dstr_1 小于 dstr_2 则返回负数，如果 dstr_1 大于 dstr_2 则返回正数。
 */
int dstr_compare(
	const dstr_adt *dstr_1,
	const dstr_adt *dstr_2
) ATTRS_NONNULL(1, 2);

#endif // DYNAMIC_STRING_H
