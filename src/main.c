#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dynamic_string.h"

// 测试辅助宏
#define TEST_START(name) printf("\n=== 测试: %s ===\n", name)
#define TEST_PASS() printf("✓ 测试通过\n")
#define TEST_FAIL(msg) printf("✗ 测试失败: %s\n", msg)

// 1. 测试 dstr_create
void test_dstr_create(void) {
	TEST_START("dstr_create");
	
	// 测试正常创建
	dstr_adt *dstr1 = dstr_create("Hello, World!");
	if (dstr1 != NULL && strcmp(dstr_cstr_const(dstr1), "Hello, World!") == 0) {
		printf("  - 正常创建: 通过\n");
	} else {
		TEST_FAIL("正常创建失败");
	}
	dstr_destroy(dstr1);
	
	// 测试创建空字符串
	dstr_adt *dstr2 = dstr_create(NULL);
	if (dstr2 != NULL && dstr_length(dstr2) == 0) {
		printf("  - 创建空字符串: 通过\n");
	} else {
		TEST_FAIL("创建空字符串失败");
	}
	dstr_destroy(dstr2);
	
	TEST_PASS();
}

// 2. 测试 dstr_destroy
void test_dstr_destroy(void) {
	TEST_START("dstr_destroy");
	
	dstr_adt *dstr = dstr_create("Test");
	if (dstr != NULL) {
		dstr_destroy(dstr);
		printf("  - 销毁非空字符串: 通过\n");
	} else {
		TEST_FAIL("创建测试字符串失败");
	}
	
	TEST_PASS();
}

// 3. 测试 dstr_clear
void test_dstr_clear(void) {
	TEST_START("dstr_clear");
	
	dstr_adt *dstr = dstr_create("Hello, World!");
	dstr_clear(dstr);
	
	if (dstr_length(dstr) == 0 && strcmp(dstr_cstr_const(dstr), "") == 0) {
		printf("  - 清空字符串: 通过\n");
	} else {
		TEST_FAIL("清空字符串失败");
	}
	
	dstr_destroy(dstr);
	TEST_PASS();
}

// 4. 测试 dstr_cstr
void test_dstr_cstr(void) {
	TEST_START("dstr_cstr");
	
	dstr_adt *dstr = dstr_create("Test String");
	char *cstr = dstr_cstr(dstr);
	
	if (strcmp(cstr, "Test String") == 0) {
		printf("  - 获取 C 字符串: 通过\n");
	} else {
		TEST_FAIL("获取 C 字符串失败");
	}
	
	dstr_destroy(dstr);
	TEST_PASS();
}

// 5. 测试 dstr_cstr_const
void test_dstr_cstr_const(void) {
	TEST_START("dstr_cstr_const");
	
	dstr_adt *dstr = dstr_create("Const Test");
	const char *cstr = dstr_cstr_const(dstr);
	
	if (strcmp(cstr, "Const Test") == 0) {
		printf("  - 获取常量 C 字符串: 通过\n");
	} else {
		TEST_FAIL("获取常量 C 字符串失败");
	}
	
	dstr_destroy(dstr);
	TEST_PASS();
}

// 6. 测试 dstr_length
void test_dstr_length(void) {
	TEST_START("dstr_length");
	
	dstr_adt *dstr = dstr_create("Hello");
	size_t len = dstr_length(dstr);
	
	if (len == 5) {
		printf("  - 获取长度 (5): 通过\n");
	} else {
		TEST_FAIL("获取长度失败");
	}
	
	dstr_destroy(dstr);
	TEST_PASS();
}

// 7. 测试 dstr_capacity
void test_dstr_capacity(void) {
	TEST_START("dstr_capacity");
	
	dstr_adt *dstr = dstr_create("Test");
	size_t cap = dstr_capacity(dstr);
	
	if (cap >= 4) {  // 容量至少应该等于长度
		printf("  - 获取容量 (%zu): 通过\n", cap);
	} else {
		TEST_FAIL("获取容量失败");
	}
	
	dstr_destroy(dstr);
	TEST_PASS();
}

