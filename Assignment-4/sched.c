#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <limits.h>

#include "queue.h"
#include "prio_queue.h"


#define SCHED_KEY 121

#define NOTIFY SIGUSR1
#define SUSPEND SIGUSR2

#define READY 2
#define IO 3
#define IOOVER 4
#define TERMINATE 5

#define RR 1000
#define PR 1001


struct msgbuf
{
	long type;
	long pid;
	int priority;
	int msg;
};
int msqid;
void printBuf(struct msgbuf buff)
{
	printf("PID = %ld, PRIO= %d MSG = %d\n", buff.type, buff.priority, buff.msg);
}
int recv_message(long type, struct msgbuf* msg)
{
	memset(msg, 0, sizeof(struct msgbuf));
	size_t msize = sizeof(struct msgbuf) - sizeof(long);
	int st = msgrcv(msqid, msg, msize, type, 0);
	if (st == -1)
	{
		perror("msgrcv");
		exit(EXIT_FAILURE);
	}
	//printBuf(*msg);
	return st;
}

int recv_message_noblock(long type, struct msgbuf* msg)
{
	memset(msg, 0, sizeof(struct msgbuf));
	size_t msize = sizeof(struct msgbuf) - sizeof(long);
	int st = msgrcv(msqid, msg, msize, type, IPC_NOWAIT);
	if (st == -1)
	{
		if (errno == ENOMSG)
			return st;
		else
		{
			perror("msgrcv");
			exit(EXIT_FAILURE);
		}
	}
	return st;
}

int notify(pid_t procid)
{
	//usleep(5);
	union sigval value;
	value.sival_int = NOTIFY;
	return sigqueue(procid, NOTIFY, value);

	//printf("Waking process %ld\n",(long)procid);
	return kill(procid, NOTIFY);
}

int suspend(pid_t procid)
{
	//usleep(5);
	union sigval value;
	value.sival_int = NOTIFY;
	return sigqueue(procid, NOTIFY, value);
	//printf("Suspending process %ld\n",(long)procid);
	return kill(procid, SUSPEND);
}

int checkForProcess_noblock(struct queue *que)
{
	struct msgbuf msgpro;
	int rst = recv_message_noblock(0, &msgpro);
	if (rst == -1)
		return rst;
	struct elem data;
	data.procid = msgpro.type;
	data.prio = msgpro.priority;
	if (msgpro.msg == READY)
	{
		enqueue(que, data);
	}
	else if (msgpro.msg == IOOVER)
	{
		printf("Process %ld completed IO\n", data.procid);
		enqueue(que, data);
	}

	else
	{
		printf("Process %ld unexpectedly sent %d\n", msgpro.type, msgpro.msg);
	}
}

int checkForProcess(struct queue *que)
{
	struct msgbuf msgpro;
	int rst = recv_message(0, &msgpro);
	struct elem data;
	data.procid = msgpro.type;
	data.prio = msgpro.priority;
	if (msgpro.msg == READY)
	{
		enqueue(que, data);
	}
	else if (msgpro.msg == IOOVER)
	{
		printf("Process %ld completed IO\n", data.procid);
		enqueue(que, data);
	}
	else
	{
		printf("Process %ld unexpectedly sent %d\n", msgpro.type, msgpro.msg);
	}
}

int prio_checkForProcess_noblock(struct prio_queue *que)
{
	struct msgbuf msgpro;
	int rst = recv_message_noblock(0, &msgpro);
	if (rst == -1)
		return rst;
	struct elem data;
	data.procid = msgpro.type;
	data.prio = msgpro.priority;
	if (msgpro.msg == READY)
	{
		prio_enqueue(que, data);
	}
	else if (msgpro.msg == IOOVER)
	{
		printf("Process %ld completed IO\n", data.procid);
		prio_enqueue(que, data);
	}

	else
	{
		printf("Process %ld unexpectedly sent %d\n", msgpro.type, msgpro.msg);
	}
}

