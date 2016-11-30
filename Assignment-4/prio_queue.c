#include "prio_queue.h"
#include <stdio.h>
#include <stdlib.h>

int compare(struct elem lhs, struct elem rhs)
{
    if (lhs.prio < rhs.prio)
        return -1;
    else if (lhs.prio > rhs.prio)
        return 1;
    else
        return 0;
}


void prio_init(struct prio_queue * pqueue)
{
    pqueue->size = 0;
    struct elem min;
    min.procid = 0;
    min.prio = -INT_MAX;
    pqueue->heap[0] = min;
}

int prio_isEmpty(struct prio_queue * pqueue)
{
    return (pqueue->size == 0);
}

int prio_isFull(struct prio_queue * pqueue)
{
    return (pqueue->size == NMAX);
}
/*Insert an element into the heap */
void prio_enqueue(struct prio_queue * pqueue, struct elem element)
{
    pqueue->size++;
    pqueue->heap[pqueue->size] = element; /*Insert in the last place*/
    /*Adjust its position*/
    int now = pqueue->size;
    while (compare(pqueue->heap[now / 2] , element) > 0 )
    {
        pqueue->heap[now] = pqueue->heap[now / 2];
        now /= 2;
    }
    pqueue->heap[now] = element;
}
struct elem prio_deque(struct prio_queue * pqueue)
{
    struct elem minElement, lastElement;
    int child, now;
    minElement = pqueue->heap[1];
    lastElement = pqueue->heap[pqueue->size--];

    for (now = 1; now * 2 <= pqueue->size ; now = child)
    {

        child = now * 2;

        if (child != pqueue->size && compare(pqueue->heap[child + 1], pqueue->heap[child]) < 0)
        {
            child++;
        }

        if(compare(lastElement, pqueue->heap[child]) > 0)
        {
            pqueue->heap[now] = pqueue->heap[child];
        }
        else 
        {
            break;
        }
    }
    pqueue->heap[now] = lastElement;
    return minElement;
}
/*int main()
{
    struct prio_queue ready_queue;
    prio_init(&ready_queue);
    struct elem e1,e2;
    e1.procid=1;
    e1.prio=3;
    e2.procid=2;
    e2.prio=5;
    prio_enqueue(&ready_queue,e1);
    prio_enqueue(&ready_queue,e2);
    printf("%d\n",ready_queue.heap[1].prio);
    printf("%d\n",prio_deque(&ready_queue).prio);
    printf("%d\n",prio_deque(&ready_queue).prio);
}*/