/*======================================================================================================================
 * tests/test_dynamic_string.c - dynamic_string 单元测试（Unity）
 *
 * 约定：
 *   - 每个公开 API 对应一个测试函数，在该函数内覆盖该 API 的各种输入与边界。
 *   - 内存分配失败视为环境问题，与逻辑错误同样计入失败（小规模用例下几乎不会触发）。
 *====================================================================================================================*/

#include "dynamic_string/dynamic_string.h"
#include "unity.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*----------------------------------------------------------------------------------------------------------------------
 * 辅助
 *--------------------------------------------------------------------------------------------------------------------*/

#define ASSERT_DSTR_EQ(dstr, cstr)                                                                                     \
    do                                                                                                                 \
    {                                                                                                                  \
        TEST_ASSERT_NOT_NULL(dstr);                                                                                    \
        TEST_ASSERT_EQUAL_STRING(cstr, dstr_cstr(dstr));                                                               \
        TEST_ASSERT_EQUAL_size_t(strlen(cstr), dstr_length(dstr));                                                     \
    } while (0)

#define ASSERT_OK(st) TEST_ASSERT_EQUAL_INT(DSTR_SUCCESS, (st))
#define ASSERT_INVALID(st) TEST_ASSERT_EQUAL_INT(DSTR_INVALID_ARGUMENT, (st))

static void free_dstr_array(dstr_adt **arr, size_t count)
{
    if (arr == NULL)
    {
        return;
    }
    for (size_t i = 0; i < count; ++i)
    {
        dstr_destroy(arr[i]);
    }
    free(arr);
}

static void free_cstr_array(char **arr, size_t count)
{
    if (arr == NULL)
    {
        return;
    }
    for (size_t i = 0; i < count; ++i)
    {
        free(arr[i]);
    }
    free(arr);
}

static dstr_adt *call_create_vformat(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    dstr_adt *r = dstr_create_vformat(fmt, ap);
    va_end(ap);
    return r;
}

static dstr_status_t call_cpy_vformat(dstr_adt *dest, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    dstr_status_t st = dstr_cpy_vformat(dest, fmt, ap);
    va_end(ap);
    return st;
}

static dstr_status_t call_cat_vformat(dstr_adt *dest, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    dstr_status_t st = dstr_cat_vformat(dest, fmt, ap);
    va_end(ap);
    return st;
}

static dstr_status_t call_insert_vformat(dstr_adt *dest, size_t index, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    dstr_status_t st = dstr_insert_vformat(dest, index, fmt, ap);
    va_end(ap);
    return st;
}

void setUp(void)
{
}
void tearDown(void)
{
}

/*----------------------------------------------------------------------------------------------------------------------
 * 创建与销毁
 *--------------------------------------------------------------------------------------------------------------------*/

void test_dstr_create(void)
{
    dstr_adt *s;

    s = dstr_create("hello");
    ASSERT_DSTR_EQ(s, "hello");
    TEST_ASSERT_FALSE(dstr_is_empty(s));
    dstr_destroy(s);

    s = dstr_create(NULL);
    ASSERT_DSTR_EQ(s, "");
    TEST_ASSERT_TRUE(dstr_is_empty(s));
    dstr_destroy(s);

    s = dstr_create("");
    ASSERT_DSTR_EQ(s, "");
    TEST_ASSERT_TRUE(dstr_is_empty(s));
    TEST_ASSERT_TRUE(dstr_capacity(s) > 0);
    dstr_destroy(s);
}

void test_dstr_destroy(void)
{
    dstr_destroy(NULL);

    dstr_adt *s = dstr_create("x");
    dstr_destroy(s);
}

void test_dstr_clone(void)
{
    dstr_adt *a = dstr_create("clone-me");
    dstr_adt *b = dstr_clone(a);
    ASSERT_DSTR_EQ(b, "clone-me");
    TEST_ASSERT_TRUE(dstr_cstr(a) != dstr_cstr(b));
    dstr_destroy(a);
    dstr_destroy(b);

    a = dstr_clone(NULL);
    ASSERT_DSTR_EQ(a, "");
    dstr_destroy(a);

    a = dstr_create("");
    b = dstr_clone(a);
    ASSERT_DSTR_EQ(b, "");
    dstr_destroy(a);
    dstr_destroy(b);
}

void test_dstr_sub_cstr(void)
{
    dstr_adt *s;

    s = dstr_sub_cstr("abcdef", 2, 3);
    ASSERT_DSTR_EQ(s, "cde");
    dstr_destroy(s);

    s = dstr_sub_cstr("abcdef", 3, 0);
    ASSERT_DSTR_EQ(s, "def");
    dstr_destroy(s);

    s = dstr_sub_cstr(NULL, 0, 1);
    ASSERT_DSTR_EQ(s, "");
    dstr_destroy(s);

    s = dstr_sub_cstr("", 0, 0);
    ASSERT_DSTR_EQ(s, "");
    dstr_destroy(s);

    TEST_ASSERT_NULL(dstr_sub_cstr("abc", 3, 1));
    TEST_ASSERT_NULL(dstr_sub_cstr("abc", 1, 5));
}

void test_dstr_sub(void)
{
    dstr_adt *src = dstr_create("hello world");
    dstr_adt *s;

    s = dstr_sub(src, 6, 5);
    ASSERT_DSTR_EQ(s, "world");
    dstr_destroy(s);

    s = dstr_sub(src, 0, 0);
    ASSERT_DSTR_EQ(s, "hello world");
    dstr_destroy(s);

    s = dstr_sub(NULL, 0, 1);
    ASSERT_DSTR_EQ(s, "");
    dstr_destroy(s);

    TEST_ASSERT_NULL(dstr_sub(src, 100, 1));
    TEST_ASSERT_NULL(dstr_sub(src, 2, 100));

    dstr_destroy(src);
}

void test_dstr_create_format(void)
{
    dstr_adt *s;

    s = dstr_create_format("num=%d str=%s", 42, "ok");
    ASSERT_DSTR_EQ(s, "num=42 str=ok");
    dstr_destroy(s);

    s = dstr_create_format(NULL);
    ASSERT_DSTR_EQ(s, "");
    dstr_destroy(s);

    s = dstr_create_format("");
    ASSERT_DSTR_EQ(s, "");
    dstr_destroy(s);
}

void test_dstr_create_vformat(void)
{
    dstr_adt *s;

    s = call_create_vformat("x=%d", 7);
    ASSERT_DSTR_EQ(s, "x=7");
    dstr_destroy(s);

    s = call_create_vformat(NULL);
    ASSERT_DSTR_EQ(s, "");
    dstr_destroy(s);

    s = call_create_vformat("");
    ASSERT_DSTR_EQ(s, "");
    dstr_destroy(s);
}

/*----------------------------------------------------------------------------------------------------------------------
 * 属性获取与设置
 *--------------------------------------------------------------------------------------------------------------------*/

void test_dstr_cstr(void)
{
    TEST_ASSERT_NULL(dstr_cstr(NULL));

    dstr_adt *s = dstr_create("abc");
    TEST_ASSERT_EQUAL_STRING("abc", dstr_cstr(s));

    dstr_clear(s);
    TEST_ASSERT_EQUAL_STRING("", dstr_cstr(s));
    dstr_destroy(s);
}

