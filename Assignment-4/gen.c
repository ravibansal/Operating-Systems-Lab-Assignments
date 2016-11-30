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


int main(int argc, char* argv[])
{
	// if(argc < 3)
	// {
	// 	printf("Usage error : gen N t\n");
	// 	exit(EXIT_FAILURE);
	// }
	// int N = atoi(argv[1]);
	// int t = atoi(argv[2]);
	// if(N < 0 || t <= 0)
	// {
	// 	printf("N and t cannot be negative\n");
	// 	exit(EXIT_FAILURE);
	// }
	int i;
	int N=4,t=1;
	for(i = 0;i < N;i++)
	{
		int NOI,prio;
		double sprob;
		long stime;

		scanf("%d%d%lf%ld",&NOI,&prio,&sprob,&stime);

		if(fork() == 0)
		{
			char cNOI[20],cprio[20],csprob[20],cstime[20];
			sprintf(cNOI,"%d",NOI);
			sprintf(cprio,"%d",prio);
			sprintf(csprob,"%lf",sprob);
			sprintf(cstime,"%ld",stime);
			execlp("xterm","xterm","-e","./process",cNOI,cprio,csprob,cstime,(char*)NULL);
		}
		sleep(t);
	}

}