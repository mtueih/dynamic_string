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
 * tests/test_dynamic_string.c - 项目主库单元测试文件
 *============================================================================*/


/*------------------------------------------------------------------------------
 * 头文件包含
 *----------------------------------------------------------------------------*/
#include "dynamic_string.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*------------------------------------------------------------------------------
 * 测试框架基础设施
 *----------------------------------------------------------------------------*/

#define MAX_TESTS 200

static int test_passed = 0;
static int test_failed = 0;
static const char *failed_tests[MAX_TESTS];
static int failed_count = 0;
static const char *current_test_name = NULL;
static int current_test_has_failure = 0;

#define TEST_START(name) do { \
	current_test_name = name; \
	current_test_has_failure = 0; \
	printf("\n=== 测试: %s ===\n", name); \
} while(0)

#define TEST_PASS() do { \
	if (current_test_has_failure) { \
		printf("  [FAIL]\n"); \
		test_failed++; \
		if (failed_count < MAX_TESTS) { \
			failed_tests[failed_count++] = current_test_name; \
		} \
	} else { \
		printf("  [PASS]\n"); \
		test_passed++; \
	} \
} while(0)

/* 断言宏：条件为真则通过子检查，否则标记失败 */
#define CHECK(cond, msg) do { \
	if (cond) { \
		printf("  - %s: OK\n", msg); \
	} else { \
		printf("  - %s: FAILED\n", msg); \
		current_test_has_failure = 1; \
	} \
} while(0)

/* 便捷的字符串相等断言 */
#define CHECK_STR_EQ(dstr, expected, msg) do { \
	const char *_cs = dstr_cstr(dstr); \
	CHECK((_cs != NULL && strcmp(_cs, expected) == 0), msg); \
} while(0)

/* 便捷的状态码断言 */
#define CHECK_STATUS_EQ(ret, expected, msg) \
	CHECK((ret) == (expected), msg)

/*------------------------------------------------------------------------------
 * 1. 创建与销毁
 *----------------------------------------------------------------------------*/

static void test_dstr_create(void) {
	TEST_START("dstr_create");

	dstr_adt *d1 = dstr_create("Hello, World!");
	CHECK(d1 != NULL, "从非空 C 字符串创建");
	CHECK_STR_EQ(d1, "Hello, World!", "内容正确");
	CHECK(dstr_length(d1) == 13, "长度正确");
	dstr_destroy(d1);

	dstr_adt *d2 = dstr_create("");
	CHECK(d2 != NULL, "从空 C 字符串创建");
	CHECK(dstr_length(d2) == 0, "长度为 0");
	CHECK(dstr_is_empty(d2), "判断为空");
	dstr_destroy(d2);

	dstr_adt *d3 = dstr_create(NULL);
	CHECK(d3 != NULL, "从 NULL 创建");
	CHECK(dstr_length(d3) == 0, "长度为 0");
	dstr_destroy(d3);

	TEST_PASS();
}

static void test_dstr_destroy(void) {
	TEST_START("dstr_destroy");

	/* 正常销毁 */
	dstr_adt *d = dstr_create("Test");
	CHECK(d != NULL, "创建成功");
	dstr_destroy(d);
	printf("  - 正常销毁: OK (无崩溃)\n");

	/* 销毁 NULL 不应崩溃 */
	dstr_destroy(NULL);
	printf("  - 销毁 NULL: OK (无崩溃)\n");

	TEST_PASS();
}

static void test_dstr_clone(void) {
	TEST_START("dstr_clone");

	dstr_adt *orig = dstr_create("Hello, World!");
	dstr_adt *clone = dstr_clone(orig);
	CHECK(clone != NULL, "克隆非空字符串成功");
	CHECK(clone != orig, "克隆对象地址不同");
	CHECK_STR_EQ(clone, "Hello, World!", "克隆内容正确");
	dstr_destroy(orig);
	dstr_destroy(clone);

	/* 克隆空字符串 */
	dstr_adt *empty = dstr_create("");
	dstr_adt *empty_clone = dstr_clone(empty);
	CHECK(empty_clone != NULL, "克隆空字符串成功");
	CHECK(dstr_is_empty(empty_clone), "克隆空字符串也为空");
	dstr_destroy(empty);
	dstr_destroy(empty_clone);

	/* 克隆 NULL */
	dstr_adt *null_clone = dstr_clone(NULL);
	CHECK(null_clone != NULL, "克隆 NULL 返回非空");
	CHECK(dstr_is_empty(null_clone), "克隆 NULL 返回空字符串");
	dstr_destroy(null_clone);

	TEST_PASS();
}

static void test_dstr_sub_cstr(void) {
	TEST_START("dstr_sub_cstr");

	dstr_adt *sub = dstr_sub_cstr("Hello, World!", 7, 5);
	CHECK(sub != NULL, "从 C 字符串提取子串成功");
	CHECK_STR_EQ(sub, "World", "子串内容正确");
	dstr_destroy(sub);

	/* sub_length 为 0 表示到末尾 */
	dstr_adt *sub2 = dstr_sub_cstr("Hello, World!", 7, 0);
	CHECK(sub2 != NULL, "sub_length=0 提取子串成功");
	CHECK_STR_EQ(sub2, "World!", "到末尾子串内容正确");
	dstr_destroy(sub2);

	/* 越界应返回 NULL */
	dstr_adt *sub3 = dstr_sub_cstr("Hi", 5, 1);
	CHECK(sub3 == NULL, "起始索引越界返回 NULL");

	/* NULL 或空字符串输入 */
	dstr_adt *sub4 = dstr_sub_cstr(NULL, 0, 0);
	CHECK(sub4 != NULL, "NULL 输入返回空 dstr");
	CHECK(dstr_is_empty(sub4), "NULL 输入返回空字符串");
	dstr_destroy(sub4);

	dstr_adt *sub5 = dstr_sub_cstr("", 0, 0);
	CHECK(sub5 != NULL, "空 C 字符串输入返回空 dstr");
	CHECK(dstr_is_empty(sub5), "空 C 字符串输入返回空字符串");
	dstr_destroy(sub5);

	TEST_PASS();
}

static void test_dstr_sub(void) {
	TEST_START("dstr_sub");

	dstr_adt *src = dstr_create("Hello, World!");
	dstr_adt *sub = dstr_sub(src, 7, 5);
	CHECK(sub != NULL, "从动态字符串提取子串成功");
	CHECK_STR_EQ(sub, "World", "子串内容正确");
	dstr_destroy(src);
	dstr_destroy(sub);

	/* sub_length 为 0 */
	dstr_adt *src2 = dstr_create("Hello, World!");
	dstr_adt *sub2 = dstr_sub(src2, 7, 0);
	CHECK(sub2 != NULL, "sub_length=0 提取成功");
	CHECK_STR_EQ(sub2, "World!", "到末尾内容正确");
	dstr_destroy(src2);
	dstr_destroy(sub2);

	/* 越界 */
	dstr_adt *src3 = dstr_create("Hi");
	dstr_adt *sub3 = dstr_sub(src3, 5, 1);
	CHECK(sub3 == NULL, "越界返回 NULL");
	dstr_destroy(src3);

	/* NULL 输入 */
	dstr_adt *sub4 = dstr_sub(NULL, 0, 0);
	CHECK(sub4 != NULL, "NULL 输入返回空 dstr");
	CHECK(dstr_is_empty(sub4), "NULL 输入返回空字符串");
	dstr_destroy(sub4);

	TEST_PASS();
}