void test_dstr_length(void)
{
    TEST_ASSERT_EQUAL_size_t(0, dstr_length(NULL));

    dstr_adt *s = dstr_create("abcd");
    TEST_ASSERT_EQUAL_size_t(4, dstr_length(s));
    dstr_clear(s);
    TEST_ASSERT_EQUAL_size_t(0, dstr_length(s));
    dstr_destroy(s);
}

void test_dstr_is_empty(void)
{
    TEST_ASSERT_TRUE(dstr_is_empty(NULL));

    dstr_adt *s = dstr_create("");
    TEST_ASSERT_TRUE(dstr_is_empty(s));
    dstr_cpy_cstr(s, "x");
    TEST_ASSERT_FALSE(dstr_is_empty(s));
    dstr_destroy(s);
}

void test_dstr_capacity(void)
{
    TEST_ASSERT_EQUAL_size_t(0, dstr_capacity(NULL));

    dstr_adt *s = dstr_create("ab");
    TEST_ASSERT_TRUE(dstr_capacity(s) >= 3);
    dstr_destroy(s);
}

void test_dstr_set_capacity(void)
{
    dstr_adt *s = dstr_create("hello");

    ASSERT_OK(dstr_set_capacity(s, 64));
    TEST_ASSERT_TRUE(dstr_capacity(s) >= 64);
    ASSERT_DSTR_EQ(s, "hello");

    ASSERT_OK(dstr_set_capacity(s, 64));
    ASSERT_OK(dstr_set_capacity(s, 3));
    {
        size_t cap = dstr_capacity(s);
        TEST_ASSERT_TRUE(cap >= 3);
        if (cap <= 5)
        {
            TEST_ASSERT_EQUAL_size_t(cap - 1, dstr_length(s));
        }
    }

    ASSERT_INVALID(dstr_set_capacity(NULL, 10));

    dstr_clear(s);
    ASSERT_OK(dstr_set_capacity(s, 0));
    TEST_ASSERT_TRUE(dstr_capacity(s) > 0);

    dstr_destroy(s);
}

void test_dstr_shrink_to_fit(void)
{
    dstr_adt *s = dstr_create("abc");
    ASSERT_OK(dstr_set_capacity(s, 128));
    TEST_ASSERT_TRUE(dstr_capacity(s) >= 128);

    dstr_shrink_to_fit(s);
    TEST_ASSERT_TRUE(dstr_capacity(s) >= 4);
    ASSERT_DSTR_EQ(s, "abc");

    dstr_shrink_to_fit(NULL);
    dstr_destroy(s);
}

/*----------------------------------------------------------------------------------------------------------------------
 * 内容编辑
 *--------------------------------------------------------------------------------------------------------------------*/

void test_dstr_cpy_cstr(void)
{
    dstr_adt *s = dstr_create("old");

    ASSERT_OK(dstr_cpy_cstr(s, "new value"));
    ASSERT_DSTR_EQ(s, "new value");

    ASSERT_OK(dstr_cpy_cstr(s, NULL));
    ASSERT_DSTR_EQ(s, "");

    ASSERT_OK(dstr_cpy_cstr(s, ""));
    ASSERT_DSTR_EQ(s, "");

    ASSERT_INVALID(dstr_cpy_cstr(NULL, "x"));
    dstr_destroy(s);
}

void test_dstr_cpy(void)
{
    dstr_adt *a = dstr_create("source");
    dstr_adt *b = dstr_create("dest");

    ASSERT_OK(dstr_cpy(b, a));
    ASSERT_DSTR_EQ(b, "source");

    ASSERT_OK(dstr_cpy(b, NULL));
    ASSERT_DSTR_EQ(b, "");

    ASSERT_INVALID(dstr_cpy(NULL, a));
    dstr_destroy(a);
    dstr_destroy(b);
}

void test_dstr_cpy_sub_cstr(void)
{
    dstr_adt *s = dstr_create("");

    ASSERT_OK(dstr_cpy_sub_cstr(s, "abcdef", 1, 3));
    ASSERT_DSTR_EQ(s, "bcd");

    ASSERT_OK(dstr_cpy_sub_cstr(s, "abcdef", 2, 0));
    ASSERT_DSTR_EQ(s, "cdef");

    ASSERT_OK(dstr_cpy_sub_cstr(s, NULL, 0, 1));
    ASSERT_DSTR_EQ(s, "");

    ASSERT_OK(dstr_cpy_sub_cstr(s, "", 0, 0));
    ASSERT_DSTR_EQ(s, "");

    ASSERT_INVALID(dstr_cpy_sub_cstr(s, "abc", 5, 1));
    ASSERT_INVALID(dstr_cpy_sub_cstr(NULL, "a", 0, 1));
    dstr_destroy(s);
}

void test_dstr_cpy_sub(void)
{
    dstr_adt *src = dstr_create("abcdef");
    dstr_adt *dst = dstr_create("old");

    ASSERT_OK(dstr_cpy_sub(dst, src, 1, 3));
    ASSERT_DSTR_EQ(dst, "bcd");

    ASSERT_OK(dstr_cpy_sub(dst, src, 3, 0));
    ASSERT_DSTR_EQ(dst, "def");

    ASSERT_OK(dstr_cpy_sub(dst, NULL, 0, 1));
    ASSERT_DSTR_EQ(dst, "");

    ASSERT_INVALID(dstr_cpy_sub(dst, src, 10, 1));
    ASSERT_INVALID(dstr_cpy_sub(NULL, src, 0, 1));
    dstr_destroy(src);
    dstr_destroy(dst);
}

void test_dstr_cpy_format(void)
{
    dstr_adt *s = dstr_create("x");

    ASSERT_OK(dstr_cpy_format(s, "%d-%s", 1, "a"));
    ASSERT_DSTR_EQ(s, "1-a");

    ASSERT_OK(dstr_cpy_format(s, NULL));
    ASSERT_DSTR_EQ(s, "");

    ASSERT_OK(dstr_cpy_format(s, ""));
    ASSERT_DSTR_EQ(s, "");

    ASSERT_INVALID(dstr_cpy_format(NULL, "%d", 1));
    dstr_destroy(s);
}

void test_dstr_cpy_vformat(void)
{
    dstr_adt *s = dstr_create("x");

    ASSERT_OK(call_cpy_vformat(s, "%s=%d", "n", 3));
    ASSERT_DSTR_EQ(s, "n=3");

    ASSERT_OK(call_cpy_vformat(s, NULL));
    ASSERT_DSTR_EQ(s, "");

    ASSERT_INVALID(call_cpy_vformat(NULL, "%d", 1));
    dstr_destroy(s);
}

void test_dstr_cat_cstr(void)
{
    dstr_adt *s = dstr_create("hello");

    ASSERT_OK(dstr_cat_cstr(s, " world"));
    ASSERT_DSTR_EQ(s, "hello world");

    ASSERT_OK(dstr_cat_cstr(s, NULL));
    ASSERT_DSTR_EQ(s, "hello world");

    ASSERT_OK(dstr_cat_cstr(s, ""));
    ASSERT_DSTR_EQ(s, "hello world");

    ASSERT_INVALID(dstr_cat_cstr(NULL, "x"));
    dstr_destroy(s);
}

