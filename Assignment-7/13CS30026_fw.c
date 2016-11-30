#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>

#define MAX_NUM_OF_NODES 101
int graph[MAX_NUM_OF_NODES][MAX_NUM_OF_NODES];
int dist[MAX_NUM_OF_NODES][MAX_NUM_OF_NODES];

struct arg_struct {
    int n;
    int i;
    int k;
};
pthread_rwlock_t  rwlock;
// pthread_cond_t read_sig;

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
		// pthread_mutex_lock(&write_mutex);
		// pthread_cond_wait(&read_sig,&write_mutex);
		int rc=pthread_rwlock_rdlock(&rwlock);
		if (rc) { /* an error has occurred */
			    perror("pthread_read_lock");
			    pthread_exit(NULL);
			}
		if((dist[i][k]+dist[k][j]) < dist[i][j])
		{
			//lock
			rc=pthread_rwlock_unlock(&rwlock);
			if(rc)
			{
				perror("pthread_read_unlock");
			    pthread_exit(NULL);
			}
			rc=pthread_rwlock_wrlock(&rwlock);
			if (rc) { /* an error has occurred */
			    perror("pthread_write_lock");
			    pthread_exit(NULL);
			}
			dist[i][j]=dist[i][k]+dist[k][j];
			// pthread_cond_broadcast(&read_sig);
			//unlock
			rc=pthread_rwlock_unlock(&rwlock);
			if (rc) {
			    perror("pthread_write_unlock");
			    pthread_exit(NULL);
			}
		}
		else{
			rc=pthread_rwlock_unlock(&rwlock);
			if(rc)
			{
				perror("pthread_read_unlock");
			    pthread_exit(NULL);
			}
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
	int rc=pthread_rwlock_init(&rwlock,NULL);
	if(rc==-1)
	{
		perror("Error while inititalizing rwlock");
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
	pthread_rwlock_destroy(&rwlock);
	// pthread_cond_destroy(&count_threshold_cv);
	pthread_exit(NULL);	
}