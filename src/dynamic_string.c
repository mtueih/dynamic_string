/*======================================================================================================================
 * src/dynamic_string.c - 项目主库实现文件
 *====================================================================================================================*/

/*----------------------------------------------------------------------------------------------------------------------
 * 头文件包含
 *--------------------------------------------------------------------------------------------------------------------*/
#include "dynamic_string/dynamic_string.h"

#include <ctype.h>
#include <limits.h>
#include <safe_calc/safe_calc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*----------------------------------------------------------------------------------------------------------------------
 * 宏定义
 *--------------------------------------------------------------------------------------------------------------------*/

/**
 * 小字符串优化（SSO）：栈缓冲区大小（字节）。
 *
 * 栈缓冲区复用「用于存储堆缓冲区（堆内存）相关信息的成员变量」的存储空间。
 * 「用于存储堆缓冲区（堆内存）相关信息的成员变量」有：
 * - data：堆内存指针。
 * - cap：堆内存大小（容量）。
 * - min_cap：最小堆内存大小（保底容量）。
 * 而 len 首先不是用来存储堆缓冲区的成员变量，其次是一个与存储细节无关的成员变量，
 * 而且不管使用堆缓冲区还是栈缓冲区来存储，都需要它。
 *
 * 复用所用的技术是 `union` 共用体，显然需要手段来区分当前存储的是哪一种数据。
 * 两种情况分别称之为栈缓冲区模式与堆缓冲区模式。
 * 新增成员不划算。考虑到可以使用 data 来天然区分是否是堆缓冲区模式，
 * 因为为防止野指针，当 data 不指向堆内存时，总是将其置为空指针。
 * 同时，为了访问数据的便捷性，可以在栈缓冲区模式下，让 data 指向栈缓冲区。
 *
 * 因此，只复用成员变量 cap 和 min_cap 的存储空间。
 * 因此，【栈缓冲区大小为两个 size_t 的大小】。
 *
 * 如果（尽管可能性很小），将来结构体成员变量的结构发生变化，此宏应更随变动。
 * 而且不论如何变动，成员变量 cap 永远都是必不可少的，因此此宏的值大概率不会小于一个 size_t 的大小，
 * 也就是说，【栈缓冲区的大小一定大于 0】，此文件中的代码可以根据此事实做优化。
 */
#define DYNAMIC_STRING_SSO_BUF_SIZE (sizeof(size_t) * 2)

/* 静态函数 replace_str 中，indexes 数组缩容绝对阈值。 */
#define INDEXES_SHRINK_BYTES 4096

/*----------------------------------------------------------------------------------------------------------------------
 * ADT 类型定义
 *--------------------------------------------------------------------------------------------------------------------*/

struct dynamic_string
{
    char *data; /* 指向数据缓冲区的指针。 */
    size_t len; /* 字符串的长度。 */

    union {
        struct
        {
            size_t cap;     /* 「动态字符串」的容量。 */
            size_t min_cap; /* 「动态字符串」的保底容量。 */
        } heap_info;

        char sso_buf[DYNAMIC_STRING_SSO_BUF_SIZE];
    } storage;
};

/*----------------------------------------------------------------------------------------------------------------------
 * 静态函数定义
 *--------------------------------------------------------------------------------------------------------------------*/

/**
 * @brief 调整一个「动态字符串」的容量。
 *
 * @param dstr[in] 目标「动态字符串」的指针。
 * @param new_cap[in] 所需容量。
 * @param should_adjust_dynamically[in] 是否应该动态调整
 *                                      （动态调整下，会确保容量不低于保底容量，且会采用几何扩容和延迟缩容策略）。
 *
 * @return 如果调整成功则返回 true，否则返回 false。
 */
static bool resize_capacity(struct dynamic_string *const dstr, size_t new_cap, const bool should_adjust_dynamically)
{
    /* 当使用栈缓冲区时。
     * 仅在请求容量 > 栈缓冲区大小时，才触发栈缓冲区转堆缓冲区。不管是否要求动态调整。
     * 否则直接返回 true。 */
    if (dstr->data == dstr->storage.sso_buf)
    {
        /* 如果请求容量 <= 栈缓冲区大小，直接返回 true。 */
        if (new_cap <= DYNAMIC_STRING_SSO_BUF_SIZE)
        {
            return true;
        }

        /* 否则，栈缓冲区转堆缓冲区。 */
        /* 分配堆缓冲区内存。 */
        char *const new_data = malloc(new_cap);
        if (new_data == NULL)
        {
            return false;
        }

        /* 拷贝栈缓冲区全部内容至堆缓冲区。 */
        memcpy(new_data, dstr->storage.sso_buf, DYNAMIC_STRING_SSO_BUF_SIZE);

        /* 让 data 指向堆缓冲区。 */
        dstr->data = new_data;
        /* 更新容量。 */
        dstr->storage.heap_info.cap = new_cap;
        /* 初始化保底容量为 0。 */
        dstr->storage.heap_info.min_cap = 0;

        return true;
    }

    /* 当使用堆缓冲区时。 */

    /* 如果需要动态调整，则考虑 storage.heap_info.min_cap 成员（保底容量）。
     * 此时，如果 new_cap < 保底容量，则更新 new_cap 的值为保底容量值，以确保容量不低于保底容量。 */
    if (should_adjust_dynamically && new_cap < dstr->storage.heap_info.min_cap)
    {
        new_cap = dstr->storage.heap_info.min_cap;
    }

    /* 如果请求容量 <= 栈缓冲区的大小，则将堆缓冲区转回栈缓冲区。 */
    if (new_cap <= DYNAMIC_STRING_SSO_BUF_SIZE)
    {
        /* 拷贝堆缓冲区内容到栈缓冲区。
         * 仅拷贝栈缓冲区大小所能容纳的数据量。 */
        memcpy(dstr->storage.sso_buf, dstr->data, DYNAMIC_STRING_SSO_BUF_SIZE);

        /* 释放堆缓冲区。*/
        free(dstr->data);
        /* 让 data 指向栈缓冲区。 */
        dstr->data = dstr->storage.sso_buf;

        return true;
    }

    /* 否则，就调整堆缓冲区的大小。 */

    /* 如果目标容量与当前容量相同，则直接返回 true。 */
    if (new_cap == dstr->storage.heap_info.cap)
    {
        return true;
    }

    /* 如果需要动态调整。 */
    if (should_adjust_dynamically)
    {
        /* 如果目标容量小于当前容量，则延迟减容：当目标容量 <= 当前容量的 1/4 时，才实际缩容。
         * 即，当目标容量 < 当前容量，且，目标容量 > 当前容量的 1/4 时，直接返回 true。 */
        if (new_cap < dstr->storage.heap_info.cap && new_cap > (dstr->storage.heap_info.cap >> 2))
        {
            return true;
        }

        /* 如果目标容量大于当前容量，则执行容量预分配。 */
        if (new_cap > dstr->storage.heap_info.cap)
        {
            size_t adjusted_cap = 0;
            if (safe_size_add(new_cap, new_cap >> 1, &adjusted_cap) == SAFE_CALC_OK)
            {
                char *const new_data = realloc(dstr->data, adjusted_cap);
                if (new_data != NULL)
                {
                    dstr->data = new_data;
                    dstr->storage.heap_info.cap = adjusted_cap;
                    return true;
                }
            }
        }
    }

    char *const new_data = realloc(dstr->data, new_cap);
    if (new_data == NULL)
    {
        return false;
    }

    /* 更新堆缓冲区指针。 */
    dstr->data = new_data;
    /* 更新容量。 */
    dstr->storage.heap_info.cap = new_cap;

    return true;
}

/**
 * @brief 从一个「C 字符串」创建一个新「动态字符串」。
 *
 * @param src 源「C 字符串」的指针。
 * @param src_len 源「C 字符串」的长度。为 0 时，创建空「动态字符串」。
 *
 * @return 所创建的「动态字符串」的指针。如果创建失败则返回空指针。
 */
static struct dynamic_string *create_dstr(const char *const src, const size_t src_len)
{
    struct dynamic_string *const new_dstr = malloc(sizeof(struct dynamic_string));
    if (new_dstr == NULL)
    {
        return NULL;
    }

    /* 初始化。 */
    *new_dstr = (struct dynamic_string){0};
    /* data 默认指向栈小字符串缓冲区。 */
    new_dstr->data = new_dstr->storage.sso_buf;

    if (src_len > 0)
    {
        if (safe_size_add(src_len, 1, NULL) != SAFE_CALC_OK || !resize_capacity(new_dstr, src_len + 1, false))
        {
            free(new_dstr);
            return NULL;
        }

        memcpy(new_dstr->data, src, src_len);

        /* 更新成员变量 len 并在结尾补 '\0'。 */
        new_dstr->len = src_len;
        new_dstr->data[src_len] = '\0';
    }

    return new_dstr;
}
/**
 * @brief 向一个「动态字符串」的指定位置处插入一个「C 字符串」。
 *        插入前可选择性删除目标位置向后的指定数量个字符，用于覆写和删除。
 *
 * @param dest 目标「动态字符串」的指针。
 * @param index 插入位置的索引。
 * @param remove_length 插入前先删除的字符数。【0 不表示删除到末尾】。
 * @param src 源「C 字符串」的指针。
 * @param src_len 源「C 字符串」的长度。
 *
 * @return 全局状态码。
 */
static dstr_status_t insert_str(struct dynamic_string *const dest, const size_t index, const size_t remove_length,
                                const char *const src, const size_t src_len)
{
    const size_t new_len = dest->len - remove_length + src_len;

    /* 当新长度大于当前长度时尝试扩容。 */
    if (new_len > dest->len)
    {
        if (safe_size_add(new_len, 1, NULL) != SAFE_CALC_OK || !resize_capacity(dest, new_len + 1, true))
        {
            return DSTR_MEMORY_ALLOC_FAILED;
        }
    }

    /* 当存在需要移动的尾部数据时，执行移动。 */
    const size_t tail_len = dest->len - index - remove_length;

    if (tail_len > 0 && remove_length != src_len)
    {
        memmove(dest->data + index + src_len, dest->data + index + remove_length, tail_len);
    }

    /* 当存在需要拷贝的数据时，执行拷贝。 */
    if (src_len > 0)
    {
        memcpy(dest->data + index, src, src_len);
    }

    /* 如果新长度小于当前长度，则在操作执行完后，尝试缩容。 */
    if (new_len < dest->len)
    {
        resize_capacity(dest, new_len + 1, true);
    }

    /* 如果长度有变化，则更新长度。 */
    if (new_len != dest->len)
    {
        dest->len = new_len;
        dest->data[new_len] = '\0';
    }

    return DSTR_SUCCESS;
}
/**
 * @brief 向一个「动态字符串」的指定位置处格式化插入一个字符串。
 *        插入前可选择性删除目标位置向后的指定数量个字符，用于覆写和删除。
 *
 * @param dstr 目标「动态字符串」的指针。
 * @param index 插入位置的索引。
 * @param remove_length 插入前先删除的字符数。【0 不表示删除到末尾】。
 * @param format 格式「C 字符串」的指针。
 * @param args 已通过 va_start() 初始化的 va_list 变量，包含与 format 对应的可变参数列表信息。
 *             该函数不会调用 va_end()，调用者需自行管理 args 的生命周期。
 *
 * @return 全局状态码。
 */
