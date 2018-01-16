#ifndef DLM_LIST_H
#define DLM_LIST_H

struct list_head {
    struct list_head *next, *prev;
};

static inline void
list_head_init(struct list_head *name)
{
    name->next = name;
    name->prev = name;
}

static inline void
__list_add(struct list_head *entry,
                struct list_head *prev, struct list_head *next)
{
    next->prev = entry;
    entry->next = next;
    entry->prev = prev;
    prev->next = entry;
}

static inline void
list_add(struct list_head *entry, struct list_head *head)
{
    __list_add(entry, head, head->next);
}

static inline void
list_add_tail(struct list_head *entry, struct list_head *head)
{
    __list_add(entry, head->prev, head);
}

static inline void
__list_del(struct list_head *prev, struct list_head *next)
{
    next->prev = prev;
    prev->next = next;
}

static inline void
list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
}

static inline bool
list_empty(struct list_head *head)
{
    return head->next == head;
}

#ifndef container_of
#define container_of(ptr, type, member) \
    (type *)((char *)(ptr) - (char *) &((type *)0)->member)
#endif

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define list_first_entry(ptr, type, member) \
    list_entry((ptr)->next, type, member)

#define list_last_entry(ptr, type, member) \
    list_entry((ptr)->prev, type, member)

#define __container_of(ptr, sample, member)             \
    (void *)container_of((ptr), typeof(*(sample)), member)

#define list_for_each_entry(pos, head, member)              \
    for (pos = __container_of((head)->next, pos, member);       \
     &pos->member != (head);                    \
     pos = __container_of(pos->member.next, pos, member))

#define list_for_each_entry_safe(pos, tmp, head, member)        \
    for (pos = __container_of((head)->next, pos, member),       \
     tmp = __container_of(pos->member.next, pos, member);       \
     &pos->member != (head);                    \
     pos = tmp, tmp = __container_of(pos->member.next, tmp, member))

#endif