static void test_dstr_create_format(void) {
	TEST_START("dstr_create_format");

	dstr_adt *d = dstr_create_format("Hello, %s! Number: %d", "World", 42);
	CHECK(d != NULL, "格式化创建成功");
	CHECK_STR_EQ(d, "Hello, World! Number: 42", "格式化内容正确");
	dstr_destroy(d);

	/* 空格式字符串 */
	dstr_adt *d2 = dstr_create_format("");
	CHECK(d2 != NULL, "空格式创建成功");
	CHECK(dstr_is_empty(d2), "空格式创建空字符串");
	dstr_destroy(d2);

	/* NULL 格式 */
	dstr_adt *d3 = dstr_create_format(NULL);
	CHECK(d3 != NULL, "NULL 格式创建成功");
	CHECK(dstr_is_empty(d3), "NULL 格式创建空字符串");
	dstr_destroy(d3);

	TEST_PASS();
}

/* va_list 版本的辅助包装：通过 ... 接收参数后构造 va_list 再调用 vformat 版本 */
static dstr_adt *wrap_create_vformat(const char *format, ...) {
	va_list args;
	va_start(args, format);
	dstr_adt *result = dstr_create_vformat(format, args);
	va_end(args);
	return result;
}

static dstr_status_t wrap_cpy_vformat(dstr_adt *dest, const char *format, ...) {
	va_list args;
	va_start(args, format);
	dstr_status_t result = dstr_cpy_vformat(dest, format, args);
	va_end(args);
	return result;
}

static dstr_status_t wrap_cat_vformat(dstr_adt *dest, const char *format, ...) {
	va_list args;
	va_start(args, format);
	dstr_status_t result = dstr_cat_vformat(dest, format, args);
	va_end(args);
	return result;
}

static dstr_status_t wrap_insert_vformat(
	dstr_adt *dest, size_t index,
	const char *format, ...
) {
	va_list args;
	va_start(args, format);
	dstr_status_t result = dstr_insert_vformat(dest, index, format, args);
	va_end(args);
	return result;
}

static void test_dstr_create_vformat(void) {
	TEST_START("dstr_create_vformat");

	dstr_adt *d = wrap_create_vformat("Hello, %s! Number: %d", "World", 42);
	CHECK(d != NULL, "格式化创建成功");
	CHECK_STR_EQ(d, "Hello, World! Number: 42", "格式化内容正确");
	dstr_destroy(d);

	/* 空格式字符串 */
	dstr_adt *d2 = wrap_create_vformat("");
	CHECK(d2 != NULL, "空格式创建成功");
	CHECK(dstr_is_empty(d2), "空格式创建空字符串");
	dstr_destroy(d2);

	/* NULL 格式 */
	dstr_adt *d3 = wrap_create_vformat(NULL);
	CHECK(d3 != NULL, "NULL 格式创建成功");
	CHECK(dstr_is_empty(d3), "NULL 格式创建空字符串");
	dstr_destroy(d3);

	TEST_PASS();
}

/*------------------------------------------------------------------------------
 * 2. 属性获取与设置
 *----------------------------------------------------------------------------*/

static void test_dstr_cstr(void) {
	TEST_START("dstr_cstr");

	dstr_adt *d = dstr_create("Test String");
	const char *cs = dstr_cstr(d);
	CHECK(cs != NULL, "获取 C 字符串指针非空");
	CHECK(strcmp(cs, "Test String") == 0, "内容正确");
	dstr_destroy(d);

	/* NULL 输入 */
	CHECK(dstr_cstr(NULL) == NULL, "NULL 输入返回 NULL");

	TEST_PASS();
}

static void test_dstr_length(void) {
	TEST_START("dstr_length");

	dstr_adt *d = dstr_create("Hello");
	CHECK(dstr_length(d) == 5, "长度为 5");
	dstr_destroy(d);

	CHECK(dstr_length(NULL) == 0, "NULL 输入返回 0");

	dstr_adt *empty = dstr_create("");
	CHECK(dstr_length(empty) == 0, "空字符串长度为 0");
	dstr_destroy(empty);

	TEST_PASS();
}

static void test_dstr_is_empty(void) {
	TEST_START("dstr_is_empty");

	dstr_adt *d = dstr_create("Hello");
	CHECK(!dstr_is_empty(d), "非空字符串返回 false");
	dstr_destroy(d);

	dstr_adt *empty = dstr_create("");
	CHECK(dstr_is_empty(empty), "空字符串返回 true");
	dstr_destroy(empty);

	CHECK(dstr_is_empty(NULL), "NULL 输入返回 true");

	TEST_PASS();
}

static void test_dstr_capacity(void) {
	TEST_START("dstr_capacity");

	dstr_adt *d = dstr_create("Test");
	size_t cap = dstr_capacity(d);
	CHECK(cap >= 4, "容量至少等于长度");
	dstr_destroy(d);

	CHECK(dstr_capacity(NULL) == 0, "NULL 输入返回 0");

	TEST_PASS();
}

static void test_dstr_set_capacity(void) {
	TEST_START("dstr_set_capacity");

	dstr_adt *d = dstr_create("Hello");
	dstr_status_t ret = dstr_set_capacity(d, 100);
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "设置容量到 100 成功");
	CHECK(dstr_capacity(d) >= 100, "容量 >= 100");
	dstr_destroy(d);

	/* 设置容量小于长度时应截断 */
	dstr_adt *d2 = dstr_create("Hello, World!");
	ret = dstr_set_capacity(d2, 6);
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "设置容量到 6 成功");
	CHECK(dstr_length(d2) == 5, "长度被截断为 5");
	dstr_destroy(d2);

	/* NULL 输入 */
	CHECK_STATUS_EQ(
		dstr_set_capacity(NULL, 10), DSTR_INVALID_ARGUMENT,
		"NULL 输入返回 INVALID_ARGUMENT"
	);

	TEST_PASS();
}

static void test_dstr_shrink_to_fit(void) {
	TEST_START("dstr_shrink_to_fit");

	dstr_adt *d = dstr_create("Hello");
	size_t old_cap = dstr_capacity(d);
	CHECK(old_cap >= 6, "初始容量 >= 6");

	dstr_shrink_to_fit(d);
	size_t new_cap = dstr_capacity(d);
	CHECK(new_cap <= old_cap, "缩容后容量 <= 原容量");
	CHECK(new_cap >= 6, "缩容后容量 >= 长度+1");
	CHECK_STR_EQ(d, "Hello", "缩容后内容不变");
	dstr_destroy(d);

	/* NULL 输入不应崩溃 */
	dstr_shrink_to_fit(NULL);
	printf("  - NULL 输入: OK (无崩溃)\n");

	/* 空字符串缩容 */
	dstr_adt *empty = dstr_create("");
	dstr_shrink_to_fit(empty);
	CHECK(dstr_is_empty(empty), "空字符串缩容后仍为空");
	dstr_destroy(empty);

	TEST_PASS();
}

/*------------------------------------------------------------------------------
 * 3. 内容编辑 —— 复制
 *----------------------------------------------------------------------------*/

static void test_dstr_cpy_cstr(void) {
	TEST_START("dstr_cpy_cstr");

	dstr_adt *d = dstr_create("Original");
	dstr_status_t ret = dstr_cpy_cstr(d, "New String");
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "复制 C 字符串成功");
	CHECK_STR_EQ(d, "New String", "内容更新正确");
	dstr_destroy(d);

	/* 复制空字符串 */
	dstr_adt *d2 = dstr_create("Original");
	ret = dstr_cpy_cstr(d2, "");
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "复制空 C 字符串成功");
	CHECK(dstr_is_empty(d2), "内容变为空");
	dstr_destroy(d2);

	/* 复制 NULL */
	dstr_adt *d3 = dstr_create("Original");
	ret = dstr_cpy_cstr(d3, NULL);
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "复制 NULL 成功");
	CHECK(dstr_is_empty(d3), "内容变为空");
	dstr_destroy(d3);

	/* NULL dest */
	CHECK_STATUS_EQ(
		dstr_cpy_cstr(NULL, "test"), DSTR_INVALID_ARGUMENT,
		"NULL dest 返回 INVALID_ARGUMENT"
	);

	TEST_PASS();
}

