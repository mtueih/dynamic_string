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

// ADT 类型定义
struct dynamic_string {
    char *data;
    size_t len;
    size_t cap;
    size_t min_cap;
};

// 静态函数定义
// 调整一个「动态字符串」的容量
static bool capacity_resize(dstr_adt *dstr, const size_t new_cap) {
    char *new_cstr;
    size_t adjusted_cap;

    if (new_cap == 0) {
        if (dstr->data != NULL) free(dstr->data);
        *dstr = (dstr_adt)
        {
            0
        };
        return 0;
    }

    adjusted_cap = new_cap > dstr->min_cap
                       ? new_cap
                       : dstr->min_cap;

    adjusted_cap = adjusted_cap % sizeof(void *) == 0
                       ? adjusted_cap
                       : (adjusted_cap / sizeof(void *) + 1) * sizeof(void *);

    new_cstr = (char *) realloc(dstr->data, adjusted_cap);
    if (new_cstr == NULL) {
        adjusted_cap = new_cap;
        new_cstr = (char *) realloc(dstr->data, adjusted_cap);
        if (new_cstr == NULL) {
            return false;
        }
    }

    dstr->data = new_cstr;
    dstr->cap = adjusted_cap;

    if (adjusted_cap <= dstr->len) {
        dstr->len -= dstr->len + 1 - adjusted_cap;
        new_cstr[dstr->len] = '\0';
    }

    return true;
}

// API 函数定义
// 创建、销毁、清空
dstr_adt *dstr_create(const char *cstr) {
    dstr_adt *new_dstr;
    size_t cstr_len;

    new_dstr = malloc(sizeof(dstr_adt));
    if (new_dstr == NULL) return NULL;

    *new_dstr = (dstr_adt){0};

    if (cstr != NULL && (cstr_len = strlen(cstr)) != 0) {
        if (!capacity_resize(new_dstr, cstr_len + 1)) {
            free(new_dstr);
            return NULL;
        }
        memcpy(new_dstr->data, cstr, cstr_len);
        new_dstr->data[new_dstr->len = cstr_len] = '\0';
    }
    return new_dstr;
}

void dstr_destroy(dstr_adt *dstr) {
    assert(dstr != NULL);

    if (dstr->data != NULL) free(dstr->data);
    free(dstr);
}

void dstr_clear(dstr_adt *dstr) {
    assert(dstr != NULL);

    if (dstr->data == NULL || dstr->len == 0) return;

    dstr->data[0] = '\0';
    dstr->len = 0;
}

// 属性获取与设置
const char *dstr_cstr(const dstr_adt *dstr) {
    assert(dstr != NULL);

    return dstr->data;
}

size_t dstr_length(const dstr_adt *dstr) {
    assert(dstr != NULL);

    return dstr->len;
}

size_t dstr_capacity(const dstr_adt *dstr) {
    assert(dstr != NULL);

    return dstr->cap;
}

bool dstr_set_capacity(dstr_adt *dstr, const size_t new_capacity) {
    size_t old_min_cap;

    assert(dstr != NULL);

    old_min_cap = dstr->min_cap;
    dstr->min_cap = 0;
    if (capacity_resize(dstr, new_capacity)) {
        dstr->min_cap = new_capacity;
        return true;
    }
    dstr->min_cap = old_min_cap;
    return false;
}

// 复制、追加、插入、删除
// 复制、追加、插入完整现有字符串到目标字符串
bool dstr_cpy_cstr(dstr_adt *dest, const char *src) {
    size_t src_len;

    assert(dest != NULL && src != NULL);

    src_len = strlen(src);
    if (src_len == 0) return false;

    if (capacity_resize(dest, src_len + 1)) {
        memcpy(dest->data, src, src_len);
        dest->data[dest->len = src_len] = '\0';
        return true;
    }
    return false;
}

