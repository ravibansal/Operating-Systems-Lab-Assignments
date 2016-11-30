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

#define BUFF 20
int writeToPipe(int pipe_fd,int data)
{
	char buf[BUFF];
	sprintf(buf,"%d",data);
	int bytes = write(pipe_fd,buf,BUFF);
	if(bytes == -1){
		printf("PID: %d \n",getpid());
		perror("Error in writing to pipess");
		return -1;
	}
	return bytes;
}

int readFromPipeBlocking(int pipe_fd,int *data)
{
	char buf[BUFF];
	int bytes = read(pipe_fd,buf,BUFF);
	if(bytes == -1){
		//printf("PID: %d ",getpid());
		perror("Error in reading from pipe");
		return -1;
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

int list[1001];
int lastlist[1001];
int lastpid[1001];
int pids[1001];
int list_num = 0;
int last_list_num = 0;
int npid = 0;


int addToList(int pid,int value)
{
	int i;
	for(i=0;i<list_num;i++)
	{
		if(pid == pids[i])
			return 0;
	}
	//printf("Adding :%d %d\n",pid,value);
	pids[list_num] = pid;
	list[list_num++] = value;
	return 1;
}

int main(int argc, char* argv[])
{
	//printf("In node\n");
	int nnbr,value;
	int N;
	
	readFromPipeBlocking(STDIN_FILENO,&N);
	readFromPipeBlocking(STDIN_FILENO,&value);

	readFromPipeBlocking(STDIN_FILENO,&nnbr);
	//scanf("%d%d",&value,&nnbr);
	//printf("value = %d nnbr =%d\n",value,nnbr);
	int sum = 0;
	if(nnbr <= 0)
	{
		exit(value);
	}

	int writefd[nnbr],readfd[nnbr];
	int i,j,k;
	for(i=0;i<nnbr;i++)
	{
		readFromPipeBlocking(STDIN_FILENO,&writefd[i]);
		readFromPipeBlocking(STDIN_FILENO,&readfd[i]);
		//scanf("%d%d",&writefd[i],&readfd[i]);
		//writefd[i] = atoi(argv[k++]);
	}

	

	lastlist[last_list_num] = value;
	lastpid[last_list_num++] = getpid();
	pids[list_num] = getpid();
	list[list_num++] = value;
	//printf("PID = %d, NNBR = %d\n",  getpid(),nnbr);
	for(i=0;i<N;i++)
	{
		for(j=0;j<nnbr;j++)
		{
			writeToPipe(writefd[j],last_list_num);
			for(k=0;k<last_list_num;k++)
			{
				writeToPipe(writefd[j],lastpid[k]);
				writeToPipe(writefd[j],lastlist[k]);
			}
		}
		last_list_num = 0;

		if(list_num == N){
			//printf("Diameter is %d\n",i+1);
			break;
		}

		for(j=0;j<nnbr;j++)
		{
			int num = 0;
			readFromPipeBlocking(readfd[j],&num);
			for(k=0;k<num;k++)
			{
				int temp;
				int ptemp;
				readFromPipeBlocking(readfd[j],&ptemp);
				readFromPipeBlocking(readfd[j],&temp);
				if(addToList(ptemp,temp))
				{
					lastpid[last_list_num] = ptemp;
					lastlist[last_list_num++] = temp;
				}
			}
		}

		

		/*for(j = 0;j<last_list_num;j++)
		{
			pids[list_num] = lastpid[j];
			list[list_num++] = lastlist[j];
			sum += lastlist[j];
		}*/

	}

	for(i=0;i<list_num;i++)
		sum += list[i];

	int mean = (sum) / (list_num);
	//printf("Returning mean = %d\n",mean);
	exit(mean);

}