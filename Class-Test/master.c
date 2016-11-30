/***
* Name : Prabhat Agarwal
* Roll : 13CS10060
* M/c No : 42
* Problem :
*/


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
#include <sys/wait.h>
#include <limits.h>
#include <errno.h>

#define MAX_N 1001
#define MAX_M 1001
#define BUFF 20

int writeToPipe(int pipe_fd, int data)
{
	char buf[BUFF];
	sprintf(buf, "%d", data);
	int bytes = write(pipe_fd, buf, BUFF);
	if (bytes == -1) {
		perror("Error in writing to pipe");
		exit(1);
	}
	return bytes;
}

int N, M;
int edges[MAX_N][MAX_N];
int readfd[MAX_N][MAX_N];
int writefd[MAX_N][MAX_N];
int ncnt[MAX_N];
int cpid[MAX_N];
int val[MAX_N];
int mean[MAX_N];

int main()
{
	printf("Enter N and M:");
	scanf("%d%d", &N, &M);

	int i, j;
	srand(time(NULL));
	for (i = 0; i < M; i++)
	{
		int u, v;
		scanf("%d%d", &u, &v);
		ncnt[u]++;
		ncnt[v]++;
		edges[u][v] = 1;
		edges[v][u] = 1;
	}

	for (i = 1; i <= N; i++)
	{
		for (j = 0; j <= N; j++)
		{
			if (edges[i][j] == 1)
			{
				int fd[2];
				if (pipe(fd) == -1)
				{
					perror("pipe");
					exit(1);
				}
				readfd[i][j] = fd[0];
				writefd[j][i] = fd[1];
			}
		}
	}

	printf("Initial value of nodes:\n");

	for (i = 1; i <= N; i++)
	{

		int fd[2];
		pipe(fd);
		int pid = fork();
		switch (pid)
		{
		case -1 : perror("Error in fork");
			exit(EXIT_FAILURE);
			break;
		case 0 : dup2(fd[0], STDIN_FILENO);
			close(fd[0]);
			close(fd[1]);
			int status  = execlp("./node", "./node", (char*)(NULL));
			printf("here %d\n",status);
			exit(0);
		default:
			close(fd[0]);
			int temp = rand() % 100 + 1;
			printf("(%d , %d) ", pid, temp);
			val[i] = temp;
			writeToPipe(fd[1], N);
			writeToPipe(fd[1], temp);
			//intf("nnbr = %d\n",ncnt[i]);
			writeToPipe(fd[1], ncnt[i]);
			for (j = 1; j <= N; j++)
			{
				if (edges[i][j] == 1)
				{
					writeToPipe(fd[1], writefd[i][j]);
					writeToPipe(fd[1], readfd[i][j]);
					close(writefd[i][j]);
					close(readfd[i][j]);
				}
			}
			break;
		}
		close(fd[1]);
	}
	int sum = 0;
	for (i = 1; i <= N; i++)
	{
		sum += val[i];
	}
	int mean = sum / N;
	printf("\nFinal menas values received from nodes:\n");
	for (i = 0; i < N; i++)
	{
		int status;
		int pid = wait(&status);
		if (WEXITSTATUS(status) != mean)
			printf("%d retuned wrong mean = %d\n", pid, WEXITSTATUS(status));
		else
			printf("(%d, %d) ", pid, WEXITSTATUS(status));
	}
	printf("\nThe mean is: %d %d\n", sum, mean);
	return 0;
}