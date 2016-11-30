#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
int poid[100000];
int curr;
int qid;
void ctrlc();
typedef struct mymsgbuf {
            long    mtype;          /* Message type */
            char command[1000];
            char msg[6000];
            int pid;
            int terminal;
}mymsgbuf;

int get_queue_ds( int qid, struct msqid_ds *qbuf )
{
        if( msgctl( qid, IPC_STAT, qbuf) == -1)
        {
                return(-1);
        }
        
        return(0);
}

int change_queue_size( int qid)
{
        struct msqid_ds tmpbuf;

        /* Retrieve a current copy of the internal data structure */
        get_queue_ds( qid, &tmpbuf);
        tmpbuf.msg_qbytes=1000000;
        /* Update the internal data structure */
        if( msgctl( qid, IPC_SET, &tmpbuf) == -1)
        {
                return(-1);
        }
        
        return(0);
}
int send_message( int qid, struct mymsgbuf *qbuf )
{
        int     result, length;

        /* The length is essentially the size of the structure minus sizeof(mtype) */
        length = sizeof(struct mymsgbuf) - sizeof(long);       

        if((result = msgsnd( qid, qbuf, length, 0)) == -1)
        {
                return(-1);
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
                return(-1);
        }
        
        return(result);
}

void *cpl(int qid)
{
    mymsgbuf msg;
    curr=1;
    while(1) //Coupling
    {
       int temp;
       if(temp=read_message(qid,1,&msg)==-1)
            continue;
        else
        {
            poid[curr]=msg.pid;
            msg.terminal=curr;
            msg.mtype=(long)msg.pid;
            curr++;
            msg.pid=-1;
            if(send_message(qid,&msg)==-1)
            {

            }
            int i;
            printf("Updated List Of Clients:\n");
            for(i=1;i<curr;i++)
            {
                if(poid[i]!=-1)
                {
                    printf("Terminal: %d   Pid: %d\n",i,poid[i]);
                }
            }
        }
    }
    pthread_exit(NULL);
}

void *uncpl(int qid)
{
    mymsgbuf msg;
    while(1) //Uncoupling
    {
        int temp;
        if(temp=read_message(qid,2,&msg)==-1)
            continue;
        else
        {
            poid[msg.terminal]=-1;
            printf("Updated List Of Clients:\n");
            int i;
            for(i=1;i<curr;i++)
            {
                if(poid[i]!=-1)
                {
                    printf("Terminal: %d   Pid: %d\n",i,poid[i]);
                    msg.mtype=poid[i];
                    if(send_message(qid,&msg)==-1)
                    {
                        perror("Error while sending message");
                        msgctl(qid,IPC_RMID,NULL);
                        exit(1);
                    }
                }
            }
        }
        //printf("Yes1\n");
    }
    pthread_exit(NULL);
}

void *rcvsend(int qid)
{
    mymsgbuf msg;
    while(1) //Receiving and and Sending Message to server
    {
        int temp;
        if(temp=read_message(qid,3,&msg)==-1)
            continue;
        else
        {
            int i=0;
            int tid=msg.pid;
            for(i=1;i<curr;i++)
            {
                if(poid[i]!=-1 && poid[i]!=tid)
                {
                    msg.mtype=poid[i];
                    if(send_message(qid,&msg)==-1)
                    {
                        perror("Error while sending message");
                        msgctl(qid,IPC_RMID,NULL);
                        exit(1);
                    }
                }
            }
        }
        //printf("Yes3\n");
    }
    pthread_exit(NULL);
}
int main() {
    key_t key=1234;
    qid=msgget(key,IPC_CREAT|0666);
    if(change_queue_size(qid)==-1)
    {
        perror("Error while changing the size of Message queue");
        exit(1);
    }
    int j=0;
    for(j=0;j<10000;j++)
        poid[j]=-1;
    printf("qid : %d\n",qid);
    pthread_t threads[3];
   int rc;
   long i;
   i=0;
   rc = pthread_create(&threads[i], NULL, 
                      cpl, qid);
   i=1;
   if (rc){
    printf("Error:unable to create thread, %d\n",rc);
    return 0;
   }
   rc = pthread_create(&threads[i], NULL, 
                      uncpl, qid);
   if (rc){
    printf("Error:unable to create thread, %d\n",rc);
    return 0;
   }
   i=2;
    rc = pthread_create(&threads[i], NULL, 
                      rcvsend, qid);
   if (rc){
    printf("Error:unable to create thread, %d\n",rc);
    return 0;
   } //printf("Yes\n");
   //while(1){
   signal(SIGINT,ctrlc);
   pthread_exit(NULL);
    return 0;
}

void ctrlc()
{
    msgctl(qid,IPC_RMID,NULL);
    exit(0);
}