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


#define ENTER 0
#define WITHDRAW 1
#define DEPOSIT 2
#define VIEW 3
#define LEAVE 4 

#define NOOFATMS 5

int shmid,semid,msgid;
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

typedef struct mymsgbuf {
    long    mtype;          /* Message type */   //here mtype is id (i.e. 1,2,3..n)
    int id;
    int accno;
    int req; //0-for account creation //1-for VIEW commaned(global consistency check)
} mymsgbuf;

typedef struct mymsgbuf2 {
    long    mtype;          /* Message type */   //here mtype is id (i.e. 1,2,3..n)
    int accno;
    int req; 
    int amount;
} mymsgbuf2;

void to_exit(int status)
{
	shmctl(shmid,IPC_RMID,0);
    msgctl(msgid,IPC_RMID,NULL);
	exit(status);
}

int send_message( int qid, struct mymsgbuf *qbuf )
{
    int     result, length;

    /* The length is essentially the size of the structure minus sizeof(mtype) */
    length = sizeof(struct mymsgbuf) - sizeof(long);       

    if((result = msgsnd( qid, qbuf, length, 0)) == -1)
    {
        perror("Error in sending message");
		to_exit(1);
    }
   
    return(result);
}
int read_message( int qid, long type, struct mymsgbuf *qbuf )
{
    int     result, length;

    /* The length is essentially the size of the structure minus sizeof(mtype) */
    length = sizeof(struct mymsgbuf) - sizeof(long);        

    if((result = msgrcv( qid, qbuf, length, type,  0)) == -1)
    {
        perror("Error in receiving message");
		to_exit(1);
    }
    
    return(result);
}

int send_message_2( int qid, struct mymsgbuf2 *qbuf )
{
    int     result, length;

    /* The length is essentially the size of the structure minus sizeof(mtype) */
    length = sizeof(struct mymsgbuf2) - sizeof(long);       

    if((result = msgsnd( qid, qbuf, length, 0)) == -1)
    {
        perror("Error in sending message");
		to_exit(1);
    }
   
    return(result);
}
int read_message_2( int qid, long type, struct mymsgbuf2 *qbuf )
{
    int     result, length;

    /* The length is essentially the size of the structure minus sizeof(mtype) */
    length = sizeof(struct mymsgbuf2) - sizeof(long);        

    if((result = msgrcv( qid, qbuf, length, type,  0)) == -1)
    {
        perror("Error in receiving message");
		to_exit(1);
    }
    
    return(result);
}

int local_cons(int accno)
{
	FILE *fp=fopen("ATM_Locator.txt","r");
	struct timeval t;
	t.tv_sec=0;
	t.tv_usec=0;
	int i,j;
	int money=0;
	for(i=0;i<NOOFATMS;i++)
	{
		int x,y,z,lmemkey;
		fscanf(fp,"%d %d %d %d",&x,&y,&z,&lmemkey);
		int lshmid=shmget((key_t)lmemkey,(100*sizeof(struct acc)+500*sizeof(struct trans)),IPC_CREAT|0666);
		void *ptr;
		struct acc *ptr1;
		struct trans *ptr2;
		ptr=shmat(lshmid,NULL,0);
	    ptr1=(struct acc*)ptr;
	    ptr2=(struct trans*)(ptr+100*sizeof(struct acc));
	    for(j=0;j<100;j++)
	    {
	    	if(ptr1[j].accno==-1)
	    	{
	    		break;
	    	}
	    	else if(ptr1[j].accno==accno)
	    	{
	    		if(timercmp(&t,&(ptr1[j].t),<)!=0)
	    		{
	    			t=ptr1[j].t;
	    			money=ptr1[j].balance;
	    		}
	    		break;
	    	}
	    }
	    int temp=shmdt(ptr);
	    if(temp==-1)
	    {	
	    	printf("%d\n",i+1);
	    	perror("failed to detach memory segment from ATM id");
	    	to_exit(1);
	    }
	}
	fclose(fp);
	fp=fopen("ATM_Locator.txt","r");
	for(i=0;i<NOOFATMS;i++)
	{
		int x,y,z,lmemkey;
		fscanf(fp,"%d %d %d %d",&x,&y,&z,&lmemkey);
		int lshmid=shmget((key_t)lmemkey,(100*sizeof(struct acc)+500*sizeof(struct trans)),IPC_CREAT|0666);
		void *ptr;
		struct acc *ptr1;
		struct trans *ptr2;
		ptr=shmat(lshmid,NULL,0);
	    ptr1=(struct acc*)ptr;
	    ptr2=(struct trans*)(ptr+100*sizeof(struct acc));
	    for(j=0;j<500;j++)
	    {
	    	if(ptr2[j].accno==accno)
	    	{
	    		if(timercmp(&t,&(ptr2[j].t),<)!=0)
	    		{
	    			if(ptr2[j].type==DEPOSIT)
	    				money=money+ptr2[j].amount;
	    			else
	    				money=money-ptr2[j].amount;
	    		}
	    	}
	    }
	    int temp=shmdt(ptr);
	    if(temp==-1)
	    {
	    	printf("%d\n",i+1);
	    	perror("failed to detach memory segment from ATM id");
	    	to_exit(1);
	    }
	}
	fclose(fp);
	return money;
}