// 8. 测试 dstr_set_capacity
void test_dstr_set_capacity(void) {
	TEST_START("dstr_set_capacity");
	
	dstr_adt *dstr = dstr_create("Hello");
	int ret = dstr_set_capacity(dstr, 100);
	
	if (ret == DSTR_SUCCESS && dstr_capacity(dstr) >= 100) {
		printf("  - 设置容量到 100: 通过\n");
	} else {
		TEST_FAIL("设置容量失败");
	}
	
	dstr_destroy(dstr);
	TEST_PASS();
}

// 9. 测试 dstr_cpy_cstr
void test_dstr_cpy_cstr(void) {
	TEST_START("dstr_cpy_cstr");
	
	dstr_adt *dstr = dstr_create("Original");
	int ret = dstr_cpy_cstr(dstr, "New String");
	
	if (ret == DSTR_SUCCESS && strcmp(dstr_cstr_const(dstr), "New String") == 0) {
		printf("  - 复制 C 字符串: 通过\n");
	} else {
		TEST_FAIL("复制 C 字符串失败");
	}
	
	dstr_destroy(dstr);
	TEST_PASS();
}

// 10. 测试 dstr_cpy
void test_dstr_cpy(void) {
	TEST_START("dstr_cpy");
	
	dstr_adt *src = dstr_create("Source");
	dstr_adt *dest = dstr_create("Destination");
	int ret = dstr_cpy(dest, src);
	
	if (ret == DSTR_SUCCESS && strcmp(dstr_cstr_const(dest), "Source") == 0) {
		printf("  - 复制动态字符串: 通过\n");
	} else {
		TEST_FAIL("复制动态字符串失败");
	}
	
	dstr_destroy(src);
	dstr_destroy(dest);
	TEST_PASS();
}

// 11. 测试 dstr_cat_cstr
void test_dstr_cat_cstr(void) {
	TEST_START("dstr_cat_cstr");
	
	dstr_adt *dstr = dstr_create("Hello");
	int ret = dstr_cat_cstr(dstr, ", World!");
	
	if (ret == DSTR_SUCCESS && strcmp(dstr_cstr_const(dstr), "Hello, World!") == 0) {
		printf("  - 追加 C 字符串: 通过\n");
	} else {
		TEST_FAIL("追加 C 字符串失败");
	}
	
	dstr_destroy(dstr);
	TEST_PASS();
}

// 12. 测试 dstr_cat
void test_dstr_cat(void) {
	TEST_START("dstr_cat");
	
	dstr_adt *dstr1 = dstr_create("Hello");
	dstr_adt *dstr2 = dstr_create(", World!");
	int ret = dstr_cat(dstr1, dstr2);
	
	if (ret == DSTR_SUCCESS && strcmp(dstr_cstr_const(dstr1), "Hello, World!") == 0) {
		printf("  - 追加快态字符串: 通过\n");
	} else {
		TEST_FAIL("追加快态字符串失败");
	}
	
	dstr_destroy(dstr1);
	dstr_destroy(dstr2);
	TEST_PASS();
}

// 13. 测试 dstr_insert_cstr
void test_dstr_insert_cstr(void) {
	TEST_START("dstr_insert_cstr");
	
	dstr_adt *dstr = dstr_create("Hello World!");
	int ret = dstr_insert_cstr(dstr, 6, "Beautiful ");
	
	if (ret == DSTR_SUCCESS && strcmp(dstr_cstr_const(dstr), "Hello Beautiful World!") == 0) {
		printf("  - 插入 C 字符串: 通过\n");
	} else {
		TEST_FAIL("插入 C 字符串失败");
	}
	
	dstr_destroy(dstr);
	TEST_PASS();
}