static void test_dstr_cpy(void) {
	TEST_START("dstr_cpy");

	dstr_adt *src = dstr_create("Source");
	dstr_adt *dest = dstr_create("Destination");
	dstr_status_t ret = dstr_cpy(dest, src);
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "复制动态字符串成功");
	CHECK_STR_EQ(dest, "Source", "内容更新正确");
	dstr_destroy(src);
	dstr_destroy(dest);

	/* 复制空 dstr */
	dstr_adt *src2 = dstr_create("");
	dstr_adt *dest2 = dstr_create("NonEmpty");
	ret = dstr_cpy(dest2, src2);
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "复制空 dstr 成功");
	CHECK(dstr_is_empty(dest2), "内容变为空");
	dstr_destroy(src2);
	dstr_destroy(dest2);

	/* NULL dest */
	dstr_adt *src3 = dstr_create("X");
	CHECK_STATUS_EQ(
		dstr_cpy(NULL, src3), DSTR_INVALID_ARGUMENT,
		"NULL dest 返回 INVALID_ARGUMENT"
	);
	dstr_destroy(src3);

	TEST_PASS();
}

static void test_dstr_cpy_sub_cstr(void) {
	TEST_START("dstr_cpy_sub_cstr");

	dstr_adt *d = dstr_create("Original");
	dstr_status_t ret = dstr_cpy_sub_cstr(d, "Hello, World!", 7, 5);
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "复制 C 字符串子串成功");
	CHECK_STR_EQ(d, "World", "子串内容正确");
	dstr_destroy(d);

	/* sub_length 为 0 */
	dstr_adt *d2 = dstr_create("Original");
	ret = dstr_cpy_sub_cstr(d2, "Hello, World!", 7, 0);
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "sub_length=0 复制成功");
	CHECK_STR_EQ(d2, "World!", "到末尾内容正确");
	dstr_destroy(d2);

	/* 越界 */
	dstr_adt *d3 = dstr_create("Original");
	ret = dstr_cpy_sub_cstr(d3, "Hi", 5, 1);
	CHECK_STATUS_EQ(ret, DSTR_INVALID_ARGUMENT, "越界返回 INVALID_ARGUMENT");
	dstr_destroy(d3);

	/* NULL dest */
	CHECK_STATUS_EQ(
		dstr_cpy_sub_cstr(NULL, "test", 0, 2), DSTR_INVALID_ARGUMENT,
		"NULL dest 返回 INVALID_ARGUMENT"
	);

	TEST_PASS();
}

static void test_dstr_cpy_sub(void) {
	TEST_START("dstr_cpy_sub");

	dstr_adt *src = dstr_create("Hello, World!");
	dstr_adt *dest = dstr_create("Original");
	dstr_status_t ret = dstr_cpy_sub(dest, src, 7, 5);
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "复制动态字符串子串成功");
	CHECK_STR_EQ(dest, "World", "子串内容正确");
	dstr_destroy(src);
	dstr_destroy(dest);

	/* 越界 */
	dstr_adt *src2 = dstr_create("Hi");
	dstr_adt *dest2 = dstr_create("Original");
	ret = dstr_cpy_sub(dest2, src2, 5, 1);
	CHECK_STATUS_EQ(ret, DSTR_INVALID_ARGUMENT, "越界返回 INVALID_ARGUMENT");
	dstr_destroy(src2);
	dstr_destroy(dest2);

	TEST_PASS();
}

static void test_dstr_cpy_format(void) {
	TEST_START("dstr_cpy_format");

	dstr_adt *d = dstr_create("Original");
	dstr_status_t ret = dstr_cpy_format(d, "Hello, %s! Num: %d", "World", 42);
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "格式化复制成功");
	CHECK_STR_EQ(d, "Hello, World! Num: 42", "格式化内容正确");
	dstr_destroy(d);

	/* 空格式 */
	dstr_adt *d2 = dstr_create("Original");
	ret = dstr_cpy_format(d2, "");
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "空格式复制成功");
	CHECK(dstr_is_empty(d2), "内容变为空");
	dstr_destroy(d2);

	/* NULL 格式 */
	dstr_adt *d3 = dstr_create("Original");
	ret = dstr_cpy_format(d3, NULL);
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "NULL 格式复制成功");
	CHECK(dstr_is_empty(d3), "内容变为空");
	dstr_destroy(d3);

	/* NULL dest */
	CHECK_STATUS_EQ(
		dstr_cpy_format(NULL, "test"), DSTR_INVALID_ARGUMENT,
		"NULL dest 返回 INVALID_ARGUMENT"
	);

	TEST_PASS();
}

static void test_dstr_cpy_vformat(void) {
	TEST_START("dstr_cpy_vformat");

	dstr_adt *d = dstr_create("Original");
	dstr_status_t ret = wrap_cpy_vformat(d, "Hello, %s! Num: %d", "World", 42);
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "vformat 复制成功");
	CHECK_STR_EQ(d, "Hello, World! Num: 42", "vformat 内容正确");
	dstr_destroy(d);

	/* 空格式 */
	dstr_adt *d2 = dstr_create("Original");
	ret = wrap_cpy_vformat(d2, "");
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "空格式复制成功");
	CHECK(dstr_is_empty(d2), "内容变为空");
	dstr_destroy(d2);

	/* NULL dest */
	CHECK_STATUS_EQ(
		wrap_cpy_vformat(NULL, "test"), DSTR_INVALID_ARGUMENT,
		"NULL dest 返回 INVALID_ARGUMENT"
	);

	TEST_PASS();
}

/*------------------------------------------------------------------------------
 * 4. 内容编辑 —— 追加
 *----------------------------------------------------------------------------*/

static void test_dstr_cat_cstr(void) {
	TEST_START("dstr_cat_cstr");

	dstr_adt *d = dstr_create("Hello");
	dstr_status_t ret = dstr_cat_cstr(d, ", World!");
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "追加 C 字符串成功");
	CHECK_STR_EQ(d, "Hello, World!", "追加后内容正确");
	dstr_destroy(d);

	/* 追加空字符串 */
	dstr_adt *d2 = dstr_create("Hello");
	ret = dstr_cat_cstr(d2, "");
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "追加空 C 字符串成功");
	CHECK_STR_EQ(d2, "Hello", "内容不变");
	dstr_destroy(d2);

	/* NULL dest */
	CHECK_STATUS_EQ(
		dstr_cat_cstr(NULL, "test"), DSTR_INVALID_ARGUMENT,
		"NULL dest 返回 INVALID_ARGUMENT"
	);

	TEST_PASS();
}

static void test_dstr_cat(void) {
	TEST_START("dstr_cat");

	dstr_adt *d1 = dstr_create("Hello");
	dstr_adt *d2 = dstr_create(", World!");
	dstr_status_t ret = dstr_cat(d1, d2);
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "追加快态字符串成功");
	CHECK_STR_EQ(d1, "Hello, World!", "追加后内容正确");
	dstr_destroy(d1);
	dstr_destroy(d2);

	/* 追加空 dstr */
	dstr_adt *d3 = dstr_create("Hello");
	dstr_adt *d4 = dstr_create("");
	ret = dstr_cat(d3, d4);
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "追加空 dstr 成功");
	CHECK_STR_EQ(d3, "Hello", "内容不变");
	dstr_destroy(d3);
	dstr_destroy(d4);

	TEST_PASS();
}

