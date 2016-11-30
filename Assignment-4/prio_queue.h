#ifndef PRIO_QUEUE_H
#define PRIO_QUEUE_H
#include <limits.h>
#include "elem.h"

#define NMAX 100



struct prio_queue
{
    struct elem heap[NMAX+1];
    int size;
};

int compare(struct elem lhs, struct elem rhs);


void prio_init(struct prio_queue * pqueue);

int  prio_isEmpty(struct prio_queue * pqueue);

int prio_isFull(struct prio_queue * pqueue);

void prio_enqueue(struct prio_queue * pqueue, struct elem element);

struct elem prio_deque(struct prio_queue * pqueue);

#endif