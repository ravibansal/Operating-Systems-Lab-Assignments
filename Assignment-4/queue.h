#ifndef QUEUE_H
#define QUEUE_H
#include "elem.h"

#define NMAX 100



struct queue
{
	struct elem array[NMAX];
	int rear;
	int front;
};

void init(struct queue* que);

int isFull(struct queue* que);

int isEmpty(struct queue* que);

int enqueue(struct queue*que,struct elem data);

/*Not called on a empty queue, otherwise result not known*/
struct elem deque(struct queue* que);

#endif