static void test_dstr_cat_sub_cstr(void) {
	TEST_START("dstr_cat_sub_cstr");

	dstr_adt *d = dstr_create("Hello");
	dstr_status_t ret = dstr_cat_sub_cstr(d, ", World! Welcome!", 0, 8);
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "追加 C 字符串子串成功");
	CHECK_STR_EQ(d, "Hello, World!", "追加后内容正确");
	dstr_destroy(d);

	/* 越界 */
	dstr_adt *d2 = dstr_create("Hello");
	ret = dstr_cat_sub_cstr(d2, "Hi", 5, 1);
	CHECK_STATUS_EQ(ret, DSTR_INVALID_ARGUMENT, "越界返回 INVALID_ARGUMENT");
	dstr_destroy(d2);

	TEST_PASS();
}

static void test_dstr_cat_sub(void) {
	TEST_START("dstr_cat_sub");

	dstr_adt *d1 = dstr_create("Hello");
	dstr_adt *d2 = dstr_create(", World! Welcome!");
	dstr_status_t ret = dstr_cat_sub(d1, d2, 0, 8);
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "追加快态字符串子串成功");
	CHECK_STR_EQ(d1, "Hello, World!", "追加后内容正确");
	dstr_destroy(d1);
	dstr_destroy(d2);

	TEST_PASS();
}

static void test_dstr_cat_format(void) {
	TEST_START("dstr_cat_format");

	dstr_adt *d = dstr_create("Hello");
	dstr_status_t ret = dstr_cat_format(d, ", %s! Num: %d", "World", 42);
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "格式化追加成功");
	CHECK_STR_EQ(d, "Hello, World! Num: 42", "格式化追加内容正确");
	dstr_destroy(d);

	/* 空格式 */
	dstr_adt *d2 = dstr_create("Hello");
	ret = dstr_cat_format(d2, "");
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "空格式追加成功");
	CHECK_STR_EQ(d2, "Hello", "内容不变");
	dstr_destroy(d2);

	/* NULL dest */
	CHECK_STATUS_EQ(
		dstr_cat_format(NULL, "test"), DSTR_INVALID_ARGUMENT,
		"NULL dest 返回 INVALID_ARGUMENT"
	);

	TEST_PASS();
}

static void test_dstr_cat_vformat(void) {
	TEST_START("dstr_cat_vformat");

	dstr_adt *d = dstr_create("Hello");
	dstr_status_t ret = wrap_cat_vformat(d, ", %s! Num: %d", "World", 42);
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "vformat 追加成功");
	CHECK_STR_EQ(d, "Hello, World! Num: 42", "vformat 追加内容正确");
	dstr_destroy(d);

	/* 空格式 */
	dstr_adt *d2 = dstr_create("Hello");
	ret = wrap_cat_vformat(d2, "");
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "空格式追加成功");
	CHECK_STR_EQ(d2, "Hello", "内容不变");
	dstr_destroy(d2);

	TEST_PASS();
}

/*------------------------------------------------------------------------------
 * 5. 内容编辑 —— 插入
 *----------------------------------------------------------------------------*/

static void test_dstr_insert_cstr(void) {
	TEST_START("dstr_insert_cstr");

	dstr_adt *d = dstr_create("Hello World!");
	dstr_status_t ret = dstr_insert_cstr(d, 6, "Beautiful ");
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "插入 C 字符串成功");
	CHECK_STR_EQ(d, "Hello Beautiful World!", "插入后内容正确");
	dstr_destroy(d);

	/* 在末尾插入 */
	dstr_adt *d2 = dstr_create("Hello");
	ret = dstr_insert_cstr(d2, 5, " World!");
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "在末尾插入成功");
	CHECK_STR_EQ(d2, "Hello World!", "末尾插入内容正确");
	dstr_destroy(d2);

	/* 越界 */
	dstr_adt *d3 = dstr_create("Hi");
	ret = dstr_insert_cstr(d3, 10, "test");
	CHECK_STATUS_EQ(ret, DSTR_INVALID_ARGUMENT, "越界返回 INVALID_ARGUMENT");
	dstr_destroy(d3);

	/* NULL dest */
	CHECK_STATUS_EQ(
		dstr_insert_cstr(NULL, 0, "test"), DSTR_INVALID_ARGUMENT,
		"NULL dest 返回 INVALID_ARGUMENT"
	);

	TEST_PASS();
}

static void test_dstr_insert(void) {
	TEST_START("dstr_insert");

	dstr_adt *d1 = dstr_create("Hello World!");
	dstr_adt *d2 = dstr_create("Beautiful ");
	dstr_status_t ret = dstr_insert(d1, 6, d2);
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "插入动态字符串成功");
	CHECK_STR_EQ(d1, "Hello Beautiful World!", "插入后内容正确");
	dstr_destroy(d1);
	dstr_destroy(d2);

	/* 插入空 dstr */
	dstr_adt *d3 = dstr_create("Hello");
	dstr_adt *d4 = dstr_create("");
	ret = dstr_insert(d3, 2, d4);
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "插入空 dstr 成功");
	CHECK_STR_EQ(d3, "Hello", "内容不变");
	dstr_destroy(d3);
	dstr_destroy(d4);

	TEST_PASS();
}

static void test_dstr_insert_sub_cstr(void) {
	TEST_START("dstr_insert_sub_cstr");

	dstr_adt *d = dstr_create("Hello!");
	dstr_status_t ret = dstr_insert_sub_cstr(d, 5, ", World! Fine.", 0, 7);
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "插入 C 字符串子串成功");
	CHECK_STR_EQ(d, "Hello, World!", "插入后内容正确");
	dstr_destroy(d);

	/* 越界 */
	dstr_adt *d2 = dstr_create("Hello");
	ret = dstr_insert_sub_cstr(d2, 0, "Hi", 5, 1);
	CHECK_STATUS_EQ(ret, DSTR_INVALID_ARGUMENT, "子串越界返回 INVALID_ARGUMENT");
	dstr_destroy(d2);

	TEST_PASS();
}

static void test_dstr_insert_sub(void) {
	TEST_START("dstr_insert_sub");

	dstr_adt *d1 = dstr_create("Hello!");
	dstr_adt *d2 = dstr_create(", World! Fine.");
	dstr_status_t ret = dstr_insert_sub(d1, 5, d2, 0, 7);
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "插入动态字符串子串成功");
	CHECK_STR_EQ(d1, "Hello, World!", "插入后内容正确");
	dstr_destroy(d1);
	dstr_destroy(d2);

	TEST_PASS();
}

static void test_dstr_insert_format(void) {
	TEST_START("dstr_insert_format");

	dstr_adt *d = dstr_create("Hello World!");
	dstr_status_t ret = dstr_insert_format(d, 6, "Beautiful %s", "Day");
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "格式化插入成功");
	CHECK_STR_EQ(d, "Hello Beautiful DayWorld!", "格式化插入内容正确");
	dstr_destroy(d);

	/* 在开头插入 */
	dstr_adt *d2 = dstr_create("World");
	ret = dstr_insert_format(d2, 0, "Hello, %s! ", "World");
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "在开头格式化插入成功");
	CHECK_STR_EQ(d2, "Hello, World! World", "开头插入内容正确");
	dstr_destroy(d2);

	TEST_PASS();
}

