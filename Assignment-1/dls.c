#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

void killall(int);
void killchild(int);

void sighand(int signo, siginfo_t *info, void *extra);
int arr[100000];
pid_t main_pid;
pid_t cpid=-1,cpid2=-1;
int readFromFile(char* filename)
{
	int num,i;
	FILE *fp = fopen(filename,"r");
	if(fp == NULL)
	{
		printf("No such file exists\n");
		exit(1);
	}
	i = 0;
	while(fscanf(fp,"%d",&num) != EOF)
	{
		arr[i++] = num;
	}
	return i;
}

void print(int n)
{
	int i = 0;
	while(i < n)
	{
		printf("%d ",arr[i++]);
	}
	printf("\nTotal = %d\n",n);
}



int main(int argc, char* argv[])
{
	if(argc < 3)
	{
		printf("Error: Usage: a.out filepath count\n");
		exit(0);
	}
	int key = atoi(argv[2]);
	int n = readFromFile(argv[1]);
	print(n);
	main_pid = getpid();

	struct sigaction action;

    action.sa_flags = SA_SIGINFO;
    action.sa_sigaction = &sighand;

    if (sigaction(SIGUSR2, &action, NULL) == -1) {
            perror("sigusr: sigaction");
            return 0;
    }
    //printf("Signal handler installed, waiting for signal\n");
    //signal(SIGTERM,ignore);
    int position = search(key,0,n-1);
    if(position == 0)
    {
    	printf("Key not present in the array\n");
    }
    else
		printf("Key found at position %d in the array\n",position);
	return 0;

}

int search(int key,int low,int high)
{
	if(high - low + 1 <= 5)
	{
		while(low<=high){
			if(arr[low] == key)
			{
				//signal();
				union sigval value;
				//printf("Found at position: %d\n",low+1);

		        value.sival_int = low+1;
		        if(!sigqueue(main_pid, SIGUSR2, value) == 0) 
		        {
		        	perror("SIGSENT-ERROR:");
		            //printf("Signal sent successfully!!\n");
		        } 
		        usleep(500);
				return low+1;
			}
			low++;
		}
		return 0;
	}
	else
	{
		cpid = fork();
		int mid = (high - low)/2 + low;
		if(cpid == 0)
		{
			int index = search(key,low,mid);
			setpgid(0,main_pid);
			//printf("Parent %d,self = %d,low=%d,high = %d,value = %d\n",getppid(),getpid(),low,mid,index);
			exit(index);
		}
		else
		{
			cpid2 = fork();
			if(cpid2 == 0)
			{
				//sleep(rand()%4);
				int index = search(key,mid+1,high);
				setpgid(0,main_pid);
				//printf("Parent %d,self = %d,low=%d,high = %d,value = %d\n",getppid(),getpid(),mid+1,high,index);
				exit(index);
			}
			else
			{
				int r1,r2,rc_pid1,rc_pid2;
				rc_pid1 = waitpid(cpid,&r1,0);
				rc_pid2 = waitpid(cpid2,&r2,0);
				if (rc_pid1 > 0)
				{
					if (WIFEXITED(r1)) {
						//printf("Child %d exited with RC=%d\n",cpid,WEXITSTATUS(r1));
						r1 = WEXITSTATUS(r1);
					}
					else if(WIFSIGNALED(r1))
					{
						//printf("Child %d  of parent %d terminated\n",cpid,getpid());
						r1 = 0;
					} 
				}
				if (rc_pid2 > 0)
				{
					if (WIFEXITED(r2)) {
						//printf("Child %d exited with RC=%d\n",cpid2,WEXITSTATUS(r1));
						r2 = WEXITSTATUS(r2);
					}
					else if(WIFSIGNALED(r1))
					{
						//printf("Child %d  of parent %d terminated\n",cpid,getpid());
						r2 = 0;
					}
				  
				}
				//printf("r1 = %d, r2 = %d\n",r1,r2);
				if(r1 == 0 && r2 == 0)
				{
					return 0;
				}
				else if(r1 == 0)
				{
					return r2;
				}
				else
				{
					return r1;
				}
			}
		}
	}
	return 0;
}

void sighand(int signo, siginfo_t *info, void *extra)
{
       int position = info->si_value.sival_int;
       printf("Key found at position %d in the array\n",position);
       killpg(main_pid,SIGTERM);
       exit(0);
}