void test_dstr_cat(void)
{
    dstr_adt *a = dstr_create("foo");
    dstr_adt *b = dstr_create("bar");

    ASSERT_OK(dstr_cat(a, b));
    ASSERT_DSTR_EQ(a, "foobar");

    ASSERT_OK(dstr_cat(a, NULL));
    ASSERT_DSTR_EQ(a, "foobar");

    ASSERT_INVALID(dstr_cat(NULL, b));
    dstr_destroy(a);
    dstr_destroy(b);
}

void test_dstr_cat_sub_cstr(void)
{
    dstr_adt *s = dstr_create("X");

    ASSERT_OK(dstr_cat_sub_cstr(s, "abcdef", 2, 2));
    ASSERT_DSTR_EQ(s, "Xcd");

    ASSERT_OK(dstr_cat_sub_cstr(s, NULL, 0, 1));
    ASSERT_DSTR_EQ(s, "Xcd");

    ASSERT_INVALID(dstr_cat_sub_cstr(s, "ab", 5, 1));
    ASSERT_INVALID(dstr_cat_sub_cstr(NULL, "a", 0, 1));
    dstr_destroy(s);
}

void test_dstr_cat_sub(void)
{
    dstr_adt *s = dstr_create("X");
    dstr_adt *src = dstr_create("abcdef");

    ASSERT_OK(dstr_cat_sub(s, src, 1, 2));
    ASSERT_DSTR_EQ(s, "Xbc");

    ASSERT_OK(dstr_cat_sub(s, NULL, 0, 1));
    ASSERT_DSTR_EQ(s, "Xbc");

    ASSERT_INVALID(dstr_cat_sub(s, src, 10, 1));
    ASSERT_INVALID(dstr_cat_sub(NULL, src, 0, 1));
    dstr_destroy(s);
    dstr_destroy(src);
}

void test_dstr_cat_format(void)
{
    dstr_adt *s = dstr_create("val=");

    ASSERT_OK(dstr_cat_format(s, "%d", 99));
    ASSERT_DSTR_EQ(s, "val=99");

    ASSERT_OK(dstr_cat_format(s, NULL));
    ASSERT_DSTR_EQ(s, "val=99");

    ASSERT_INVALID(dstr_cat_format(NULL, "%d", 1));
    dstr_destroy(s);
}

void test_dstr_cat_vformat(void)
{
    dstr_adt *s = dstr_create("p=");

    ASSERT_OK(call_cat_vformat(s, "%d", 5));
    ASSERT_DSTR_EQ(s, "p=5");

    ASSERT_OK(call_cat_vformat(s, NULL));
    ASSERT_DSTR_EQ(s, "p=5");

    ASSERT_INVALID(call_cat_vformat(NULL, "%d", 1));
    dstr_destroy(s);
}

void test_dstr_insert_cstr(void)
{
    dstr_adt *s = dstr_create("ace");

    ASSERT_OK(dstr_insert_cstr(s, 1, "b"));
    ASSERT_DSTR_EQ(s, "abce");

    ASSERT_OK(dstr_insert_cstr(s, 0, "X"));
    ASSERT_DSTR_EQ(s, "Xabce");

    ASSERT_OK(dstr_insert_cstr(s, dstr_length(s), "Z"));
    ASSERT_DSTR_EQ(s, "XabceZ");

    ASSERT_OK(dstr_insert_cstr(s, 1, NULL));
    ASSERT_DSTR_EQ(s, "XabceZ");

    ASSERT_OK(dstr_insert_cstr(s, 1, ""));
    ASSERT_DSTR_EQ(s, "XabceZ");

    ASSERT_INVALID(dstr_insert_cstr(s, 100, "no"));
    ASSERT_INVALID(dstr_insert_cstr(NULL, 0, "x"));
    dstr_destroy(s);
}

void test_dstr_insert(void)
{
    dstr_adt *s = dstr_create("ac");
    dstr_adt *mid = dstr_create("b");

    ASSERT_OK(dstr_insert(s, 1, mid));
    ASSERT_DSTR_EQ(s, "abc");

    ASSERT_OK(dstr_insert(s, 0, NULL));
    ASSERT_DSTR_EQ(s, "abc");

    ASSERT_INVALID(dstr_insert(s, 100, mid));
    ASSERT_INVALID(dstr_insert(NULL, 0, mid));
    dstr_destroy(s);
    dstr_destroy(mid);
}

void test_dstr_insert_sub_cstr(void)
{
    dstr_adt *s = dstr_create("ae");

    ASSERT_OK(dstr_insert_sub_cstr(s, 1, "bcdxy", 0, 3));
    ASSERT_DSTR_EQ(s, "abcde");

    ASSERT_OK(dstr_insert_sub_cstr(s, 0, NULL, 0, 1));
    ASSERT_DSTR_EQ(s, "abcde");

    ASSERT_INVALID(dstr_insert_sub_cstr(s, 1, "ab", 5, 1));
    ASSERT_INVALID(dstr_insert_sub_cstr(s, 100, "ab", 0, 1));
    ASSERT_INVALID(dstr_insert_sub_cstr(NULL, 0, "a", 0, 1));
    dstr_destroy(s);
}

void test_dstr_insert_sub(void)
{
    dstr_adt *s = dstr_create("ae");
    dstr_adt *src = dstr_create("bcdxy");

    ASSERT_OK(dstr_insert_sub(s, 1, src, 0, 3));
    ASSERT_DSTR_EQ(s, "abcde");

    ASSERT_OK(dstr_insert_sub(s, 0, NULL, 0, 1));
    ASSERT_DSTR_EQ(s, "abcde");

    ASSERT_INVALID(dstr_insert_sub(s, 1, src, 10, 1));
    ASSERT_INVALID(dstr_insert_sub(NULL, 0, src, 0, 1));
    dstr_destroy(s);
    dstr_destroy(src);
}

void test_dstr_insert_format(void)
{
    dstr_adt *s = dstr_create("a_c");

    ASSERT_OK(dstr_insert_format(s, 2, "%d", 2));
    ASSERT_DSTR_EQ(s, "a_2c");

    ASSERT_OK(dstr_insert_format(s, 0, NULL));
    ASSERT_DSTR_EQ(s, "a_2c");

    ASSERT_INVALID(dstr_insert_format(s, 100, "%d", 1));
    ASSERT_INVALID(dstr_insert_format(NULL, 0, "%d", 1));
    dstr_destroy(s);
}

void test_dstr_insert_vformat(void)
{
    dstr_adt *s = dstr_create("A_C");

    ASSERT_OK(call_insert_vformat(s, 2, "%s", "B"));
    ASSERT_DSTR_EQ(s, "A_BC");

    ASSERT_OK(call_insert_vformat(s, 0, NULL));
    ASSERT_DSTR_EQ(s, "A_BC");

    ASSERT_INVALID(call_insert_vformat(s, 100, "%d", 1));
    ASSERT_INVALID(call_insert_vformat(NULL, 0, "%d", 1));
    dstr_destroy(s);
}

void test_dstr_clear(void)
{
    dstr_adt *s = dstr_create("content");
    size_t cap = dstr_capacity(s);

    dstr_clear(s);
    ASSERT_DSTR_EQ(s, "");
    TEST_ASSERT_EQUAL_size_t(cap, dstr_capacity(s));

    dstr_clear(NULL);
    dstr_destroy(s);
}