int prio_checkForProcess(struct prio_queue *que)
{
	struct msgbuf msgpro;
	int rst = recv_message(0, &msgpro);
	struct elem data;
	data.procid = msgpro.type;
	data.prio = msgpro.priority;
	if (msgpro.msg == READY)
	{
		prio_enqueue(que, data);
	}
	else if (msgpro.msg == IOOVER)
	{
		printf("Process %ld completed IO\n", data.procid);
		prio_enqueue(que, data);
	}
	else
	{
		printf("Process %ld unexpectedly sent %d\n", msgpro.type, msgpro.msg);
	}
}

void ctrlc()
{
	if (msgctl(msqid, IPC_RMID, NULL) < 0)
	{
		perror("msgctl");
		exit(EXIT_FAILURE);
	}
	exit(1);
}

void scheduler_PR(int time_quantum, int N)
{
	struct prio_queue ready_queue;
	prio_init(&ready_queue);
	int term_process = 0;
	while (prio_isEmpty(&ready_queue))
	{
		struct msgbuf msgpro;
		recv_message(0, &msgpro);
		struct elem data;
		data.procid = msgpro.type;
		data.prio = msgpro.priority;
		if (msgpro.msg != READY)
		{
			printf("Process %ld unexpectedly sent %d\n", msgpro.type, msgpro.msg);
			continue;
		}
		prio_enqueue(&ready_queue, data);
	}

	while (1)
	{
		struct msgbuf msgpro;
		int rst = recv_message_noblock(0, &msgpro);
		if (rst == -1)
			break;
		struct elem data;
		data.procid = msgpro.type;
		data.prio = msgpro.priority;
		if (msgpro.msg != READY)
		{
			printf("Process %ld unexpectedly sent %d\n", msgpro.type, msgpro.msg);
			continue;
		}
		prio_enqueue(&ready_queue, data);
	}


	while (term_process < N)
	{
		struct elem curr_process = prio_deque(&ready_queue);

		notify(curr_process.procid);
		printf("Proces %ld is running\n", curr_process.procid);
		int flag = 0;
		int i;
		for (i = 0; i < time_quantum; i++)
		{
			//printf("%d ",i);
			struct msgbuf msgpro;
			if (i % 100 == 0)
			{
				//printf("Running %ld %d\n",curr_process.procid,i);
			}
			int rst = recv_message_noblock(curr_process.procid, &msgpro);
			if (rst != -1)
			{
				if (msgpro.msg == IO)
				{
					printf("Process %ld requests IO\n", curr_process.procid);
					flag = 1;
					break;
				}
				else if (msgpro.msg == TERMINATE)
				{
					printf("Process %ld terminated\n", curr_process.procid);
					while (recv_message_noblock(curr_process.procid, &msgpro) != -1);
					term_process++;
					flag = 1;
					break;
				}
				else
				{
					printf("Process %ld unexpectedly sent %d\n", curr_process.procid, msgpro.msg);
				}
			}
		}
		//printf("\n");
		if (!flag)
		{
			suspend(curr_process.procid);
			struct msgbuf msgpro;
			int rst = recv_message_noblock(curr_process.procid, &msgpro);
			if (rst != -1)
			{
				//printf("HULALA\n");
				if (msgpro.msg == IO)
				{
					printf("Process %ld requests IO\n", curr_process.procid);
				}
				else if (msgpro.msg == TERMINATE)
				{
					printf("Process %ld terminated\n", curr_process.procid);
					while (recv_message_noblock(curr_process.procid, &msgpro) != -1);
					term_process++;
				}
				else if (msgpro.msg == IOOVER)
				{
					prio_enqueue(&ready_queue, curr_process);
				}
				else
				{
					printf("Process %ld unexpectedly sent %d\n", curr_process.procid, msgpro.msg);
				}
			}
			else
				prio_enqueue(&ready_queue, curr_process);
		}
		while (1)
		{
			if (prio_checkForProcess_noblock(&ready_queue) == -1)
				break;
		}
		if (term_process == N)
			break;
		while (prio_isEmpty(&ready_queue))
			prio_checkForProcess(&ready_queue);
		//print(&ready_queue);
	}
}