static void test_dstr_insert_vformat(void) {
	TEST_START("dstr_insert_vformat");

	dstr_adt *d = dstr_create("Hello World!");
	dstr_status_t ret = wrap_insert_vformat(d, 6, "Beautiful %s", "Day");
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "vformat 插入成功");
	CHECK_STR_EQ(d, "Hello Beautiful DayWorld!", "vformat 插入内容正确");
	dstr_destroy(d);

	/* 在开头插入 */
	dstr_adt *d2 = dstr_create("World");
	ret = wrap_insert_vformat(d2, 0, "Hello, %s! ", "World");
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "在开头 vformat 插入成功");
	CHECK_STR_EQ(d2, "Hello, World! World", "开头插入内容正确");
	dstr_destroy(d2);

	TEST_PASS();
}

/*------------------------------------------------------------------------------
 * 6. 内容编辑 —— 删除与清空
 *----------------------------------------------------------------------------*/

static void test_dstr_clear(void) {
	TEST_START("dstr_clear");

	dstr_adt *d = dstr_create("Hello, World!");
	dstr_clear(d);
	CHECK(dstr_length(d) == 0, "清空后长度为 0");
	CHECK(strcmp(dstr_cstr(d), "") == 0, "清空后为空字符串");
	dstr_destroy(d);

	/* 清空 NULL 不应崩溃 */
	dstr_clear(NULL);
	printf("  - 清空 NULL: OK (无崩溃)\n");

	TEST_PASS();
}

static void test_dstr_remove(void) {
	TEST_START("dstr_remove");

	dstr_adt *d = dstr_create("Hello, Beautiful World!");
	dstr_remove(d, 6, 10);
	CHECK_STR_EQ(d, "Hello, World!", "删除子串后内容正确");
	dstr_destroy(d);

	/* sub_length 为 0 删除到末尾 */
	dstr_adt *d2 = dstr_create("Hello, World!");
	dstr_remove(d2, 7, 0);
	CHECK_STR_EQ(d2, "Hello, ", "删除到末尾内容正确");
	dstr_destroy(d2);

	/* 越界不应崩溃 */
	dstr_adt *d3 = dstr_create("Hi");
	dstr_remove(d3, 5, 1);
	CHECK_STR_EQ(d3, "Hi", "越界删除内容不变");
	dstr_destroy(d3);

	/* NULL 输入 */
	dstr_remove(NULL, 0, 1);
	printf("  - NULL 输入: OK (无崩溃)\n");

	TEST_PASS();
}

static void test_dstr_trim(void) {
	TEST_START("dstr_trim");

	/* 去除空白字符 */
	dstr_adt *d = dstr_create("   Hello, World!   ");
	dstr_trim(d, NULL);
	CHECK_STR_EQ(d, "Hello, World!", "去除首尾空白正确");
	dstr_destroy(d);

	/* 指定字符集 */
	dstr_adt *d2 = dstr_create("***Hello***");
	dstr_trim(d2, "*");
	CHECK_STR_EQ(d2, "Hello", "去除指定字符正确");
	dstr_destroy(d2);

	/* 全是空白字符 */
	dstr_adt *d3 = dstr_create("   ");
	dstr_trim(d3, NULL);
	CHECK(dstr_is_empty(d3), "全空白去除后为空");
	dstr_destroy(d3);

	/* NULL 输入 */
	dstr_trim(NULL, NULL);
	printf("  - NULL 输入: OK (无崩溃)\n");

	TEST_PASS();
}

/*------------------------------------------------------------------------------
 * 7. 关系判断与比较
 *----------------------------------------------------------------------------*/

static void test_dstr_starts_with_cstr(void) {
	TEST_START("dstr_starts_with_cstr");

	dstr_adt *d = dstr_create("Hello, World!");
	CHECK(dstr_starts_with_cstr(d, "Hello"), "匹配前缀返回 true");
	CHECK(!dstr_starts_with_cstr(d, "World"), "不匹配返回 false");
	CHECK(
		!dstr_starts_with_cstr(d, "Hello, World! Extra"),
		"前缀长于字符串返回 false"
	);
	dstr_destroy(d);

	/* NULL / 空字符串 */
	dstr_adt *empty = dstr_create("");
	CHECK(!dstr_starts_with_cstr(empty, "a"), "空 dstr 返回 false");
	dstr_destroy(empty);

	CHECK(!dstr_starts_with_cstr(NULL, "a"), "NULL dstr 返回 false");

	TEST_PASS();
}

static void test_dstr_starts_with(void) {
	TEST_START("dstr_starts_with");

	dstr_adt *d = dstr_create("Hello, World!");
	dstr_adt *prefix = dstr_create("Hello");
	CHECK(dstr_starts_with(d, prefix), "匹配前缀返回 true");
	dstr_destroy(d);
	dstr_destroy(prefix);

	/* 不匹配 */
	dstr_adt *d2 = dstr_create("Hello, World!");
	dstr_adt *prefix2 = dstr_create("World");
	CHECK(!dstr_starts_with(d2, prefix2), "不匹配返回 false");
	dstr_destroy(d2);
	dstr_destroy(prefix2);

	TEST_PASS();
}

static void test_dstr_ends_with_cstr(void) {
	TEST_START("dstr_ends_with_cstr");

	dstr_adt *d = dstr_create("Hello, World!");
	CHECK(dstr_ends_with_cstr(d, "World!"), "匹配后缀返回 true");
	CHECK(!dstr_ends_with_cstr(d, "Hello"), "不匹配返回 false");
	CHECK(
		!dstr_ends_with_cstr(d, "Hello, World! Extra"),
		"后缀长于字符串返回 false"
	);
	dstr_destroy(d);

	TEST_PASS();
}

static void test_dstr_ends_with(void) {
	TEST_START("dstr_ends_with");

	dstr_adt *d = dstr_create("Hello, World!");
	dstr_adt *suffix = dstr_create("World!");
	CHECK(dstr_ends_with(d, suffix), "匹配后缀返回 true");
	dstr_destroy(d);
	dstr_destroy(suffix);

	/* 不匹配 */
	dstr_adt *d2 = dstr_create("Hello, World!");
	dstr_adt *suffix2 = dstr_create("Hello");
	CHECK(!dstr_ends_with(d2, suffix2), "不匹配返回 false");
	dstr_destroy(d2);
	dstr_destroy(suffix2);

	TEST_PASS();
}

static void test_dstr_contains_cstr(void) {
	TEST_START("dstr_contains_cstr");

	dstr_adt *d = dstr_create("Hello, World!");
	CHECK(dstr_contains_cstr(d, "World"), "包含返回 true");
	CHECK(!dstr_contains_cstr(d, "xyz"), "不包含返回 false");
	CHECK(
		!dstr_contains_cstr(d, "Hello, World! Extra"),
		"子串长于字符串返回 false"
	);
	dstr_destroy(d);

	TEST_PASS();
}

static void test_dstr_contains(void) {
	TEST_START("dstr_contains");

	dstr_adt *d = dstr_create("Hello, World!");
	dstr_adt *sub = dstr_create("World");
	CHECK(dstr_contains(d, sub), "包含返回 true");
	dstr_destroy(d);
	dstr_destroy(sub);

	/* 不包含 */
	dstr_adt *d2 = dstr_create("Hello, World!");
	dstr_adt *sub2 = dstr_create("xyz");
	CHECK(!dstr_contains(d2, sub2), "不包含返回 false");
	dstr_destroy(d2);
	dstr_destroy(sub2);

	TEST_PASS();
}