void test_dstr_remove(void)
{
    dstr_adt *s = dstr_create("abcdef");

    dstr_remove(s, 2, 2);
    ASSERT_DSTR_EQ(s, "abef");

    dstr_remove(s, 2, 0);
    ASSERT_DSTR_EQ(s, "ab");

    dstr_remove(s, 10, 1);
    ASSERT_DSTR_EQ(s, "ab");

    dstr_remove(NULL, 0, 1);
    dstr_remove(s, 0, 0);
    ASSERT_DSTR_EQ(s, "");
    dstr_destroy(s);
}

void test_dstr_trim(void)
{
    dstr_adt *s = dstr_create("  \thello  \n");

    dstr_trim(s, NULL);
    ASSERT_DSTR_EQ(s, "hello");

    ASSERT_OK(dstr_cpy_cstr(s, "   "));
    dstr_trim(s, NULL);
    ASSERT_DSTR_EQ(s, "");

    ASSERT_OK(dstr_cpy_cstr(s, "xxhelloyy"));
    dstr_trim(s, "xy");
    ASSERT_DSTR_EQ(s, "hello");

    ASSERT_OK(dstr_cpy_cstr(s, "--ok--"));
    dstr_trim(s, "-");
    ASSERT_DSTR_EQ(s, "ok");

    ASSERT_OK(dstr_cpy_cstr(s, "abc"));
    dstr_trim(s, "");
    ASSERT_DSTR_EQ(s, "abc");

    dstr_trim(NULL, NULL);
    dstr_destroy(s);
}

/*----------------------------------------------------------------------------------------------------------------------
 * 关系判断与比较
 *--------------------------------------------------------------------------------------------------------------------*/

void test_dstr_starts_with_cstr(void)
{
    dstr_adt *s = dstr_create("hello world");

    TEST_ASSERT_TRUE(dstr_starts_with_cstr(s, "hello"));
    TEST_ASSERT_FALSE(dstr_starts_with_cstr(s, "world"));
    TEST_ASSERT_FALSE(dstr_starts_with_cstr(s, ""));
    TEST_ASSERT_FALSE(dstr_starts_with_cstr(s, NULL));
    TEST_ASSERT_FALSE(dstr_starts_with_cstr(NULL, "h"));
    TEST_ASSERT_FALSE(dstr_starts_with_cstr(s, "hello world!"));
    dstr_destroy(s);
}

void test_dstr_starts_with(void)
{
    dstr_adt *s = dstr_create("hello world");
    dstr_adt *p = dstr_create("hello");
    dstr_adt *q = dstr_create("world");

    TEST_ASSERT_TRUE(dstr_starts_with(s, p));
    TEST_ASSERT_FALSE(dstr_starts_with(s, q));
    TEST_ASSERT_FALSE(dstr_starts_with(s, NULL));
    TEST_ASSERT_FALSE(dstr_starts_with(NULL, p));

    dstr_adt *empty = dstr_create("");
    TEST_ASSERT_FALSE(dstr_starts_with(s, empty));
    dstr_destroy(s);
    dstr_destroy(p);
    dstr_destroy(q);
    dstr_destroy(empty);
}

void test_cstr_starts_with(void)
{
    TEST_ASSERT_TRUE(cstr_starts_with("abc", "ab"));
    TEST_ASSERT_FALSE(cstr_starts_with("abc", "bc"));
    TEST_ASSERT_FALSE(cstr_starts_with(NULL, "a"));
    TEST_ASSERT_FALSE(cstr_starts_with("abc", NULL));
    TEST_ASSERT_FALSE(cstr_starts_with("abc", ""));
    TEST_ASSERT_FALSE(cstr_starts_with("", "a"));
}

void test_dstr_ends_with_cstr(void)
{
    dstr_adt *s = dstr_create("hello world");

    TEST_ASSERT_TRUE(dstr_ends_with_cstr(s, "world"));
    TEST_ASSERT_FALSE(dstr_ends_with_cstr(s, "hello"));
    TEST_ASSERT_FALSE(dstr_ends_with_cstr(s, ""));
    TEST_ASSERT_FALSE(dstr_ends_with_cstr(s, NULL));
    TEST_ASSERT_FALSE(dstr_ends_with_cstr(NULL, "d"));
    dstr_destroy(s);
}

void test_dstr_ends_with(void)
{
    dstr_adt *s = dstr_create("hello world");
    dstr_adt *suf = dstr_create("world");
    dstr_adt *bad = dstr_create("hello");

    TEST_ASSERT_TRUE(dstr_ends_with(s, suf));
    TEST_ASSERT_FALSE(dstr_ends_with(s, bad));
    TEST_ASSERT_FALSE(dstr_ends_with(s, NULL));
    TEST_ASSERT_FALSE(dstr_ends_with(NULL, suf));
    dstr_destroy(s);
    dstr_destroy(suf);
    dstr_destroy(bad);
}

void test_cstr_ends_with(void)
{
    TEST_ASSERT_TRUE(cstr_ends_with("abc", "bc"));
    TEST_ASSERT_FALSE(cstr_ends_with("abc", "ab"));
    TEST_ASSERT_FALSE(cstr_ends_with(NULL, "c"));
    TEST_ASSERT_FALSE(cstr_ends_with("abc", NULL));
    TEST_ASSERT_FALSE(cstr_ends_with("abc", ""));
}

void test_dstr_contains_cstr(void)
{
    dstr_adt *s = dstr_create("hello world");

    TEST_ASSERT_TRUE(dstr_contains_cstr(s, "lo wo"));
    TEST_ASSERT_FALSE(dstr_contains_cstr(s, "xyz"));
    TEST_ASSERT_TRUE(dstr_contains_cstr(s, ""));
    TEST_ASSERT_TRUE(dstr_contains_cstr(s, NULL));
    TEST_ASSERT_TRUE(dstr_contains_cstr(NULL, ""));
    TEST_ASSERT_FALSE(dstr_contains_cstr(NULL, "a"));
    dstr_destroy(s);
}

void test_dstr_contains(void)
{
    dstr_adt *s = dstr_create("hello world");
    dstr_adt *sub = dstr_create("world");
    dstr_adt *miss = dstr_create("xyz");
    dstr_adt *empty = dstr_create("");

    TEST_ASSERT_TRUE(dstr_contains(s, sub));
    TEST_ASSERT_FALSE(dstr_contains(s, miss));
    TEST_ASSERT_TRUE(dstr_contains(s, empty));
    TEST_ASSERT_TRUE(dstr_contains(s, NULL));
    TEST_ASSERT_FALSE(dstr_contains(NULL, sub));
    dstr_destroy(s);
    dstr_destroy(sub);
    dstr_destroy(miss);
    dstr_destroy(empty);
}

void test_cstr_contains(void)
{
    TEST_ASSERT_TRUE(cstr_contains("abc", "b"));
    TEST_ASSERT_FALSE(cstr_contains("abc", "x"));
    TEST_ASSERT_TRUE(cstr_contains("abc", ""));
    TEST_ASSERT_TRUE(cstr_contains("abc", NULL));
    TEST_ASSERT_FALSE(cstr_contains(NULL, "a"));
    TEST_ASSERT_TRUE(cstr_contains(NULL, ""));
}

