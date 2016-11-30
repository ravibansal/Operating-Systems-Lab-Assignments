#include <sys/shm.h>
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
#include <errno.h>
#include <unistd.h>

struct acc{
	int accno;
	struct timeval t;
	int balance;
};

struct trans{
	int accno;
	struct timeval t;
	int type; 
	int amount;
};

void hello()
{
	int shmid,key=1200,semid,msgid;
    void *ptr;
    struct acc *ptr1;
    struct trans *ptr2;
    shmid=shmget((key_t)key,(100*sizeof(struct acc)+500*sizeof(struct trans)),IPC_CREAT|0666);
    if(shmid==-1)
    {
    	perror("Shared memory creation failed");
    	exit(1);
    }
    ptr=shmat(shmid,NULL,0);
    // ptr=shmat(shmid,NULL,0);
    ptr1=(struct acc*)ptr;
    ptr2=(struct trans*)(ptr+100*sizeof(struct acc));
    ptr1[99].accno=2;
    ptr1[99].balance=2;
    printf("da %d\n",shmdt(ptr));
}
int main()
{
	int shmid,key=1200,semid,msgid;
    void *ptr;
    struct acc *ptr1;
    struct trans *ptr2;
    shmid=shmget((key_t)key,(100*sizeof(struct acc)+500*sizeof(struct trans)),IPC_EXCL|IPC_CREAT|0666);
    if(shmid==-1)
    {
    	perror("Shared memory creation failed");
    	exit(1);
    }
    ptr=shmat(shmid,NULL,0);
    // ptr=shmat(shmid,NULL,0);
    ptr1=(struct acc*)ptr;
    ptr2=(struct trans*)(ptr+100*sizeof(struct acc));
    ptr1[99].accno=1;
    ptr1[99].balance=1;
    gettimeofday(&(ptr1[99].t),NULL);
    ptr2[0].accno=2;
    ptr2[0].amount=10;
    usleep(1);
    gettimeofday(&(ptr2[0].t),NULL);
    printf("%d %d %d %d\n",ptr1[99].accno,ptr1[99].balance,ptr2[0].accno,ptr2[0].amount);
    struct timeval t;
    timersub(&(ptr2[0].t),&(ptr1[99].t),&t);
    printf("%ld %ld\n",t.tv_sec,t.tv_usec);
    printf("%d\n",timercmp(&(ptr1[99].t),&(ptr2[0].t),<));
    	// printf("Yeah\n");
    // else
    	// printf("no\n");
    // printf("%d\n",shmdt(ptr));
    // printf("%d\n",shmdt(ptr));
    hello();
    printf("%d %d %d %d\n",ptr1[99].accno,ptr1[99].balance,ptr2[0].accno,ptr2[0].amount);
    printf("%d\n",shmdt(ptr));
    // printf("%d\n",shmdt(ptr));
    shmctl(shmid,IPC_RMID,0);
    return 0;
}