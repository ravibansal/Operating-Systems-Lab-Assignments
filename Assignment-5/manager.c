#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <time.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <limits.h>

#define MAXSEQLEN 100
#define SEMKEY 2058

#define FMUTEX 5
#define MUTEX 4
#define NORTH 0
#define WEST 1
#define SOUTH 2
#define EAST 3


char *dirname[] = {"North", "West", "South", "East", "Junction"};
int semid;
FILE *fp;
int deadchld = 0;
char seq[MAXSEQLEN];
int pidarr[MAXSEQLEN];
int n;

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

int readSequence()
{
	FILE *fp = fopen("sequence.txt", "r");
	if(fp==NULL)
	{
		printf("Please create a sequence.txt file \n");
		exit(1);
	}
	int i = 0;
	char ch;
	while ((ch = (char)fgetc(fp) ) != EOF)
	{
		seq[i++] = ch;
	}
	fclose(fp);
	printf("Sequece length: %d\n", i);
	return i;
}
void countdead(int sig)
{
	pid_t p;
	int status;

	/* loop as long as there are children to process */
	while (1) {

		/* retrieve child process ID (if any) */
		p = waitpid(-1, &status, WNOHANG);

		/* check for conditions causing the loop to terminate */
		if (p == -1) {
			/* continue on interruption (EINTR) */
			if (errno == EINTR) {
				continue;
			}
			break;
		}
		else if (p == 0) {
			/* no more children to process, so break */
			break;
		}
		else if (p > 0)
			deadchld++;
	}
}

void initializeMatrix()
{
	fp = fopen("matrix.txt", "w");
	if (fp == NULL)
	{
		printf("Error creating file\n");
		exit(1);
	}
	int i, j;
	for (i = 0; i < n; i++)
	{
		for (j = 0; j < 4; j++)
		{
			fprintf(fp, "%d ", 0);
		}
		//char ch = '\n';
		//fwrite(&ch, sizeof(char),1,fp);
		fprintf(fp, "\n");
	}
	fclose(fp);
}

void readfile()
{
	fp = fopen("matrix.txt", "r");
	if (fp == NULL)
	{
		printf("Error opening file\n");
		exit(1);
	}
	int i, j;
	for (i = 0; i < n; i++)
	{
		for (j = 0; j < 4; j++)
		{
			int val = 0;
			fscanf(fp, "%d", &val);
			printf("%d ", val);

		}
		printf("\n");
	}
	fclose(fp);
}

void printCycle(int parent[], int v, int i)
{
	printf("System Deadlocked\n");
	int p = v;
	parent[i] = v;
	while(p >= n) p = parent[p];
	int start = p;
	int cycle[n];
	int k = 0;
	do
	{
		int waittrain = parent[parent[p]];
		cycle[k++] = p; 
		p = waittrain;
	}while(p != start);
	k--;
	int j = k;
	while(k > 0)
	{
		p = cycle[k-1];
		int waittrain = cycle[k];
		printf("Train %d from %s is waiting for train %d from %s",pidarr[waittrain],dirname[getDirection(seq[waittrain])],
			pidarr[p],dirname[getDirection(seq[p])]);
		printf(" ---> ");
		k--;
	}
	int waittrain = cycle[0];
	p = cycle[j];
	printf("Train %d from %s is waiting for train %d from %s\n",pidarr[waittrain],dirname[getDirection(seq[waittrain])],
			pidarr[p],dirname[getDirection(seq[p])]);




 	/*while (p != i)
	{
		if (p >= n)
		{
			printf("%s <-- ", dirname[p - n]);
		}
		else
		{
			printf("%d(%c) <-- ", pidarr[p], seq[p]);
		}
		//printf("%d <-- ",p);
		p = parent[p];
	}
	if (p >= n)
	{
		printf("%s\n", dirname[p - n]);
	}
	else
	{
		printf("%d(%c)\n", pidarr[p], seq[p]);
	}*/
}

int isCyclicUtil(int v, int graph[][MAXSEQLEN + 4], int *visited,int *parent)
{
	int i;
	if (!visited[v])
	{
		visited[v] = 1;

		for (i = 0; i < n + 4; i++)
		{
			if (graph[v][i] == 1)
			{
				parent[i] = v;
				if ( !visited[i] && isCyclicUtil(i, graph, visited, parent))
					return 1;
				else if (visited[i] == 1)
				{
					printCycle(parent, v, i);
					return 1;
				}
			}
		}
	}
	visited[v] = 2;
	return 0;
}
int isCycle(int graph[][MAXSEQLEN + 4], int *visited, int*parent)
{
	int i;
	for (i = 0; i < n + 4; i++)
	{
		visited[i] = 0;
		parent[i] = i;
	}
	for (i = 0; i < n + 4; i++)
	{
		if (isCyclicUtil(i, graph, visited, parent))
			return 1;
	}
	return 0;
}



void detectDeadlock()
{
	int i, j;
	sem_wait(semid, FMUTEX);
	int graph[MAXSEQLEN + 4][MAXSEQLEN + 4];
	for (i = 0; i < n + 4; i++)
	{
		for (j = 0; j < n + 4; j++)
		{
			graph[i][j] = 0;
		}
	}
	fp = fopen("matrix.txt", "r");
	for (i = 0; i < n; i++)
	{
		for (j = 0; j < 4; j++)
		{
			int val = 0;
			fscanf(fp, "%d", &val);
			if (val == 1)
			{
				graph[i][n + j] = 1;
			}
			else if (val == 2)
			{
				graph[n + j][i] = 1;
			}
		}
	}
	fclose(fp);
	int visited[n + 4], parent[n + 4];
	int last;
	if (isCycle(graph, visited, parent))
	{
		signal(SIGTERM, SIG_IGN);
		killpg(0, SIGTERM);
		exit(0);
		//printf("Cycle Detected\n");
		//printCycle(last,graph, visited, rstack);
	}
	sem_signal(semid, FMUTEX);


}



int main(int argc, char* argv[])
{
	if(argc < 2)
	{
		printf("manager prob\n");
		exit(1);
	}
	n = readSequence();
	signal(SIGCHLD, countdead);
	srand(time(NULL));
	semid = semget(SEMKEY, 6, IPC_CREAT | 0666);

	if (semid < 0)
	{
		perror("semget");
		exit(EXIT_FAILURE);
	}
	int i;
	for (i = 0; i <= 5; i++)
	{
		if (semctl(semid, i, SETVAL, 1) < 0)
		{
			perror("semctl");
			exit(EXIT_FAILURE);
		}
	}


	initializeMatrix();
	float prob;
	prob = atof(argv[1]);
	int index = 0;
	while (index < n)
	{
		float rno = rand() * 1.0 / RAND_MAX;
		if (rno < prob)
		{
			detectDeadlock();
		}
		else
		{
			int pid = fork();
			if (pid == 0)
			{
				char dir[2];
				dir[0] = seq[index];
				dir[1] = '\0';
				char sidx[10];
				sprintf(sidx, "%d", index);
				char sn[10];
				sprintf(sn, "%d", n);
				execlp( "./train", "./train", dir, sidx, sn, (char *)(NULL));
				exit(0);
			}
			else
			{
				//printf("Process %d created\n", pid);
				pidarr[index] = pid;
				index++;
			}
		}
	}

	while (deadchld < n)
	{
		sleep(1);
		detectDeadlock();
	}
	printf("No deadlock occurred\n");
	return 0;

}