void test_dstr_equals_cstr(void)
{
    dstr_adt *a = dstr_create("same");

    TEST_ASSERT_TRUE(dstr_equals_cstr(a, "same"));
    TEST_ASSERT_FALSE(dstr_equals_cstr(a, "diff"));
    TEST_ASSERT_TRUE(dstr_equals_cstr(NULL, NULL));
    TEST_ASSERT_TRUE(dstr_equals_cstr(NULL, ""));
    TEST_ASSERT_FALSE(dstr_equals_cstr(a, NULL));
    TEST_ASSERT_FALSE(dstr_equals_cstr(NULL, "x"));
    dstr_destroy(a);
}

void test_dstr_equals(void)
{
    dstr_adt *a = dstr_create("same");
    dstr_adt *b = dstr_create("same");
    dstr_adt *c = dstr_create("diff");

    TEST_ASSERT_TRUE(dstr_equals(a, b));
    TEST_ASSERT_FALSE(dstr_equals(a, c));
    TEST_ASSERT_TRUE(dstr_equals(NULL, NULL));
    TEST_ASSERT_FALSE(dstr_equals(a, NULL));
    TEST_ASSERT_FALSE(dstr_equals(NULL, a));
    dstr_destroy(a);
    dstr_destroy(b);
    dstr_destroy(c);
}

void test_cstr_equals(void)
{
    TEST_ASSERT_TRUE(cstr_equals("x", "x"));
    TEST_ASSERT_FALSE(cstr_equals("x", "y"));
    TEST_ASSERT_TRUE(cstr_equals(NULL, NULL));
    TEST_ASSERT_TRUE(cstr_equals(NULL, ""));
    TEST_ASSERT_FALSE(cstr_equals("x", NULL));
}

void test_dstr_compare_cstr(void)
{
    dstr_adt *a = dstr_create("abc");

    TEST_ASSERT_EQUAL_INT(0, dstr_compare_cstr(a, "abc"));
    TEST_ASSERT_TRUE(dstr_compare_cstr(a, "abd") < 0);
    TEST_ASSERT_TRUE(dstr_compare_cstr(a, "abb") > 0);
    TEST_ASSERT_TRUE(dstr_compare_cstr(NULL, "x") < 0);
    TEST_ASSERT_TRUE(dstr_compare_cstr(a, NULL) > 0);
    TEST_ASSERT_EQUAL_INT(0, dstr_compare_cstr(NULL, NULL));
    dstr_destroy(a);
}

void test_dstr_compare(void)
{
    dstr_adt *a = dstr_create("abc");
    dstr_adt *b = dstr_create("abd");
    dstr_adt *c = dstr_create("abc");

    TEST_ASSERT_EQUAL_INT(0, dstr_compare(a, c));
    TEST_ASSERT_TRUE(dstr_compare(a, b) < 0);
    TEST_ASSERT_TRUE(dstr_compare(b, a) > 0);
    TEST_ASSERT_TRUE(dstr_compare(NULL, a) < 0);
    TEST_ASSERT_TRUE(dstr_compare(a, NULL) > 0);
    TEST_ASSERT_EQUAL_INT(0, dstr_compare(NULL, NULL));
    dstr_destroy(a);
    dstr_destroy(b);
    dstr_destroy(c);
}

void test_cstr_compare(void)
{
    TEST_ASSERT_EQUAL_INT(0, cstr_compare("x", "x"));
    TEST_ASSERT_TRUE(cstr_compare("a", "b") < 0);
    TEST_ASSERT_TRUE(cstr_compare("b", "a") > 0);
    TEST_ASSERT_TRUE(cstr_compare(NULL, "x") < 0);
    TEST_ASSERT_TRUE(cstr_compare("x", NULL) > 0);
    TEST_ASSERT_EQUAL_INT(0, cstr_compare(NULL, NULL));
}

/*----------------------------------------------------------------------------------------------------------------------
 * 查找、统计与替换
 *--------------------------------------------------------------------------------------------------------------------*/

void test_dstr_find_cstr(void)
{
    dstr_adt *s = dstr_create("ababa");
    size_t idx = 999;

    TEST_ASSERT_TRUE(dstr_find_cstr(s, "aba", &idx, DSTR_DIR_FORWARD));
    TEST_ASSERT_EQUAL_size_t(0, idx);

    TEST_ASSERT_TRUE(dstr_find_cstr(s, "aba", &idx, DSTR_DIR_BACKWARD));
    TEST_ASSERT_EQUAL_size_t(2, idx);

    TEST_ASSERT_FALSE(dstr_find_cstr(s, "xyz", &idx, DSTR_DIR_FORWARD));
    TEST_ASSERT_FALSE(dstr_find_cstr(s, "", &idx, DSTR_DIR_FORWARD));
    TEST_ASSERT_FALSE(dstr_find_cstr(s, NULL, &idx, DSTR_DIR_FORWARD));
    TEST_ASSERT_FALSE(dstr_find_cstr(NULL, "a", &idx, DSTR_DIR_FORWARD));
    TEST_ASSERT_TRUE(dstr_find_cstr(s, "aba", NULL, DSTR_DIR_FORWARD));
    dstr_destroy(s);
}

void test_dstr_find(void)
{
    dstr_adt *s = dstr_create("one two one");
    dstr_adt *sub = dstr_create("one");
    dstr_adt *miss = dstr_create("zzz");
    size_t idx = 0;

    TEST_ASSERT_TRUE(dstr_find(s, sub, &idx, DSTR_DIR_FORWARD));
    TEST_ASSERT_EQUAL_size_t(0, idx);

    TEST_ASSERT_TRUE(dstr_find(s, sub, &idx, DSTR_DIR_BACKWARD));
    TEST_ASSERT_EQUAL_size_t(8, idx);

    TEST_ASSERT_FALSE(dstr_find(s, miss, &idx, DSTR_DIR_FORWARD));
    TEST_ASSERT_FALSE(dstr_find(s, NULL, &idx, DSTR_DIR_FORWARD));
    TEST_ASSERT_FALSE(dstr_find(NULL, sub, &idx, DSTR_DIR_FORWARD));
    dstr_destroy(s);
    dstr_destroy(sub);
    dstr_destroy(miss);
}

void test_cstr_find(void)
{
    size_t idx = 0;

    TEST_ASSERT_TRUE(cstr_find("abcabc", "bc", &idx, DSTR_DIR_FORWARD));
    TEST_ASSERT_EQUAL_size_t(1, idx);

    TEST_ASSERT_TRUE(cstr_find("abcabc", "bc", &idx, DSTR_DIR_BACKWARD));
    TEST_ASSERT_EQUAL_size_t(4, idx);

    TEST_ASSERT_FALSE(cstr_find("abc", "x", &idx, DSTR_DIR_FORWARD));
    TEST_ASSERT_FALSE(cstr_find(NULL, "a", &idx, DSTR_DIR_FORWARD));
    TEST_ASSERT_FALSE(cstr_find("abc", "", &idx, DSTR_DIR_FORWARD));
}