bool dstr_cpy(dstr_adt *dest, const dstr_adt *src) {
    assert(dest != NULL && src != NULL);

    // ReSharper disable once CppDFANullDereference
    if (src->len == 0) return false;
    if (capacity_resize(dest, src->len + 1)) {
        memcpy(dest->data, src->data, src->len);
        dest->data[dest->len = src->len] = '\0';
        return true;
    }
    return false;
}

bool dstr_cat_cstr(dstr_adt *dest, const char *src) {
    size_t src_len;

    assert(dest != NULL && src != NULL);

    src_len = strlen(src);
    if (src_len == 0) return false;
    if (capacity_resize(dest, dest->len + src_len + 1)) {
        memcpy(dest->data + dest->len, src, src_len);
        dest->data[dest->len += src_len] = '\0';
        return true;
    }
    return false;
}

bool dstr_cat(dstr_adt *dest, const dstr_adt *src) {
    assert(dest != NULL && src != NULL);

    if (src->len == 0) return false;

    if (capacity_resize(dest, dest->len + src->len + 1)) {
        memcpy(dest->data + dest->len, src->data, src->len);
        dest->data[dest->len += src->len] = '\0';
        return true;
    }
    return false;
}

bool dstr_insert_cstr(dstr_adt *dest, const char *src, const size_t index) {
    size_t src_len;

    assert(dest != NULL && src != NULL);

    if (index > dest->len) return false;
    src_len = strlen(src);
    if (src_len == 0) return false;

    if (capacity_resize(dest, dest->len + src_len + 1)) {
        if (index < dest->len) {
            memmove(dest->data + index + src_len,
                    dest->data + index,
                    dest->len - index
            );
        }
        memcpy(dest->data + index, src, src_len);
        dest->data[dest->len += src_len] = '\0';
        return true;
    }
    return false;
}

bool dstr_insert(dstr_adt *dest, const dstr_adt *src, const size_t index) {
    assert(dest != NULL && src != NULL);

    if (index > dest->len || src->len == 0) return 0;

    if (capacity_resize(dest, dest->len + src->len + 1)) {
        if (index < dest->len) {
            memmove(dest->data + index + src->len,
                    dest->data + index,
                    dest->len - index
            );
        }
        memcpy(dest->data + index, src->data, src->len);
        dest->data[dest->len += src->len] = '\0';
        return true;
    }
    return false;
}

// 复制、追加、插入现有字符串的子串到目标字符串
bool dstr_cpy_sub_cstr(dstr_adt *dest, const char *src, const size_t sub_index, const size_t sub_count) {
    size_t src_len, sub_len;

    assert(dest != NULL && src != NULL);

    src_len = strlen(src);
    if (sub_index >= src_len || sub_index + sub_count > src_len) return false;

    sub_len = sub_count == 0 ? src_len - sub_index : sub_count;

    if (capacity_resize(dest, sub_len + 1)) {
        memcpy(dest->data, src + sub_index, sub_len);
        dest->data[dest->len = sub_len] = '\0';
        return true;
    }
    return false;
}

bool dstr_cpy_sub(dstr_adt *dest, const dstr_adt *src, const size_t sub_index, const size_t sub_count) {
    size_t sub_len;

    assert(dest != NULL && src != NULL);

    if (sub_index >= src->len || sub_index + sub_count > src->len) return false;

    sub_len = sub_count == 0 ? src->len - sub_index : sub_count;

    if (capacity_resize(dest, sub_len + 1)) {
        memcpy(dest->data, src->data + sub_index, sub_len);
        dest->data[dest->len = sub_len] = '\0';
        return true;
    }
    return false;
}

bool dstr_cat_sub_cstr(dstr_adt *dest, const char *src, const size_t sub_index, const size_t sub_count) {
    size_t src_len, sub_len;

    assert(dest != NULL && src != NULL);

    src_len = strlen(src);
    if (sub_index >= src_len || sub_index + sub_count > src_len) return false;

    sub_len = sub_count == 0 ? src_len - sub_index : sub_count;

    if (capacity_resize(dest, dest->len + sub_count + 1)) {
        memcpy(dest->data + dest->len, src + sub_index, sub_len);
        dest->data[dest->len += src_len] = '\0';
        return true;
    }
    return false;
}

