/*
 * Copyright (c) 2006 Maxim Yegorushkin <maxim.yegorushkin@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _MIN_HEAP_H_
#define _MIN_HEAP_H_

#include "event.h"
#include "evutil.h"

typedef struct min_heap
{
    struct event** p;//动态分配的数组，是由realloc分配的连续内存空间，数组元素为event*
    unsigned n, a;//n队列元素的多少,a代表队列空间的大小.
} min_heap_t;

static inline void           min_heap_ctor(min_heap_t* s);
static inline void           min_heap_dtor(min_heap_t* s);
static inline void           min_heap_elem_init(struct event* e);
static inline int            min_heap_elem_greater(struct event *a, struct event *b);//比较两个event超时值大小
static inline int            min_heap_empty(min_heap_t* s);
static inline unsigned       min_heap_size(min_heap_t* s);//返回heap的size数量
static inline struct event*  min_heap_top(min_heap_t* s);
static inline int            min_heap_reserve(min_heap_t* s, unsigned n);//调整内存空间大小，也是调整p指向的内存区域大小
static inline int            min_heap_push(min_heap_t* s, struct event* e);
static inline struct event*  min_heap_pop(min_heap_t* s);
static inline int            min_heap_erase(min_heap_t* s, struct event* e);
static inline void           min_heap_shift_up_(min_heap_t* s, unsigned hole_index, struct event* e);
static inline void           min_heap_shift_down_(min_heap_t* s, unsigned hole_index, struct event* e);

int min_heap_elem_greater(struct event *a, struct event *b)
{
    return evutil_timercmp(&a->ev_timeout, &b->ev_timeout, >);
}

void min_heap_ctor(min_heap_t* s) { s->p = 0; s->n = 0; s->a = 0; }
void min_heap_dtor(min_heap_t* s) { free(s->p); }
void min_heap_elem_init(struct event* e) { e->min_heap_idx = -1; }
int min_heap_empty(min_heap_t* s) { return 0u == s->n; }
unsigned min_heap_size(min_heap_t* s) { return s->n; }
struct event* min_heap_top(min_heap_t* s) { return s->n ? *s->p : 0; }

int min_heap_push(min_heap_t* s, struct event* e)
{
    if(min_heap_reserve(s, s->n + 1))
        return -1;
    min_heap_shift_up_(s, s->n++, e);
    return 0;
}

struct event* min_heap_pop(min_heap_t* s)
{
    if(s->n)
    {
        struct event* e = *s->p;
        min_heap_shift_down_(s, 0u, s->p[--s->n]);
        e->min_heap_idx = -1;
        return e;
    }
    return 0;
}

int min_heap_erase(min_heap_t* s, struct event* e)
{
    if(((unsigned int)-1) != e->min_heap_idx)
    {
        struct event *last = s->p[--s->n];//把最后一个值作为要填入hole_index的值
        unsigned parent = (e->min_heap_idx - 1) / 2;
	/* we replace e with the last element in the heap.  We might need to
	   shift it upward if it is less than its parent, or downward if it is
	   greater than one or both its children. Since the children are known
	   to be less than the parent, it can't need to shift both up and
	   down. */
        if (e->min_heap_idx > 0 && min_heap_elem_greater(s->p[parent], last))
             min_heap_shift_up_(s, e->min_heap_idx, last);
        else
             min_heap_shift_down_(s, e->min_heap_idx, last);
        e->min_heap_idx = -1;
        return 0;
    }
    return -1;
}

//分配队列大小，n代表队列元素个数多少
int min_heap_reserve(min_heap_t* s, unsigned n)
{
    if(s->a < n)
    {
        struct event** p;
        unsigned a = s->a ? s->a * 2 : 8;//初始分配8个指针大小空间,否则原空间大小翻倍.每次以二倍速度增加
        if(a < n)
            a = n;//翻倍后空间依然不足，则分配n
        if(!(p = (struct event**)realloc(s->p, a * sizeof *p)))
            return -1;
        s->p = p;//重新赋值队列地址及大小
        s->a = a;
    }
    return 0;
}

void min_heap_shift_up_(min_heap_t* s, unsigned hole_index, struct event* e)
{
	//获取父节点
    unsigned parent = (hole_index - 1) / 2;
    while(hole_index && min_heap_elem_greater(s->p[parent], e))//比父节点小或是到达根节点.则交换位置.循环.
    {
    //交换位置
        (s->p[hole_index] = s->p[parent])->min_heap_idx = hole_index;
        hole_index = parent;
        parent = (hole_index - 1) / 2;
    }
    (s->p[hole_index] = e)->min_heap_idx = hole_index;
}

/* hole_index 为取出的元素的位置，e为最右最下的元素值 */
void min_heap_shift_down_(min_heap_t* s, unsigned hole_index, struct event* e)
{
	//取得hole_index的右孩子结点索引
    unsigned min_child = 2 * (hole_index + 1);
    while(min_child <= s->n)
	{
		//找出比较小子节点
        min_child -= min_child == s->n || min_heap_elem_greater(s->p[min_child], s->p[min_child - 1]);
		//比子节点小正常，不需要交换位置，跳出循环
		if(!(min_heap_elem_greater(e, s->p[min_child])))
            break;
		//比子节点大，要交换位置
        (s->p[hole_index] = s->p[min_child])->min_heap_idx = hole_index;
        hole_index = min_child;
        min_child = 2 * (hole_index + 1);
	}
	//执行第二步过程，将最右下的结点插入到空缺处
    min_heap_shift_up_(s, hole_index,  e);
}

#endif /* _MIN_HEAP_H_ */
