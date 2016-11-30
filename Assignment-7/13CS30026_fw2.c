#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>

#define MAX_NUM_OF_NODES 101
int graph[MAX_NUM_OF_NODES][MAX_NUM_OF_NODES];
int dist[MAX_NUM_OF_NODES][MAX_NUM_OF_NODES];

int readcount=0;
int writecount=0;
struct arg_struct {
    int n;
    int i;
    int k;
};
// pthread_rwlock_t  rwlock;
// pthread_cond_t read_sig;
pthread_mutex_t resource;
pthread_mutex_t rmutex;
pthread_mutex_t wmutex;
pthread_mutex_t tryread;

void *jloop(void *arg)
{
	struct arg_struct *args=(struct arg_struct *)arg;
	int i,j,k,n;
	n=(*args).n;
	i=(*args).i;
	k=(*args).k;
	// printf("%d %d %d\n",n,i,k);
	for(j=0;j<n;j++)
	{
		
		int rc=pthread_mutex_lock(&tryread);
		if (rc) { /* an error has occurred */
		    perror("pthread_tryread_lock");
		    pthread_exit(NULL);
		}
		//initial block of reading
		rc= pthread_mutex_lock(&rmutex);
		if (rc) { /* an error has occurred */
			    perror("pthread_read_lock");
			    pthread_exit(NULL);
		}
		readcount++;
		if(readcount==1)
		{
			pthread_mutex_lock(&resource);
		}
		rc=pthread_mutex_unlock(&rmutex);
		if(rc)
		{
			perror("pthread_read_unlock");
		    pthread_exit(NULL);
		}
		///////////////////////////////
		rc=pthread_mutex_unlock(&tryread);
		if (rc) { /* an error has occurred */
		    perror("pthread_tryread_unlock");
		    pthread_exit(NULL);
		}

		//Reading Starts
		if((dist[i][k]+dist[k][j]) < dist[i][j])
		{
			
		//////Reading is over/////////////////////
			pthread_mutex_lock(&rmutex);
			readcount--;
			if(readcount==0)
			{
				pthread_mutex_unlock(&resource);
			}
			pthread_mutex_unlock(&rmutex);
		///////Final read block is over///////////////

		////////Write Block Starts//////////////
			
			//Initialize Write
			rc=pthread_mutex_lock(&wmutex);
			if(rc){
				perror("pthread_wmutex_lock");
			    pthread_exit(NULL);
			}
			writecount++;
			if(writecount==1)
			{
				rc=pthread_mutex_lock(&tryread);
				if (rc) { /* an error has occurred */
				    perror("pthread_tryread_lock");
				    pthread_exit(NULL);
				}
			}
			rc=pthread_mutex_unlock(&wmutex);
			if(rc){
				perror("pthread_wmutex_unlock");
			    pthread_exit(NULL);
			}

			//Writing
			rc=pthread_mutex_lock(&resource);
			if (rc) { /* an error has occurred */
			    perror("pthread_write_lock");
			    pthread_exit(NULL);
			}
			dist[i][j]=dist[i][k]+dist[k][j];
			// pthread_cond_broadcast(&read_sig);
			//unlock
			rc=pthread_mutex_unlock(&resource);
			if (rc) {
			    perror("pthread_write_unlock");
			    pthread_exit(NULL);
			}

			//finsihed writing
			rc=pthread_mutex_lock(&wmutex);
			if(rc){
				perror("pthread_wmutex_lock");
			    pthread_exit(NULL);
			}
			writecount--;
			if(writecount==0)
			{
				rc=pthread_mutex_unlock(&tryread);
				if (rc) { /* an error has occurred */
				    perror("pthread_tryread_unlock");
				    pthread_exit(NULL);
				}
			}
			rc=pthread_mutex_unlock(&wmutex);
			if(rc){
				perror("pthread_wmutex_unlock");
			    pthread_exit(NULL);
			}
			
		}
		else{
			//////Reading is over/////////////////////
			pthread_mutex_lock(&rmutex);
			readcount--;
			if(readcount==0)
			{
				pthread_mutex_unlock(&resource);
			}
			pthread_mutex_unlock(&rmutex);
			///////Final read block is over///////////////
		}
		// pthread_mutex_unlock(&write_mutex);

	}
	pthread_exit(NULL);
}

int main()
{
	int i,j,k;
	int m,n;
	scanf("%d %d",&n,&m);
	for(i=0;i<n;i++)
	{
		for(j=0;j<n;j++)
		{
			graph[i][j]=0;
			if(i!=j)
				dist[i][j]=INT_MAX/2-1;
			else
				dist[i][j]=0;
		}
	}
	for(i=0;i<m;i++)
	{
		int u,v,w;
		scanf("%d %d %d",&u,&v,&w);
		dist[u-1][v-1]=w;
		dist[v-1][u-1]=w;
		graph[u-1][v-1]=1;
	}
	printf("\n\n");
	printf("Initial Dist Matrix:\n");
	for(i=0;i<n;i++)
	{
		for(j=0;j<n;j++)
		{
			if(dist[i][j]==((INT_MAX/2)-1))
			{
				printf("INF\t");
			}
			else
			{
				printf("%d\t",dist[i][j]);
			}
		}
		printf("\n");
	}
	printf("\n\n");
	pthread_t threads[n];
	pthread_attr_t attr;
	// int rc=pthread_rwlock_init(&rwlock,NULL);
	int rc=pthread_mutex_init(&resource,NULL);
	if(rc==-1)
	{
		perror("Error while inititalizing wrt");
		pthread_exit(NULL);
	}
	rc=pthread_mutex_init(&rmutex,NULL);
	if(rc==-1)
	{
		perror("Error while inititalizing mutex");
		pthread_exit(NULL);
	}
	rc=pthread_mutex_init(&wmutex,NULL);
	if(rc==-1)
	{
		perror("Error while inititalizing mutex");
		pthread_exit(NULL);
	}
	rc=pthread_mutex_init(&tryread,NULL);
	if(rc==-1)
	{
		perror("Error while inititalizing nord");
		pthread_exit(NULL);
	}
	// pthread_cond_init(&read_sig,NULL);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	printf("\n\n");
	struct arg_struct args[n];
	for(k=0;k<n;k++)
	{
		for(i=0;i<n;i++)
		{
			args[i].n=n;
			args[i].i=i;
			args[i].k=k;
			pthread_create(&threads[i],&attr,jloop,(void *)&(args[i]));	
			// usleep(10000);
		}
		// pthread_cond_signal(&read_sig);
		for(i=0;i<n;i++)
		{
			pthread_join(threads[i],NULL);
		}
	}
	printf("Final Dist Matrix:\n");
	for(i=0;i<n;i++)
	{
		for(j=0;j<n;j++)
		{
			if(dist[i][j]==((INT_MAX/2)-1))
			{
				printf("INF\t");
			}
			else
			{
				printf("%d\t",dist[i][j]);
			}
		}
		printf("\n");
	}
	pthread_attr_destroy(&attr);
	// pthread_rwlock_destroy(&rwlock);
	// pthread_cond_destroy(&count_threshold_cv);
	pthread_mutex_destroy(&resource);
	pthread_mutex_destroy(&rmutex);
	pthread_mutex_destroy(&wmutex);
	pthread_mutex_destroy(&tryread);

	pthread_exit(NULL);	
}