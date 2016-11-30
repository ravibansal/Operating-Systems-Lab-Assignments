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

#define TOSLEEP 1 

int semid,msgid1,msgid2,indexi,deletions;
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

int read_message( int qid, long type, struct mymsgbuf *qbuf )
{
    int     result, length;

    /* The length is essentially the size of the structure minus sizeof(mtype) */
    length = sizeof(struct mymsgbuf) - sizeof(long);

    if ((result = msgrcv( qid, qbuf, length, type,  0)) == -1)
    {
        return (-1);
    }

    return (result);
}

void writeResult()
{
	char res[100];
	sprintf(res,"consum_%d",indexi);
	FILE *fp=fopen(res,"w");
	fprintf(fp, "%d\n", deletions);
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

int delete_prod(int queue)
{
	int val;
	if(queue==0)
	{
		mymsgbuf msgb;
		if(read_message(msgid1,10,&msgb)==-1)
		{
			perror("Error while reading Message\n");
			exit(1);
		}
		val=msgb.msg;
	}
	else{
		mymsgbuf msgb;
		if(read_message(msgid2,10,&msgb)==-1)
		{
			perror("Error while reading Message\n");
			exit(1);
		}
		val=msgb.msg;
	}
	sleep(TOSLEEP);
	return val;
}

void consume_one(int queue)
{
	
	
	printf("Trying to consume PID:%d , queue: %d\n",getpid(),queue+1);
	

	//down(&full)
	if(queue==0)
		semop_init(FQ0MUT,-1);
	else
		semop_init(FQ1MUT,-1);
	//////////////

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

	int val=delete_prod(queue);
	deletions++;
	writeResult();

	printf("Successfully consumed PID: %d, val: %d, queue: %d\n",getpid(),val,queue+1);
	//If you want sleepy
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

	//up(&empty)
	if(queue==0)
		semop_init(EQ0MUT,1);
	else
		semop_init(EQ1MUT,1);
	//////////////
}

void consume_both(int dlp)
{
	int queue=rand()%2;
	if(dlp==1)
		queue=0;

	/******************Request Lock on q ********************/

	
	printf("Trying to consume PID:%d , queue: %d\n",getpid(),queue+1);
	
	

	//down(&full)
	semop_init(FQ0MUT,-1);
	semop_init(FQ1MUT,-1);
	//////////////

	
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

	
	/******************Consume item from q ********************/
	//Critical Section

	//Acquiring lock on matrix file for updating hold lock
	semop_init(MATRIXMUT,-1);
	updatematrix(2,queue);
	semop_init(MATRIXMUT,1);
	///////

	int val=delete_prod(queue);
	deletions++;
	writeResult();
	printf("Successfully consumed PID: %d, val: %d, queue: %d\n",getpid(),val,queue+1);
	///////////////



	/******************Request Lock on q' ********************/
	queue=1-queue;
	
	printf("Trying to consume PID:%d , queue: %d\n",getpid(),queue+1);
	
	//Acquiring lock on matrix file for updating request lock
	semop_init(MATRIXMUT,-1);
	updatematrix(1,queue);
	semop_init(MATRIXMUT,1);
	///////

	/*//down(&full)
	if(queue==0)
		semop_init(FQ0MUT,-1);
	else
		semop_init(FQ1MUT,-1);
	//////////////*/

	
	//down(&mutex)
	if(queue==0)
		semop_init(Q0MUT,-1);
	else
		semop_init(Q1MUT,-1);
	///////

	
	/******************Consume item from q' ********************/
	//Critical Section

	//Acquiring lock on matrix file for updating hold lock
	semop_init(MATRIXMUT,-1);
	updatematrix(2,queue);
	semop_init(MATRIXMUT,1);
	///////

	val=delete_prod(queue);
	deletions++;
	writeResult();
	printf("Successfully consumed PID: %d, val: %d, queue: %d\n",getpid(),val,queue+1);
	///////////////
	//If you want sleepy

	/***************Releasing Locks***********/
	/****************Releasing for q********************/
	queue=1-queue;

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

	//up(&empty)
	if(queue==0)
		semop_init(EQ0MUT,1);
	else
		semop_init(EQ1MUT,1);
	//////////////

	/***************Releasing for q' ********************/
	queue=1-queue;

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

	//up(&empty)
	if(queue==0)
		semop_init(EQ0MUT,1);
	else
		semop_init(EQ1MUT,1);
	//////////////

}

int main(int argc, char *argv[])
{
	int key=20,keym1=1234,keym2=1235;
	if(argc<4)
	{
		printf("Usage ./consumer <dlp> <index> <prob>\n");
		exit(1);
	}
	int dlp;
	float prob;
	deletions=0;
	sscanf(argv[1],"%d",&dlp);
	sscanf(argv[2],"%d",&indexi);
	sscanf(argv[3],"%f",&prob);
	printf("%d %d %f\n",dlp,indexi,prob);
	// printf("%d\n",dlp);
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
		int randp=rand()%10;
		float randomprob=(((float)randp)/10.0);
		if(randomprob < prob)
		{
			consume_one((rand()%2));
		}
		else
		{
			consume_both(dlp);
		}
	}
}