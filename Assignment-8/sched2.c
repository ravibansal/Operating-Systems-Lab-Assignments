#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/msg.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <ctype.h>
#include <sys/shm.h>


#define MAX_BUFFER_SIZE 100
#define MAX_PAGES 1000
#define MAX_PROCESS 1000
#define FROMPROCESS 10
#define TOPROCESS 20  // TOPROCESS+id will be used to read msg from sch
// #define TOMMU 10
#define FROMMMU 20

#define PAGEFAULT_HANDLED 5
#define TERMINATED 10

int k; //no. of processes
typedef struct _mmutosch {
	long    mtype;
	char mbuf[1];
} mmutosch;

// int running[MAX_PROCESS];
// int runh,runt;

typedef struct mymsgbuf {
	long    mtype;
	int id;
} mymsgbuf;


int send_message( int qid, struct mymsgbuf *qbuf )
{
	int     result, length;

	/* The length is essentially the size of the structure minus sizeof(mtype) */
	length = sizeof(struct mymsgbuf) - sizeof(long);

	if ((result = msgsnd( qid, qbuf, length, 0)) == -1)
	{
		perror("Error in sending message");
		exit(1);
	}

	return (result);
}

int read_message( int qid, long type, struct mymsgbuf *qbuf )
{
	int     result, length;

	/* The length is essentially the size of the structure minus sizeof(mtype) */
	length = sizeof(struct mymsgbuf) - sizeof(long);

	if ((result = msgrcv( qid, qbuf, length, type,  0)) == -1)
	{
		perror("Error in receiving message");
		exit(1);
	}

	return (result);
}

int read_message_mmu( int qid, long type,mmutosch *qbuf )
{
	int result, length;

	/* The length is essentially the size of the structure minus sizeof(mtype) */
	length = sizeof(mmutosch) - sizeof(long);

	if ((result = msgrcv(qid, qbuf, length, type,  0)) == -1)
	{
		perror("Error in receiving message");
		exit(1);
	}

	return (result);
}
int main(int argc , char * argv[])
{
	int mq1_key, mq2_key, master_pid;
	if (argc < 5) {
		printf("Scheduler rkey q2key k mpid\n");
		exit(EXIT_FAILURE);
	}
	mq1_key = atoi(argv[1]);
	mq2_key = atoi(argv[2]);
	k = atoi(argv[3]);
	master_pid = atoi(argv[4]);

	mymsgbuf msg_send, msg_recv;

	int mq1 = msgget(mq1_key, 0666);
	int mq2 = msgget(mq2_key, 0666);
	if (mq1 == -1)
	{
		perror("Message Queue1 creation failed");
		exit(1);
	}
	if (mq2 == -1)
	{
		perror("Message Queue2 creation failed");
		exit(1);
	}
	printf("Total No. of Process received = %d\n", k);

	//initialize the variables for running array
	int terminated_process = 0; //to keep track of running process to exit at last
	// int terminate[MAX_PROCESS];
	while (1)
	{
		read_message(mq1, FROMPROCESS, &msg_recv);
		int curr_id = msg_recv.id;

		msg_send.mtype = TOPROCESS + curr_id;
		send_message(mq1, &msg_send);

		//recv messages from mmu
		mmutosch mmu_recv;
		read_message_mmu(mq2, 0, &mmu_recv);
		//printf("received %ld\n", mmu_recv.mtype);
		if (mmu_recv.mtype == PAGEFAULT_HANDLED)
		{
			msg_send.mtype = FROMPROCESS;
			msg_send.id=curr_id;
			send_message(mq1, &msg_send);
		}
		else if (mmu_recv.mtype == TERMINATED)
		{
			terminated_process++;
			//printf("Got terminate %d\n", curr_id);
		}
		else
		{
			perror("Wrong message from mmu\n");
			exit(1);
		}
		if (terminated_process == k)
			break;
		//printf("====Term %d====\n", terminated_process);
	}
	//printf("Sending sinal\n");
	kill(master_pid, SIGUSR1);
	pause();
	printf("Scheduler terminating ...\n") ;
	exit(1) ;
}