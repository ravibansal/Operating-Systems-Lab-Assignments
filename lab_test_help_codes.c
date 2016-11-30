#define CHAIRS 5

typedef int semaphore;

semaphore customers = 0;
semaphore barbers = 0;
semaphore mutex = 1;
int waiting = 0;

void barber(void)
{
	while(true)
	{
		down(&customers);
		down(&mutex);
		waiting = waiting - 1;
		up(&barbers);
		up(&mutex);
		cut_hair();
	}
}


void customer(void)
{
	down(&mutex);
	if(waiting< CHAIRS)
	{
		waiting = waiting + 1;
		up(&customers);
		up(&mutex);
		down(&barbers);
		get_haircut();
	}
	else
	{
		up(&mutex);
	}
}





////////////////////////DINING PHILOSOPHER

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