int main(int argc,char *argv[])
{
	if(argc<5)
	{
		printf("Usage: <id> <msgqkey> <semkey> <memsegkey>\n");
		exit(1);
	}
    signal(SIGINT,to_exit);
	signal(SIGTERM,to_exit);
    signal(SIGKILL,to_exit);
    int msgkey,semkey,memsegkey,mastermsgid,id,mastermsgkey=1230;
    sscanf(argv[1],"%d",&id);
    sscanf(argv[2],"%d",&msgkey);
    sscanf(argv[3],"%d",&semkey);
    sscanf(argv[4],"%d",&memsegkey);
    // printf("%d %d %d %d\n",id,msgkey,semkey,memsegkey);
    void *ptr;
    struct acc *ptr1;
    struct trans *ptr2;
    // printf("hello\n");
    shmid=shmget((key_t)memsegkey,(100*sizeof(struct acc)+500*sizeof(struct trans)),IPC_CREAT|0666);
	msgid=msgget((key_t)msgkey,IPC_CREAT|0666); //to communicate with ATM process
	if(shmid==-1)
    {
    	perror("Shared memory creation failed");
    	exit(1);
    }
    if(msgid==-1)
    {
    	perror("Message Queue creation failed");
    	exit(1);
    }
	mastermsgid=msgget((key_t)mastermsgkey,IPC_CREAT|0666);
    if(mastermsgid==-1)
    {
        perror("Message Queue creation failed for master");
        exit(1);
    }
    semid=semget((key_t)semkey,NOOFATMS,0666|IPC_CREAT);
    if(semid==-1)
    {
        perror("Semaphore creation failed");
        exit(1);
    }
    semctl(semid,(id-1),SETVAL,1); //for subsemaphore
    ptr=shmat(shmid,NULL,0);
    ptr1=(struct acc*)ptr;
    ptr2=(struct trans*)(ptr+100*sizeof(struct acc));
    int i;
    for(i=0;i<500;i++)
    {
        ptr2[i].accno=-1;
    }
    for(i=0;i<100;i++)
    {
        ptr1[i].accno=-1;
    }
    while(1)
    {	
    	mymsgbuf2 msg2;
    	// printf("reading\n");
        read_message_2(msgid,(long)id,&msg2);
        // printf("Client Connected\n");
    	int accno=msg2.accno;
    	int req=msg2.req;
    	if(req==ENTER)
    	{
    		mymsgbuf msg_init;
	    	msg_init.accno=accno;
	    	msg_init.id=id;
	    	msg_init.req=ENTER;
	    	msg_init.mtype=1230;
    		send_message(mastermsgid,&msg_init);
            printf("ATM%d: Client %d entered\n",id,accno);
    	}
    	else if(req==WITHDRAW)
    	{
    		printf("ATM%d: running a local consistency check for a/c %d\n",id,accno);
            int money=local_cons(accno);
    		mymsgbuf2 msgtemp; //here mtype to send to client is client's account number /pid
    		if(money>=msg2.amount)
    		{
    			msgtemp.mtype=(long)accno;
	    		msgtemp.accno=accno;
	    		msgtemp.req=-1;
	    		msgtemp.amount=1;
    			int flg=0;
	    		for(i=0;i<500;i++)
	    		{
	    			if(ptr2[i].accno==-1)
	    			{
	    				ptr2[i].accno=accno;
	    				gettimeofday(&(ptr2[i].t),NULL);
	    				ptr2[i].type=WITHDRAW;
	    				ptr2[i].amount=msg2.amount;
	    				flg=1;
	    				break;
	    			}
	    		}
	    		if(flg==0)
	    		{
	    			perror("Memory Overflow");
	    			to_exit(1);
	    		}
    		}
    		else
    		{
	    		msgtemp.mtype=(long)accno;
	    		msgtemp.accno=accno;
	    		msgtemp.req=-1;
	    		msgtemp.amount=-1;
    		}
    		send_message_2(msgid,&msgtemp);
    	}
    	else if(req==DEPOSIT)
    	{
    		int flg=0;
    		for(i=0;i<500;i++)
    		{
    			if(ptr2[i].accno==-1)
    			{
    				ptr2[i].accno=accno;
    				gettimeofday(&(ptr2[i].t),NULL);
    				ptr2[i].type=DEPOSIT;
    				ptr2[i].amount=msg2.amount;
    				flg=1;
    				break;
    			}
    		}
    		if(flg==0)
    		{
    			perror("Memory Overflow");
    			to_exit(1);
    		}
    	}	
    	else if(req==VIEW)
    	{
    		printf("ATM%d: running a consistency check for a/c %d\n",id,accno);
            mymsgbuf msgsend,msgrecv;
    		msgsend.id=id;
    		msgsend.mtype=1230;
    		msgsend.accno=accno;
    		msgsend.req=VIEW;
    		send_message(mastermsgid,&msgsend);
    		read_message(mastermsgid,(long)id,&msgrecv);
    		//to send balance to client
            // printf("%d\n",msgrecv.req);
    		for(i=0;i<100;i++)
    		{
    			if(ptr1[i].accno==accno)
    			{
    				ptr1[i].balance=msgrecv.req;
    				break;
    			}
                else if(ptr1[i].accno==-1)
                {
                    ptr1[i].accno=accno;
                    gettimeofday(&(ptr1[i].t),NULL);
                    ptr1[i].balance=msgrecv.req;
                    break;
                }
    		}
    		mymsgbuf2 msgtemp;
    		msgtemp.mtype=accno;
    		msgtemp.accno=accno;
    		msgtemp.req=-1;
    		msgtemp.amount=ptr1[i].balance;
            // printf("%d\n",msgtemp.amount);
    		send_message_2(msgid,&msgtemp);
    	}
        else if(req==LEAVE)
        {
            printf("ATM%d: Client %d left\n",id,accno);
        }
    	else
    	{
    		perror("Invalide Request");
    	}
    }
    return 0;	
}
