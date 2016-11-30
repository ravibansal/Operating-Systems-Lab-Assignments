#include <stdio.h>
#include <unistd.h>
#include  <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#define AVAILABLE 30001
#define BUSY 30002
#define BUFF 10
#define MAX 30000

void killAllChilds(int child_id[]);
int isPrime(int n);
void printarr(int primmearr[]);
int writeToPipe(int pipe_fd,int data);
int readFromPipeBlocking(int pipe_fd,int *data);
int readFromPipe(int pipe_fd,int *data);
int addToPrimeList(int primmearr[],int number);
int detect_primes(int read_end,int write_end);
int deal_with_childs(int read_end[],int write_end[],int primmearr[]);

int isPrime(int n)
{
	int i;
	int n_sqaure = n * n;
	for(i = 2; i*i <= n; i++){
		if(n%i == 0)
			return 0;
	}
	return 1;
}

int numprimes = 0;
int N,K;

int main()
{
	//printf("Enter the number of primes to generate: ");
	//scanf("%d",&N);
	printf("Enter the number of processes to create: ");
	scanf("%d",&K);
	N  = 2*K;
	int primmearr[N];
	int numprimes = 0;
	int i,cid;
	int child_id[K];
	int read_end[K];
	int write_end[K];

	for(i=0;i<K;i++)
	{
		int p_to_c[2];
		int c_to_p[2];
		pipe(p_to_c);
		pipe2(c_to_p, O_NONBLOCK);
		cid = fork();
		if(cid==0)
		{
			detect_primes(p_to_c[0],c_to_p[1]);
			//child process
		}
		else
		{
			child_id[i] = cid;
			read_end[i] = c_to_p[0];
			write_end[i] = p_to_c[1];
			//deal_with_child(p_to_c[1],c_to_p[0]);
		}
	}
	sleep(2);
	deal_with_childs(read_end,write_end,primmearr);
	killAllChilds(child_id);
	return 0;

}

void printarr(int primmearr[])
{	
	int i;
	for (i = 0; i < 2*K; ++i)
	{
		printf("%d ",primmearr[i]);
		/* code */
	}
	printf("\n");
}

int writeToPipe(int pipe_fd,int data)
{
	char buf[BUFF];
	sprintf(buf,"%d",data);
	int bytes = write(pipe_fd,buf,BUFF);
	if(bytes == -1){
		perror("Error in writing to pipe");
		exit(1);
	}
	return bytes;
}

int readFromPipeBlocking(int pipe_fd,int *data)
{
	char buf[BUFF];
	int bytes = read(pipe_fd,buf,BUFF);
	if(bytes == -1){
		perror("Error in reading from pipe");
		exit(1);
	}
	sscanf(buf,"%d",data);
	return bytes;
}

int readFromPipe(int pipe_fd,int *data)
{
	char buf[BUFF];
	int bytes = read(pipe_fd,buf,BUFF);
	if(bytes == -1){
		return bytes;
	}
	sscanf(buf,"%d",data);
	return bytes;
}

int addToPrimeList(int primmearr[],int number)
{
	int i;
	for(i=0;i<numprimes;i++)
	{
		if(primmearr[i] == number)
		{
			return numprimes;
		}
	}
	primmearr[numprimes++] = number;
	return numprimes;
}

void killAllChilds(int child_id[])
{
	int i;
	for(i=0;i<K;i++)
	{
		kill(child_id[i],SIGTERM);
	}
}


int detect_primes(int read_end,int write_end)
{

	//printf("In chl\n");
	int numbers[K];
	int i;
	while(1)
	{
		writeToPipe(write_end,AVAILABLE);

		for (i = 0; i < K; ++i)
		{
			readFromPipe(read_end,&numbers[i]);
		}
		writeToPipe(write_end,BUSY);
		for (i = 0; i < K; ++i)
		{
			if(isPrime(numbers[i]))
			{
				writeToPipe(write_end,numbers[i]);
			}
		}
	}
}



int deal_with_childs(int read_end[],int write_end[],int primmearr[])
{
	//printf("In parent\n");
	int i,j,flag=0;
	while(1)
	{
		for(i = 0; i < K; i++)
		{
			//printf("Writing numbers\n");
			int num;
			int res = readFromPipe(read_end[i],&num);
			//printf("%d %d\n",res,num);
			if(res == -1 || num == BUSY)
				continue;
			if(num != AVAILABLE)
			{
				addToPrimeList(primmearr,num);
				if(numprimes >= 2*K)
				{
					flag = 1;
					break;
				}
			}
			else
			{
				//printf("Writinf numbrs");
				for (j = 0; j < K; ++j)
				{
					int rand_no = rand()%MAX;
					writeToPipe(write_end[i],rand_no);
				}
				
			}
		}
		if(flag)
			break;
	}
	printarr(primmearr);
	return 0;
}