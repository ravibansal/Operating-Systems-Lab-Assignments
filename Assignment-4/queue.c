#include "queue.h"
#include <stdio.h>
#define NMAX 100


void init(struct queue* que)
{
	que->rear = 0;
	que->front = 0;
}

int isFull(struct queue * que)
{
	return ((que->rear+1)%NMAX == que->front);
}

int isEmpty(struct queue * que)
{
	return (que->rear == que->front);
}

int enqueue(struct queue* que,struct elem data)
{
	//printf("Enqueu: %ld\n",data.procid);
	if(isFull(que))
	{
		return -1;
	}
	que->array[que->rear] = data;
	que->rear = (que->rear + 1)%NMAX;
}

/*Not called on a empty queue, otherwise result not known*/
struct elem deque(struct queue* que)
{
	struct elem data = que->array[que->front];
	//printf("Dequeu: %ld\n",data.procid);
	que->front = (que->front + 1)%NMAX;
	return data;
}

void print(struct queue* que)
{
	int t = que->front;;
	while(t != que->rear)
	{
		printf("%ld\n",que->array[t].procid);
		t = (t + 1)%NMAX;
	}
}