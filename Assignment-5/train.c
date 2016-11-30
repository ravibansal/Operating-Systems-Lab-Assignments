#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <time.h>
#include <sys/sem.h>

#define MAXSEQLEN 2000
#define SEMKEY 2058

#define FMUTEX 5
#define MUTEX 4
#define NORTH 0
#define WEST 1
#define SOUTH 2
#define EAST 3

#define REQUEST 11
#define RELEASE 10
#define ACQUIRE 12

#define LINEBYTES (4*sizeof(int))

char *dirname[] = {"North","West","South","East","Junction"};
int semid;
int idx;
int n;
FILE *fp;

int sem_wait(int semid, int subsem)
{
	struct sembuf sop;
	sop.sem_num = subsem;
	sop.sem_op = -1;
	sop.sem_flg = 0;
	return semop(semid, &sop, 1);
}

int sem_signal(int semid, int subsem)
{
	struct sembuf sop;
	sop.sem_num = subsem;
	sop.sem_op = 1;
	sop.sem_flg = 0;
	return semop(semid, &sop, 1);
}
int getDirection(char c)
{
	int dir;
	switch (c)
	{
	case 'N' : dir = NORTH;
		break;
	case 'W' : dir = WEST;
		break;
	case 'E' : dir = EAST;
		break;
	case 'S' : dir = SOUTH;
		break;
	}
	return dir;
}


void updateMatrix(int type, int direction)
{

	int i,j;
	fp = fopen("matrix.txt","r");
	if(fp == NULL)
	{
		printf("Error reading file\n");
		exit(1);
	}
	int matrix[n][4];
	for(i=0;i<n;i++)
	{
		for(j=0;j<4;j++)
		{
			fscanf(fp,"%d",&matrix[i][j]);
		}
	}
	fclose(fp);
	matrix[idx][direction] = type - 10;
	fp = fopen("matrix.txt","w");
	if(fp == NULL)
	{
		printf("Error opening file\n");
		exit(1);
	}
	for(i=0;i<n;i++)
	{
		for(j=0;j<4;j++)
		{ 
			fprintf(fp,"%d ",matrix[i][j]);
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
}

int main(int argc, char *argv[])
{
	if (argc < 4)
	{
		printf("Usage: ./train direction index");
		exit(EXIT_FAILURE);
	}
	char cdir = argv[1][0];
	idx = atoi(argv[2]);
	n = atoi(argv[3]);
	int dir = getDirection(cdir);
	int pid = getpid();
	printf("Train %d: %s train started\n",pid,dirname[dir]);
	semid = semget(SEMKEY, 6, 0666);
	int left = (dir + 1) % 4;
	sem_wait(semid, FMUTEX);
	updateMatrix(REQUEST, dir);
	sem_signal(semid, FMUTEX);

	printf("Train %d: Requests %s lock\n",pid,dirname[dir]);
	sem_wait(semid, dir);
	printf("Train %d: Acquires %s lock\n",pid,dirname[dir]);


	sem_wait(semid, FMUTEX);
	updateMatrix(ACQUIRE, dir);
	updateMatrix(REQUEST, left);
	sem_signal(semid, FMUTEX);

	printf("Train %d: Requests %s lock\n",pid,dirname[left]);
	sem_wait(semid, left);
	printf("Train %d: Acquires %s lock\n",pid,dirname[left]);

	sem_wait(semid, FMUTEX);
	updateMatrix(ACQUIRE, left);
	sem_signal(semid, FMUTEX);

	printf("Train %d: Requests %s lock\n",pid,dirname[MUTEX]);
	sem_wait(semid, MUTEX);
	printf("Train %d: Acquires %s lock; passing Junction\n",pid,dirname[MUTEX]);

	sleep(2);

	sem_signal(semid, MUTEX);
	printf("Train %d: Releases %s lock\n",pid,dirname[MUTEX]);

	sem_signal(semid, dir);
	printf("Train %d: Releases %s lock\n",pid,dirname[dir]);
	sem_wait(semid, FMUTEX);
	updateMatrix(RELEASE, dir);
	sem_signal(semid, FMUTEX);

	sem_signal(semid, left);
	printf("Train %d: Releases %s lock\n",pid,dirname[left]);
	sem_wait(semid, FMUTEX);
	updateMatrix(RELEASE, left);
	sem_signal(semid, FMUTEX);
	return 0;
}