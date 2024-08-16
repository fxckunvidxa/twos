#pragma once
#include <types.h>

struct list_head {
    struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) {&(name), &(name)}
#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list)
{
    list->next = list;
    list->prev = list;
}

static inline void __list_add(struct list_head *nw, struct list_head *prev, struct list_head *next)
{
    next->prev = nw;
    nw->next = next;
    nw->prev = prev;
    prev->next = nw;
}

static inline void list_add(struct list_head *nw, struct list_head *head)
{
    __list_add(nw, head, head->next);
}

static inline void list_add_tail(struct list_head *nw, struct list_head *head)
{
    __list_add(nw, head->prev, head);
}

static inline void __list_del(struct list_head *prev, struct list_head *next)
{
    next->prev = prev;
    prev->next = next;
}

static inline void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
    entry->next = NULL;
    entry->prev = NULL;
}

static inline int list_empty(const struct list_head *head)
{
    return head->next == head;
}

#define list_entry(ptr, type, member) ((type *)((uptr)(ptr) - offsetof(type, member)))

#define list_first_entry(ptr, type, member) list_entry((ptr)->next, type, member)

#define list_last_entry(ptr, type, member) list_entry((ptr)->prev, type, member)

#define list_next_entry(pos, member) list_entry((pos)->member.next, typeof(*(pos)), member)

#define list_prev_entry(pos, member) list_entry((pos)->member.prev, typeof(*(pos)), member)

#define list_for_each(pos, head) for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_reverse(pos, head) for (pos = (head)->prev; pos != (head); pos = pos->prev)

static inline usize list_count_nodes(struct list_head *head)
{
    struct list_head *pos;
    usize count = 0;
    list_for_each(pos, head) count++;
    return count;
}

#define list_entry_is_head(pos, head, member) (&pos->member == (head))

#define list_for_each_entry(pos, head, member)                                                                         \
    for (pos = list_first_entry(head, typeof(*pos), member); !list_entry_is_head(pos, head, member);                   \
         pos = list_next_entry(pos, member))

#define list_for_each_entry_reverse(pos, head, member)                                                                 \
    for (pos = list_last_entry(head, typeof(*pos), member); !list_entry_is_head(pos, head, member);                    \
         pos = list_prev_entry(pos, member))