static dstr_status_t insert_str_format(struct dynamic_string *const dstr, const size_t index,
                                       const size_t remove_length, const char *const format, va_list args)
{
    va_list temp_args;

    va_copy(temp_args, args);
    const int temp_len = vsnprintf(NULL, 0, format, temp_args);
    va_end(temp_args);

    if (temp_len < 0)
    {
        return DSTR_INVALID_ARGUMENT;
    }

    const size_t format_len = temp_len;
    const size_t new_len = dstr->len - remove_length + format_len;

    /* 当新长度大于当前长度时尝试扩容。 */
    if (new_len > dstr->len)
    {
        if (safe_size_add(new_len, 1, NULL) != SAFE_CALC_OK || !resize_capacity(dstr, new_len + 1, true))
        {
            return DSTR_MEMORY_ALLOC_FAILED;
        }
    }

    /* 当存在需要移动的尾部数据时，执行移动。 */
    const size_t tail_len = dstr->len - index - remove_length;
    if (tail_len > 0 && remove_length != format_len)
    {
        memmove(dstr->data + index + format_len, dstr->data + index + remove_length, tail_len);
    }

    /* 当存在需要格式化的数据时，格式化写入到 dstr->data。 */
    if (format_len > 0)
    {
        if (tail_len > 0)
        {
            /* 插入中间：vsnprintf 写入后会在末尾强制写入 '\0'，该 '\0' 会覆盖被搬移到 format_len 末尾的第一个尾部字符。
             * 因此，在写入前暂存该字符，写入后恢复。 */
            const char saved_char = dstr->data[index + format_len];
            vsnprintf(dstr->data + index, format_len + 1, format, args);
            dstr->data[index + format_len] = saved_char;
        }
        else
        {
            /* 追加到末尾：vsnprintf 写入的 '\0' 恰好是字符串结束符，无需处理。 */
            vsnprintf(dstr->data + index, format_len + 1, format, args);
        }
    }

    /* 如果新长度小于当前长度，则在操作执行完后尝试缩容。 */
    if (new_len < dstr->len)
    {
        resize_capacity(dstr, new_len + 1, true);
    }

    /* 如果长度有变化，则更新长度。 */
    if (new_len != dstr->len)
    {
        dstr->len = new_len;
        /* 确保缓冲区以 '\0' 结尾。 */
        dstr->data[new_len] = '\0';
    }

    return DSTR_SUCCESS;
}

/**
 * @brief 查找一个「C 字符串」中，指定子「C 字符串」第 n 次或前 n 次出现的位置，并返回截止第 n 次，实际一共出现的次数。
 *
 * @param cstr 目标「C 字符串」的指针。
 * @param cstr_len 目标「C 字符串」的长度。
 * @param sub 子「C 字符串」的指针。
 * @param sub_len 子「C 字符串」的长度。
 * @param out_index 存储查找结果（位置索引）的 size_t 变量的指针。为空指针时不写入。
 * @param out_indexes 存储查找结果（位置索引）的 size_t 数组的指针。为空指针时不写入。请确保容量足够。
 *                    通常需要先进行一次统计，获得确切出现次数后使用此参数，也可预备一个足够大的数组，以减少一次查找。
 * @param direction[in] 查找方向。
 * @param n 出现的次序。从 1 开始。为 0 表示最后一次。
 *
 * @return 指定子「C 字符串」截止第 n 次，实际一共出现的次数。
 */
static size_t find_str(const char *const cstr, const size_t cstr_len, const char *const sub, const size_t sub_len,
                       size_t *const out_index, size_t *const out_indexes, const dstr_direction_t direction,
                       const size_t n)
{
    /* 用于迭代的指针变量。 */
    const char *p;
    /* 用于存储当前出现位置的指针。 */
    const char *find = NULL;
    /* 用于统计出现的次数。 */
    size_t find_count = 0;

    /* 指针常量，指向查找区间的后一个位置。用于迭代边界。 */
    const char *const end = cstr + cstr_len - sub_len + 1;

    /* 分块执行正向与逆向查找，避免每次循环中都存在条件判断，增加分支开销。 */
    if (direction == DSTR_DIR_BACKWARD)
    {
        p = end;

        while (p > cstr)
        {
            if (memcmp(p - 1, sub, sub_len) == 0)
            {
                find = p - 1;

                if (out_indexes != NULL)
                {
                    out_indexes[find_count] = find - cstr;
                }

                if (++find_count == n)
                {
                    break;
                }

                if ((p - cstr) > sub_len)
                {
                    p -= sub_len;
                }
                else
                {
                    break;
                }
            }
            else
            {
                --p;
            }
        }
    }
    else
    {
        p = cstr;

        while (p < end)
        {
            if (memcmp(p, sub, sub_len) == 0)
            {
                find = p;

                if (out_indexes != NULL)
                {
                    out_indexes[find_count] = find - cstr;
                }

                if (++find_count == n)
                {
                    break;
                }

                p += sub_len;
            }
            else
            {
                ++p;
            }
        }
    }

    if (out_index != NULL && find != NULL)
    {
        *out_index = find - cstr;
    }

    return find_count;
}

/**
 * @brief 将一个「动态字符串」中指定的旧「C 字符串」替换为指定的新「C 字符串」。可指定替换方向和替换次数。
 *
 * @param dstr 目标「动态字符串」的指针。
 * @param old_str 旧「C 字符串」的指针。
 * @param old_str_len 旧「C 字符串」的长度。
 * @param new_str 新「C 字符串」的指针。为空指针时视为替换为空字符串（即删除旧字符串）。
 * @param new_str_len 新「C 字符串」的长度。new_str 为空指针时该值应为 0。
 * @param direction 替换方向。
 * @param n 替换次数。从 1 开始。为 0 表示全部替换。
 *
 * @return 全局状态码。
 */
