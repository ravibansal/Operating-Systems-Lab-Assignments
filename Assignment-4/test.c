#include <stdio.h>
#include <stdlib.h>
int main()
{
	FILE *fp;
	char s[100];
	int pid=109;
	double response_time=10.09,total_waiting_time=10.99,total_turnaround_time=10.90;
	sprintf(s,"resultrt_%d.txt",pid);
	fp=fopen(s,"w");
	fprintf(fp,"%d %lf\n",pid,response_time);
	fprintf(fp,"%d %lf\n",pid,total_waiting_time );
	fprintf(fp,"%d %lf\n",pid,total_turnaround_time );
	fclose(fp);
}