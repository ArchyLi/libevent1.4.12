/*
 * Copyright (c) 2000-2004 Niels Provos <provos@citi.umich.edu>
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
#ifndef _EVENT_INTERNAL_H_
#define _EVENT_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "min_heap.h"
#include "evsignal.h"
//比如对于epoll，libevent实现了5个对应的接口函数，并在初始化时并将eventop的5个函数指针指向这5个函数，
//那么程序就可以使用epoll作为I/O demultiplex机制了
struct eventop {
	const char *name;
	void *(*init)(struct event_base *);//初始化
	int (*add)(void *, struct event *);//注册事件
	int (*del)(void *, struct event *);//删除事件
	int (*dispatch)(struct event_base *, void *, struct timeval *);//事件分发
	void (*dealloc)(struct event_base *, void *);//注销，释放资源
	/* set if we need to reinitialize the event base */
	int need_reinit;
};

struct event_base {
	///指定某个eventop结构体，它决定了该event_base使用哪一种I/O复用技术
	//指向了全局变量static const struct eventop* eventops[]中的一个，里面包含了select\poll\epoll\kequeue等等若干全局实例对象
	const struct eventop *evsel;
	//实际上是eventop实例对象
	void *evbase;
	//总事件个数
	int event_count;		/* counts number of total events */
	//活跃事件个数
	int event_count_active;	/* counts number of active events */

	//处理完所有的事件之后退出
	int event_gotterm;		/* Set to terminate loop */
	//立刻退出
	int event_break;		/* Set to terminate loop immediately */

	//activequeues[priority]是一个链表，链表的每一个结点指向一个优先级为priority的就绪事件event
	/* active event management */
	struct event_list **activequeues;
	int nactivequeues;

	/* signal handling info */
	struct evsignal_info sig;//管理信号的结构体
	//事件链表，保存了所有注册事件的event指针
	struct event_list eventqueue;
	//事件时间
	struct timeval event_tv;
	//管理定时事件的最小堆
	struct min_heap timeheap;
	//同时间管理变量
	struct timeval tv_cache;
};

/* Internal use only: Functions that might be missing from <sys/queue.h> */
#ifndef HAVE_TAILQFOREACH
#define	TAILQ_FIRST(head)		((head)->tqh_first)
#define	TAILQ_END(head)			NULL
#define	TAILQ_NEXT(elm, field)		((elm)->field.tqe_next)
#define TAILQ_FOREACH(var, head, field)					\
	for((var) = TAILQ_FIRST(head);					\
	    (var) != TAILQ_END(head);					\
	    (var) = TAILQ_NEXT(var, field))
#define	TAILQ_INSERT_BEFORE(listelm, elm, field) do {			\
	(elm)->field.tqe_prev = (listelm)->field.tqe_prev;		\
	(elm)->field.tqe_next = (listelm);				\
	*(listelm)->field.tqe_prev = (elm);				\
	(listelm)->field.tqe_prev = &(elm)->field.tqe_next;		\
} while (0)
#endif /* TAILQ_FOREACH */

int _evsignal_set_handler(struct event_base *base, int evsignal,
			  void (*fn)(int));
int _evsignal_restore_handler(struct event_base *base, int evsignal);

#ifdef __cplusplus
}
#endif

#endif /* _EVENT_INTERNAL_H_ */