// 14. 测试 dstr_insert
void test_dstr_insert(void) {
	TEST_START("dstr_insert");
	
	dstr_adt *dstr1 = dstr_create("Hello World!");
	dstr_adt *dstr2 = dstr_create("Beautiful ");
	int ret = dstr_insert(dstr1, 6, dstr2);
	
	if (ret == DSTR_SUCCESS && strcmp(dstr_cstr_const(dstr1), "Hello Beautiful World!") == 0) {
		printf("  - 插入动态字符串: 通过\n");
	} else {
		TEST_FAIL("插入动态字符串失败");
	}
	
	dstr_destroy(dstr1);
	dstr_destroy(dstr2);
	TEST_PASS();
}

// 15. 测试 dstr_cpy_sub_cstr
void test_dstr_cpy_sub_cstr(void) {
	TEST_START("dstr_cpy_sub_cstr");
	
	dstr_adt *dstr = dstr_create("Original");
	int ret = dstr_cpy_sub_cstr(dstr, "Hello, World!", 7, 5);
	
	if (ret == DSTR_SUCCESS && strcmp(dstr_cstr_const(dstr), "World") == 0) {
		printf("  - 复制 C 字符串子串: 通过\n");
	} else {
		TEST_FAIL("复制 C 字符串子串失败");
	}
	
	dstr_destroy(dstr);
	TEST_PASS();
}

// 16. 测试 dstr_cpy_sub
void test_dstr_cpy_sub(void) {
	TEST_START("dstr_cpy_sub");
	
	dstr_adt *src = dstr_create("Hello, World!");
	dstr_adt *dest = dstr_create("Original");
	int ret = dstr_cpy_sub(dest, src, 7, 5);
	
	if (ret == DSTR_SUCCESS && strcmp(dstr_cstr_const(dest), "World") == 0) {
		printf("  - 复制动态字符串子串: 通过\n");
	} else {
		TEST_FAIL("复制动态字符串子串失败");
	}
	
	dstr_destroy(src);
	dstr_destroy(dest);
	TEST_PASS();
}

// 17. 测试 dstr_cat_sub_cstr
void test_dstr_cat_sub_cstr(void) {
	TEST_START("dstr_cat_sub_cstr");
	
	dstr_adt *dstr = dstr_create("Hello");
	int ret = dstr_cat_sub_cstr(dstr, ", World! Welcome!", 0, 8);
	
	if (ret == DSTR_SUCCESS && strcmp(dstr_cstr_const(dstr), "Hello, World!") == 0) {
		printf("  - 追加 C 字符串子串: 通过\n");
	} else {
		TEST_FAIL("追加 C 字符串子串失败");
	}
	
	dstr_destroy(dstr);
	TEST_PASS();
}

// 18. 测试 dstr_cat_sub
void test_dstr_cat_sub(void) {
	TEST_START("dstr_cat_sub");
	
	dstr_adt *dstr1 = dstr_create("Hello");
	dstr_adt *dstr2 = dstr_create(", World! Welcome!");
	int ret = dstr_cat_sub(dstr1, dstr2, 0, 8);
	
	if (ret == DSTR_SUCCESS && strcmp(dstr_cstr_const(dstr1), "Hello, World!") == 0) {
		printf("  - 追加快态字符串子串: 通过\n");
	} else {
		TEST_FAIL("追加快态字符串子串失败");
	}
	
	dstr_destroy(dstr1);
	dstr_destroy(dstr2);
	TEST_PASS();
}

// 19. 测试 dstr_insert_sub_cstr
void test_dstr_insert_sub_cstr(void) {
	TEST_START("dstr_insert_sub_cstr");
	
	dstr_adt *dstr = dstr_create("Hello!");
	int ret = dstr_insert_sub_cstr(dstr, 5, ", World! Fine.", 0, 7);
	
	if (ret == DSTR_SUCCESS && strcmp(dstr_cstr_const(dstr), "Hello, World!") == 0) {
		printf("  - 插入 C 字符串子串: 通过\n");
	} else {
		TEST_FAIL("插入 C 字符串子串失败");
	}
	
	dstr_destroy(dstr);
	TEST_PASS();
}

