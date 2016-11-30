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
#include <math.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/wait.h>


typedef struct {
	int frameno;
	int isvalid;
	int count;
}ptbentry;

typedef struct {
	pid_t pid;
	int m;
	int f_cnt;
	int f_allo;
}pcb;

typedef struct 
{
	int current;
	int flist[];
}freelist;

int k,m,f;
int flag = 0;
key_t freekey,pagetbkey;
key_t readykey, msgq2key, msgq3key;
key_t pcbkey;

int ptbid, freelid;
int readyid, msgq2id, msgq3id;
int pcbid;

void print_PCB(pcb p)
{
	printf("PID = %d m = %d f_cnt = %d\n",p.pid,p.m,p.f_cnt);

}
int max(int a, int b)
{
	return (a>b)?a:b;
}

int min(int a,int b)
{
	return (a<b)?a:b;	
}

void myexit(int status);

void createFreeList()
{
	int i;
	freekey = ftok("master.c",56);
	if(freekey == -1)
	{	
		perror("freekey");
		myexit(EXIT_FAILURE);
	}
	freelid = shmget(freekey, sizeof(freelist)+f*sizeof(int), 0666 | IPC_CREAT | IPC_EXCL);
	if(freelid == -1)
	{	
		perror("free-shmget");
		myexit(EXIT_FAILURE);
	}

	freelist *ptr = (freelist*)(shmat(freelid, NULL, 0));
	if(*((int *)ptr) == -1)
	{
		perror("freel-shmat");
		myexit(EXIT_FAILURE);
	}
	for(i=0;i<f;i++)
	{
		ptr->flist[i] = i;
	}
	ptr->current = f-1;

	if(shmdt(ptr) == -1)
	{
		perror("freel-shmdt");
		myexit(EXIT_FAILURE);
	}
}

void createPageTables()
{
	int i;
	pagetbkey = ftok("master.c",100);
	if(pagetbkey == -1)
	{	
		perror("pagetbkey");
		myexit(EXIT_FAILURE);
	}
	ptbid = shmget(pagetbkey, m*sizeof(ptbentry)*k, 0666 | IPC_CREAT | IPC_EXCL);
	if(ptbid == -1)
	{	
		perror("pcb-shmget");
		myexit(EXIT_FAILURE);
	}

	ptbentry *ptr = (ptbentry*)(shmat(ptbid, NULL, 0));
	if(*(int *)ptr == -1)
	{
		perror("pcb-shmat");
		myexit(EXIT_FAILURE);
	}

	for(i=0;i<k*m;i++)
	{
		ptr[i].frameno = -1;
		ptr[i].isvalid = 0;
	}

	if(shmdt(ptr) == -1)
	{
		perror("pcb-shmdt");
		myexit(EXIT_FAILURE);
	}
}


void createMessageQueues()
{
	readykey = ftok("master.c",200);
	if(readykey == -1)
	{	
		perror("readykey");
		myexit(EXIT_FAILURE);
	}
	readyid = msgget(readykey, 0666 | IPC_CREAT| IPC_EXCL);
	if(readyid == -1)
	{
		perror("ready-msgget");
		myexit(EXIT_FAILURE);
	}

	msgq2key = ftok("master.c",300);
	if(msgq2key == -1)
	{	
		perror("msgq2key");
		myexit(EXIT_FAILURE);
	}
	msgq2id = msgget(msgq2key, 0666 | IPC_CREAT| IPC_EXCL );
	if(msgq2id == -1)
	{
		perror("msgq2-msgget");
		myexit(EXIT_FAILURE);
	} 

	msgq3key = ftok("master.c",400);
	if(msgq3key == -1)
	{	
		perror("msgq3key");
		myexit(EXIT_FAILURE);
	}
	msgq3id = msgget(msgq3key, 0666 | IPC_CREAT| IPC_EXCL);
	if(msgq3id == -1)
	{
		perror("msgq3-msgget");
		myexit(EXIT_FAILURE);
	} 
}

void createPCBs()
{
	int i;
	pcbkey = ftok("master.c",500);
	if(pcbkey == -1)
	{	
		perror("pcbkey");
		myexit(EXIT_FAILURE);
	}
	pcbid = shmget(pcbkey, sizeof(pcb)*k, 0666 | IPC_CREAT | IPC_EXCL );
	if(pcbid == -1)
	{	
		perror("pcb-shmget");
		myexit(EXIT_FAILURE);
	}

	pcb *ptr = (pcb*)(shmat(pcbid, NULL, 0));
	if(*(int *)ptr == -1)
	{
		perror("pcb-shmat");
		myexit(EXIT_FAILURE);
	}

	int totpages = 0;
	for(i=0;i<k;i++)
	{
		ptr[i].pid = i;
		ptr[i].m = rand()%m + 1;
		ptr[i].f_allo = 0;
		totpages +=  ptr[i].m;
	}
	int allo_frame = 0;
	printf("tot = %d, k = %d, f=  %d\n",totpages,k,f);
	int max = 0,maxi = 0;
	for(i=0;i<k;i++)
	{
		ptr[i].pid = -1;
		int allo = (int)round(ptr[i].m*(f-k)/(float)totpages) + 1;
		if(ptr[i].m > max)
		{
			max = ptr[i].m;
			maxi = i;
		}
		allo_frame = allo_frame + allo;
		//printf("%d\n",allo);
		ptr[i].f_cnt = allo;
		
	}
	ptr[maxi].f_cnt += f - allo_frame; 

	for(i=0;i<k;i++)
	{
		print_PCB(ptr[i]);
	}

	if(shmdt(ptr) == -1)
	{
		perror("freel-shmdt");
		myexit(EXIT_FAILURE);
	}

}

