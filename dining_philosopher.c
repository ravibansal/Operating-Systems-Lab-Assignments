#include <stdio.h>

#define N 5
#define LEFT (i+N-1)%N
#define RIGHT (i+1)%N
#define THINKING 0
#define HUNGRY 1
#define EATING 2
typedef int semaphore;
int state[N];
semaphore mutex = 1;
semaphore s[N];

void philosopher(int i)
{
	while(TRUE)
	{
		think();
		take_forks(i);
		eat();
		put_forks(i);
	}
}

void take_forks(int i)
{
	down(&mutex);
	state[i]=HUNGRY;
	test(i);
	up(&mutex);
	down(&s[i]);
}
void test(int i)
{
	if(state[i]==HUNGRY && state[LEFT]!=EATING && state[RIGHT]!=EATING)
	{
		state[i]=EATING;
		up(&s[i]);
	}
}

void put_forks(int i)
{
	down(&mutex);
	state[i]=THINKING;
	test(LEFT);
	test(RIGHT);
	up(&mutex);
}