// 20. 测试 dstr_insert_sub
void test_dstr_insert_sub(void) {
	TEST_START("dstr_insert_sub");
	
	dstr_adt *dstr1 = dstr_create("Hello!");
	dstr_adt *dstr2 = dstr_create(", World! Fine.");
	int ret = dstr_insert_sub(dstr1, 5, dstr2, 0, 7);
	
	if (ret == DSTR_SUCCESS && strcmp(dstr_cstr_const(dstr1), "Hello, World!") == 0) {
		printf("  - 插入动态字符串子串: 通过\n");
	} else {
		TEST_FAIL("插入动态字符串子串失败");
	}
	
	dstr_destroy(dstr1);
	dstr_destroy(dstr2);
	TEST_PASS();
}

// 21. 测试 dstr_remove
void test_dstr_remove(void) {
	TEST_START("dstr_remove");
	
	dstr_adt *dstr = dstr_create("Hello, Beautiful World!");
	dstr_remove(dstr, 6, 10);  // 删除 "Beautiful "
	
	if (strcmp(dstr_cstr_const(dstr), "Hello, World!") == 0) {
		printf("  - 删除子串: 通过\n");
	} else {
		TEST_FAIL("删除子串失败");
	}
	
	dstr_destroy(dstr);
	TEST_PASS();
}

// 22. 测试 dstr_trim
void test_dstr_trim(void) {
	TEST_START("dstr_trim");
	
	dstr_adt *dstr = dstr_create("   Hello, World!   ");
	dstr_trim(dstr, NULL);  // 去除空白字符
	
	if (strcmp(dstr_cstr_const(dstr), "Hello, World!") == 0) {
		printf("  - 去除首尾空白: 通过\n");
	} else {
		TEST_FAIL("去除首尾空白失败");
	}
	
	dstr_destroy(dstr);
	TEST_PASS();
}

// 23. 测试 dstr_printf
void test_dstr_printf(void) {
	TEST_START("dstr_printf");
	
	dstr_adt *dstr = dstr_create("");
	int ret = dstr_printf(dstr, "Hello, %s! Number: %d", "World", 42);
	
	if (ret == DSTR_SUCCESS && strcmp(dstr_cstr_const(dstr), "Hello, World! Number: 42") == 0) {
		printf("  - 格式化写入: 通过\n");
	} else {
		TEST_FAIL("格式化写入失败");
	}
	
	dstr_destroy(dstr);
	TEST_PASS();
}

// 24. 测试 dstr_sub_cstr
void test_dstr_sub_cstr(void) {
	TEST_START("dstr_sub_cstr");
	
	dstr_adt *sub = dstr_sub_cstr("Hello, World!", 7, 5);
	
	if (sub != NULL && strcmp(dstr_cstr_const(sub), "World") == 0) {
		printf("  - 从 C 字符串提取子串: 通过\n");
	} else {
		TEST_FAIL("从 C 字符串提取子串失败");
	}
	
	dstr_destroy(sub);
	TEST_PASS();
}

// 25. 测试 dstr_sub
void test_dstr_sub(void) {
	TEST_START("dstr_sub");
	
	dstr_adt *src = dstr_create("Hello, World!");
	dstr_adt *sub = dstr_sub(src, 7, 5);
	
	if (sub != NULL && strcmp(dstr_cstr_const(sub), "World") == 0) {
		printf("  - 从动态字符串提取子串: 通过\n");
	} else {
		TEST_FAIL("从动态字符串提取子串失败");
	}
	
	dstr_destroy(src);
	dstr_destroy(sub);
	TEST_PASS();
}

// 26. 测试 dstr_clone
void test_dstr_clone(void) {
	TEST_START("dstr_clone");
	
	dstr_adt *original = dstr_create("Hello, World!");
	dstr_adt *clone = dstr_clone(original);
	
	if (clone != NULL && strcmp(dstr_cstr_const(clone), "Hello, World!") == 0 && clone != original) {
		printf("  - 克隆动态字符串: 通过\n");
	} else {
		TEST_FAIL("克隆动态字符串失败");
	}
	
	dstr_destroy(original);
	dstr_destroy(clone);
	TEST_PASS();
}