void test_dstr_find_nth_cstr(void)
{
    dstr_adt *s = dstr_create("aXaXaX");
    size_t idx = 0;

    TEST_ASSERT_TRUE(dstr_find_nth_cstr(s, "aX", &idx, DSTR_DIR_FORWARD, 1));
    TEST_ASSERT_EQUAL_size_t(0, idx);
    TEST_ASSERT_TRUE(dstr_find_nth_cstr(s, "aX", &idx, DSTR_DIR_FORWARD, 2));
    TEST_ASSERT_EQUAL_size_t(2, idx);

    TEST_ASSERT_TRUE(dstr_find_nth_cstr(s, "aX", &idx, DSTR_DIR_FORWARD, 0));
    TEST_ASSERT_EQUAL_size_t(4, idx);

    TEST_ASSERT_TRUE(dstr_find_nth_cstr(s, "aX", &idx, DSTR_DIR_BACKWARD, 1));
    TEST_ASSERT_EQUAL_size_t(4, idx);

    TEST_ASSERT_FALSE(dstr_find_nth_cstr(s, "", &idx, DSTR_DIR_FORWARD, 1));
    TEST_ASSERT_FALSE(dstr_find_nth_cstr(NULL, "aX", &idx, DSTR_DIR_FORWARD, 1));
    dstr_destroy(s);
}

void test_dstr_find_nth(void)
{
    dstr_adt *s = dstr_create("ab-ab-ab");
    dstr_adt *sub = dstr_create("ab");
    size_t idx = 0;

    TEST_ASSERT_TRUE(dstr_find_nth(s, sub, &idx, DSTR_DIR_FORWARD, 2));
    TEST_ASSERT_EQUAL_size_t(3, idx);

    TEST_ASSERT_TRUE(dstr_find_nth(s, sub, &idx, DSTR_DIR_FORWARD, 0));
    TEST_ASSERT_EQUAL_size_t(6, idx);

    TEST_ASSERT_FALSE(dstr_find_nth(s, NULL, &idx, DSTR_DIR_FORWARD, 1));
    dstr_destroy(s);
    dstr_destroy(sub);
}

void test_cstr_find_nth(void)
{
    size_t idx = 0;

    TEST_ASSERT_TRUE(cstr_find_nth("x-x-x", "x", &idx, DSTR_DIR_FORWARD, 2));
    TEST_ASSERT_EQUAL_size_t(2, idx);

    TEST_ASSERT_TRUE(cstr_find_nth("x-x-x", "x", &idx, DSTR_DIR_BACKWARD, 1));
    TEST_ASSERT_EQUAL_size_t(4, idx);

    TEST_ASSERT_FALSE(cstr_find_nth(NULL, "x", &idx, DSTR_DIR_FORWARD, 1));
    TEST_ASSERT_FALSE(cstr_find_nth("x", "", &idx, DSTR_DIR_FORWARD, 1));
}

void test_dstr_find_indexes_cstr(void)
{
    dstr_adt *s = dstr_create("aa-aa-aa");
    size_t indexes[8] = {0};
    size_t n;

    n = dstr_find_indexes_cstr(s, "aa", indexes, DSTR_DIR_FORWARD, 0);
    TEST_ASSERT_EQUAL_size_t(3, n);
    TEST_ASSERT_EQUAL_size_t(0, indexes[0]);
    TEST_ASSERT_EQUAL_size_t(3, indexes[1]);
    TEST_ASSERT_EQUAL_size_t(6, indexes[2]);

    n = dstr_find_indexes_cstr(s, "aa", indexes, DSTR_DIR_FORWARD, 2);
    TEST_ASSERT_EQUAL_size_t(2, n);

    TEST_ASSERT_EQUAL_size_t(0, dstr_find_indexes_cstr(s, "", indexes, DSTR_DIR_FORWARD, 0));
    TEST_ASSERT_EQUAL_size_t(0, dstr_find_indexes_cstr(NULL, "aa", indexes, DSTR_DIR_FORWARD, 0));
    dstr_destroy(s);
}

void test_dstr_find_indexes(void)
{
    dstr_adt *s = dstr_create("ab-ab-ab");
    dstr_adt *sub = dstr_create("ab");
    size_t indexes[8] = {0};
    size_t n;

    n = dstr_find_indexes(s, sub, indexes, DSTR_DIR_FORWARD, 0);
    TEST_ASSERT_EQUAL_size_t(3, n);
    TEST_ASSERT_EQUAL_size_t(0, indexes[0]);
    TEST_ASSERT_EQUAL_size_t(3, indexes[1]);
    TEST_ASSERT_EQUAL_size_t(6, indexes[2]);

    TEST_ASSERT_EQUAL_size_t(0, dstr_find_indexes(s, NULL, indexes, DSTR_DIR_FORWARD, 0));
    dstr_destroy(s);
    dstr_destroy(sub);
}

void test_cstr_find_indexes(void)
{
    size_t indexes[8] = {0};
    size_t n = cstr_find_indexes("a-a-a", "a", indexes, DSTR_DIR_FORWARD, 0);

    TEST_ASSERT_EQUAL_size_t(3, n);
    TEST_ASSERT_EQUAL_size_t(0, indexes[0]);
    TEST_ASSERT_EQUAL_size_t(2, indexes[1]);
    TEST_ASSERT_EQUAL_size_t(4, indexes[2]);

    TEST_ASSERT_EQUAL_size_t(0, cstr_find_indexes(NULL, "a", indexes, DSTR_DIR_FORWARD, 0));
}

void test_dstr_count_cstr(void)
{
    dstr_adt *s = dstr_create("aa-aa-aa");

    TEST_ASSERT_EQUAL_size_t(3, dstr_count_cstr(s, "aa"));
    TEST_ASSERT_EQUAL_size_t(0, dstr_count_cstr(s, ""));
    TEST_ASSERT_EQUAL_size_t(0, dstr_count_cstr(s, NULL));
    TEST_ASSERT_EQUAL_size_t(0, dstr_count_cstr(NULL, "aa"));
    dstr_destroy(s);
}

void test_dstr_count(void)
{
    dstr_adt *s = dstr_create("ab-ab-ab");
    dstr_adt *sub = dstr_create("ab");

    TEST_ASSERT_EQUAL_size_t(3, dstr_count(s, sub));
    TEST_ASSERT_EQUAL_size_t(0, dstr_count(s, NULL));
    TEST_ASSERT_EQUAL_size_t(0, dstr_count(NULL, sub));
    dstr_destroy(s);
    dstr_destroy(sub);
}

void test_cstr_count(void)
{
    TEST_ASSERT_EQUAL_size_t(1, cstr_count("aaa", "aa"));
    TEST_ASSERT_EQUAL_size_t(3, cstr_count("a-a-a", "a"));
    TEST_ASSERT_EQUAL_size_t(0, cstr_count(NULL, "a"));
    TEST_ASSERT_EQUAL_size_t(0, cstr_count("a", ""));
}