bool dstr_cat_sub(dstr_adt *dest, const dstr_adt *src, const size_t sub_index, const size_t sub_count) {
    size_t sub_len;

    assert(dest != NULL && src != NULL);

    if (sub_index >= src->len || sub_index + sub_count > src->len) return false;

    sub_len = sub_count == 0 ? src->len - sub_index : sub_count;

    if (capacity_resize(dest, dest->len + sub_len + 1)) {
        memcpy(dest->data + dest->len, src->data + sub_index, sub_len);
        dest->data[dest->len += sub_len] = '\0';
        return true;
    }
    return false;
}

bool dstr_insert_sub_cstr(dstr_adt *dest, const char *src, const size_t index, const size_t sub_index,
                          const size_t sub_count) {
    size_t src_len, sub_len;

    assert(dest != NULL && src != NULL);

    if (index > dest->len) return false;
    src_len = strlen(src);
    if (sub_index >= src_len || sub_index + sub_count > src_len) return false;

    sub_len = sub_count == 0 ? src_len - sub_index : sub_count;

    if (capacity_resize(dest, dest->len + sub_len + 1)) {
        if (index < dest->len) {
            memmove(dest->data + index + sub_len,
                    dest->data + index,
                    dest->len - index
            );
        }
        memcpy(dest->data + index, src + sub_index, sub_len);
        dest->data[dest->len += sub_len] = '\0';
        return true;
    }
    return false;
}

bool dstr_insert_sub(dstr_adt *dest, const dstr_adt *src, const size_t index, const size_t sub_index,
                     const size_t sub_count) {
    size_t sub_len;

    assert(dest != NULL && src != NULL);

    if (index > dest->len) return false;
    if (sub_index >= src->len || sub_index + sub_count > src->len) return false;

    sub_len = sub_count == 0 ? src->len - sub_index : sub_count;

    if (capacity_resize(dest, dest->len + sub_len + 1)) {
        if (index < dest->len) {
            memmove(dest->data + index + sub_len,
                    dest->data + index,
                    dest->len - index
            );
        }
        memcpy(dest->data + index, src->data + sub_index, sub_len);
        dest->data[dest->len += sub_len] = '\0';
        return true;
    }
    return false;
}

// 删除子串
void dstr_remove(dstr_adt *dstr, const size_t sub_index, const size_t sub_count) {
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
    capacity_resize(dstr, dstr->len + 1);
}

// 删除特定内容
void dstr_trim(dstr_adt *dstr) {
    char *find;
    size_t space_len;

    assert(dstr != NULL);

    if (dstr->len == 0) return;

    // 去除前导空白字符
    for (find = dstr->data;
         find < dstr->data + dstr->len;
         ++find
    ) {
        if (!isspace(*find)) break;
    }

    if (find > dstr->data) {
        space_len = find - dstr->data;
        memmove(dstr->data, find, dstr->len - space_len);
        dstr->data[dstr->len -= space_len] = '\0';
    }

    for (find = dstr->data + dstr->len - 1;
         find >= dstr->data;
         --find) {
        if (!isspace(*find)) break;
    }

    if (find < dstr->data + dstr->len - 1) {
        space_len = dstr->data + dstr->len - 1 - find;
        dstr->data[dstr->len -= space_len] = '\0';
    }

    capacity_resize(dstr, dstr->len + 1);
}

// 格式化写入
bool dstr_printf(dstr_adt *dstr, const char *format, ...) {
    va_list args, temp_args;
    size_t old_cap;
    int needed_len, written_len;

    assert(dstr != NULL && format != NULL);

    va_start(args, format);
    va_copy(temp_args, args);
    needed_len = vsnprintf(NULL, 0, format, temp_args);
    va_end(temp_args);
    if (needed_len <= 0) {
        va_end(args);
        return false;
    }

    old_cap = dstr->cap;
    if (capacity_resize(dstr, needed_len + 1)) {
        written_len = vsnprintf(dstr->data, needed_len + 1, format, args);
        va_end(args);
        if (written_len < 0) {
            capacity_resize(dstr, old_cap);
            return false;
        }

        dstr->data[dstr->len = written_len] = '\0';
        return true;
    }
    return false;
}