static void test_dstr_equals_cstr(void) {
	TEST_START("dstr_equals_cstr");

	dstr_adt *d = dstr_create("Hello, World!");
	CHECK(dstr_equals_cstr(d, "Hello, World!"), "相等返回 true");
	CHECK(!dstr_equals_cstr(d, "Hello"), "不等返回 false");
	dstr_destroy(d);

	/* 双方都为空 */
	CHECK(dstr_equals_cstr(NULL, NULL), "双方 NULL 视为相等");
	CHECK(dstr_equals_cstr(NULL, ""), "NULL 与空 C 字符串视为相等");

	dstr_adt *empty = dstr_create("");
	CHECK(dstr_equals_cstr(empty, ""), "空 dstr 与空 C 字符串相等");
	CHECK(dstr_equals_cstr(empty, NULL), "空 dstr 与 NULL 相等");
	dstr_destroy(empty);

	TEST_PASS();
}

static void test_dstr_equals(void) {
	TEST_START("dstr_equals");

	dstr_adt *d1 = dstr_create("Hello, World!");
	dstr_adt *d2 = dstr_create("Hello, World!");
	CHECK(dstr_equals(d1, d2), "相等返回 true");
	dstr_destroy(d1);
	dstr_destroy(d2);

	/* 不等 */
	dstr_adt *d3 = dstr_create("ABC");
	dstr_adt *d4 = dstr_create("XYZ");
	CHECK(!dstr_equals(d3, d4), "不等返回 false");
	dstr_destroy(d3);
	dstr_destroy(d4);

	/* 双方都为空 */
	CHECK(dstr_equals(NULL, NULL), "双方 NULL 视为相等");

	TEST_PASS();
}

static void test_dstr_compare_cstr(void) {
	TEST_START("dstr_compare_cstr");

	dstr_adt *d = dstr_create("ABC");
	CHECK(dstr_compare_cstr(d, "ABC") == 0, "相等返回 0");
	CHECK(dstr_compare_cstr(d, "AB") > 0, "大于返回正值");
	CHECK(dstr_compare_cstr(d, "ABCD") < 0, "小于返回负值");
	dstr_destroy(d);

	/* NULL 比较 */
	CHECK(dstr_compare_cstr(NULL, "A") < 0, "NULL < 非空字符串");
	CHECK(dstr_compare_cstr(NULL, NULL) == 0, "双方 NULL 返回 0");

	dstr_adt *empty = dstr_create("");
	CHECK(dstr_compare_cstr(empty, NULL) == 0, "空 dstr 与 NULL 相等");
	dstr_destroy(empty);

	TEST_PASS();
}

static void test_dstr_compare(void) {
	TEST_START("dstr_compare");

	dstr_adt *d1 = dstr_create("ABC");
	dstr_adt *d2 = dstr_create("ABC");
	CHECK(dstr_compare(d1, d2) == 0, "相等返回 0");
	dstr_destroy(d1);
	dstr_destroy(d2);

	dstr_adt *d3 = dstr_create("ABC");
	dstr_adt *d4 = dstr_create("AB");
	CHECK(dstr_compare(d3, d4) > 0, "大于返回正值");
	dstr_destroy(d3);
	dstr_destroy(d4);

	dstr_adt *d5 = dstr_create("AB");
	dstr_adt *d6 = dstr_create("ABC");
	CHECK(dstr_compare(d5, d6) < 0, "小于返回负值");
	dstr_destroy(d5);
	dstr_destroy(d6);

	TEST_PASS();
}

/*------------------------------------------------------------------------------
 * 8. 查找、统计与替换
 *----------------------------------------------------------------------------*/

static void test_dstr_find_cstr(void) {
	TEST_START("dstr_find_cstr");

	dstr_adt *d = dstr_create("Hello, World!");
	size_t index;
	bool found;

	/* 正向查找 */
	found = dstr_find_cstr(d, "World", &index, DSTR_DIR_FORWARD);
	CHECK(found && index == 7, "正向查找找到 World 在索引 7");

	/* 反向查找 */
	found = dstr_find_cstr(d, "l", &index, DSTR_DIR_BACKWARD);
	CHECK(found && index == 10, "反向查找找到 l 在索引 10");

	/* 未找到 */
	found = dstr_find_cstr(d, "xyz", &index, DSTR_DIR_FORWARD);
	CHECK(!found, "未找到返回 false");

	/* out_index 为 NULL */
	found = dstr_find_cstr(d, "World", NULL, DSTR_DIR_FORWARD);
	CHECK(found, "out_index 为 NULL 仍能正常工作");

	/* 空 dstr */
	dstr_adt *empty = dstr_create("");
	found = dstr_find_cstr(empty, "a", &index, DSTR_DIR_FORWARD);
	CHECK(!found, "空 dstr 返回 false");
	dstr_destroy(empty);

	dstr_destroy(d);

	TEST_PASS();
}

static void test_dstr_find(void) {
	TEST_START("dstr_find");

	dstr_adt *d = dstr_create("Hello, World!");
	dstr_adt *sub = dstr_create("World");
	size_t index;
	bool found = dstr_find(d, sub, &index, DSTR_DIR_FORWARD);
	CHECK(found && index == 7, "正向查找找到 World 在索引 7");
	dstr_destroy(d);
	dstr_destroy(sub);

	/* 反向查找 */
	dstr_adt *d2 = dstr_create("Hello, World!");
	dstr_adt *sub2 = dstr_create("o");
	found = dstr_find(d2, sub2, &index, DSTR_DIR_BACKWARD);
	CHECK(found && index == 8, "反向查找找到 o 在索引 8");
	dstr_destroy(d2);
	dstr_destroy(sub2);

	TEST_PASS();
}

static void test_dstr_find_nth_cstr(void) {
	TEST_START("dstr_find_nth_cstr");

	dstr_adt *d = dstr_create("abc def abc ghi abc");
	size_t index;
	bool found;

	/* 正向第 2 次 */
	found = dstr_find_nth_cstr(d, "abc", &index, DSTR_DIR_FORWARD, 2);
	CHECK(found && index == 8, "正向第 2 次出现: 索引 8");

	/* n 为 0 表示最后一次 */
	found = dstr_find_nth_cstr(d, "abc", &index, DSTR_DIR_FORWARD, 0);
	CHECK(found && index == 16, "n=0 (最后一次): 索引 16");

	/* n 超出实际次数应视为最后一次 */
	found = dstr_find_nth_cstr(d, "abc", &index, DSTR_DIR_FORWARD, 100);
	CHECK(found && index == 16, "n 超出范围: 返回最后一次位置");

	/* 未找到 */
	found = dstr_find_nth_cstr(d, "xyz", &index, DSTR_DIR_FORWARD, 1);
	CHECK(!found, "未找到返回 false");

	dstr_destroy(d);

	TEST_PASS();
}

static void test_dstr_find_nth(void) {
	TEST_START("dstr_find_nth");

	dstr_adt *d = dstr_create("abc def abc ghi abc");
	dstr_adt *sub = dstr_create("abc");
	size_t index;
	bool found = dstr_find_nth(d, sub, &index, DSTR_DIR_FORWARD, 2);
	CHECK(found && index == 8, "正向第 2 次出现: 索引 8");
	dstr_destroy(d);
	dstr_destroy(sub);

	/* 反向第 1 次 */
	dstr_adt *d2 = dstr_create("abc def abc ghi abc");
	dstr_adt *sub2 = dstr_create("abc");
	found = dstr_find_nth(d2, sub2, &index, DSTR_DIR_BACKWARD, 1);
	CHECK(found && index == 16, "反向第 1 次出现: 索引 16");
	dstr_destroy(d2);
	dstr_destroy(sub2);

	TEST_PASS();
}

