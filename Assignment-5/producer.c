#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/time.h>
#include <sys/sem.h> 
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MATRIXMUT 0

#define FQ0MUT 1
#define EQ0MUT 2
#define Q0MUT 3

#define FQ1MUT 4
#define EQ1MUT 5
#define Q1MUT 6

#define TOSLEEP 2

int semid,msgid1,msgid2,indexi,insertions;
int mat[2][10];

typedef struct mymsgbuf {
    long    mtype;          /* Message type */
    int msg;
} mymsgbuf;

void updatematrix(int val,int queue)
{
	FILE *fp=fopen("matrix.txt","r");
	int i,j;
	for(i=0;i<2;i++)
	{
		for(j=0;j<10;j++)
		{
			fscanf(fp,"%d",&mat[i][j]);
		}
	}
	fclose(fp);
	mat[queue][indexi]=val;
	fp=fopen("matrix.txt","w");
	for(i=0;i<2;i++)
	{
		for(j=0;j<10;j++)
		{
			fprintf(fp,"%d ",mat[i][j]);
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
}

int send_message( int qid, struct mymsgbuf *qbuf )
{
    int     result, length;

    /* The length is essentially the size of the structure minus sizeof(mtype) */
    length = sizeof(struct mymsgbuf) - sizeof(long);

    if ((result = msgsnd( qid, qbuf, length, 0)) == -1)
    {
        return (-1);
    }
    //printf("%d\n",result);
    return (result);
}

void insert_prod(int val,int queue)
{
	if(queue==0)
	{
		mymsgbuf msgb;
		msgb.mtype=10;
		msgb.msg=val;
		if(send_message(msgid1,&msgb)==-1)
		{
			perror("Error while sending Message\n");
			exit(1);
		}
	}
	else{
		mymsgbuf msgb;
		msgb.mtype=10;
		msgb.msg=val;
		if(send_message(msgid2,&msgb)==-1)
		{
			perror("Error while sending Message\n");
			exit(1);
		}
	}
	sleep(TOSLEEP);
}

void writeResult()
{
	char res[100];
	sprintf(res,"prod_%d",indexi);
	FILE *fp=fopen(res,"w");
	fprintf(fp, "%d\n", insertions);
	fclose(fp);
}

void semop_init(int semindex,int check)
{
	struct sembuf sop;
	sop.sem_num=semindex;
	sop.sem_op=check;
	sop.sem_flg=0;
	semop(semid,&sop,1);
}

int semop_init_nowait(int semindex,int check)
{
	struct sembuf sop;
	sop.sem_num=semindex;
	sop.sem_op=check;
	sop.sem_flg=IPC_NOWAIT;
	return semop(semid,&sop,1);
}

void produce(int queue)
{
	
	int val=1+rand()%50;
	
	//down(&empty)
	if(queue==0)
	{
		if(semop_init_nowait(EQ0MUT,-1)==-1)
			return;
	}
	else
	{
		if(semop_init_nowait(EQ1MUT,-1)==-1)
			return;
	}
	//////////////


	printf("Trying to insert PID:%d , val: %d, queue: %d\n",getpid(),val,queue+1);
	//Acquiring lock on matrix file for updating request lock
	semop_init(MATRIXMUT,-1);
	updatematrix(1,queue);
	semop_init(MATRIXMUT,1);
	///////

	//down(&mutex)
	if(queue==0)
		semop_init(Q0MUT,-1);
	else
		semop_init(Q1MUT,-1);
	///////

	//Critical Section

	//Acquiring lock on matrix file for updating hold lock
	semop_init(MATRIXMUT,-1);
	updatematrix(2,queue);
	semop_init(MATRIXMUT,1);
	///////

	insert_prod(val,queue);
	insertions++;
	writeResult();
	printf("Successfully inserted PID: %d, val: %d, queue: %d\n",getpid(),val,queue+1);
	///////////////

	//up(&mutex)
	if(queue==0)
		semop_init(Q0MUT,1);
	else
		semop_init(Q1MUT,1);
	//////////

	//Acquiring lock on matrix file for updating release lock
	semop_init(MATRIXMUT,-1);
	updatematrix(0,queue);
	semop_init(MATRIXMUT,1);
	///////

	//up(&full)
	if(queue==0)
		semop_init(FQ0MUT,1);
	else
		semop_init(FQ1MUT,1);
	//////////////
}

int main(int argc,char *argv[])
{
	int key=20,keym1=1234,keym2=1235;
	if(argc<2)
	{
		printf("Usage: ./producer <index>\n");
		exit(1);
	}
	insertions=0;
	sscanf(argv[1],"%d",&indexi);
	printf("%d\n",indexi);
	semid=semget(key,7,0666);
	msgid1=msgget(keym1,0666);
	msgid2=msgget(keym2,0666);
	if(semid == -1 || msgid1 == -1 || msgid2 == -1)
	{
		printf("Error in creaing resources\n");
		exit(1);
	}
	srand(time(NULL));
	while(1)
	{
		produce(rand()%2);
		usleep((1+rand()%5)*1000);
	}
	return 0;
}