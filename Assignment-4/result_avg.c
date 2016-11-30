#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/time.h>
int main()
{
	FILE* fp1=fopen("/home/ravi/Documents/os4th/Result/result_test1.txt","w");
	DIR *dir;
	struct timeval rta,wta,tta,ltime;
	rta.tv_sec=0;rta.tv_usec=0;wta.tv_sec=0;wta.tv_usec=0;tta.tv_sec=0;tta.tv_usec=0;
	struct dirent *ent;
	if ((dir = opendir ("/home/ravi/Documents/os4th/test1")) != NULL) {
	  /* print all the files and directories within directory */
	  while ((ent = readdir (dir)) != NULL) {
	    if(strcmp(ent->d_name,".")!=0&&strcmp(ent->d_name,"..")!=0)
	   		//printf("%s\n",ent->d_name);
	    {
	    	char s[1000];
	    	sprintf(s,"/home/ravi/Documents/os4th/test1/%s",ent->d_name);
	    	FILE *fp=fopen(s,"r");
		    int noi,prio,slpt,pid;
		    long int rt1,wt1,tt1,rt2,wt2,tt2;
		    double slpb;
		    //printf("Helo\n");
		    fscanf(fp,"%d %d %lf %d",&noi,&prio,&slpb,&slpt);
		    //printf("polo\n");
		    fprintf(fp1,"NOI: %d PRIORITY: %d SLEEPPROB: %lf SLEEPTIME: %d\n",noi,prio,slpb,slpt);
		    fscanf(fp,"%d %ld %ld",&pid,&rt1,&rt2);
		    fscanf(fp,"%d %ld %ld",&pid,&wt1,&wt2);
		    fscanf(fp,"%d %ld %ld",&pid,&tt1,&tt2);
		    ltime.tv_sec=rt1;
		    ltime.tv_usec=rt2;
		    timeradd(&rta,&ltime,&rta);
		    ltime.tv_sec=wt1;
		    ltime.tv_usec=wt2;
		    timeradd(&wta,&ltime,&wta);
		    ltime.tv_sec=tt1;
		    ltime.tv_usec=tt2;
		    timeradd(&tta,&ltime,&tta);
		    fprintf(fp1,"PID: %d\n",pid);
		    fprintf(fp1,"Response Time: %ld sec %ld usec\n",rt1,rt2);
		    fprintf(fp1,"Total Waiting Time: %ld sec %ld usec\n",wt1,wt2);
		    fprintf(fp1,"Turn-aroud Time: %ld sec %ld usec\n\n",tt1,tt2);
		    fclose(fp);
		}
	  }
	  closedir (dir);
	} else {
	  /* could not open directory */
	  perror ("");
	  return EXIT_FAILURE;
	}
	fprintf(fp1,"\nAverage Response Time: %lf sec  %lf usec\n",((rta.tv_sec)*1.0)/4.0,((rta.tv_usec)*1.0)/4.0);
	fprintf(fp1,"\nAverage Waiting Time: %lf sec  %lf usec\n",((wta.tv_sec)*1.0)/4.0,((wta.tv_usec)*1.0)/4.0);
	fprintf(fp1,"\nAverage Turn-aroud Time: %lf sec  %lf usec\n",((tta.tv_sec)*1.0)/4.0,((tta.tv_usec)*1.0)/4.0);
	fclose(fp1);
}