static void test_dstr_find_indexes_cstr(void) {
	TEST_START("dstr_find_indexes_cstr");

	dstr_adt *d = dstr_create("abc def abc ghi abc");
	size_t indexes[10] = {0};

	/* 查找全部 */
	size_t count = dstr_find_indexes_cstr(
		d, "abc", indexes,
		DSTR_DIR_FORWARD, 0
	);
	CHECK(count == 3, "查找全部返回 3 次");
	CHECK(
		indexes[0] == 0 && indexes[1] == 8 && indexes[2] == 16,
		"全部位置正确"
	);

	/* 查找前 2 次 */
	count = dstr_find_indexes_cstr(
		d, "abc", indexes,
		DSTR_DIR_FORWARD, 2
	);
	CHECK(count == 2, "查找前 2 次返回 2");

	/* 反向查找 */
	count = dstr_find_indexes_cstr(
		d, "abc", indexes,
		DSTR_DIR_BACKWARD, 0
	);
	CHECK(count == 3, "反向查找全部返回 3");

	/* out_indexes 为 NULL */
	count = dstr_find_indexes_cstr(
		d, "abc", NULL,
		DSTR_DIR_FORWARD, 0
	);
	CHECK(count == 3, "out_indexes 为 NULL 返回正确计数");

	dstr_destroy(d);

	TEST_PASS();
}

static void test_dstr_find_indexes(void) {
	TEST_START("dstr_find_indexes");

	dstr_adt *d = dstr_create("abc def abc ghi abc");
	dstr_adt *sub = dstr_create("abc");
	size_t indexes[10] = {0};

	size_t count = dstr_find_indexes(
		d, sub, indexes,
		DSTR_DIR_FORWARD, 0
	);
	CHECK(count == 3, "查找全部返回 3 次");
	CHECK(
		indexes[0] == 0 && indexes[1] == 8 && indexes[2] == 16,
		"全部位置正确"
	);

	dstr_destroy(d);
	dstr_destroy(sub);

	TEST_PASS();
}

static void test_dstr_count_cstr(void) {
	TEST_START("dstr_count_cstr");

	dstr_adt *d = dstr_create("abc abc abc");
	size_t count = dstr_count_cstr(d, "abc");
	CHECK(count == 3, "统计 abc 出现 3 次");

	count = dstr_count_cstr(d, "xyz");
	CHECK(count == 0, "未出现返回 0");

	dstr_destroy(d);

	/* 空 dstr */
	dstr_adt *empty = dstr_create("");
	count = dstr_count_cstr(empty, "a");
	CHECK(count == 0, "空 dstr 返回 0");
	dstr_destroy(empty);

	TEST_PASS();
}

static void test_dstr_count(void) {
	TEST_START("dstr_count");

	dstr_adt *d = dstr_create("abc abc abc");
	dstr_adt *sub = dstr_create("abc");
	size_t count = dstr_count(d, sub);
	CHECK(count == 3, "统计 abc 出现 3 次");
	dstr_destroy(d);
	dstr_destroy(sub);

	TEST_PASS();
}

static void test_dstr_replace_cstr(void) {
	TEST_START("dstr_replace_cstr");

	/* 替换全部 */
	dstr_adt *d = dstr_create("Hello World, Hello Everyone");
	dstr_status_t ret = dstr_replace_cstr(
		d, "Hello", "Hi",
		DSTR_DIR_FORWARD, 0
	);
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "替换全部成功");
	CHECK_STR_EQ(d, "Hi World, Hi Everyone", "替换全部内容正确");
	dstr_destroy(d);

	/* 只替换 1 次 */
	dstr_adt *d2 = dstr_create("Hello World, Hello Everyone");
	ret = dstr_replace_cstr(
		d2, "Hello", "Hi",
		DSTR_DIR_FORWARD, 1
	);
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "替换 1 次成功");
	CHECK_STR_EQ(d2, "Hi World, Hello Everyone", "替换 1 次内容正确");
	dstr_destroy(d2);

	/* 替换为空（删除） */
	dstr_adt *d3 = dstr_create("Hello World");
	ret = dstr_replace_cstr(
		d3, "Hello ", "",
		DSTR_DIR_FORWARD, 0
	);
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "替换为空成功");
	CHECK_STR_EQ(d3, "World", "替换为空内容正确");
	dstr_destroy(d3);

	/* old_str 不存在 */
	dstr_adt *d4 = dstr_create("Hello");
	ret = dstr_replace_cstr(
		d4, "xyz", "abc",
		DSTR_DIR_FORWARD, 0
	);
	CHECK_STATUS_EQ(ret, DSTR_INVALID_ARGUMENT, "old_str 不存在返回 INVALID_ARGUMENT");
	dstr_destroy(d4);

	TEST_PASS();
}

static void test_dstr_replace(void) {
	TEST_START("dstr_replace");

	dstr_adt *d = dstr_create("Hello World, Hello Everyone");
	dstr_adt *old = dstr_create("Hello");
	dstr_adt *new_str = dstr_create("Hi");
	dstr_status_t ret = dstr_replace(
		d, old, new_str,
		DSTR_DIR_FORWARD, 0
	);
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "替换全部成功");
	CHECK_STR_EQ(d, "Hi World, Hi Everyone", "替换全部内容正确");
	dstr_destroy(d);
	dstr_destroy(old);
	dstr_destroy(new_str);

	/* 反向替换 */
	dstr_adt *d2 = dstr_create("Hello World, Hello Everyone");
	dstr_adt *old2 = dstr_create("Hello");
	dstr_adt *new2 = dstr_create("Hi");
	ret = dstr_replace(
		d2, old2, new2,
		DSTR_DIR_BACKWARD, 1
	);
	CHECK_STATUS_EQ(ret, DSTR_SUCCESS, "反向替换 1 次成功");
	CHECK_STR_EQ(d2, "Hello World, Hi Everyone", "反向替换 1 次内容正确");
	dstr_destroy(d2);
	dstr_destroy(old2);
	dstr_destroy(new2);

	TEST_PASS();
}

/*------------------------------------------------------------------------------
 * 9. 分隔与合并
 *----------------------------------------------------------------------------*/

static void test_dstr_split_cstr(void) {
	TEST_START("dstr_split_cstr");

	size_t count;
	dstr_adt **parts = dstr_split_cstr("apple,banana,cherry", ",", &count);
	CHECK(parts != NULL, "分隔成功");
	CHECK(count == 3, "分隔为 3 部分");
	CHECK_STR_EQ(parts[0], "apple", "第 1 部分: apple");
	CHECK_STR_EQ(parts[1], "banana", "第 2 部分: banana");
	CHECK_STR_EQ(parts[2], "cherry", "第 3 部分: cherry");
	for (size_t i = 0; i < count; ++i) { dstr_destroy(parts[i]); }
	free(parts);

	/* 包含空子串 */
	parts = dstr_split_cstr("a,,b", ",", &count);
	CHECK(parts != NULL, "含空子串分隔成功");
	CHECK(count == 3, "含空子串分隔为 3 部分");
	CHECK_STR_EQ(parts[0], "a", "第 1 部分: a");
	CHECK(parts[1] == NULL, "第 2 部分: NULL (空)");
	CHECK_STR_EQ(parts[2], "b", "第 3 部分: b");
	for (size_t i = 0; i < count; ++i) { if (parts[i]) dstr_destroy(parts[i]); }
	free(parts);

	/* 没有分隔符 */
	parts = dstr_split_cstr("hello", ",", &count);
	CHECK(parts == NULL, "无分隔符返回 NULL");

	/* NULL 输入 */
	parts = dstr_split_cstr(NULL, ",", &count);
	CHECK(parts == NULL, "NULL cstr 返回 NULL");

	TEST_PASS();
}