// 27. 测试 dstr_find_cstr
void test_dstr_find_cstr(void) {
	TEST_START("dstr_find_cstr");
	
	dstr_adt *dstr = dstr_create("Hello, World!");
	size_t index;
	bool found = dstr_find_cstr(dstr, "World", &index, false);
	
	if (found && index == 7) {
		printf("  - 查找 C 字符串 (正向): 通过\n");
	} else {
		TEST_FAIL("查找 C 字符串失败");
	}
	
	dstr_destroy(dstr);
	TEST_PASS();
}

// 28. 测试 dstr_find
void test_dstr_find(void) {
	TEST_START("dstr_find");
	
	dstr_adt *dstr = dstr_create("Hello, World!");
	dstr_adt *sub = dstr_create("World");
	size_t index;
	bool found = dstr_find(dstr, sub, &index, false);
	
	if (found && index == 7) {
		printf("  - 查找动态字符串 (正向): 通过\n");
	} else {
		TEST_FAIL("查找动态字符串失败");
	}
	
	dstr_destroy(dstr);
	dstr_destroy(sub);
	TEST_PASS();
}

// 29. 测试 dstr_count_cstr
void test_dstr_count_cstr(void) {
	TEST_START("dstr_count_cstr");
	
	dstr_adt *dstr = dstr_create("abc abc abc");
	size_t count = dstr_count_cstr(dstr, "abc");
	
	if (count == 3) {
		printf("  - 统计 C 字符串出现次数: 通过\n");
	} else {
		TEST_FAIL("统计 C 字符串出现次数失败");
	}
	
	dstr_destroy(dstr);
	TEST_PASS();
}

// 30. 测试 dstr_count
void test_dstr_count(void) {
	TEST_START("dstr_count");
	
	dstr_adt *dstr = dstr_create("abc abc abc");
	dstr_adt *sub = dstr_create("abc");
	size_t count = dstr_count(dstr, sub);
	
	if (count == 3) {
		printf("  - 统计动态字符串出现次数: 通过\n");
	} else {
		TEST_FAIL("统计动态字符串出现次数失败");
	}
	
	dstr_destroy(dstr);
	dstr_destroy(sub);
	TEST_PASS();
}

// 31. 测试 dstr_find_nth_cstr
void test_dstr_find_nth_cstr(void) {
	TEST_START("dstr_find_nth_cstr");
	
	dstr_adt *dstr = dstr_create("abc def abc ghi abc");
	size_t index;
	bool found = dstr_find_nth_cstr(dstr, "abc", &index, 2, false);
	
	if (found && index == 8) {
		printf("  - 查找第 n 次出现的 C 字符串: 通过\n");
	} else {
		TEST_FAIL("查找第 n 次出现的 C 字符串失败");
	}
	
	dstr_destroy(dstr);
	TEST_PASS();
}

// 32. 测试 dstr_find_nth
void test_dstr_find_nth(void) {
	TEST_START("dstr_find_nth");
	
	dstr_adt *dstr = dstr_create("abc def abc ghi abc");
	dstr_adt *sub = dstr_create("abc");
	size_t index;
	bool found = dstr_find_nth(dstr, sub, &index, 2, false);
	
	if (found && index == 8) {
		printf("  - 查找第 n 次出现的动态字符串: 通过\n");
	} else {
		TEST_FAIL("查找第 n 次出现的动态字符串失败");
	}
	
	dstr_destroy(dstr);
	dstr_destroy(sub);
	TEST_PASS();
}

// 33. 测试 dstr_replace_cstr
void test_dstr_replace_cstr(void) {
	TEST_START("dstr_replace_cstr");
	
	dstr_adt *dstr = dstr_create("Hello World, Hello Everyone");
	int ret = dstr_replace_cstr(dstr, "Hello", "Hi", 0, false);  // 替换所有
	
	if (ret == DSTR_SUCCESS && strcmp(dstr_cstr_const(dstr), "Hi World, Hi Everyone") == 0) {
		printf("  - 替换 C 字符串: 通过\n");
	} else {
		TEST_FAIL("替换 C 字符串失败");
	}
	
	dstr_destroy(dstr);
	TEST_PASS();
}