static dstr_status_t replace_str(struct dynamic_string *const dstr, const char *const old_str, const size_t old_str_len,
                                 const char *const new_str, const size_t new_str_len, const dstr_direction_t direction,
                                 const size_t n)
{
    /* old_str 在 dstr 中最多可能出现的次数。 */
    const size_t max_possible = dstr->len / old_str_len;
    /* 提前返回：不可能有匹配（max_possible == 0）或 n 超出理论上界。 */
    if (max_possible == 0 || n > max_possible)
    {
        return DSTR_INVALID_ARGUMENT;
    }

    /* 指向“用于存储 old_str 在 dstr 中出现的位置的动态数组”的指针。 */
    size_t *indexes = NULL;
    /* old_str 在 dstr 中出现的实际次数。 */
    size_t old_str_count = 0;

    /* 最优路径：用空间换时间，分配足够大的数组，来记录出现的位置。
     * 当 n 不为 0 时，即用户指定了替换的次数时，分配 n 个，因为最多也只会用到 n 个；否则为 max_possible。 */
    const size_t indexes_count = (n > 0) ? n : max_possible;

    /* 尝试分配 indexes_count 个元素的动态数组。 */
    /* 判断并记录「indexes_count * sizeof(size_t)」是否不会溢出。 */
    const bool indexes_count_ok = (safe_size_mul(indexes_count, sizeof(size_t), NULL) == SAFE_CALC_OK);
    if (indexes_count_ok)
    {
        indexes = malloc(indexes_count * sizeof(size_t));
    }

    /* 如果分配失败，回退到次优路径，先进行一次查找以统计实际出现的次数，然后分配 old_str_count 个元素的动态数组。 */
    if (indexes == NULL)
    {
        old_str_count = find_str(dstr->data, dstr->len, old_str, old_str_len, NULL, NULL, direction, n);

        /* 如果 old_str 实际出现次数为 0 次或不足 n 次，视为参数不合法并返回。 */
        if (old_str_count == 0 || old_str_count < n)
        {
            return DSTR_INVALID_ARGUMENT;
        }

        /* 尝试分配 old_str_count 个元素的动态数组。
         * 如果「indexes_count * sizeof(size_t)」不会溢出，那「old_str_count * sizeof(size_t)」也一定不会溢出，
         * 因为 old_str_count <= indexes_count（为 n 时，实际出现的次数完全有可能大于n，此时分配失败，意味着）。
         * 如果 old_str_count == indexes_count，那就没有尝试的必要了。
         */
        if (old_str_count < indexes_count &&
            (indexes_count_ok || safe_size_mul(old_str_count, sizeof(size_t), NULL) == SAFE_CALC_OK))
        {
            indexes = malloc(old_str_count * sizeof(size_t));
        }
    }

    /* 此时，如果 indexes 不是空指针，则进行二次/一次查找以记录出现位置。*/
    if (indexes != NULL)
    {
        /* 二次查找（次优路径下）或一次查找（最优路径下），记录出现的位置。 */
        const size_t temp_count = find_str(dstr->data, dstr->len, old_str, old_str_len, NULL, indexes, direction, n);

        /* 如果 old_str_count 为 0，则说明走的是最优路径。 */
        if (old_str_count == 0)
        {
            /* 此时，由于是第一次查找，需要对统计次数做判断。 */
            if (temp_count == 0 || temp_count < n)
            {
                free(indexes);
                return DSTR_INVALID_ARGUMENT;
            }

            /* 此时，分配的数组大小有可能大于需要，因此缩容以释放多余空间。 */
            if (temp_count < indexes_count)
            {
                size_t *const temp_data = realloc(indexes, temp_count * sizeof(size_t));
                if (temp_data != NULL)
                {
                    indexes = temp_data;
                }
            }

            /* 更新 old_str_count 的值。 */
            old_str_count = temp_count;
        }
    }

    /* new_str 与 old_str 长度比较情况。 */
    const int new_cmp_old = (new_str_len > old_str_len) ? 1 : ((new_str_len < old_str_len) ? -1 : 0);
    /* new_str_len 与 old_str_len 差的绝对值。 */
    const size_t len_diff = (new_cmp_old > 0) ? (new_str_len - old_str_len) : (old_str_len - new_str_len);

    /* 替换完成后新的字符串长度。 */
    size_t new_len;

    /* 扩容缓冲区。 */
    if (new_cmp_old > 0)
    {
        /* 新增的长度。 */
        size_t increased_length;

        if (
            /* 安全计算 size_t 乘法（len_diff * old_str_count），防止溢出。 */
            !safe_size_t_mul(len_diff, old_str_count, &increased_length) ||
            /* 安全计算 size_t 加法（dstr->len + increased_length），防止溢出。 */
            !safe_size_t_add(dstr->len, increased_length, &new_len) ||
            /* 安全计算 size_t 加法（new_len + 1），防止溢出。 */
            !safe_size_t_add(new_len, 1, NULL) ||
            /* 调整容量。 */
            !resize_capacity_dynamic(dstr, new_len + 1))
        {
            free(indexes);
            return DSTR_MEMORY_ALLOC_FAILED;
        }
    }

    /* 此时，如果 indexes 是空指针，则放弃空间换时间，回退到最坏路径。*/
    if (indexes == NULL)
    {
        goto worst_case;
    }

    /**
     * 执行替换。
     * 当 new_str_len > old_str_len 时，需要先搬移后面的数据，防止覆盖，
     * 即遍历时要从大索引到小索引，而查找函数写入索引的大小顺序取决于查找方向，
     * 当从前往后查找时，索引按从小到大顺序写入，反之从大到小的顺序写入；
     * 因此，当 new_str_len > old_str_len 时，如果索引是按从大到小顺序写入的，
     * 即 backward 为 true，那么就要正向遍历数组，
     * 由此总结规律，当（new_str_len > old_str_len）与（backward）同为真时，
     * 正向遍历数组，否则逆向遍历。
     */
    if ((new_cmp_old > 0) == backward)
    {
        for (size_t i = 0; i < old_str_count; ++i)
        {
            /* 如果 new_str 与 old_str 长度不等，则需移动数据。 */
            if (new_cmp_old != 0)
            {
                /* 被搬移数据的起始索引。 */
                const size_t move_start = indexes[i] + old_str_len;
                /**
                 * 被搬移数据的结束索引。
                 * 如果此块被搬移数据，是最后一块（从前往后），
                 * 那么值应该为 dstr->len，否则就是后一次 old_str 出现位置。
                 */
                const size_t move_end = backward ? ((i == 0) ? dstr->len : indexes[i - 1])
                                                 : ((i == (old_str_count - 1)) ? dstr->len : indexes[i + 1]);
                /* 被搬移数据的长度。 */
                const size_t move_len = move_end - move_start;

                if (move_len > 0)
                {
                    const size_t move_step = len_diff * (i + 1);

                    char *const move_src = dstr->data + move_start;
                    char *const move_dest = (new_cmp_old > 0) ? (move_src + move_step) : (move_src - move_step);

                    memmove(move_dest, move_src, move_len);
                }
            }

            /* 拷贝新数据。 */
            if (new_str_len > 0)
            {
                const size_t copy_target = indexes[i] - len_diff * i;

                memcpy(dstr->data + copy_target, new_str, new_str_len);
            }
        }
    }
    else
    {
        for (size_t i = old_str_count; i > 0; --i)
        {
            const size_t cur = i - 1;

            /* 如果 new_str 与 old_str 长度不等，则需移动数据。 */
            if (new_cmp_old != 0)
            {
                const size_t move_start = indexes[cur] + old_str_len;
                const size_t move_end = backward ? ((cur == 0) ? dstr->len : indexes[cur - 1])
                                                 : ((i == old_str_count) ? dstr->len : indexes[i]);
                const size_t move_len = move_end - move_start;

                if (move_len > 0)
                {
                    const size_t move_step = len_diff * i;

                    char *const move_src = dstr->data + move_start;
                    char *const move_dest = (new_cmp_old > 0) ? (move_src + move_step) : (move_src - move_step);

                    memmove(move_dest, move_src, move_len);
                }
            }

            /* 拷贝新数据。 */
            if (new_str_len > 0)
            {
                const size_t copy_target = indexes[cur] - len_diff * cur;

                memcpy(dstr->data + copy_target, new_str, new_str_len);
            }
        }
    }

    free(indexes);

    /* 长度有变化时，更新长度。 */
    if (new_cmp_old != 0)
    {
        /* 之前仅在 new_cmp_old > 0 分支计算了 new_len，因此此处进行计算。 */
        if (new_cmp_old < 0)
        {
            new_len = dstr->len - len_diff * old_str_count;
        }

        dstr->len = new_len;
    }

    goto end;

worst_case:
    {
        /**
         * 最坏路径：所有分配均失败，使用边查找边替换的算法（最笨算法），完全的时间换空间算法。
         * 边查找边替换：每找到一个匹配就立即执行 memmove + memcpy。
         */

        /* 用于迭代。 */
        char *p;
        /* 迭代中的边界指针，在边查找边替换的逻辑中，每次迭代都有可能改变其值。 */
        char *end = dstr->data + dstr->len - old_str_len + 1;
        /* 已替换的次数。用于在等于 n 时跳出循环。 */
        size_t replaced_count = 0;

        if (backward)
        {
            p = end;

            while (p > dstr->data)
            {
                /* 指向当前待比较数据指针。 */
                char *const cur = p - 1;

                if (memcmp(cur, old_str, old_str_len) == 0)
                {
                    /* 移动尾部数据。 */
                    if (new_cmp_old != 0)
                    {
                        const char *const move_src = cur + old_str_len;

                        if (move_src < end)
                        {
                            memmove(cur + new_str_len, move_src, dstr->data + dstr->len - move_src);
                        }
                    }

                    /* 拷贝新数据。 */
                    if (new_str_len > 0)
                    {
                        memcpy(cur, new_str, new_str_len);
                    }

                    /* 更新 dstr->len。 */
                    if (new_cmp_old < 0)
                    {
                        dstr->len -= len_diff;
                    }
                    else
                    {
                        dstr->len += len_diff;
                    }

                    /* 如果替换足够次数则跳出循环。 */
                    if (++replaced_count == n)
                    {
                        break;
                    }

                    /* 如果 (p - dstr->data) > old_str_len 则继续迭代，否则跳出。 */
                    if ((size_t)(p - dstr->data) > old_str_len)
                    {
                        p -= old_str_len;
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    --p;
                }
            }
        }
        else
        {
            p = dstr->data;

            while (p < end)
            {
                if (memcmp(p, old_str, old_str_len) == 0)
                {
                    /* 移动尾部数据。 */
                    if (new_cmp_old != 0)
                    {
                        const char *const move_src = p + old_str_len;

                        if (move_src < end)
                        {
                            memmove(p + new_str_len, move_src, dstr->data + dstr->len - move_src);
                        }
                    }

                    /* 拷贝新数据。 */
                    if (new_str_len > 0)
                    {
                        memcpy(p, new_str, new_str_len);
                    }

                    /* 更新 dstr->len。 */
                    if (new_cmp_old < 0)
                    {
                        dstr->len -= len_diff;
                    }
                    else
                    {
                        dstr->len += len_diff;
                    }

                    /* 如果替换足够次数则跳出循环。 */
                    if (++replaced_count == n)
                    {
                        break;
                    }

                    /* 更新 end 指针。 */
                    if (new_cmp_old < 0)
                    {
                        end -= len_diff;
                    }
                    else
                    {
                        end += len_diff;
                    }

                    p += new_str_len;
                }
                else
                {
                    ++p;
                }
            }
        }
    }

end: /* 收尾工作。 */
    /* 长度变短，尝试缩容。 */
    if (new_cmp_old < 0)
    {
        resize_capacity_dynamic(dstr, (dstr->len > 0) ? (dstr->len + 1) : 0);
    }

    if (dstr->data != NULL)
    {
        dstr->data[dstr->len] = '\0';
    }

    return DSTR_SUCCESS;
}

/**
 * @brief
 * 将一个「C 字符串」按指定分隔符分割为多个「动态字符串」。
 *
 * @param cstr
 * 目标「C 字符串」的指针。
 * @param cstr_len
 * 目标「C 字符串」的长度。
 * @param separator
 * 分隔「C 字符串」的指针。
 * @param separator_len
 * 分隔「C 字符串」的长度。
 * @param out_dstr_count
 * 存储分割后「动态字符串」个数的 size_t 变量的指针。
 *
 * @return
 * 分割后「动态字符串」指针数组的指针。
 * 如果分割失败则返回空指针。
 * 注意：数组中的某些元素可能为空指针，表示该部分为空字符串。
 */
static struct dynamic_string **split_str(const char *const cstr, const size_t cstr_len, const char *const separator,
                                         const size_t separator_len, size_t *const out_dstr_count)
{
    /* 第一遍查找，统计 separator 在 cstr 中，出现的次数。 */
    const size_t separator_count = find_str(cstr, cstr_len, separator, separator_len, NULL, NULL, 0, NULL, NULL);

    /* 如果 separator 在 cstr 中一次都没有出现，则直接返回空指针。  */
    if (separator_count == 0)
    {
        return NULL;
    }

    /* dstrs 数组元素个数（分隔后的子串个数）。 */
    const size_t dstr_count = separator_count + 1;

    /* 动态分配 struct dynamic_string * 数组（比 separator_count 多 1）。 */
    struct dynamic_string **dstrs;
    if (!safe_size_t_mul(dstr_count, sizeof(struct dynamic_string *), NULL) ||
        (dstrs = malloc(dstr_count * sizeof(struct dynamic_string *))) == NULL)
    {
        return NULL;
    }

    /* 执行一次边查找边创建。 */
    /* 指针变量，用于迭代。 */
    const char *p = cstr;
    /* 指针常量，指向查找区间的后一个位置。用于迭代边界。 */
    const char *const end = p + cstr_len - separator_len + 1;
    /* 找到的次数。 */
    size_t find_count = 0;
    /* 记录上一次找到的位置指针。 */
    const char *last = p;

    while (p < end)
    {
        if (memcmp(p, separator, separator_len) == 0)
        {
        create_sub:
            /* 以当前区间子串创建动态字符串。 */
            const char *const sub_start = (find_count > 0) ? last + separator_len : cstr;
            const char *const sub_end = (find_count == separator_count) ? cstr + cstr_len : p;
            const size_t sub_len = sub_end - sub_start;

            if (sub_len > 0)
            {
                dstrs[find_count] = create_dstr(sub_start, sub_len);

                if (dstrs[find_count] == NULL)
                {
                    /* 创建失败，释放之前已创建的「动态字符串」。 */
                    for (size_t j = 0; j < find_count; ++j)
                    {
                        if (dstrs[j] != NULL)
                        {
                            free(dstrs[j]->data);
                            free(dstrs[j]);
                        }
                    }
                    free(dstrs);
                    return NULL;
                }
            }
            else
            {
                dstrs[find_count] = NULL;
            }

            ++find_count;
            last = p;

            if (find_count == separator_count)
            {
                goto create_sub;
            }

            if (find_count > separator_count)
            {
                break;
            }

            p += separator_len;
        }
        else
        {
            ++p;
        }
    }

    *out_dstr_count = dstr_count;
    return dstrs;
}

/*----------------------------------------------------------------------------------------------------------------------
 * 接口函数定义
 *--------------------------------------------------------------------------------------------------------------------*/

/* 创建与销毁。 */

/* 创建一个「动态字符串」。 */
struct dynamic_string *dstr_create(const char *const cstr)
{
    /* 委托 create_dstr() 函数，创建新「动态字符串」，并使用 cstr 初始化。 */
    return create_dstr(cstr, (cstr != NULL && cstr[0] != '\0') ? strlen(cstr) : 0);
}

/* 销毁一个「动态字符串」。 */
void dstr_destroy(struct dynamic_string *const dstr)
{
    /* 参数检查。 */
    if (dstr == NULL)
    {
        return;
    }

    if (dstr->data != dstr->storage.sso_buf)
    {
        free(dstr->data);
    }

    free(dstr);
}

/* 克隆一个「动态字符串」。 */
struct dynamic_string *dstr_clone(const struct dynamic_string *const dstr)
{
    /* 委托 create_dstr() 函数，创建新「动态字符串」，并使用 dstr 初始化。 */
    return (dstr != NULL && dstr->len > 0) ? create_dstr(dstr->data, dstr->len) : create_dstr(NULL, 0);
}

/* 提取一个「C 字符串」的子串为一个新的「动态字符串」。 */
struct dynamic_string *dstr_sub_cstr(const char *const cstr, const size_t sub_start, const size_t sub_length)
{
    /* 参数检查。 */
    if (cstr == NULL || cstr[0] == '\0')
    {
        /* 委托 create_dstr() 函数，创建空的新「动态字符串」。 */
        return create_dstr(NULL, 0);
    }

    /* 越界检查。 */
    const size_t cstr_len = strlen(cstr);
    if (sub_start >= cstr_len || safe_size_add(sub_start, sub_length, NULL) != SAFE_CALC_OK ||
        sub_start + sub_length > cstr_len)
    {
        return NULL;
    }

    /* 委托 create_dstr() 函数，创建新「动态字符串」，并使用 cstr 子串初始化。 */
    return create_dstr(cstr + sub_start, (sub_length > 0) ? sub_length : (cstr_len - sub_start));
}

/* 提取一个「动态字符串」的子串为一个新的「动态字符串」。 */
struct dynamic_string *dstr_sub(const struct dynamic_string *const dstr, const size_t sub_start,
                                const size_t sub_length)
{
    /* 参数检查。 */
    if (dstr == NULL || dstr->len == 0)
    {
        /* 委托 create_dstr() 函数，创建空的新「动态字符串」。 */
        return create_dstr(NULL, 0);
    }

    /* 越界检查。 */
    if (sub_start >= dstr->len || safe_size_add(sub_start, sub_length, NULL) != SAFE_CALC_OK ||
        sub_start + sub_length > dstr->len)
    {
        return NULL;
    }

    /* 委托 create_dstr() 函数，创建新「动态字符串」，并使用 dstr 子串初始化。 */
    return create_dstr(dstr->data + sub_start, (sub_length > 0) ? sub_length : (dstr->len - sub_start));
}

/* 格式化创建一个「动态字符串」。 */
struct dynamic_string *dstr_create_format(const char *const format, ...)
{
    /* 委托 create_dstr() 函数，创建空的新「动态字符串」。 */
    struct dynamic_string *const new_dstr = create_dstr(NULL, 0);
    if (new_dstr == NULL)
    {
        return NULL;
    }

    /* 参数检查。 */
    /**
     * 如果 format 为空指针或指向空「C 字符串」，则创建空的「动态字符串」。
     * 而在此函数中，不论 format 情况如何，都要经历一次 create_dstr() 创建空「动态字符串」的过程，
     * 因此先执行 create_dstr() 操作，后判断，如果 format 为空，则直接返回创建好的空「动态字符串」。
     */
    if (format == NULL || format[0] == '\0')
    {
        return new_dstr;
    }

    /* 变参列表变量。 */
    va_list args;

    va_start(args, format);
    /* 委托 insert_str_format() 函数，不删除，格式化插入。 */
    const dstr_status_t result = insert_str_format(new_dstr, 0, 0, format, args);
    va_end(args);

    if (result != DSTR_SUCCESS)
    {
        free(new_dstr);
        return NULL;
    }

    return new_dstr;
}

/* 格式化创建一个「动态字符串」（va_list 版本）。 */
struct dynamic_string *dstr_create_vformat(const char *const format, va_list args)
{
    /* 委托 create_dstr() 函数，创建空的新「动态字符串」。 */
    struct dynamic_string *const new_dstr = create_dstr(NULL, 0);
    if (new_dstr == NULL)
    {
        return NULL;
    }

    /* 参数检查。 */
    /**
     * 如果 format 为空指针或指向空「C 字符串」，则创建空的「动态字符串」。
     * 而在此函数中，不论 format 情况如何，都要经历一次 create_dstr() 创建空「动态字符串」的过程，
     * 因此先执行 create_dstr() 操作，后判断，如果 format 为空，则直接返回创建好的空「动态字符串」。
     */
    if (format == NULL || format[0] == '\0')
    {
        return new_dstr;
    }

    /* 委托 insert_str_format() 函数，不删除，格式化插入。 */
    const dstr_status_t result = insert_str_format(new_dstr, 0, 0, format, args);
    if (result != DSTR_SUCCESS)
    {
        free(new_dstr);
        return NULL;
    }

    return new_dstr;
}

/* 属性获取与设置。 */

/* 获取一个「动态字符串」的内部「C 字符串」指针。 */
const char *dstr_cstr(const struct dynamic_string *const dstr)
{
    return (dstr != NULL) ? dstr->data : NULL;
}

/* 获取一个「动态字符串」的长度。 */
size_t dstr_length(const struct dynamic_string *const dstr)
{
    return (dstr != NULL) ? dstr->len : 0;
}

/* 判断一个「动态字符串」是否是空「动态字符串」。 */
bool dstr_is_empty(const struct dynamic_string *const dstr)
{
    return (dstr != NULL) ? (dstr->len == 0) : true;
}

/* 获取一个「动态字符串」的容量。 */
size_t dstr_capacity(const struct dynamic_string *const dstr)
{
    return (dstr != NULL)
               ? ((dstr->data == dstr->storage.sso_buf) ? DYNAMIC_STRING_SSO_BUF_SIZE : dstr->storage.heap_info.cap)
               : 0;
}

/* 设置一个「动态字符串」的容量。 */
dstr_status_t dstr_set_capacity(struct dynamic_string *const dstr, size_t new_capacity)
{
    /* 参数检查。 */
    if (dstr == NULL)
    {
        return DSTR_INVALID_ARGUMENT;
    }

    /* 委托 resize_capacity() 函数调整容量。 */
    if (!resize_capacity(dstr, new_capacity, false))
    {
        return DSTR_MEMORY_ALLOC_FAILED;
    }

    /* resize_capacity() 函数如果调用成功，目前的 dstr 有两种情况，取决于 new_capacity 的值。
     * 如果 new_capacity ≤ 栈缓冲区大小，那么目前一定是栈缓冲区模式，否则一定是堆缓冲区模式。
     */
    /* 如果是堆缓冲区模式，则更新保底容量。 */
    if (dstr->data != dstr->storage.sso_buf)
    {
        dstr->storage.heap_info.min_cap = new_capacity;
    }
    /* 否则，是栈缓冲区模式，说明 new_capacity <= 栈缓冲区大小，
     * 此时就要更新 new_capacity 的值为栈缓冲区大小，以使得 new_capacity 的值不小于栈缓冲区大小，
     * 避免后续截断字符串时，进行大量判断。
     */
    else
    {
        new_capacity = DYNAMIC_STRING_SSO_BUF_SIZE;
    }

    /* 如果 new_capacity <= 当前长度，则截断字符串。
     * new_capacity > 栈缓冲区大小时，无条件执行截断：长度更新为 new_capacity - 1，数据于新长度处补 '\0'；
     * new_capacity <= 栈缓冲区大小时，长度应最多截断到栈缓冲区大小 - 1，因为理论上，容量下限为栈缓冲区的大小。
     * 为避免此处，进行这些关于 new_capacity 的判断，在这之前确保 new_capacity 的值 >= 栈缓冲区大小。
     */
    if (new_capacity <= dstr->len)
    {
        dstr->len = new_capacity - 1;
        dstr->data[dstr->len] = '\0';
    }

    return DSTR_SUCCESS;
}

/* 调整一个「动态字符串」的容量到刚合适。 */
void dstr_shrink_to_fit(struct dynamic_string *const dstr)
{
    /* 参数检查。 */
    if (dstr == NULL)
    {
        return;
    }

    /* 委托 resize_capacity() 函数调整容量。 */
    resize_capacity(dstr, dstr->len + 1, false);

    /* 如果是堆缓冲区，则将保底容量归 0。 */
    if (dstr->data != dstr->storage.sso_buf)
    {
        dstr->storage.heap_info.min_cap = 0;
    }
}

/* 内容编辑。 */

/* 复制一个「C 字符串」到一个「动态字符串」。 */
dstr_status_t dstr_cpy_cstr(struct dynamic_string *const dest, const char *const src)
{
    /* 参数检查。 */
    if (dest == NULL)
    {
        return DSTR_INVALID_ARGUMENT;
    }

    /* 委托 insert_str() 函数，删除 dest 全部，插入 src 全部。 */
    return insert_str(dest, 0, dest->len, src, (src != NULL && src[0] != '\0') ? strlen(src) : 0);
}

/* 复制一个「动态字符串」到另一个「动态字符串」。 */
dstr_status_t dstr_cpy(struct dynamic_string *const dest, const struct dynamic_string *const src)
{
    /* 参数检查。 */
    if (dest == NULL)
    {
        return DSTR_INVALID_ARGUMENT;
    }

    /* 委托 insert_str() 函数，删除 dest 全部，插入 src 全部。 */
    return (src != NULL) ? insert_str(dest, 0, dest->len, src->data, src->len)
                         : insert_str(dest, 0, dest->len, NULL, 0);
}

/* 复制一个「C 字符串」的子串到一个「动态字符串」。 */
dstr_status_t dstr_cpy_sub_cstr(struct dynamic_string *const dest, const char *const src, const size_t sub_start,
                                const size_t sub_length)
{
    /* 参数检查。 */
    if (dest == NULL)
    {
        return DSTR_INVALID_ARGUMENT;
    }

    /* src 为空指针或指向空字符串，均视为复制空字符串。 */
    if (src == NULL || src[0] == '\0')
    {
        /* 委托 insert_str() 函数，删除 dest 全部，不插入。 */
        return insert_str(dest, 0, dest->len, NULL, 0);
    }

    /* 越界检查。 */
    const size_t src_len = strlen(src);
    if (sub_start >= src_len || safe_size_add(sub_start, sub_length, NULL) != SAFE_CALC_OK ||
        sub_start + sub_length > src_len)
    {
        return DSTR_INVALID_ARGUMENT;
    }

    /* 委托 insert_str() 函数，删除 dest 全部，插入 src 子串。 */
    return insert_str(dest, 0, dest->len, src + sub_start, (sub_length > 0) ? sub_length : (src_len - sub_start));
}

/* 复制一个「动态字符串」的子串到另一个「动态字符串」。 */
dstr_status_t dstr_cpy_sub(struct dynamic_string *const dest, const struct dynamic_string *const src,
                           const size_t sub_start, const size_t sub_length)
{
    /* 参数检查。 */
    if (dest == NULL)
    {
        return DSTR_INVALID_ARGUMENT;
    }

    /* src 为空指针或指向空字符串，均视为复制空字符串。 */
    if (src == NULL || src->len == 0)
    {
        /* 委托 insert_str() 函数，删除 dest 全部，不插入。 */
        return insert_str(dest, 0, dest->len, NULL, 0);
    }

    /* 越界检查。 */
    if (sub_start >= src->len || safe_size_add(sub_start, sub_length, NULL) != SAFE_CALC_OK ||
        sub_start + sub_length > src->len)
    {
        return DSTR_INVALID_ARGUMENT;
    }

    /* 委托 insert_str() 函数，删除 dest 全部，插入 src 子串。 */
    return insert_str(dest, 0, dest->len, src->data + sub_start,
                      (sub_length > 0) ? sub_length : (src->len - sub_start));
}

/* 格式化复制一个字符串到一个「动态字符串」。 */
dstr_status_t dstr_cpy_format(struct dynamic_string *const dest, const char *const format, ...)
{
    /* 参数检查。 */
    if (dest == NULL)
    {
        return DSTR_INVALID_ARGUMENT;
    }

    /* format 为空指针或指向空字符串，均视为复制空字符串。 */
    if (format == NULL || format[0] == '\0')
    {
        /* 委托 insert_str() 函数，删除 dest 全部，不插入。 */
        return insert_str(dest, 0, dest->len, NULL, 0);
    }

    /* 变参列表变量。 */
    va_list args;

    va_start(args, format);
    /* 委托 insert_str_format() 函数，删除 dest 全部，格式化插入。 */
    const dstr_status_t result = insert_str_format(dest, 0, dest->len, format, args);
    va_end(args);

    return result;
}

/* 格式化复制一个字符串到一个「动态字符串」（va_list 版本）。 */
dstr_status_t dstr_cpy_vformat(struct dynamic_string *const dest, const char *const format, va_list args)
{
    /* 参数检查。 */
    if (dest == NULL)
    {
        return DSTR_INVALID_ARGUMENT;
    }

    /* format 为空指针或指向空字符串，均视为复制空字符串。 */
    if (format == NULL || format[0] == '\0')
    {
        /* 委托 insert_str() 函数，删除 dest 全部，不插入。 */
        return insert_str(dest, 0, dest->len, NULL, 0);
    }

    /* 委托 insert_str_format() 函数，删除 dest 全部，格式化插入。 */
    return insert_str_format(dest, 0, dest->len, format, args);
}

/* 追加一个「C 字符串」到一个「动态字符串」。 */
dstr_status_t dstr_cat_cstr(struct dynamic_string *const dest, const char *const src)
{
    /* 参数检查。 */
    if (dest == NULL)
    {
        return DSTR_INVALID_ARGUMENT;
    }

    /* 委托 insert_str() 函数，不删除，尾部插入 src 全部。 */
    return (src != NULL && src[0] != '\0') ? insert_str(dest, dest->len, 0, src, strlen(src)) : DSTR_SUCCESS;
}

/* 追加一个「动态字符串」到另一个「动态字符串」。 */
dstr_status_t dstr_cat(struct dynamic_string *const dest, const struct dynamic_string *const src)
{
    /* 参数检查。 */
    if (dest == NULL)
    {
        return DSTR_INVALID_ARGUMENT;
    }

    /* 委托 insert_str() 函数，不删除，尾部插入 src 全部。 */
    return (src != NULL && src->len > 0) ? insert_str(dest, dest->len, 0, src->data, src->len) : DSTR_SUCCESS;
}

/* 追加一个「C 字符串」的子串到一个「动态字符串」。 */
dstr_status_t dstr_cat_sub_cstr(struct dynamic_string *const dest, const char *const src, const size_t sub_start,
                                const size_t sub_length)
{
    /* 参数检查。 */
    if (dest == NULL)
    {
        return DSTR_INVALID_ARGUMENT;
    }

    /* src 为空指针或指向空字符串，均视为追加空字符串。 */
    /* 不删除，不插入，直接返回，无须委托。 */
    if (src == NULL || src[0] == '\0')
    {
        return DSTR_SUCCESS;
    }

    /* 越界检查。 */
    const size_t src_len = strlen(src);
    if (sub_start >= src_len || safe_size_add(sub_start, sub_length, NULL) != SAFE_CALC_OK ||
        sub_start + sub_length > src_len)
    {
        return DSTR_INVALID_ARGUMENT;
    }

    return insert_str(dest, dest->len, 0, src + sub_start, (sub_length > 0) ? sub_length : (src_len - sub_start));
}

/* 追加一个「动态字符串」的子串到另一个「动态字符串」。 */
dstr_status_t dstr_cat_sub(struct dynamic_string *const dest, const struct dynamic_string *const src,
                           const size_t sub_start, const size_t sub_length)
{
    /* 参数检查。 */
    if (dest == NULL)
    {
        return DSTR_INVALID_ARGUMENT;
    }

    /* src 为空指针或指向空字符串，均视为追加空字符串。 */
    /* 不删除，不插入，直接返回，无须委托。 */
    if (src == NULL || src->len == 0)
    {
        return DSTR_SUCCESS;
    }

    /* 越界检查。 */
    if (sub_start >= src->len || safe_size_add(sub_start, sub_length, NULL) != SAFE_CALC_OK ||
        sub_start + sub_length > src->len)
    {
        return DSTR_INVALID_ARGUMENT;
    }

    /* 委托 insert_str() 函数，不删除，尾部插入 src 子串。 */
    return insert_str(dest, dest->len, 0, src->data + sub_start,
                      (sub_length > 0) ? sub_length : (src->len - sub_start));
}

/* 格式化追加一个字符串到一个「动态字符串」。 */
dstr_status_t dstr_cat_format(struct dynamic_string *const dest, const char *const format, ...)
{
    /* 参数检查。 */
    if (dest == NULL)
    {
        return DSTR_INVALID_ARGUMENT;
    }

    /* format 为空指针或指向空字符串，均视为追加空字符串。 */
    /* 不删除，不插入，直接返回，无须委托。 */
    if (format == NULL || format[0] == '\0')
    {
        return DSTR_SUCCESS;
    }

    /* 变参列表变量。 */
    va_list args;

    va_start(args, format);
    /* 委托 insert_str_format() 函数，不删除，尾部格式化插入。 */
    const dstr_status_t result = insert_str_format(dest, dest->len, 0, format, args);
    va_end(args);

    return result;
}

/* 格式化追加一个字符串到一个「动态字符串」（va_list 版本）。 */
dstr_status_t dstr_cat_vformat(struct dynamic_string *const dest, const char *const format, va_list args)
{
    /* 参数检查。 */
    if (dest == NULL)
    {
        return DSTR_INVALID_ARGUMENT;
    }

    /* format 为空指针或指向空字符串，均视为追加空字符串。 */
    /* 不删除，不插入，直接返回，无须委托。 */
    if (format == NULL || format[0] == '\0')
    {
        return DSTR_SUCCESS;
    }

    /* 委托 insert_str_format() 函数，不删除，尾部格式化插入。 */
    return insert_str_format(dest, dest->len, 0, format, args);
}

/* 插入一个「C 字符串」到一个「动态字符串」。 */
dstr_status_t dstr_insert_cstr(struct dynamic_string *const dest, const size_t index, const char *const src)
{
    /* 参数检查。 */
    if (dest == NULL || index > dest->len)
    {
        return DSTR_INVALID_ARGUMENT;
    }

    /* 委托 insert_str() 函数，不删除，指定位置插入 src 全部。 */
    return (src != NULL && src[0] != '\0') ? insert_str(dest, index, 0, src, strlen(src)) : DSTR_SUCCESS;
}

/* 插入一个「动态字符串」到另一个「动态字符串」。 */
dstr_status_t dstr_insert(struct dynamic_string *const dest, const size_t index, const struct dynamic_string *const src)
{
    /* 参数检查。 */
    if (dest == NULL || index > dest->len)
    {
        return DSTR_INVALID_ARGUMENT;
    }

    /* 委托 insert_str() 函数，不删除，指定位置插入 src 全部。 */
    return (src != NULL && src->len > 0) ? insert_str(dest, index, 0, src->data, src->len) : DSTR_SUCCESS;
}

/* 插入一个「C 字符串」的子串到一个「动态字符串」。 */
dstr_status_t dstr_insert_sub_cstr(struct dynamic_string *const dest, const size_t index, const char *const src,
                                   const size_t sub_start, const size_t sub_length)
{
    /* 参数检查。 */
    if (dest == NULL || index > dest->len)
    {
        return DSTR_INVALID_ARGUMENT;
    }

    /* src 为空指针或指向空字符串，均视为插入空字符串。 */
    /* 不删除，不插入，直接返回，无须委托。 */
    if (src == NULL || src[0] == '\0')
    {
        return DSTR_SUCCESS;
    }

    /* 越界检查。 */
    const size_t src_len = strlen(src);
    if (sub_start >= src_len || safe_size_add(sub_start, sub_length, NULL) != SAFE_CALC_OK ||
        sub_start + sub_length > src_len)
    {
        return DSTR_INVALID_ARGUMENT;
    }

    /* 委托 insert_str() 函数，不删除，指定位置插入 src 子串。 */
    return insert_str(dest, index, 0, src + sub_start, (sub_length > 0) ? sub_length : (src_len - sub_start));
}

/* 插入一个「动态字符串」的子串到另一个「动态字符串」。 */
dstr_status_t dstr_insert_sub(struct dynamic_string *const dest, const size_t index,
                              const struct dynamic_string *const src, const size_t sub_start, const size_t sub_length)
{
    /* 参数检查。 */
    if (dest == NULL || index > dest->len)
    {
        return DSTR_INVALID_ARGUMENT;
    }

    /* src 为空指针或指向空字符串，均视为插入空字符串。 */
    /* 不删除，不插入，直接返回，无须委托。 */
    if (src == NULL || src->len == 0)
    {
        return DSTR_SUCCESS;
    }

    /* 越界检查。 */
    if (sub_start >= src->len || safe_size_add(sub_start, sub_length, NULL) != SAFE_CALC_OK ||
        sub_start + sub_length > src->len)
    {
        return DSTR_INVALID_ARGUMENT;
    }

    /* 委托 insert_str() 函数，不删除，指定位置插入 src 子串。 */
    return insert_str(dest, index, 0, src->data + sub_start, (sub_length > 0) ? sub_length : (src->len - sub_start));
}

/* 格式化插入一个字符串到一个「动态字符串」。 */
dstr_status_t dstr_insert_format(struct dynamic_string *const dest, const size_t index, const char *const format, ...)
{
    /* 参数检查。 */
    if (dest == NULL || index > dest->len)
    {
        return DSTR_INVALID_ARGUMENT;
    }

    /* format 为空指针或指向空字符串，均视为插入空字符串。 */
    /* 不删除，不插入，直接返回，无须委托。 */
    if (format == NULL || format[0] == '\0')
    {
        return DSTR_SUCCESS;
    }

    /* 变参列表变量。 */
    va_list args;

    va_start(args, format);
    /* 委托 insert_str_format() 函数，不删除，指定位置格式化插入。 */
    const dstr_status_t result = insert_str_format(dest, index, 0, format, args);
    va_end(args);

    return result;
}

/* 格式化插入一个字符串到一个「动态字符串」（va_list 版本）。 */
dstr_status_t dstr_insert_vformat(struct dynamic_string *const dest, const size_t index, const char *const format,
                                  va_list args)
{
    /* 参数检查。 */
    if (dest == NULL || index > dest->len)
    {
        return DSTR_INVALID_ARGUMENT;
    }

    /* format 为空指针或指向空字符串，均视为插入空字符串。 */
    /* 不删除，不插入，直接返回，无须委托。 */
    if (format == NULL || format[0] == '\0')
    {
        return DSTR_SUCCESS;
    }

    /* 委托 insert_str_format() 函数，不删除，指定位置格式化插入。 */
    return insert_str_format(dest, index, 0, format, args);
}

/* 清空一个「动态字符串」。 */
void dstr_clear(struct dynamic_string *const dstr)
{
    /* 参数检查。 */
    if (dstr == NULL)
    {
        return;
    }

    /* 长度归 0。 */
    dstr->len = 0;

    /* 补 '\0'。 */
    dstr->data[0] = '\0';
}

/* 删除一个「动态字符串」的子串。 */
void dstr_remove(struct dynamic_string *const dstr, const size_t sub_start, const size_t sub_length)
{
    /* 参数检查。 */
    if (dstr == NULL || sub_start >= dstr->len || safe_size_add(sub_start, sub_length, NULL) != SAFE_CALC_OK ||
        sub_start + sub_length > dstr->len)
    {
        return;
    }

    /* 委托 insert_str() 函数，只删除，不插入。 */
    insert_str(dstr, sub_start, (sub_length > 0) ? sub_length : (dstr->len - sub_start), NULL, 0);
}

/* 删除一个「动态字符串」首尾的空白字符或指定字符。 */
void dstr_trim(struct dynamic_string *const dstr, const char *const trim_chars)
{
    /* 如果 dstr 为空指针，或指向空「动态字符串」，则直接返回。 */
    if (dstr == NULL || dstr->len == 0)
    {
        return;
    }

    /* trim_chars 为空指针或指向空字符串时，均视为没有指定字符。 */
    const bool is_specified_trim_chars = (trim_chars != NULL && trim_chars[0] != '\0');

    /* 用于迭代和区间定位。 */
    const char *p = dstr->data;
    const char *q = p + dstr->len;

    /* 定位剩余区间。 */
    /* 指定字符分支。 */
    if (is_specified_trim_chars)
    {
        /* 使用查表法提升性能，用空间换时间。 */
        bool mask[UCHAR_MAX + 1] = {false};
        for (const char *c = trim_chars; *c != '\0'; ++c)
        {
            mask[(unsigned char)*c] = true;
        }

        /**
         * p 向后寻找第一个非待删除元素。
         * 结束后，p 指向第一个非待删除元素，或者结尾 '\0'。
         */
        while (mask[(unsigned char)*p])
        {
            ++p;
        }

        /**
         * q 向前寻找最后一个非待删除元素。
         * 结束后，q 指向最后一个非待删除元素的下一个位置，或者结尾 '\0'。。
         */
        while (q > p && mask[(unsigned char)*(q - 1)])
        {
            --q;
        }
    }
    else
    {
        /* 未指定字符分支。 */

        /**
         * p 向后寻找第一个非待删除元素。
         * 结束后，p 指向第一个非待删除元素，或者结尾 '\0'。
         */
        while (isspace((unsigned char)*p))
        {
            ++p;
        }

        /**
         * q 向前寻找最后一个非待删除元素。
         * 结束后，q 指向最后一个非待删除元素的下一个位置，或者结尾 '\0'。。
         */
        while (q > p && isspace((unsigned char)*(q - 1)))
        {
            --q;
        }
    }

    /* 收尾工作。 */
    /**
     * 计算剩余区间的长度。
     * q 一定大于等于 p，因此可放心执行减法。
     */
    const size_t new_len = q - p;
    /* 长度不变，提前返回。 */
    if (new_len == dstr->len)
    {
        return;
    }

    /* 新长度不为 0 时，执行‘可能的数据移动’与缩容。 */
    if (new_len > 0 && p > dstr->data)
    {
        memmove(dstr->data, p, new_len);
    }

    /* 更新长度。 */
    dstr->len = new_len;

    /* 长度减小，动态缩容。 */
    resize_capacity(dstr, new_len + 1, true);

    /* 补 '\0'。 */
    dstr->data[new_len] = '\0';
}

/* 关系判断与比较。 */

/* 判断一个「动态字符串」是否以指定「C 字符串」前缀开头。 */
bool dstr_starts_with_cstr(const struct dynamic_string *const dstr, const char *const prefix)
{
    /* 参数检查。 */
    if (dstr == NULL || dstr->len == 0 || prefix == NULL || prefix[0] == '\0')
    {
        return false;
    }

    /* 计算 prefix 长度。 */
    const size_t prefix_len = strlen(prefix);
    /* 如果 prefix 长度大于 dstr，则一定不是前缀，直接返回。 */
    if (prefix_len > dstr->len)
    {
        return false;
    }

    return (memcmp(dstr->data, prefix, prefix_len) == 0);
}

/* 判断一个「动态字符串」是否以指定「动态字符串」前缀开头。 */
bool dstr_starts_with(const struct dynamic_string *const dstr, const struct dynamic_string *const prefix)
{
    /* 参数检查。 */
    if (dstr == NULL || dstr->len == 0 || prefix == NULL || prefix->len == 0)
    {
        return false;
    }

    /* 如果 prefix 长度大于 dstr，则一定不是前缀，直接返回。 */
    if (prefix->len > dstr->len)
    {
        return false;
    }

    return (memcmp(dstr->data, prefix->data, prefix->len) == 0);
}

/* 判断一个「C 字符串」是否以指定「C 字符串」前缀开头。 */
bool cstr_starts_with(const char *const cstr, const char *const prefix)
{
    /* 参数检查。 */
    if (cstr == NULL || cstr[0] == '\0' || prefix == NULL || prefix[0] == '\0')
    {
        return false;
    }

    /* 计算 prefix 长度。 */
    const size_t prefix_len = strlen(prefix);

    return (strncmp(cstr, prefix, prefix_len) == 0);
}

/* 判断一个「动态字符串」是否以指定「C 字符串」后缀结尾。 */
bool dstr_ends_with_cstr(const struct dynamic_string *const dstr, const char *const suffix)
{
    /* 参数检查。 */
    if (dstr == NULL || dstr->len == 0 || suffix == NULL || suffix[0] == '\0')
    {
        return false;
    }

    /* 计算 suffix 长度。 */
    const size_t suffix_len = strlen(suffix);
    /* 如果 suffix 长度大于 dstr，则一定不是后缀，直接返回。 */
    if (suffix_len > dstr->len)
    {
        return false;
    }

    return (memcmp(dstr->data + dstr->len - suffix_len, suffix, suffix_len) == 0);
}

/* 判断一个「动态字符串」是否以指定「动态字符串」后缀结尾。 */
bool dstr_ends_with(const struct dynamic_string *const dstr, const struct dynamic_string *const suffix)
{
    /* 参数检查。 */
    if (dstr == NULL || dstr->len == 0 || suffix == NULL || suffix->len == 0)
    {
        return false;
    }

    /* 如果 suffix 长度大于 dstr，则一定不是后缀，直接返回。 */
    if (suffix->len > dstr->len)
    {
        return false;
    }

    return (memcmp(dstr->data + dstr->len - suffix->len, suffix->data, suffix->len) == 0);
}

/* 判断一个「C 字符串」是否以指定「C 字符串」后缀结尾。 */
bool cstr_ends_with(const char *const cstr, const char *const suffix)
{
    /* 参数检查。 */
    if (cstr == NULL || cstr[0] == '\0' || suffix == NULL || suffix[0] == '\0')
    {
        return false;
    }

    const size_t cstr_len = strlen(cstr);
    /* 计算 suffix 长度。 */
    const size_t suffix_len = strlen(suffix);
    /* 如果 suffix 长度大于 dstr，则一定不是后缀，直接返回。 */
    if (suffix_len > cstr_len)
    {
        return false;
    }

    return (memcmp(cstr + cstr_len - suffix_len, suffix, suffix_len) == 0);
}

/* 判断一个「动态字符串」是否包含指定子「C 字符串」。 */
bool dstr_contains_cstr(const struct dynamic_string *const dstr, const char *const sub)
{
    /* 遵循 C 标准规定：空字符串是任何字符串的子串。 */
    if (sub == NULL || sub[0] == '\0')
    {
        return true;
    }

    /**
     * 由于要访问 dstr->data，因此需要对 dstr 做非空指针检查；
     * 由于 strstr() 靠 '\0' 标识字符串结束，
     * 而仅在 dstr->len 不为 0 时，dstr->data 才保证以 '\0' 结尾，
     * 且 dstr->len 不为 0 时，dstr->data 保证不为空指针，
     * 因此需要且只需要对 dstr->len 做非 0 检查；
     *
     * 以上两检查合起来正好是对 dstr 做非「空字符串」检查，
     * 而当 sub 不为「空字符串」，而 dstr 为「空字符串」时，
     * dstr 一定不包含 sub，因此返回 false。
     */
    if (dstr == NULL || dstr->len == 0)
    {
        return false;
    }

    return (strstr(dstr->data, sub) != NULL);
}

/* 判断一个「动态字符串」是否包含指定子「动态字符串」。 */
bool dstr_contains(const struct dynamic_string *const dstr, const struct dynamic_string *const sub)
{
    /* 遵循 C 标准规定：空字符串是任何字符串的子串。 */
    if (sub == NULL || sub->len == 0)
    {
        return true;
    }

    /**
     * 由于要访问 dstr->data，因此需要对 dstr 做非空指针检查；
     * 由于 strstr() 靠 '\0' 标识字符串结束，
     * 而仅在 dstr->len 不为 0 时，dstr->data 才保证以 '\0' 结尾，
     * 且 dstr->len 不为 0 时，dstr->data 保证不为空指针，
     * 因此需要且只需要对 dstr->len 做非 0 检查；
     *
     * 以上两检查合起来正好是对 dstr 做非「空字符串」检查，
     * 而当 sub 不为「空字符串」，而 dstr 为「空字符串」时，
     * dstr 一定不包含 sub，因此返回 false。
     */
    if (dstr == NULL || dstr->len == 0)
    {
        return false;
    }

    return (strstr(dstr->data, sub->data) != NULL);
}

/* 判断一个「C 字符串」是否包含指定子「C 字符串」。 */
bool cstr_contains(const char *const cstr, const char *const sub)
{
    /* 遵循 C 标准规定：空字符串是任何字符串的子串。 */
    if (sub == NULL || sub[0] == '\0')
    {
        return true;
    }

    /**
     * 当 sub 不为「空字符串」，而 cstr 为「空字符串」时，
     * cstr 一定不包含 sub，因此返回 false。
     */
    if (cstr == NULL || cstr[0] == '\0')
    {
        return false;
    }

    return (strstr(cstr, sub) != NULL);
}

/* 判断一个「动态字符串」是否与一个「C 字符串」相等。 */
bool dstr_equals_cstr(const struct dynamic_string *const lhs, const char *const rhs)
{
    /* 参数检查。 */
    const int str_1_valid = (lhs && lhs->len) ? 1 : 0;
    const int str_2_valid = (rhs && rhs[0]) ? 1 : 0;

    /* 存在无效串时，需要提前返回。 */
    if (str_1_valid + str_2_valid < 2)
    {
        /* 都为无效串视为相等，否则视为不相等。 */
        return (str_1_valid == str_2_valid);
    }

    return (strcmp(lhs->data, rhs) == 0);
}

/* 判断两个「动态字符串」是否相等。 */
bool dstr_equals(const struct dynamic_string *const lhs, const struct dynamic_string *const rhs)
{
    /* 参数检查。 */
    const int str_1_valid = (lhs && lhs->len) ? 1 : 0;
    const int str_2_valid = (rhs && rhs->len) ? 1 : 0;

    /* 存在无效串时，需要提前返回。 */
    if (str_1_valid + str_2_valid < 2)
    {
        /* 都为无效串视为相等，否则视为不相等。 */
        return (str_1_valid == str_2_valid);
    }

    /* 长度不相等，则一定不相等。 */
    if (lhs->len != rhs->len)
    {
        return false;
    }

    return (memcmp(lhs->data, rhs->data, rhs->len) == 0);
}

/* 判断两个「C 字符串」是否相等。 */
bool cstr_equals(const char *const lhs, const char *const rhs)
{
    /* 参数检查。 */
    const int str_1_valid = (lhs && lhs[0]) ? 1 : 0;
    const int str_2_valid = (rhs && rhs[0]) ? 1 : 0;
    /* 存在无效串时，需要提前返回。 */
    if (str_1_valid + str_2_valid < 2)
    {
        /* 都为无效串视为相等，否则视为不相等。 */
        return (str_1_valid == str_2_valid);
    }

    return (strcmp(lhs, rhs) == 0);
}

/* 比较一个「动态字符串」与一个「C 字符串」。 */
int dstr_compare_cstr(const struct dynamic_string *const lhs, const char *const rhs)
{
    /* 参数检查。 */
    const int str_1_valid = (lhs && lhs->len) ? 1 : 0;
    const int str_2_valid = (rhs && rhs[0]) ? 1 : 0;

    /* 存在无效串时，需要提前返回。 */
    if (str_1_valid + str_2_valid < 2)
    {
        /**
         * 都为无效串视为相等，否则视为不相等，
         * 且有效者大于无效者。
         */
        return str_1_valid - str_2_valid;
    }

    return strcmp(lhs->data, rhs);
}

/* 比较两个「动态字符串」。 */
int dstr_compare(const struct dynamic_string *const lhs, const struct dynamic_string *const rhs)
{
    /* 参数检查。 */
    const int str_1_valid = (lhs && lhs->len) ? 1 : 0;
    const int str_2_valid = (rhs && rhs->len) ? 1 : 0;

    /* 存在无效串时，需要提前返回。 */
    if (str_1_valid + str_2_valid < 2)
    {
        /**
         * 都为无效串视为相等，否则视为不相等，
         * 且有效者大于无效者。
         */
        return str_1_valid - str_2_valid;
    }

    return strcmp(lhs->data, rhs->data);
}

/* 比较两个「C 字符串」。 */
int cstr_compare(const char *const lhs, const char *const rhs)
{
    /* 参数检查。 */
    const int str_1_valid = (lhs && lhs[0]) ? 1 : 0;
    const int str_2_valid = (rhs && rhs[0]) ? 1 : 0;

    /* 存在无效串时，需要提前返回。 */
    if (str_1_valid + str_2_valid < 2)
    {
        /**
         * 都为无效串视为相等，否则视为不相等，
         * 且有效者大于无效者。
         */
        return str_1_valid - str_2_valid;
    }

    return strcmp(lhs, rhs);
}

/* 查找、统计与替换。 */

/* 查找一个「动态字符串」中指定子「C 字符串」第一次出现的位置。 */
bool dstr_find_cstr(const struct dynamic_string *const dstr, const char *const sub, size_t *const out_index,
                    const dstr_direction_t direction)
{
    /* 参数合法性检查。 */
    if (dstr == NULL || dstr->len == 0 || sub == NULL || sub[0] == '\0')
    {
        return false;
    }

    /* 计算 sub 长度。 */
    const size_t sub_len = strlen(sub);
    /* 如果 sub 长度大于 dstr，则一定不存在，直接返回 false。 */
    if (sub_len > dstr->len)
    {
        return false;
    }

    /* 委托 find_str() 函数，查找 sub 第一次出现的位置。 */
    return (find_str(dstr->data, dstr->len, sub, sub_len, out_index, NULL, direction, 1) > 0);
}

/* 查找一个「动态字符串」中指定子「动态字符串」第一次出现的位置。 */
bool dstr_find(const struct dynamic_string *const dstr, const struct dynamic_string *const sub, size_t *const out_index,
               const dstr_direction_t direction)
{
    /* 参数合法性检查。 */
    if (dstr == NULL || dstr->len == 0 || sub == NULL || sub->len == 0)
    {
        return false;
    }

    /* 如果 sub 长度大于 dstr，则一定不存在，直接返回 false。 */
    if (sub->len > dstr->len)
    {
        return false;
    }

    /* 委托 find_str() 函数，查找 sub 第一次出现的位置。 */
    return (find_str(dstr->data, dstr->len, sub->data, sub->len, out_index, NULL, direction, 1) > 0);
}

/* 查找一个「C 字符串」中指定子「C 字符串」第一次出现的位置。 */
bool cstr_find(const char *const cstr, const char *const sub, size_t *const out_index, const dstr_direction_t direction)
{
    /* 参数合法性检查。 */
    if (cstr == NULL || cstr[0] == '\0' || sub == NULL || sub[0] == '\0')
    {
        return false;
    }

    /* 计算 cstr 长度。 */
    const size_t cstr_len = strlen(cstr);
    /* 计算 sub 长度。 */
    const size_t sub_len = strlen(sub);
    /* 如果 sub 长度大于 cstr，则一定不存在，直接返回 false。 */
    if (sub_len > cstr_len)
    {
        return false;
    }

    /* 委托 find_str() 函数，查找 sub 第一次出现的位置。 */
    return (find_str(cstr, cstr_len, sub, sub_len, out_index, NULL, direction, 1) > 0);
}

/* 查找一个「动态字符串」中指定子「C 字符串」第 n 次出现的位置。 */
bool dstr_find_nth_cstr(const struct dynamic_string *const dstr, const char *const sub, size_t *const out_index,
                        const dstr_direction_t direction, const size_t n)
{
    /* 参数合法性检查。 */
    if (dstr == NULL || dstr->len == 0 || sub == NULL || sub[0] == '\0')
    {
        return false;
    }

    /* 计算 sub 长度。 */
    const size_t sub_len = strlen(sub);
    /* 如果 sub 长度大于 dstr，则一定不存在，直接返回 false。 */
    if (sub_len > dstr->len)
    {
        return false;
    }

    /* 委托 find_str() 函数，查找 sub 第 n 次出现的位置。 */
    return (find_str(dstr->data, dstr->len, sub, sub_len, out_index, NULL, direction, n) > 0);
}

/* 查找一个「动态字符串」中指定子「动态字符串」第 n 次出现的位置。 */
bool dstr_find_nth(const struct dynamic_string *const dstr, const struct dynamic_string *const sub,
                   size_t *const out_index, const dstr_direction_t direction, const size_t n)
{
    /* 参数合法性检查。 */
    if (dstr == NULL || dstr->len == 0 || sub == NULL || sub->len == 0)
    {
        return false;
    }

    /* 如果 sub 长度大于 dstr，则一定不存在，直接返回 false。 */
    if (sub->len > dstr->len)
    {
        return false;
    }

    /* 委托 find_str() 函数，查找 sub 第 n 次出现的位置。 */
    return (find_str(dstr->data, dstr->len, sub->data, sub->len, out_index, NULL, direction, n) > 0);
}

/* 查找一个「C 字符串」中指定子「C 字符串」第 n 次出现的位置。 */
bool cstr_find_nth(const char *const cstr, const char *const sub, size_t *const out_index,
                   const dstr_direction_t direction, const size_t n)
{
    /* 参数合法性检查。 */
    if (cstr == NULL || cstr[0] == '\0' || sub == NULL || sub[0] == '\0')
    {
        return false;
    }

    /* 计算 cstr 长度。 */
    const size_t cstr_len = strlen(cstr);
    /* 计算 sub 长度。 */
    const size_t sub_len = strlen(sub);
    /* 如果 sub 长度大于 cstr，则一定不存在，直接返回 false。 */
    if (sub_len > cstr_len)
    {
        return false;
    }

    /* 委托 find_str() 函数，查找 sub 第 n 次出现的位置。 */
    return (find_str(cstr, cstr_len, sub, sub_len, out_index, NULL, direction, n) > 0);
}

/* 查找一个「动态字符串」中指定子「C 字符串」前 n 次出现的位置。 */
size_t dstr_find_indexes_cstr(const struct dynamic_string *const dstr, const char *const sub, size_t *const out_indexes,
                              const dstr_direction_t direction, const size_t n)
{
    /* 参数合法性检查。 */
    if (dstr == NULL || dstr->len == 0 || sub == NULL || sub[0] == '\0')
    {
        return 0;
    }

    /* 计算 sub 长度。 */
    const size_t sub_len = strlen(sub);
    /* 如果 sub 长度大于 dstr，则一定不存在，直接返回 false。 */
    if (sub_len > dstr->len)
    {
        return 0;
    }

    /* 委托 find_str() 函数，查找 sub 前 n 次出现的位置。 */
    return find_str(dstr->data, dstr->len, sub, sub_len, NULL, out_indexes, direction, n);
}

/* 查找一个「动态字符串」中指定子「动态字符串」前 n 次出现的位置。 */
size_t dstr_find_indexes(const struct dynamic_string *const dstr, const struct dynamic_string *const sub,
                         size_t *const out_indexes, const dstr_direction_t direction, const size_t n)
{
    /* 参数合法性检查。 */
    if (dstr == NULL || dstr->len == 0 || sub == NULL || sub->len == 0)
    {
        return 0;
    }

    /* 如果 sub 长度大于 dstr，则一定不存在，直接返回 false。 */
    if (sub->len > dstr->len)
    {
        return 0;
    }

    /* 委托 find_str() 函数，查找 sub 前 n 次出现的位置。 */
    return find_str(dstr->data, dstr->len, sub->data, sub->len, NULL, out_indexes, direction, n);
}

/* 查找一个「C 字符串」中指定子「C 字符串」前 n 次出现的位置。 */
size_t cstr_find_indexes(const char *const cstr, const char *const sub, size_t *const out_indexes,
                         const dstr_direction_t direction, const size_t n)
{
    /* 参数合法性检查。 */
    if (cstr == NULL || cstr[0] == '\0' || sub == NULL || sub[0] == '\0')
    {
        return 0;
    }

    /* 计算 cstr 长度。 */
    const size_t cstr_len = strlen(cstr);
    /* 计算 sub 长度。 */
    const size_t sub_len = strlen(sub);
    /* 如果 sub 长度大于 cstr，则一定不存在，直接返回 false。 */
    if (sub_len > cstr_len)
    {
        return 0;
    }

    /* 委托 find_str() 函数，查找 sub 前 n 次出现的位置。 */
    return find_str(cstr, cstr_len, sub, sub_len, NULL, out_indexes, direction, n);
}

/* 统计一个「动态字符串」中指定子「C 字符串」出现的次数。 */
size_t dstr_count_cstr(const struct dynamic_string *const dstr, const char *const sub)
{
    /* 参数合法性检查。 */
    if (dstr == NULL || dstr->len == 0 || sub == NULL || sub[0] == '\0')
    {
        return 0;
    }

    /* 计算 sub 长度。 */
    const size_t sub_len = strlen(sub);
    /* 如果 sub 长度大于 dstr，则一定不存在，直接返回 false。 */
    if (sub_len > dstr->len)
    {
        return 0;
    }

    /* 委托 find_str() 函数，统计 sub 出现的次数。 */
    return find_str(dstr->data, dstr->len, sub, sub_len, NULL, NULL, DSTR_DIR_FORWARD, 0);
}

/* 统计一个「动态字符串」中指定子「动态字符串」出现的次数。 */
size_t dstr_count(const struct dynamic_string *const dstr, const struct dynamic_string *const sub)
{
    /* 参数合法性检查。 */
    if (dstr == NULL || dstr->len == 0 || sub == NULL || sub->len == 0)
    {
        return 0;
    }

    /* 如果 sub 长度大于 dstr，则一定不存在，直接返回 false。 */
    if (sub->len > dstr->len)
    {
        return 0;
    }

    /* 委托 find_str() 函数，统计 sub 出现的次数。 */
    return find_str(dstr->data, dstr->len, sub->data, sub->len, NULL, NULL, DSTR_DIR_FORWARD, 0);
}

/* 统计一个「C 字符串」中指定子「C 字符串」出现的次数。 */
size_t cstr_count(const char *const cstr, const char *const sub)
{
    /* 参数合法性检查。 */
    if (cstr == NULL || cstr[0] == '\0' || sub == NULL || sub[0] == '\0')
    {
        return 0;
    }

    /* 计算 cstr 长度。 */
    const size_t cstr_len = strlen(cstr);
    /* 计算 sub 长度。 */
    const size_t sub_len = strlen(sub);
    /* 如果 sub 长度大于 cstr，则一定不存在，直接返回 false。 */
    if (sub_len > cstr_len)
    {
        return 0;
    }

    /* 委托 find_str() 函数，统计 sub 出现的次数。 */
    return find_str(cstr, cstr_len, sub, sub_len, NULL, NULL, DSTR_DIR_FORWARD, 0);
}

/* 替换一个「动态字符串」中指定旧「C 字符串」从左往右前 n 次为指定新「C 字符串」。 */
dstr_status_t dstr_replace_cstr(struct dynamic_string *const dstr, const char *const old_str, const char *const new_str,
                                dstr_direction_t direction, const size_t n)
{
    /* TODO: 待静态辅助函数按方向拆分后实现。 */
    (void)dstr;
    (void)old_str;
    (void)new_str;
    (void)n;
    return DSTR_INVALID_ARGUMENT;
}

/* 替换一个「动态字符串」中指定旧「动态字符串」从左往右前 n 次为指定新「动态字符串」。 */
dstr_status_t dstr_replace(struct dynamic_string *const dstr, const struct dynamic_string *const old_str,
                           const struct dynamic_string *const new_str, dstr_direction_t direction, const size_t n)
{
    /* TODO: 待静态辅助函数按方向拆分后实现。 */
    (void)dstr;
    (void)old_str;
    (void)new_str;
    (void)n;
    return DSTR_INVALID_ARGUMENT;
}

/* 替换一个「动态字符串」中指定旧「C 字符串」从左往右第 n 次为指定新「C 字符串」。 */
dstr_status_t dstr_replace_nth_cstr(struct dynamic_string *const dstr, const char *const old_str,
                                    const char *const new_str, dstr_direction_t direction, const size_t n)
{
    /* TODO: 待静态辅助函数按方向拆分后实现。 */
    (void)dstr;
    (void)old_str;
    (void)new_str;
    (void)n;
    return DSTR_INVALID_ARGUMENT;
}

/* 替换一个「动态字符串」中指定旧「动态字符串」从左往右第 n 次为指定新「动态字符串」。 */
dstr_status_t dstr_replace_nth(struct dynamic_string *const dstr, const struct dynamic_string *const old_str,
                               const struct dynamic_string *const new_str, dstr_direction_t direction, const size_t n)
{
    /* TODO: 待静态辅助函数按方向拆分后实现。 */
    (void)dstr;
    (void)old_str;
    (void)new_str;
    (void)n;
    return DSTR_INVALID_ARGUMENT;
}

/* 分隔与合并。 */

/* 分隔一个「C 字符串」为多个「动态字符串」。 */
struct dynamic_string **dstr_split_cstr(const char *const cstr, const char *const separator,
                                        size_t *const out_dstr_count)
{
    /* 参数合法性检查。 */
    if (cstr == NULL || cstr[0] == '\0' || separator == NULL || separator[0] == '\0' || out_dstr_count == NULL)
    {
        return NULL;
    }

    /* 计算 cstr 长度。 */
    const size_t cstr_len = strlen(cstr);
    /* 计算 separator 长度。 */
    const size_t separator_len = strlen(separator);

    /* 如果 separator 长度大于 cstr，则视为不合法参数，直接返回。  */
    if (separator_len > cstr_len)
    {
        return NULL;
    }

    /* 委托 split_str 函数，完成分隔。 */
    return split_str(cstr, cstr_len, separator, separator_len, out_dstr_count);
}

/* 分隔一个「动态字符串」为多个「动态字符串」。 */
struct dynamic_string **dstr_split(const struct dynamic_string *const dstr,
                                   const struct dynamic_string *const separator, size_t *const out_dstr_count)
{
    /* 参数合法性检查。 */
    if (dstr == NULL || dstr->len == 0 || separator == NULL || separator->len == 0 || out_dstr_count == NULL)
    {
        return NULL;
    }

    /* 如果 separator 长度大于 dstr，则视为不合法参数，直接返回。  */
    if (separator->len > dstr->len)
    {
        return NULL;
    }

    /* 委托 split_str 函数，完成分隔。 */
    return split_str(dstr->data, dstr->len, separator->data, separator->len, out_dstr_count);
}

/* 分隔一个「C 字符串」为多个「C 字符串」。 */
char **cstr_split(const char *const cstr, const char *const separator, size_t *const out_cstr_count)
{
    /* TODO: 待静态辅助函数按方向拆分后实现。 */
    (void)cstr;
    (void)separator;
    (void)out_cstr_count;
    return NULL;
}

/* 合并多个「C 字符串」为一个「动态字符串」。 */
struct dynamic_string *dstr_join_cstr(const char *const *const cstrs, const size_t cstr_count,
                                      const char *const separator)
{
    /* 参数合法性检查。 */
    if (cstrs == NULL || cstr_count == 0)
    {
        return NULL;
    }

    /* separator 为空指针或指向空字符串，则使用空串连接。 */
    const size_t separator_len = (separator != NULL && separator[0] != '\0') ? strlen(separator) : 0;

    /* 合并后的字符串的长度。 */
    size_t target_len = 0;

    /* 计算 separator 多次出现的总长度。 */
    if (separator_len > 0 && !safe_size_t_mul(separator_len, cstr_count - 1, &target_len))
    {
        return NULL;
    }

    /**
     * 尝试分配一个 size_t 数组，用于缓存每个「C 字符串」的长度，
     * 避免在第一遍累加和第二遍拷贝时各执行一次 strlen。
     * 分配失败则不缓存，后续重新计算，不视为致命错误。
     */
    size_t *cstr_lens = NULL;
    if (safe_size_t_mul(cstr_count, sizeof(size_t), NULL))
    {
        cstr_lens = malloc(cstr_count * sizeof(size_t));
    }

    /**
     * 第一遍遍历：累加每个「C 字符串」的长度，
     * 以计算合并后的字符串长度。
     * 指针本身可能为空（视为长度 0）；不为空时长度也可能为 0；
     * 通过 cstrs[i][0] == '\0' 判断，以减少不必要的 strlen 调用。
     */
    for (size_t i = 0; i < cstr_count; ++i)
    {
        const size_t cstr_len = (cstrs[i] != NULL && cstrs[i][0] != '\0') ? strlen(cstrs[i]) : 0;

        /* 缓存长度。 */
        if (cstr_lens != NULL)
        {
            cstr_lens[i] = cstr_len;
        }

        /* 累加到 target_len，检测溢出。 */
        if (!safe_size_t_add(target_len, cstr_len, &target_len))
        {
            free(cstr_lens);
            return NULL;
        }
    }

    /* 创建空的「动态字符串」。 */
    struct dynamic_string *const result = create_dstr(NULL, 0);
    if (result == NULL)
    {
        free(cstr_lens);
        return NULL;
    }

    /* 如果合并后的长度为 0，则直接返回空的「动态字符串」。 */
    if (target_len == 0)
    {
        free(cstr_lens);
        return result;
    }

    /* 扩容。 */
    if (!safe_size_t_add(target_len, 1, NULL) || !resize_capacity(result, target_len + 1, false))
    {
        free(cstr_lens);
        free(result);
        return NULL;
    }

    /* 第二遍遍历：执行拷贝。 */
    char *dest = result->data;

    for (size_t i = 0; i < cstr_count; ++i)
    {
        /* 如果不是第一个元素，先拷贝 separator。 */
        if (i > 0 && separator_len > 0)
        {
            memcpy(dest, separator, separator_len);
            dest += separator_len;
        }

        /* 从缓存获取，或重新计算长度。 */
        const size_t cstr_len =
            (cstr_lens != NULL) ? cstr_lens[i] : ((cstrs[i] != NULL && cstrs[i][0] != '\0') ? strlen(cstrs[i]) : 0);

        /* 拷贝 cstr。 */
        if (cstr_len > 0)
        {
            memcpy(dest, cstrs[i], cstr_len);
            dest += cstr_len;
        }
    }

    /* 设置终止符和长度。 */
    *dest = '\0';
    result->len = target_len;

    free(cstr_lens);
    return result;
}

/* 合并多个「动态字符串」为一个「动态字符串」。 */
struct dynamic_string *dstr_join(const struct dynamic_string *const *const dstrs, const size_t dstr_count,
                                 const struct dynamic_string *const separator)
{
    /* 参数合法性检查。 */
    if (dstrs == NULL || dstr_count == 0)
    {
        return NULL;
    }

    /* separator 为空指针或指向空字符串，则使用空串连接。 */
    const size_t separator_len = (separator != NULL) ? separator->len : 0;

    /* 合并后的字符串的长度。 */
    size_t target_len = 0;

    /* 计算 separator 多次出现的总长度。 */
    if (separator_len > 0 && !safe_size_t_mul(separator_len, dstr_count - 1, &target_len))
    {
        return NULL;
    }

    /* 第一遍遍历：累加每个「动态字符串」的长度。 */
    for (size_t i = 0; i < dstr_count; ++i)
    {
        if (!safe_size_t_add(target_len, (dstrs[i] != NULL) ? dstrs[i]->len : 0, &target_len))
        {
            return NULL;
        }
    }

    /* 创建空的「动态字符串」。 */
    struct dynamic_string *const result = create_dstr(NULL, 0);
    if (result == NULL)
    {
        return NULL;
    }

    /* 如果合并后的长度为 0，则直接返回空的「动态字符串」。 */
    if (target_len == 0)
    {
        return result;
    }

    /* 扩容。 */
    if (!safe_size_t_add(target_len, 1, NULL) || !resize_capacity(result, target_len + 1, false))
    {
        free(result);
        return NULL;
    }

    /* 第二遍遍历：执行拷贝。 */
    char *dest = result->data;

    for (size_t i = 0; i < dstr_count; ++i)
    {
        /* 如果不是第一个元素，先拷贝 separator。 */
        if (i > 0 && separator_len > 0)
        {
            memcpy(dest, separator->data, separator_len);
            dest += separator_len;
        }

        /* 拷贝 dstr。 */
        if (dstrs[i] != NULL && dstrs[i]->len > 0)
        {
            memcpy(dest, dstrs[i]->data, dstrs[i]->len);
            dest += dstrs[i]->len;
        }
    }

    /* 设置终止符和长度。 */
    *dest = '\0';
    result->len = target_len;

    return result;
}

/* 合并多个「C 字符串」为一个「C 字符串」。 */
char *cstr_join(const char *const *const cstrs, const size_t cstr_count, const char *const separator)
{
    /* TODO: 待静态辅助函数按方向拆分后实现。 */
    (void)cstrs;
    (void)cstr_count;
    (void)separator;
    return NULL;
}
