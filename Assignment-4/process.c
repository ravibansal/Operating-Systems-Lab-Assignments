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
#include <sys/time.h>
#define SCHED_KEY 121

#define NOTIFY SIGUSR1
#define SUSPEND SIGUSR2

#define READY 2
#define IO 3
#define IOOVER 4
#define TERMINATE 5

struct msgbuf
{
	long type;
	long pid;
	int priority;
	int msg;
};

pid_t pid;
int priority;
int msqid;

int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y)
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}
int sendmsg(int msg)
{
	//printf("%d Sending message %d\n", pid, msg);
	struct msgbuf buf;
	buf.type = pid;
	buf.pid = pid;
	buf.priority = priority;
	buf.msg = msg;

	size_t msgsize = sizeof(struct msgbuf) - sizeof(long);

	int st = msgsnd(msqid, &buf, msgsize, 0);
	if (st == -1)
	{
		//printf("onmy god Eror %d\n",errno);
		//perror("msgsnd");
		exit(EXIT_FAILURE);
	}
	//printf("Sent message IO\n");
	return st;
}
//time_t ltime;
struct timeval ltime;
struct timeval total_waiting_time;
int notified = 0;
void sig_suspend(int sig)
{
	sigset_t mask, oldmask;

	/* Set up the mask of signals to temporarily block. */
	sigemptyset (&mask);
	struct timeval t1,t2,result;
	sigaddset (&mask, SUSPEND);
	gettimeofday(&t1, NULL);
	/* Wait for a signal to arrive. */
	sigprocmask (SIG_BLOCK, &mask, &oldmask);
	while (!notified)
		sigsuspend (&oldmask);
	notified = 0;
	gettimeofday(&t2, NULL);
	timersub(&t2,&t1,&ltime);
	if(ltime.tv_sec>=0 && ltime.tv_usec>=0)
	{
		timeradd(&total_waiting_time,&ltime,&total_waiting_time);
	}
	//printf("Waitin time: %f\n",total_waiting_time);
	//FILE *fp1=fopen("wait.txt","a");
	//fprintf(fp1,"Pid %d: %lf",pid,total_waiting_time);
	//fclose(fp1);
	//printf("Waitin time: %lf\n",total_waiting_time);
	sigprocmask (SIG_UNBLOCK, &mask, NULL);

}

void sig_notify(int sig)
{
	notified = 1;
}

int main(int argc, char *argv[])
{
	//time_t start_time;
	struct timeval start_time,t2,result;
	struct timeval response_time, total_turnaround_time;
	total_waiting_time.tv_sec= 0;
	total_waiting_time.tv_usec=0;

	struct sigaction saction;
	saction.sa_handler = sig_notify;
	sigaction(NOTIFY, &saction, NULL);

	struct sigaction saction2;
	saction2.sa_handler = sig_suspend;
	sigaction(SUSPEND, &saction2, NULL);

	gettimeofday(&start_time, NULL);
	//printf("%d\n",argc);
	if (argc < 5)
	{
		printf("Usage Error:: process NOI priority sleep_p sleep_t\n");
		exit(EXIT_FAILURE);
	}
	pid = getpid();
	int NOI = atoi(argv[1]);
	priority = atoi(argv[2]);
	double sleep_p = atof(argv[3]);
	long sleep_t = atoi(argv[4]);
	printf("%d %d %lf %ld\n", NOI, priority, sleep_p, sleep_t);
	msqid = msgget(SCHED_KEY, 0666);
	if (msqid == -1)
	{
		perror("msgget");
		exit(EXIT_FAILURE);
	}
	srand(time(NULL));
	int i;
	sendmsg(READY);
	ltime = start_time;
	sig_suspend(0);
	gettimeofday(&t2, NULL);
	//response_time = (t2.tv_usec - start_time.tv_usec);
	timersub(&t2,&start_time,&ltime);
	if(ltime.tv_sec>=0 && ltime.tv_usec>=0)
	{
		response_time =ltime;
	}
	for (i = 0; i < NOI; i++)
	{
		double r = rand() * 1.0 / INT_MAX;
		if (r < sleep_p)
		{
			signal(SUSPEND,SIG_IGN);
			sendmsg(IO);
			printf("PID:%d Going for IO\n", pid);
			sleep(sleep_t);
			printf("PID:%d Came back from IO\n", pid);
			//ltime = time(0);
			sendmsg(IOOVER);
			sig_suspend(0);
			signal(SUSPEND,sig_suspend);
		}
		printf("PID:%d, %d\n", pid, i);
	}
	gettimeofday(&t2, NULL);
	//total_turnaround_time = (t2.tv_sec - start_time.tv_sec);
	timersub(&t2,&start_time,&ltime);
	if(ltime.tv_sec>=0 && ltime.tv_usec>=0)
	{
		total_turnaround_time =ltime;
	}
	sendmsg(TERMINATE);
	printf("file writing\n");
	FILE *fp;
	char s[100];
	sprintf(s,"resultRR_%d.txt",pid);
	fp=fopen(s,"w");
	fprintf(fp,"%d %d %lf %ld\n", NOI, priority, sleep_p, sleep_t);
	fprintf(fp,"%d %ld %ld\n",pid,response_time.tv_sec,response_time.tv_usec);
	fprintf(fp,"%d %ld %ld\n",pid,total_waiting_time.tv_sec,total_waiting_time.tv_usec);
	fprintf(fp,"%d %ld %ld\n",pid,total_turnaround_time.tv_sec,total_turnaround_time.tv_usec);
	fclose(fp);
	/*printf("Press any key to continue.....");
	int n;
	scanf("%d",&n);*/
	printf("Bye\n");
	return 0;
}