// 34. 测试 dstr_replace
void test_dstr_replace(void) {
	TEST_START("dstr_replace");
	
	dstr_adt *dstr = dstr_create("Hello World, Hello Everyone");
	dstr_adt *old_str = dstr_create("Hello");
	dstr_adt *new_str = dstr_create("Hi");
	int ret = dstr_replace(dstr, old_str, new_str, 0, false);  // 替换所有

	if (ret == DSTR_SUCCESS && strcmp(dstr_cstr_const(dstr), "Hi World, Hi Everyone") == 0) {
		printf("  - 替换动态字符串: 通过\n");
	} else {
		TEST_FAIL("替换动态字符串失败");
	}
	
	dstr_destroy(dstr);
	dstr_destroy(old_str);
	dstr_destroy(new_str);
	TEST_PASS();
}

// 35. 测试 dstr_starts_with_cstr
void test_dstr_starts_with_cstr(void) {
	TEST_START("dstr_starts_with_cstr");
	
	dstr_adt *dstr = dstr_create("Hello, World!");
	bool result = dstr_starts_with_cstr(dstr, "Hello");
	
	if (result) {
		printf("  - 判断前缀 (C 字符串): 通过\n");
	} else {
		TEST_FAIL("判断前缀失败");
	}
	
	dstr_destroy(dstr);
	TEST_PASS();
}

// 36. 测试 dstr_starts_with
void test_dstr_starts_with(void) {
	TEST_START("dstr_starts_with");
	
	dstr_adt *dstr = dstr_create("Hello, World!");
	dstr_adt *prefix = dstr_create("Hello");
	bool result = dstr_starts_with(dstr, prefix);
	
	if (result) {
		printf("  - 判断前缀 (动态字符串): 通过\n");
	} else {
		TEST_FAIL("判断前缀失败");
	}
	
	dstr_destroy(dstr);
	dstr_destroy(prefix);
	TEST_PASS();
}

// 37. 测试 dstr_ends_with_cstr
void test_dstr_ends_with_cstr(void) {
	TEST_START("dstr_ends_with_cstr");
	
	dstr_adt *dstr = dstr_create("Hello, World!");
	bool result = dstr_ends_with_cstr(dstr, "World!");
	
	if (result) {
		printf("  - 判断后缀 (C 字符串): 通过\n");
	} else {
		TEST_FAIL("判断后缀失败");
	}
	
	dstr_destroy(dstr);
	TEST_PASS();
}

// 38. 测试 dstr_ends_with
void test_dstr_ends_with(void) {
	TEST_START("dstr_ends_with");
	
	dstr_adt *dstr = dstr_create("Hello, World!");
	dstr_adt *suffix = dstr_create("World!");
	bool result = dstr_ends_with(dstr, suffix);
	
	if (result) {
		printf("  - 判断后缀 (动态字符串): 通过\n");
	} else {
		TEST_FAIL("判断后缀失败");
	}
	
	dstr_destroy(dstr);
	dstr_destroy(suffix);
	TEST_PASS();
}

// 39. 测试 dstr_contains_cstr
void test_dstr_contains_cstr(void) {
	TEST_START("dstr_contains_cstr");
	
	dstr_adt *dstr = dstr_create("Hello, World!");
	bool result = dstr_contains_cstr(dstr, "World");
	
	if (result) {
		printf("  - 判断包含 (C 字符串): 通过\n");
	} else {
		TEST_FAIL("判断包含失败");
	}
	
	dstr_destroy(dstr);
	TEST_PASS();
}

