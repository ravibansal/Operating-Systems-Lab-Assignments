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

int shmid,key=1230,semid,msgid;
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

void to_exit(int status)
{
	shmctl(shmid,IPC_RMID,0);
    semctl(semid,IPC_RMID,0);
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

int global_cons(int accno)
{
	int i,j;
	int atmsid,lshmid;
	struct timeval t;
	int money=0;
	lshmid=shmget((key_t)key,100*sizeof(struct acc),IPC_CREAT|0666);
	struct acc *ptr;
	ptr=(struct acc*)shmat(lshmid,NULL,0);
	for(i=0;i<100;i++)
	{
		if(ptr[i].accno==accno)
		{
			t=ptr[i].t;
			money=ptr[i].balance;
			break;
		}
	}
	int index=i;
	FILE *fp=fopen("ATM_Locator.txt","r");
	for(i=0;i<NOOFATMS;i++)
	{
		int x,y,z,lmemkey;
		fscanf(fp,"%d %d %d %d",&x,&y,&z,&lmemkey);
		int lshmid1=shmget((key_t)lmemkey,(100*sizeof(struct acc)+500*sizeof(struct trans)),IPC_CREAT|0666);
		void *ptri;
		struct acc *ptr1;
		struct trans *ptr2;
		ptri=shmat(lshmid1,NULL,0);
	    ptr1=(struct acc*)ptri;
	    ptr2=(struct trans*)(ptri+100*sizeof(struct acc));
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
	    		ptr2[j].accno=-1;
	    	}
	    }
	    int temp=shmdt(ptri);
	    if(temp==-1)
	    {
	    	printf("%d\n",i+1);
        perror("failed to detach memory segment from ATM id");
	    	to_exit(1);
	    }
	}
	fclose(fp);
	ptr[index].balance=money;
	gettimeofday(&(ptr[index].t),NULL);
	int temp=shmdt(ptr);
  if(temp==-1)
  {
  	printf("%d\n",i+1);
    perror("failed to detach memory segment from ATM id");
  	to_exit(1);
  }
    return money;
}

int main()
{
	struct acc *ptr;
	signal(SIGINT,to_exit);
  shmid=shmget((key_t)key,100*sizeof(struct acc),IPC_EXCL|IPC_CREAT|0666);
	msgid=msgget((key_t)key,IPC_CREAT|IPC_EXCL|0666); //to communicate with ATM process
  if(shmid==-1)
  {
  	perror("Shared memory creation failed");
  	to_exit(1);
  }
  if(msgid==-1)
  {
  	perror("Message Queue creation failed");
  	to_exit(1);
  }
  ptr=(struct acc*)shmat(shmid,NULL,0);
  // printf("shmid=%d ptr=%p\n",shmid, ptr);
  int i;
  for(i=0;i<100;i++)
  {
  	ptr[i].accno=-1;
  }
  semid=semget((key_t)key,NOOFATMS,0666|IPC_CREAT|IPC_EXCL);
  if(semid==-1)
  {
  	perror("Semaphores creation failed");
  	to_exit(1);
  }
  FILE *fp=fopen("ATM_Locator.txt","w");
  i=0;
  for(i=1;i<=NOOFATMS;i++)
  {
  	fprintf(fp,"%d %d %d %d\n",i,(key+i),key,(key+i));//ATM: Id MsgQueKey SemKey MemorySegKey
  }
  fclose(fp);
  for(i=1;i<=NOOFATMS;i++)
  {
  	// if(fork() == 0)
   //  {
      // printf("Hello\n");
      // perror("");
      // int temp=execlp("xterm","xterm","-hold","-e","atm",i,(key+i),key,(key+i),(char *)(NULL));
      char command[100];
      sprintf(command,"./atm %d %d %d %d &",i,(key+i),key,(key+i));
      system(command);
      // printf("%d\n",temp);
      // printf("done\n");
      // perror("");
    // }
  }
  while(1)
  {
		mymsgbuf msg;
    //printf("reading\n");
    long type=1230;
		read_message(msgid,type,&msg);
		int id=msg.id;
		int accno=msg.accno;
		if(msg.req==ENTER) //account checking and creation
		{
			int flg=0;
			for(i=0;i<100;i++)
			{
				if(ptr[i].accno==-1)
				{
					break;
				}
				else if(ptr[i].accno==accno)
				{
					flg=1;
					break;
				}
			}
			if(flg==0)
			{
				if(i>=100)
				{
					perror("Memory Exceeded");
					to_exit(1);
				}
				ptr[i].accno=accno;
				gettimeofday(&(ptr[i].t),NULL);
				ptr[i].balance=0;
			  // printf("Account :%d added with balance: %d\n",accno,ptr[i].balance);
      }
		}
		else if(msg.req==VIEW)//VIEW
		{
			int money=global_cons(accno);
      printf("Master: Done\n");
			mymsgbuf msg_temp;
			msg_temp.id=id;
			msg_temp.mtype=(long)id;
			msg_temp.accno=accno;
			msg_temp.req=money;
			send_message(msgid,&msg_temp);
		}
  }
  shmctl(shmid,IPC_RMID,0);
  semctl(semid,IPC_RMID,0);
  msgctl(msgid,IPC_RMID,NULL);
  return 0;
}