static void test_dstr_split(void) {
	TEST_START("dstr_split");

	dstr_adt *d = dstr_create("apple,banana,cherry");
	dstr_adt *sep = dstr_create(",");
	size_t count;
	dstr_adt **parts = dstr_split(d, sep, &count);
	CHECK(parts != NULL, "分隔成功");
	CHECK(count == 3, "分隔为 3 部分");
	CHECK_STR_EQ(parts[0], "apple", "第 1 部分正确");
	CHECK_STR_EQ(parts[1], "banana", "第 2 部分正确");
	CHECK_STR_EQ(parts[2], "cherry", "第 3 部分正确");
	for (size_t i = 0; i < count; ++i) { dstr_destroy(parts[i]); }
	free(parts);
	dstr_destroy(d);
	dstr_destroy(sep);

	TEST_PASS();
}

static void test_dstr_join_cstr(void) {
	TEST_START("dstr_join_cstr");

	const char *words[] = {"apple", "banana", "cherry"};
	dstr_adt *result = dstr_join_cstr(words, 3, ",");
	CHECK(result != NULL, "合并成功");
	CHECK_STR_EQ(result, "apple,banana,cherry", "合并内容正确");
	dstr_destroy(result);

	/* 无分隔符 */
	result = dstr_join_cstr(words, 3, NULL);
	CHECK(result != NULL, "无分隔符合并成功");
	CHECK_STR_EQ(result, "applebananacherry", "无分隔符合并内容正确");
	dstr_destroy(result);

	/* 包含空字符串 */
	const char *words2[] = {"a", "", "b"};
	result = dstr_join_cstr(words2, 3, ",");
	CHECK(result != NULL, "含空字符串合并成功");
	CHECK_STR_EQ(result, "a,,b", "含空字符串合并内容正确");
	dstr_destroy(result);

	/* NULL 输入 */
	result = dstr_join_cstr(NULL, 3, ",");
	CHECK(result == NULL, "NULL 数组返回 NULL");

	result = dstr_join_cstr(words, 0, ",");
	CHECK(result == NULL, "count=0 返回 NULL");

	TEST_PASS();
}

static void test_dstr_join(void) {
	TEST_START("dstr_join");

	dstr_adt *d1 = dstr_create("apple");
	dstr_adt *d2 = dstr_create("banana");
	dstr_adt *d3 = dstr_create("cherry");
	dstr_adt *sep = dstr_create(",");

	const dstr_adt *dstrs[] = {d1, d2, d3};
	dstr_adt *result = dstr_join(dstrs, 3, sep);
	CHECK(result != NULL, "合并成功");
	CHECK_STR_EQ(result, "apple,banana,cherry", "合并内容正确");

	dstr_destroy(result);
	dstr_destroy(d1);
	dstr_destroy(d2);
	dstr_destroy(d3);
	dstr_destroy(sep);

	TEST_PASS();
}

/*------------------------------------------------------------------------------
 * 10. 综合与压力测试
 *----------------------------------------------------------------------------*/

static void test_comprehensive(void) {
	TEST_START("综合测试");

	/* 链式操作 */
	dstr_adt *d = dstr_create("");
	dstr_cat_cstr(d, "Hello");
	dstr_cat_cstr(d, ", ");
	dstr_cat_cstr(d, "World");
	dstr_cat_cstr(d, "!");
	CHECK_STR_EQ(d, "Hello, World!", "链式追加正确");

	/* 插入、替换、删除组合 */
	dstr_insert_cstr(d, 5, " Beautiful");
	CHECK_STR_EQ(d, "Hello Beautiful, World!", "插入后正确");

	dstr_replace_cstr(
		d, "Beautiful", "Amazing",
		DSTR_DIR_FORWARD, 1
	);
	CHECK_STR_EQ(d, "Hello Amazing, World!", "替换后正确");

	dstr_remove(d, 5, 8);
	CHECK_STR_EQ(d, "Hello, World!", "删除后正确");

	dstr_destroy(d);

	/* 大量操作压力测试 */
	dstr_adt *big = dstr_create("");
	for (int i = 0; i < 1000; ++i) {
		dstr_cat_cstr(big, "a");
	}
	CHECK(dstr_length(big) == 1000, "1000 次追加后长度正确");

	dstr_clear(big);
	CHECK(dstr_length(big) == 0, "清空后长度为 0");

	/* 再次追加验证 */
	for (int i = 0; i < 500; ++i) {
		dstr_cat_cstr(big, "b");
	}
	CHECK(dstr_length(big) == 500, "清空后再追加 500 次长度正确");
	dstr_destroy(big);

	TEST_PASS();
}

/*------------------------------------------------------------------------------
 * 主函数
 *----------------------------------------------------------------------------*/

int main(void) {
	printf("========================================\n");
	printf("   Dynamic String API 全面测试套件\n");
	printf("========================================\n");

	/* 创建与销毁 */
	test_dstr_create();
	test_dstr_destroy();
	test_dstr_clone();
	test_dstr_sub_cstr();
	test_dstr_sub();
	test_dstr_create_format();
	test_dstr_create_vformat();

	/* 属性获取与设置 */
	test_dstr_cstr();
	test_dstr_length();
	test_dstr_is_empty();
	test_dstr_capacity();
	test_dstr_set_capacity();
	test_dstr_shrink_to_fit();

	/* 复制 */
	test_dstr_cpy_cstr();
	test_dstr_cpy();
	test_dstr_cpy_sub_cstr();
	test_dstr_cpy_sub();
	test_dstr_cpy_format();
	test_dstr_cpy_vformat();

	/* 追加 */
	test_dstr_cat_cstr();
	test_dstr_cat();
	test_dstr_cat_sub_cstr();
	test_dstr_cat_sub();
	test_dstr_cat_format();
	test_dstr_cat_vformat();

	/* 插入 */
	test_dstr_insert_cstr();
	test_dstr_insert();
	test_dstr_insert_sub_cstr();
	test_dstr_insert_sub();
	test_dstr_insert_format();
	test_dstr_insert_vformat();

	/* 删除与清空 */
	test_dstr_clear();
	test_dstr_remove();
	test_dstr_trim();

	/* 关系判断与比较 */
	test_dstr_starts_with_cstr();
	test_dstr_starts_with();
	test_dstr_ends_with_cstr();
	test_dstr_ends_with();
	test_dstr_contains_cstr();
	test_dstr_contains();
	test_dstr_equals_cstr();
	test_dstr_equals();
	test_dstr_compare_cstr();
	test_dstr_compare();

	/* 查找、统计与替换 */
	test_dstr_find_cstr();
	test_dstr_find();
	test_dstr_find_nth_cstr();
	test_dstr_find_nth();
	test_dstr_find_indexes_cstr();
	test_dstr_find_indexes();
	test_dstr_count_cstr();
	test_dstr_count();
	test_dstr_replace_cstr();
	test_dstr_replace();

	/* 分隔与合并 */
	test_dstr_split_cstr();
	test_dstr_split();
	test_dstr_join_cstr();
	test_dstr_join();

	/* 综合 */
	test_comprehensive();

	printf("\n========================================\n");
	printf("   所有测试完成！\n");
	printf("========================================\n");
	printf("总测试数: %d\n", test_passed + test_failed);
	printf("通过: %d\n", test_passed);
	printf("失败: %d\n", test_failed);

	if (test_failed > 0) {
		printf("\n以下 %d 个测试失败：\n", test_failed);
		for (int i = 0; i < failed_count; i++) {
			printf("  - %s\n", failed_tests[i]);
		}
	} else {
		printf("\n所有测试均通过！\n");
	}

	return test_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