void test_dstr_replace_cstr(void)
{
    dstr_adt *s = dstr_create("one two one two one");

    ASSERT_OK(dstr_replace_cstr(s, "one", "X", DSTR_DIR_FORWARD, 0));
    ASSERT_DSTR_EQ(s, "X two X two X");

    ASSERT_OK(dstr_cpy_cstr(s, "one two one two one"));
    ASSERT_OK(dstr_replace_cstr(s, "one", "X", DSTR_DIR_FORWARD, 2));
    ASSERT_DSTR_EQ(s, "X two X two one");

    ASSERT_OK(dstr_cpy_cstr(s, "ab ab ab"));
    ASSERT_OK(dstr_replace_cstr(s, "ab", "X", DSTR_DIR_BACKWARD, 1));
    ASSERT_DSTR_EQ(s, "ab ab X");

    ASSERT_OK(dstr_cpy_cstr(s, "foo-foo"));
    ASSERT_OK(dstr_replace_cstr(s, "foo", "bar", DSTR_DIR_FORWARD, 0));
    ASSERT_DSTR_EQ(s, "bar-bar");

    ASSERT_OK(dstr_cpy_cstr(s, "a_a_a"));
    ASSERT_OK(dstr_replace_cstr(s, "a", "", DSTR_DIR_FORWARD, 0));
    ASSERT_DSTR_EQ(s, "__");

    ASSERT_OK(dstr_cpy_cstr(s, "x-x"));
    ASSERT_OK(dstr_replace_cstr(s, "x", NULL, DSTR_DIR_FORWARD, 0));
    ASSERT_DSTR_EQ(s, "-");

    ASSERT_OK(dstr_cpy_cstr(s, "a_a_a"));
    ASSERT_OK(dstr_replace_cstr(s, "a", "AA", DSTR_DIR_FORWARD, 0));
    ASSERT_DSTR_EQ(s, "AA_AA_AA");

    ASSERT_OK(dstr_cpy_cstr(s, "one"));
    ASSERT_INVALID(dstr_replace_cstr(s, "one", "X", DSTR_DIR_FORWARD, 2));
    ASSERT_DSTR_EQ(s, "one");
    ASSERT_INVALID(dstr_replace_cstr(s, "", "X", DSTR_DIR_FORWARD, 0));
    ASSERT_INVALID(dstr_replace_cstr(s, NULL, "X", DSTR_DIR_FORWARD, 0));
    ASSERT_INVALID(dstr_replace_cstr(NULL, "a", "b", DSTR_DIR_FORWARD, 0));
    dstr_destroy(s);
}

void test_dstr_replace(void)
{
    dstr_adt *s = dstr_create("foo bar foo");
    dstr_adt *old = dstr_create("foo");
    dstr_adt *nw = dstr_create("baz");

    ASSERT_OK(dstr_replace(s, old, nw, DSTR_DIR_FORWARD, 0));
    ASSERT_DSTR_EQ(s, "baz bar baz");

    ASSERT_INVALID(dstr_replace(s, NULL, nw, DSTR_DIR_FORWARD, 0));
    ASSERT_INVALID(dstr_replace(NULL, old, nw, DSTR_DIR_FORWARD, 0));
    dstr_destroy(s);
    dstr_destroy(old);
    dstr_destroy(nw);
}

void test_dstr_replace_nth_cstr(void)
{
    dstr_adt *s = dstr_create("a-a-a-a");

    ASSERT_OK(dstr_replace_nth_cstr(s, "a", "B", DSTR_DIR_FORWARD, 2));
    ASSERT_DSTR_EQ(s, "a-B-a-a");

    ASSERT_OK(dstr_cpy_cstr(s, "a-a-a-a"));
    ASSERT_OK(dstr_replace_nth_cstr(s, "a", "Z", DSTR_DIR_FORWARD, 0));
    ASSERT_DSTR_EQ(s, "a-a-a-Z");

    ASSERT_OK(dstr_cpy_cstr(s, "a_a"));
    ASSERT_OK(dstr_replace_nth_cstr(s, "a", "AA", DSTR_DIR_FORWARD, 1));
    ASSERT_DSTR_EQ(s, "AA_a");

    ASSERT_INVALID(dstr_replace_nth_cstr(s, "", "X", DSTR_DIR_FORWARD, 1));
    ASSERT_INVALID(dstr_replace_nth_cstr(NULL, "a", "b", DSTR_DIR_FORWARD, 1));
    dstr_destroy(s);
}

void test_dstr_replace_nth(void)
{
    dstr_adt *s = dstr_create("x-x-x");
    dstr_adt *old = dstr_create("x");
    dstr_adt *nw = dstr_create("Y");

    ASSERT_OK(dstr_replace_nth(s, old, nw, DSTR_DIR_FORWARD, 2));
    ASSERT_DSTR_EQ(s, "x-Y-x");

    ASSERT_INVALID(dstr_replace_nth(s, NULL, nw, DSTR_DIR_FORWARD, 1));
    dstr_destroy(s);
    dstr_destroy(old);
    dstr_destroy(nw);
}

/*----------------------------------------------------------------------------------------------------------------------
 * 分隔与合并
 *--------------------------------------------------------------------------------------------------------------------*/

void test_dstr_split_cstr(void)
{
    size_t count = 0;
    dstr_adt **parts;

    parts = dstr_split_cstr("a,b,c", ",", &count);
    TEST_ASSERT_NOT_NULL(parts);
    TEST_ASSERT_EQUAL_size_t(3, count);
    ASSERT_DSTR_EQ(parts[0], "a");
    ASSERT_DSTR_EQ(parts[1], "b");
    ASSERT_DSTR_EQ(parts[2], "c");
    free_dstr_array(parts, count);

    parts = dstr_split_cstr("a,,c", ",", &count);
    TEST_ASSERT_NOT_NULL(parts);
    TEST_ASSERT_EQUAL_size_t(3, count);
    ASSERT_DSTR_EQ(parts[0], "a");
    TEST_ASSERT_NULL(parts[1]);
    ASSERT_DSTR_EQ(parts[2], "c");
    free_dstr_array(parts, count);

    parts = dstr_split_cstr(",a,", ",", &count);
    TEST_ASSERT_NOT_NULL(parts);
    TEST_ASSERT_EQUAL_size_t(3, count);
    TEST_ASSERT_NULL(parts[0]);
    ASSERT_DSTR_EQ(parts[1], "a");
    TEST_ASSERT_NULL(parts[2]);
    free_dstr_array(parts, count);

    TEST_ASSERT_NULL(dstr_split_cstr("abc", ",", &count));
    TEST_ASSERT_NULL(dstr_split_cstr(NULL, ",", &count));
    TEST_ASSERT_NULL(dstr_split_cstr("a,b", "", &count));
    TEST_ASSERT_NULL(dstr_split_cstr("a,b", ",", NULL));
}

void test_dstr_split(void)
{
    dstr_adt *s = dstr_create("x::y::z");
    dstr_adt *sep = dstr_create("::");
    size_t count = 0;
    dstr_adt **parts = dstr_split(s, sep, &count);

    TEST_ASSERT_NOT_NULL(parts);
    TEST_ASSERT_EQUAL_size_t(3, count);
    ASSERT_DSTR_EQ(parts[0], "x");
    ASSERT_DSTR_EQ(parts[1], "y");
    ASSERT_DSTR_EQ(parts[2], "z");
    free_dstr_array(parts, count);

    TEST_ASSERT_NULL(dstr_split(NULL, sep, &count));
    TEST_ASSERT_NULL(dstr_split(s, NULL, &count));
    dstr_destroy(s);
    dstr_destroy(sep);
}