// 从现有字符串生成新字符串
// 提取子串
dstr_adt *dstr_sub_cstr(const char *cstr, const size_t sub_index, const size_t sub_count) {
    size_t cstr_len, sub_len;
    dstr_adt *new_dstr;

    assert(cstr != NULL);

    cstr_len = strlen(cstr);
    if (sub_index >= cstr_len || sub_index + sub_count > cstr_len) return NULL;

    new_dstr = malloc(sizeof(dstr_adt));
    if (new_dstr == NULL) return NULL;

    *new_dstr = (dstr_adt){0};

    sub_len = sub_count == 0 ? cstr_len - sub_index : sub_count;

    if (capacity_resize(new_dstr, sub_len + 1)) {
        memcpy(new_dstr->data, cstr + sub_index, sub_len);
        new_dstr->data[new_dstr->len = sub_len] = '\0';
        return new_dstr;
    }

    free(new_dstr);
    return NULL;
}

dstr_adt *dstr_sub(const dstr_adt *dstr, const size_t sub_index, const size_t sub_count) {
    size_t sub_len;
    dstr_adt *new_dstr;

    assert(dstr != NULL);

    if (sub_index >= dstr->len || sub_index + sub_count > dstr->len) return NULL;

    new_dstr = malloc(sizeof(dstr_adt));
    if (new_dstr == NULL) return NULL;

    *new_dstr = (dstr_adt){0};

    sub_len = sub_count == 0 ? dstr->len - sub_index : sub_count;

    if (capacity_resize(new_dstr, sub_len + 1)) {
        memcpy(new_dstr->data, dstr->data + sub_index, sub_len);
        new_dstr->data[new_dstr->len = sub_len] = '\0';
        return new_dstr;
    }

    free(new_dstr);
    return NULL;
}

// 克隆
dstr_adt *dstr_clone(const dstr_adt *dstr) {
    dstr_adt *new_dstr;

    assert(dstr != NULL);

    new_dstr = malloc(sizeof(dstr_adt));
    if (new_dstr == NULL) return NULL;

    *new_dstr = (dstr_adt){0};
    if (capacity_resize(new_dstr, dstr->len + 1)) {
        memcpy(new_dstr->data, dstr->data, dstr->len);
        new_dstr->data[new_dstr->len = dstr->len] = '\0';
        return new_dstr;
    }

    free(new_dstr);
    return NULL;
}

// 查找、统计与替换
bool dstr_find_cstr(const dstr_adt *dstr, const char *sub, size_t *out_index, const bool backward) {
    size_t sub_len;
    char *find;

    assert(dstr != NULL && sub != NULL && out_index != NULL);

    sub_len = strlen(sub);
    if (sub_len == 0 || sub_len > dstr->len) return false;

    if (backward) {
        for (find = dstr->data + dstr->len - sub_len;
             find >= dstr->data;
             --find
        ) {
            if (strncmp(find, sub, sub_len) == 0) {
                *out_index = find - dstr->data;
                return true;
            }
        }
    } else {
        for (find = dstr->data;
             find <= dstr->data + dstr->len - sub_len;
             ++find
        ) {
            if (strncmp(find, sub, sub_len) == 0) {
                *out_index = find - dstr->data;
                return true;
            }
        }
    }

    return false;
}

bool dstr_find(const dstr_adt *dstr, const dstr_adt *sub, size_t *out_index, const bool backward) {
    char *find;

    assert(dstr != NULL && sub != NULL && out_index != NULL);

    if (sub->len == 0 || sub->len > dstr->len) return false;

    if (backward) {
        for (find = dstr->data + dstr->len - sub->len;
             find >= dstr->data;
             --find
        ) {
            if (strncmp(find, sub->data, sub->len) == 0) {
                *out_index = find - dstr->data;
                return true;
            }
        }
    } else {
        for (find = dstr->data;
             find <= dstr->data + dstr->len - sub->len;
             ++find
        ) {
            if (strncmp(find, sub->data, sub->len) == 0) {
                *out_index = find - dstr->data;
                return true;
            }
        }
    }

    return false;
}