void scheduler_RR(int time_quantum, int N)
{
	struct queue ready_queue;
	init(&ready_queue);
	int term_process = 0;
	while (isEmpty(&ready_queue))
	{
		struct msgbuf msgpro;
		recv_message(0, &msgpro);
		struct elem data;
		data.procid = msgpro.type;
		data.prio = msgpro.priority;
		if (msgpro.msg != READY)
		{
			printf("Process %ld unexpectedly sent %d\n", msgpro.type, msgpro.msg);
			continue;
		}
		enqueue(&ready_queue, data);
	}

	while (1)
	{
		struct msgbuf msgpro;
		int rst = recv_message_noblock(0, &msgpro);
		if (rst == -1)
			break;
		struct elem data;
		data.procid = msgpro.type;
		data.prio = msgpro.priority;
		if (msgpro.msg != READY)
		{
			printf("Process %ld unexpectedly sent %d\n", msgpro.type, msgpro.msg);
			continue;
		}
		enqueue(&ready_queue, data);
	}


	while (term_process < N)
	{
		struct elem curr_process = deque(&ready_queue);

		notify(curr_process.procid);
		printf("Proces %ld is running\n", curr_process.procid);
		int flag = 0;
		int i;
		for (i = 0; i < time_quantum; i++)
		{
			//printf("%d ",i);
			struct msgbuf msgpro;
			if (i % 100 == 0)
			{
				//printf("Running %ld %d\n",curr_process.procid,i);
			}
			int rst = recv_message_noblock(curr_process.procid, &msgpro);
			if (rst != -1)
			{
				if (msgpro.msg == IO)
				{
					printf("Process %ld requests IO\n", curr_process.procid);
					flag = 1;
					break;
				}
				else if (msgpro.msg == TERMINATE)
				{
					printf("Process %ld terminated\n", curr_process.procid);
					while (recv_message_noblock(curr_process.procid, &msgpro) != -1);
					term_process++;
					flag = 1;
					break;
				}
				else
				{
					printf("Process %ld unexpectedly sent %d\n", curr_process.procid, msgpro.msg);
				}
			}
		}
		//printf("\n");
		if (!flag)
		{
			suspend(curr_process.procid);
			struct msgbuf msgpro;
			int rst = recv_message_noblock(curr_process.procid, &msgpro);
			if (rst != -1)
			{
				//printf("HULALA\n");
				if (msgpro.msg == IO)
				{
					printf("Process %ld requests IO\n", curr_process.procid);
				}
				else if (msgpro.msg == TERMINATE)
				{
					printf("Process %ld terminated\n", curr_process.procid);
					while (recv_message_noblock(curr_process.procid, &msgpro) != -1);
					term_process++;
				}
				else if (msgpro.msg == IOOVER)
				{
					enqueue(&ready_queue, curr_process);
				}
				else
				{
					printf("Process %ld unexpectedly sent %d\n", curr_process.procid, msgpro.msg);
				}
			}
			else
				enqueue(&ready_queue, curr_process);
		}
		while (1)
		{
			if (checkForProcess_noblock(&ready_queue) == -1)
				break;
		}
		if (term_process == N)
			break;
		while (isEmpty(&ready_queue))
			checkForProcess(&ready_queue);
		//print(&ready_queue);
	}
}

int main(int argc, char* argv[])
{
	signal(SIGINT,ctrlc);
	if (argc < 4)
	{
		printf("Usage: sched <N> <algo> <time_quantum>\n");
		exit(EXIT_FAILURE);
	}
	int N = atoi(argv[1]);
	int time_quantum = atoi(argv[3]);
	char* algo = argv[2];
	printf("%d %d\n", N, time_quantum);
	msqid = msgget(SCHED_KEY, IPC_CREAT | 0666);
	if (msqid < 0)
	{
		perror("msgget");
		exit(EXIT_FAILURE);
	}

	if (strcmp(algo, "RR") == 0)
	{
		scheduler_RR(time_quantum, N);
	}
	else if (strcmp(algo, "PR") == 0)
	{
		scheduler_PR(time_quantum, N);
	}
	else
	{
		printf("Unknown Algorithm\n");
	}

	if (msgctl(msqid, IPC_RMID, NULL) < 0)
	{
		perror("msgctl");
		exit(EXIT_FAILURE);
	}

	return 0;
}