void clearResources()
{
	if(shmctl(ptbid,IPC_RMID, NULL) == -1)
	{
		perror("shmctl-ptb");
	}
	if(shmctl(freelid,IPC_RMID, NULL) == -1)
	{
		perror("shmctl-freel");
	}
	if(shmctl(pcbid,IPC_RMID, NULL) == -1)
	{
		perror("shmctl-pcb");
	}
	if(msgctl(readyid, IPC_RMID, NULL) == -1)
	{
		perror("msgctl-ready");
	}
	if(msgctl(msgq2id, IPC_RMID, NULL) == -1)
	{
		perror("msgctl-msgq2");
	}
	if(msgctl(msgq3id, IPC_RMID, NULL) == -1)
	{
		perror("msgctl-msgq3");
	}
}

void myexit(int status)
{
	clearResources();
	exit(status);
}


void createProcesses()
{
	pcb *ptr = (pcb*)(shmat(pcbid, NULL, 0));
	/*if(*(int *)ptr == -1)
	{
		perror("pcb-shmat");
		myexit(EXIT_FAILURE);
	}*/

	int i,j;
	for(i=0;i<k;i++)
	{
		int rlen = rand()%(8*ptr[i].m) + 2*ptr[i].m + 1;
		char rstring[m*20*40];
		printf("rlen = %d\n",rlen);
		int l = 0;
		for(j=0;j<rlen;j++)
		{
			int r;
			r = rand()%ptr[i].m;
			float p = (rand()%100)/100.0;
			if(p < 0.2)
			{
				r = rand()%(1000*m) + ptr[i].m;
			}
			l += sprintf(rstring+l,"%d|",r);
		}
		printf("Ref string = %s\n",rstring);
		if(fork() == 0)
		{
			char buf1[20],buf2[20],buf3[20];
			sprintf(buf1,"%d",i);
			sprintf(buf2,"%d",readykey);
			sprintf(buf3,"%d",msgq3key);
			execlp("./process","./process",buf1,buf2,buf3,rstring,(char *)(NULL));
			exit(0);

		}
		//TODO: fork proecess here
		usleep(250*1000);	
	}

}
int pid,spid,mpid;

void timetoend(int sig)
{
	//printf("Mater: gi=o the signal\n");
	sleep(1);
	kill(spid, SIGTERM);
	kill(mpid, SIGUSR2);
	sleep(2);
	flag = 1;

}
int main(int argc, char const *argv[])
{
	srand(time(NULL));
	signal(SIGUSR1, timetoend);
	signal(SIGINT, myexit);
	if(argc < 4)
	{
		printf("master k m f\n");
		myexit(EXIT_FAILURE);
	}
	k = atoi(argv[1]);
	m = atoi(argv[2]);
	f = atoi(argv[3]);
	pid = getpid();
	if(k <= 0 || m <= 0 || f <=0 || f < k)
	{
		printf("Invalid input\n");
		myexit(EXIT_FAILURE);
	}

	createPageTables();
	createFreeList();
	createPCBs();
	createMessageQueues();

	if((spid = fork()) == 0)
	{
		char buf1[20],buf2[20],buf3[20],buf4[20];
		sprintf(buf1,"%d",readykey);
		sprintf(buf2,"%d",msgq2key);
		sprintf(buf3,"%d",k);
		sprintf(buf4,"%d",pid);
		execlp("./scheduler","./scheduler",buf1,buf2,buf3,buf4,(char *)(NULL));
		exit(0);
	}


	if((mpid = fork()) == 0)
	{
		char buf1[20],buf2[20],buf3[20],buf4[20],buf5[20],buf6[20],buf7[20];
		sprintf(buf1,"%d",msgq2id);
		sprintf(buf2,"%d",msgq3id);
		sprintf(buf3,"%d",ptbid);
		sprintf(buf4,"%d",freelid);
		sprintf(buf5,"%d",pcbid);
		sprintf(buf6,"%d",m);
		sprintf(buf7,"%d",k);
		execlp("./mmu","./mmu",buf1,buf2,buf3,buf4,buf5,buf6,buf7,(char *)(NULL));
		exit(0);
	}
	printf("generating processed\n");
	createProcesses();
	if(flag == 0)
		pause();
	clearResources();

	return 0;
}