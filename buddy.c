#include "buddy.h"
#include <stdlib.h>
#define NULL ((void *)0)

struct node {
    void *addr;
    int rank;
    struct node *next;
};

void *start_addr;
struct node *aval[MAX_RANK + 1], *used[MAX_RANK + 1];

struct node* new_node(void *addr, int rank, struct node *next) {
    struct node *p = (struct node *)malloc(sizeof(struct node));
    p->addr = addr;
    p->rank = rank;
    p->next = next;
    return p;
}

int init_page(void *p, int pgcount) {
    start_addr = p;
    for (int i = 0; i <= MAX_RANK; ++i) {
        aval[i] = NULL;
        used[i] = NULL;
    }
    int rk = 0;
    while (pgcount) rk++, pgcount >>= 1;
    aval[rk] = new_node(p, rk, NULL);
    return OK;
}

void *alloc_pages(int rank) {
    if (rank < 1 || rank > MAX_RANK)
        return -EINVAL;
    int rk = rank;
    while (rk <= MAX_RANK && aval[rk] == NULL) rk++;
    if (rk > MAX_RANK) return -ENOSPC;
    while (rk > rank) {
        struct node *p = aval[rk];
        aval[rk] = p->next;
        rk--;
        aval[rk] = new_node(p->addr + (1 << (rk - 1)) * PAGE_SIZE, rk, aval[rk]);
        aval[rk] = new_node(p->addr, rk, aval[rk]);
        free(p);
    }
    struct node *p = aval[rk];
    aval[rk] = p->next;
    used[rk] = new_node(p->addr, rk, used[rk]);
    free(p);
    return used[rk]->addr;
}

struct node *find_node(struct node **list, void *addr, int erase) {
    for (int rk = 1; rk <= MAX_RANK; ++rk) {
        if (list[rk] == NULL) continue;
        if (list[rk]->addr == addr) {
            struct node *p = list[rk];
            if (erase) list[rk] = p->next;
            return p;
        }
        for (struct node *p = list[rk]; p->next; p = p->next) {
            if (p->next->addr == addr) {
                struct node *q = p->next;
                if (erase) p->next = q->next;
                return q;
            }
        }
    }
    return NULL;
}

struct node *combine(struct node *p, struct node *q) {
    if (p->rank != q->rank) return NULL;
    else if (p->addr + (1 << (p->rank - 1)) * PAGE_SIZE == q->addr
        && ((p->addr - start_addr) / ((1 << (p->rank - 1)) * PAGE_SIZE)) % 2 == 0)
        return new_node(p->addr, p->rank + 1, NULL);
    else if (q->addr + (1 << (q->rank - 1)) * PAGE_SIZE == p->addr
        && ((q->addr - start_addr) / ((1 << (q->rank - 1)) * PAGE_SIZE)) % 2 == 0)
        return new_node(q->addr, q->rank + 1, NULL);
    else return NULL;
}

int return_pages(void *p) {
    struct node *q = find_node(used, p, 1);
    if (q == NULL) return -EINVAL;
    for (int rk = q->rank; rk <= MAX_RANK; ++rk) {
        struct node *c = NULL;
        if (aval[rk]) {
            if ((c = combine(q, aval[rk]))) {
                struct node *t = aval[rk];
                aval[rk] = t->next;
                free(t);
            }
            for (struct node *t = aval[rk]; !c && t->next; t = t->next) {
                if (c = combine(q, t->next)) {
                    struct node *s = t->next;
                    t->next = s->next;
                    free(s);
                    break;
                }
            }
        }
        if (c) free(q), q = c;
        else {
            aval[rk] = new_node(q->addr, rk, aval[rk]);
            free(q);
            break;
        }
    }
    return OK;
}

int query_ranks(void *p) {
    struct node *q = find_node(used, p, 0);
    if (q == NULL) {
        q = find_node(aval, p, 0);
        if (q == NULL) return -EINVAL;
    }
    return q->rank;
}

int query_page_counts(int rank) {
    if (rank < 1 || rank > MAX_RANK) return -EINVAL;
    int cnt = 0;
    for (struct node *p = aval[rank]; p; p = p->next) cnt++;
    return cnt;
}