// 40. 测试 dstr_contains
void test_dstr_contains(void) {
	TEST_START("dstr_contains");
	
	dstr_adt *dstr = dstr_create("Hello, World!");
	dstr_adt *sub = dstr_create("World");
	bool result = dstr_contains(dstr, sub);
	
	if (result) {
		printf("  - 判断包含 (动态字符串): 通过\n");
	} else {
		TEST_FAIL("判断包含失败");
	}
	
	dstr_destroy(dstr);
	dstr_destroy(sub);
	TEST_PASS();
}

// 41. 测试 dstr_equals_cstr
void test_dstr_equals_cstr(void) {
	TEST_START("dstr_equals_cstr");
	
	dstr_adt *dstr = dstr_create("Hello, World!");
	bool result = dstr_equals_cstr(dstr, "Hello, World!");
	
	if (result) {
		printf("  - 判断相等 (C 字符串): 通过\n");
	} else {
		TEST_FAIL("判断相等失败");
	}
	
	dstr_destroy(dstr);
	TEST_PASS();
}

// 42. 测试 dstr_equals
void test_dstr_equals(void) {
	TEST_START("dstr_equals");
	
	dstr_adt *dstr1 = dstr_create("Hello, World!");
	dstr_adt *dstr2 = dstr_create("Hello, World!");
	bool result = dstr_equals(dstr1, dstr2);
	
	if (result) {
		printf("  - 判断相等 (动态字符串): 通过\n");
	} else {
		TEST_FAIL("判断相等失败");
	}
	
	dstr_destroy(dstr1);
	dstr_destroy(dstr2);
	TEST_PASS();
}

// 43. 测试 dstr_compare_cstr
void test_dstr_compare_cstr(void) {
	TEST_START("dstr_compare_cstr");
	
	dstr_adt *dstr = dstr_create("ABC");
	int result = dstr_compare_cstr(dstr, "ABC");
	
	if (result == 0) {
		printf("  - 比较字符串 (C 字符串, 相等): 通过\n");
	} else {
		TEST_FAIL("比较字符串失败");
	}
	
	dstr_destroy(dstr);
	TEST_PASS();
}

// 44. 测试 dstr_compare
void test_dstr_compare(void) {
	TEST_START("dstr_compare");
	
	dstr_adt *dstr1 = dstr_create("ABC");
	dstr_adt *dstr2 = dstr_create("ABC");
	int result = dstr_compare(dstr1, dstr2);
	
	if (result == 0) {
		printf("  - 比较字符串 (动态字符串, 相等): 通过\n");
	} else {
		TEST_FAIL("比较字符串失败");
	}
	
	dstr_destroy(dstr1);
	dstr_destroy(dstr2);
	TEST_PASS();
}

int main(void) {
	printf("========================================\n");
	printf("   Dynamic String API 测试套件\n");
	printf("========================================\n");
	
	// 运行所有测试
	test_dstr_create();
	test_dstr_destroy();
	test_dstr_clear();
	test_dstr_cstr();
	test_dstr_cstr_const();
	test_dstr_length();
	test_dstr_capacity();
	test_dstr_set_capacity();
	test_dstr_cpy_cstr();
	test_dstr_cpy();
	test_dstr_cat_cstr();
	test_dstr_cat();
	test_dstr_insert_cstr();
	test_dstr_insert();
	test_dstr_cpy_sub_cstr();
	test_dstr_cpy_sub();
	test_dstr_cat_sub_cstr();
	test_dstr_cat_sub();
	test_dstr_insert_sub_cstr();
	test_dstr_insert_sub();
	test_dstr_remove();
	test_dstr_trim();
	test_dstr_printf();
	test_dstr_sub_cstr();
	test_dstr_sub();
	test_dstr_clone();
	test_dstr_find_cstr();
	test_dstr_find();
	test_dstr_count_cstr();
	test_dstr_count();
	test_dstr_find_nth_cstr();
	test_dstr_find_nth();
	test_dstr_replace_cstr();
	test_dstr_replace();
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
	
	printf("\n========================================\n");
	printf("   所有测试完成！\n");
	printf("========================================\n");
	
	return EXIT_SUCCESS;
}