void test_cstr_split(void)
{
    size_t count = 0;
    char **parts = cstr_split("p|q|r", "|", &count);

    TEST_ASSERT_NOT_NULL(parts);
    TEST_ASSERT_EQUAL_size_t(3, count);
    TEST_ASSERT_EQUAL_STRING("p", parts[0]);
    TEST_ASSERT_EQUAL_STRING("q", parts[1]);
    TEST_ASSERT_EQUAL_STRING("r", parts[2]);
    free_cstr_array(parts, count);

    parts = cstr_split("a||b", "|", &count);
    TEST_ASSERT_NOT_NULL(parts);
    TEST_ASSERT_EQUAL_size_t(3, count);
    TEST_ASSERT_EQUAL_STRING("a", parts[0]);
    TEST_ASSERT_NULL(parts[1]);
    TEST_ASSERT_EQUAL_STRING("b", parts[2]);
    free_cstr_array(parts, count);

    TEST_ASSERT_NULL(cstr_split(NULL, "|", &count));
    TEST_ASSERT_NULL(cstr_split("a|b", "", &count));
    TEST_ASSERT_NULL(cstr_split("a|b", "|", NULL));
}

void test_dstr_join_cstr(void)
{
    const char *items[] = {"a", "b", "c"};
    dstr_adt *s = dstr_join_cstr(items, 3, "-");
    ASSERT_DSTR_EQ(s, "a-b-c");
    dstr_destroy(s);

    const char *items2[] = {"a", NULL, "c"};
    s = dstr_join_cstr(items2, 3, "-");
    ASSERT_DSTR_EQ(s, "a--c");
    dstr_destroy(s);

    s = dstr_join_cstr(items, 3, NULL);
    ASSERT_DSTR_EQ(s, "abc");
    dstr_destroy(s);

    s = dstr_join_cstr(items, 3, "");
    ASSERT_DSTR_EQ(s, "abc");
    dstr_destroy(s);

    TEST_ASSERT_NULL(dstr_join_cstr(NULL, 1, "-"));
    TEST_ASSERT_NULL(dstr_join_cstr(items, 0, "-"));
}

void test_dstr_join(void)
{
    dstr_adt *a = dstr_create("one");
    dstr_adt *b = dstr_create("two");
    dstr_adt *c = dstr_create("three");
    const dstr_adt *arr[] = {a, b, c};
    dstr_adt *sep = dstr_create(",");
    dstr_adt *joined = dstr_join(arr, 3, sep);

    ASSERT_DSTR_EQ(joined, "one,two,three");
    dstr_destroy(joined);

    joined = dstr_join(arr, 3, NULL);
    ASSERT_DSTR_EQ(joined, "onetwothree");
    dstr_destroy(joined);

    TEST_ASSERT_NULL(dstr_join(NULL, 1, sep));
    TEST_ASSERT_NULL(dstr_join(arr, 0, sep));
    dstr_destroy(a);
    dstr_destroy(b);
    dstr_destroy(c);
    dstr_destroy(sep);
}

void test_cstr_join(void)
{
    const char *items[] = {"x", "y", "z"};
    char *s = cstr_join(items, 3, ":");
    TEST_ASSERT_NOT_NULL(s);
    TEST_ASSERT_EQUAL_STRING("x:y:z", s);
    free(s);

    s = cstr_join(items, 3, NULL);
    TEST_ASSERT_NOT_NULL(s);
    TEST_ASSERT_EQUAL_STRING("xyz", s);
    free(s);

    const char *with_null[] = {"a", NULL, "c"};
    s = cstr_join(with_null, 3, "-");
    TEST_ASSERT_NOT_NULL(s);
    TEST_ASSERT_EQUAL_STRING("a--c", s);
    free(s);

    TEST_ASSERT_NULL(cstr_join(NULL, 1, "-"));
    TEST_ASSERT_NULL(cstr_join(items, 0, "-"));
}

/*----------------------------------------------------------------------------------------------------------------------
 * main
 *--------------------------------------------------------------------------------------------------------------------*/

int main(void)
{
    UnityBegin("test_dynamic_string.c");

    RUN_TEST(test_dstr_create);
    RUN_TEST(test_dstr_destroy);
    RUN_TEST(test_dstr_clone);
    RUN_TEST(test_dstr_sub_cstr);
    RUN_TEST(test_dstr_sub);
    RUN_TEST(test_dstr_create_format);
    RUN_TEST(test_dstr_create_vformat);

    RUN_TEST(test_dstr_cstr);
    RUN_TEST(test_dstr_length);
    RUN_TEST(test_dstr_is_empty);
    RUN_TEST(test_dstr_capacity);
    RUN_TEST(test_dstr_set_capacity);
    RUN_TEST(test_dstr_shrink_to_fit);

    RUN_TEST(test_dstr_cpy_cstr);
    RUN_TEST(test_dstr_cpy);
    RUN_TEST(test_dstr_cpy_sub_cstr);
    RUN_TEST(test_dstr_cpy_sub);
    RUN_TEST(test_dstr_cpy_format);
    RUN_TEST(test_dstr_cpy_vformat);
    RUN_TEST(test_dstr_cat_cstr);
    RUN_TEST(test_dstr_cat);
    RUN_TEST(test_dstr_cat_sub_cstr);
    RUN_TEST(test_dstr_cat_sub);
    RUN_TEST(test_dstr_cat_format);
    RUN_TEST(test_dstr_cat_vformat);
    RUN_TEST(test_dstr_insert_cstr);
    RUN_TEST(test_dstr_insert);
    RUN_TEST(test_dstr_insert_sub_cstr);
    RUN_TEST(test_dstr_insert_sub);
    RUN_TEST(test_dstr_insert_format);
    RUN_TEST(test_dstr_insert_vformat);
    RUN_TEST(test_dstr_clear);
    RUN_TEST(test_dstr_remove);
    RUN_TEST(test_dstr_trim);

    RUN_TEST(test_dstr_starts_with_cstr);
    RUN_TEST(test_dstr_starts_with);
    RUN_TEST(test_cstr_starts_with);
    RUN_TEST(test_dstr_ends_with_cstr);
    RUN_TEST(test_dstr_ends_with);
    RUN_TEST(test_cstr_ends_with);
    RUN_TEST(test_dstr_contains_cstr);
    RUN_TEST(test_dstr_contains);
    RUN_TEST(test_cstr_contains);
    RUN_TEST(test_dstr_equals_cstr);
    RUN_TEST(test_dstr_equals);
    RUN_TEST(test_cstr_equals);
    RUN_TEST(test_dstr_compare_cstr);
    RUN_TEST(test_dstr_compare);
    RUN_TEST(test_cstr_compare);

    RUN_TEST(test_dstr_find_cstr);
    RUN_TEST(test_dstr_find);
    RUN_TEST(test_cstr_find);
    RUN_TEST(test_dstr_find_nth_cstr);
    RUN_TEST(test_dstr_find_nth);
    RUN_TEST(test_cstr_find_nth);
    RUN_TEST(test_dstr_find_indexes_cstr);
    RUN_TEST(test_dstr_find_indexes);
    RUN_TEST(test_cstr_find_indexes);
    RUN_TEST(test_dstr_count_cstr);
    RUN_TEST(test_dstr_count);
    RUN_TEST(test_cstr_count);
    RUN_TEST(test_dstr_replace_cstr);
    RUN_TEST(test_dstr_replace);
    RUN_TEST(test_dstr_replace_nth_cstr);
    RUN_TEST(test_dstr_replace_nth);

    RUN_TEST(test_dstr_split_cstr);
    RUN_TEST(test_dstr_split);
    RUN_TEST(test_cstr_split);
    RUN_TEST(test_dstr_join_cstr);
    RUN_TEST(test_dstr_join);
    RUN_TEST(test_cstr_join);

    return UnityEnd();
}