size_t dstr_count_cstr(const dstr_adt *dstr, const char *sub) {
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

size_t dstr_count(const dstr_adt *dstr, const dstr_adt *sub) {
    char *find;
    size_t find_count;

    assert(dstr != NULL && sub != NULL);

    // ReSharper disable once CppDFANullDereference
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

bool dstr_find_nth_cstr(const dstr_adt *dstr, const char *sub, size_t *out_index, const size_t n, const bool backward) {
    size_t sub_len;
    char *find;
    size_t find_count;

    if (n == 0) return false;
    assert(dstr != NULL && sub != NULL && out_index != NULL);
    sub_len = strlen(sub);
    if (sub_len == 0 || sub_len > dstr->len) return false;

    if (backward) {
        for (find_count = 0, find = dstr->data + dstr->len - sub_len;
             find >= dstr->data;
        ) {
            if (strncmp(find, sub, sub_len) == 0) {
                ++find_count;
                if (find_count == n) {
                    *out_index = find - dstr->data;
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
                    *out_index = find - dstr->data;
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

bool dstr_find_nth(const dstr_adt *dstr, const dstr_adt *sub, size_t *out_index, const size_t n, const bool backward) {
    char *find;
    size_t find_count;

    if (n == 0) return false;
    assert(dstr != NULL && sub != NULL && out_index != NULL);
    if (sub->len == 0 || sub->len > dstr->len) return false;

    if (backward) {
        for (find_count = 0, find = dstr->data + dstr->len - sub->len;
             find >= dstr->data;
        ) {
            if (strncmp(find, sub->data, sub->len) == 0) {
                ++find_count;
                if (find_count == n) {
                    *out_index = find - dstr->data;
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
                    *out_index = find - dstr->data;
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


size_t dstr_replace_cstr(dstr_adt *dstr, const char *old, const char *new, const size_t n,
                         const bool backward) {
    // 局部变量声明
    size_t find_count;
    char *find; // 指向当前比较的字符串
    size_t old_len, new_len;

    // 参数检查
    assert(dstr != NULL && old != NULL && new != NULL);

    old_len = strlen(old);
    if (old_len == 0 || old_len > dstr->len) return 0;

    new_len = strlen(new);
    // 替换
    if (backward) {
        for (find_count = 0, find = dstr->data + dstr->len - old_len;
             find >= dstr->data && (n == 0 ? true : find_count < n);
        ) {
            if (strncmp(find, old, old_len) == 0) {
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
                    memcpy(find, new, new_len);
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
             find <= dstr->data + dstr->len - old_len && (n == 0 ? true : find_count < n);
        ) {
            if (strncmp(find, old, old_len) == 0) {
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
                    memcpy(find, new, new_len);
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

    return find_count;
}


size_t dstr_replace(dstr_adt *dstr, const dstr_adt *old, const dstr_adt *new,
                    const size_t n, const bool backward) {
    // 局部变量声明
    size_t find_count;
    char *find; // 指向当前比较的字符串

    // 参数检查
    assert(dstr != NULL && old != NULL && new != NULL);

    if (old->len == 0 || old->len > dstr->len) return 0;

    // 替换
    if (backward) {
        for (find_count = 0, find = dstr->data + dstr->len - old->len;
             find >= dstr->data && (n == 0 ? true : find_count < n);
        ) {
            if (strncmp(find, old->data, old->len) == 0) {
                ++find_count;
                // 替换
                if (old->len != new->len && find + old->len < dstr->data + dstr->len) {
                    memmove(
                        find + new->len,
                        find + old->len,
                        dstr->data + dstr->len - find - old->len
                    );
                }
                if (new->len > 0) {
                    memcpy(find, new, new->len);
                }
                dstr->len -= old->len;
                dstr->len += new->len;
                dstr->data[dstr->len] = '\0';

                find -= old->len;
            } else {
                --find;
            }
        }
    } else {
        for (find_count = 0, find = dstr->data;
             find <= dstr->data + dstr->len - old->len && (n == 0 ? true : find_count < n);
        ) {
            if (strncmp(find, old->data, old->len) == 0) {
                ++find_count;
                // 替换
                if (old->len != new->len && find + old->len < dstr->data + dstr->len) {
                    memmove(
                        find + new->len,
                        find + old->len,
                        dstr->data + dstr->len - find - old->len
                    );
                }
                if (new->len > 0) {
                    memcpy(find, new, new->len);
                }
                dstr->len -= old->len;
                dstr->len += new->len;
                dstr->data[dstr->len] = '\0';

                find += new->len;
            } else {
                ++find;
            }
        }
    }

    return find_count;
}

// 判断与比较
bool dstr_starts_with_cstr(const dstr_adt *dstr, const char *prefix) {
    size_t prefix_len;

    assert(dstr != NULL && prefix != NULL);

    prefix_len = strlen(prefix);
    if (prefix_len == 0 || prefix_len > dstr->len) return false;

    return strncmp(dstr->data, prefix, prefix_len) == 0;
}


bool dstr_starts_with(const dstr_adt *dstr, const dstr_adt *prefix) {
    assert(dstr != NULL && prefix != NULL);

    if (prefix->len == 0 || prefix->len > dstr->len) return false;

    return strncmp(dstr->data, prefix->data, prefix->len) == 0;
}


bool dstr_ends_with_cstr(const dstr_adt *dstr, const char *suffix) {
    size_t suffix_len;

    assert(dstr != NULL && suffix != NULL);
    suffix_len = strlen(suffix);
    if (suffix_len == 0 || suffix_len > dstr->len) return false;

    return strncmp(dstr->data + dstr->len - suffix_len,
                   suffix, suffix_len) == 0;
}


bool dstr_ends_with(const dstr_adt *dstr, const dstr_adt *suffix) {
    assert(dstr != NULL && suffix != NULL);
    if (suffix->len == 0 || suffix->len > dstr->len) return false;

    return strncmp(dstr->data + dstr->len - suffix->len,
                   suffix->data, suffix->len) == 0;
}


bool dstr_contains_cstr(const dstr_adt *dstr, const char *sub) {
    size_t sub_len;

    assert(dstr != NULL && sub != NULL);
    sub_len = strlen(sub);
    if (sub_len == 0 || sub_len > dstr->len) return false;

    return strstr(dstr->data, sub) != NULL;
}

bool dstr_contains(const dstr_adt *dstr, const dstr_adt *sub) {
    assert(dstr != NULL && sub != NULL);
    if (sub->len == 0 || sub->len > dstr->len) return false;

    return strstr(dstr->data, sub->data) != NULL;
}


bool dstr_equals_cstr(const dstr_adt *dstr, const char *cstr) {
    size_t cstr_len;

    assert(dstr != NULL && cstr != NULL);
    cstr_len = strlen(cstr);
    if (cstr_len != dstr->len) return false;

    return strncmp(dstr->data, cstr, cstr_len) == 0;
}


bool dstr_equals(const dstr_adt *dstr_1, const dstr_adt *dstr_2) {
    assert(dstr_1 != NULL && dstr_2 != NULL);
    if (dstr_1->len != dstr_2->len) return false;

    return strncmp(dstr_1->data, dstr_2->data, dstr_2->len) == 0;
}

int dstr_compare_cstr(const dstr_adt *dstr, const char *cstr) {
    assert(dstr != NULL && cstr != NULL);

    return strcmp(dstr->data, cstr);
}

int dstr_compare(const dstr_adt *dstr_1, const dstr_adt *dstr_2) {
    assert(dstr_1 != NULL && dstr_2 != NULL);

    return strcmp(dstr_1->data, dstr_2->data);
}
