/*
* Assignment 1: Question 2: Basic Shell
* Prabhat Agarwal, 13CS10060
* Ravi Bansal, 13CS30026
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <pwd.h>
#include <sys/stat.h>
#include <grp.h>
#include <string.h>
#include <time.h>
//Global config defintions for the shell
#define SH_RL_BUFSIZE 1024
#define size_of_buff 1024
#define MAX_ARGS_SIZE 1024
#define BUFFER_SIZE 1024

//Define ANSI colors
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32;1m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_BACK_BLACK    "\x1b[40m"

extern char** environ;

void sh_clear();
void sh_env();
void clear();
void print_file_stat(char *filename);
void print_file_info(char *filename, struct stat *info_p);
char history_name[1024],offset_name[1024];

void sh_clear()
{
    if(!printf("\033[2J\033[1;1H"))
    {
        perror("");
    }           
}

void sh_env()
{
    char *envvar = *environ;
    int i = 0;
    while(envvar)
    {
        printf("%s\n",envvar);
        envvar = *(environ + i);
        i++;
    }
}

void sh_pwd()
{
    char buff[SH_RL_BUFSIZE];
    char *pwd = getcwd(buff,SH_RL_BUFSIZE);
    if(pwd)
    {
        printf("%s\n",pwd);
    }
    else
    {
        perror("The following error occurred");
    }
}

void sh_ls(char *pathname,int option)
{
    if(pathname == NULL)
        pathname = getenv("PWD");
    char buff[SH_RL_BUFSIZE];
    struct dirent **files;
    int i,count;
    count = scandir(pathname, &files, NULL, alphasort);

    /* If no files found, make a non-selectable menu item */
    if(count <= 0)
        perror("ls");
    else
    {
        for(i=0;i<count;i++)
        {
            if(strcmp(files[i]->d_name,".") == 0 || strcmp(files[i]->d_name,"..") == 0)
                continue;
            if(files[i]->d_name[0] == '.')
                continue;
            if(option == 0)
                printf("%s\n",files[i]->d_name);
            else
                print_file_stat(files[i]->d_name);
            free(files[i]);
        }
        free(files);
    }
}

void print_file_stat(char *filename)
{
    struct stat info;
    if (stat(filename, &info) == -1)        /* cannot stat   */
        perror( filename );         /* say why   */
    else                    /* else show info    */
        print_file_info(filename, &info);
}


void convert_mode_to_letters( int mode, char str[] )
{
    strcpy( str, "?---------" );           /* default=no perms */

    if ( S_ISREG(mode) )  str[0] = '-';
    if ( S_ISDIR(mode) )  str[0] = 'd';    /* directory       */
    if ( S_ISLNK(mode) )  str[0] = 'l';    /* link           */
    if ( S_ISBLK(mode) )  str[0] = 'b';
    if ( S_ISCHR(mode) )  str[0] = 'c';
    if ( S_ISFIFO(mode) )  str[0] = 'p';


    if ( mode & S_IRUSR ) str[1] = 'r';    /* 3 bits for user  */
    if ( mode & S_IWUSR ) str[2] = 'w';
    if ( mode & S_IXUSR ) str[3] = 'x';

    if ( mode & S_IRGRP ) str[4] = 'r';    /* 3 bits for group */
    if ( mode & S_IWGRP ) str[5] = 'w';
    if ( mode & S_IXGRP ) str[6] = 'x';

    if ( mode & S_IROTH ) str[7] = 'r';    /* 3 bits for other */
    if ( mode & S_IWOTH ) str[8] = 'w';
    if ( mode & S_IXOTH ) str[9] = 'x';
}

char *uid_to_name( uid_t uid )
{
    struct  passwd *getpwuid(), *pw_ptr;
    static  char numstr[10];

    if ( ( pw_ptr = getpwuid( uid ) ) == NULL ){
        sprintf(numstr,"%d", uid);
        return numstr;
    }
    else
        return pw_ptr->pw_name ;
}


char *gid_to_name( gid_t gid )
{
    struct group  *grp_ptr;
    static  char numstr[10];

    if ( ( grp_ptr = getgrgid(gid) ) == NULL ){
        sprintf(numstr,"%d", gid);
        return numstr;
    }
    else
        return grp_ptr->gr_name;
}
void print_file_info(char *filename, struct stat *info_p)
{
    //char  *uid_to_name(), *ctime(), *gid_to_name(), *filemode();
    void    mode_to_letters();
    char    modestr[11];

    convert_mode_to_letters(info_p->st_mode, modestr);

    printf( "%s"    , modestr );
    printf( "%4d "  , (int) info_p->st_nlink);  
    printf( "%-8s " , uid_to_name(info_p->st_uid) );
    printf( "%-8s " , gid_to_name(info_p->st_gid) );
    printf( "%8ld " , (long)info_p->st_size);
    printf( "%.16s ", ctime(&info_p->st_mtime));
    printf( "%s\n"  , filename );

}

blkcnt_t get_block_for_file(char *filename, struct stat *info_p)
{
    return info_p->st_blocks;
}

void sh_cd(char *s)
{
    int flag;
    if(s==NULL)
    {
        s = getenv("HOME");
    }
    flag=chdir(s);
    if(flag==-1)
    {
        char buf[size_of_buff];
        buf[0]='\0';
        strcat(buf,"cd: ");
        strcat(buf,s);
        perror(buf);
    }
    else
    {
        char *oldpwd = getenv("PWD");
        char buff[SH_RL_BUFSIZE];
        char *pwd = getcwd(buff,SH_RL_BUFSIZE);
        if(pwd == NULL)
        {
            perror("cd");
        }
        else
        {
            setenv("PWD",pwd,1);
            setenv("OLDPWD",oldpwd,1);
        }
    }
}

void sh_mkdir(char *s)
{
    int flag,mode;
    mode=0777;
    flag=mkdir(s,mode);
    if(flag==-1)
    {
        char buf[size_of_buff];
        buf[0]='\0';
        strcat(buf,"mkdir: cannot create directory '");
        strcat(buf,s);
        strcat(buf,"'");
        perror(buf);
    }
    else
        printf("%s directory is successfully created\n",s);
}

void sh_rmdir(char *s)
{
    int flag;
    flag=rmdir(s);
    if(flag==-1)
    {
        char buf[size_of_buff];
        buf[0]='\0';
        strcat(buf,"rmdir: failed to remove '");
        strcat(buf,s);
        strcat(buf,"'");
        perror(buf);
    }
    else
        printf("%s directory is successfully removed\n",s);
}

void write_history(char *s,int off)
{
    if(s[0]=='\n'&&off==1)
        return;
    FILE *fp,*fp2;
    fp=fopen(history_name,"a");
    fp2=fopen(offset_name,"r");
    if (fp == NULL)
    {
        printf("error while writing to history file\n");
        return;
    }
    if(fp2== NULL)
    {
        fp2=fopen(offset_name,"w");
        fprintf(fp2,"0");
        fclose(fp2);
        fp2=fopen(offset_name,"r");
        if(fp2==NULL)
        {
            printf("error while retreiving offset\n");
            return;
        }
    }
    int offset;
    fscanf(fp2,"%d",&offset);
    char buf[size_of_buff];
    buf[0]='\0';
    char str[15];
    if(off==1)
    {
        offset+=1;
        sprintf(str," %d",offset);
        strcat(buf,str);
        strcat(buf,"  ");
    }
    strcat(buf,s);
    fprintf(fp,"%s",buf);
    fclose(fp2);
    fp2=fopen(offset_name,"w");
    fprintf(fp2,"%d",offset);
    fclose(fp);
    fclose(fp2);
}

void read_history()
{
    FILE *fp;
    fp=fopen(history_name,"r");
    if (fp == NULL)
    {
        fp=fopen(history_name,"w");
        //fprintf(fp2,"0");
        fclose(fp);
        fp=fopen(history_name,"r");
        if(fp==NULL)
        {
            printf("error while reading from history\n");
            return;
        }
    }
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    while ((read = getline(&line, &len, fp)) != -1) {
           //printf("Retrieved line of length %zu :\n", read);
           printf("%s", line);
       }
    fclose(fp);
    if(line)
        free(line);
}

void read_history_arg(int arg)
{
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    if(arg<0)
    {
        printf("history: %d: invalid option\n",arg);
        printf("history: usage: history or history <argument>\n");
        return;
    }
    FILE *fp;
    int count=0,i=0;
    fp=fopen(history_name,"r");
    if (fp == NULL)
    {
        fp=fopen(history_name,"w");
        fclose(fp);
        fp=fopen(history_name,"r");
        if(fp==NULL)
        {
            printf("error while reading from history\n");
            return;
        }
    }
    while ((read = getline(&line, &len, fp)) != -1) {
           count++;
       }
    fclose(fp);
    fp=fopen(history_name,"r");
    while ((read = getline(&line, &len, fp)) != -1) {
           //printf("Retrieved line of length %zu :\n", read);
           if(i>=(count-arg))
                printf("%s", line);
           i++;
       }
    fclose(fp);
    if(line)
        free(line);
}

void execute(char *filename,char *argv[],int back)
{
    //printf("%s %s\n",filename,argv[0]);
    pid_t pid=fork();
    int status;
    if(pid==0)
    {
        int flag=execvp(filename,argv);
        if(flag==-1)
        {
            perror("");
            exit(0);
        }
    }
    else
    {
        if(back==0)
            waitpid(pid,&status,0);
    }
}

char *sh_read_line()
{
    char *line = NULL;
    ssize_t bufsize = 0; 
    int no_chars = getline(&line, &bufsize, stdin);
    /*if(line[no_chars-1] == '\n')
        line[no_chars-1] = '\0';*/
    return line;
}

char** parse_command(char* line,int *count,int *bg)
{
    int state = 0;
    /*
    * 0: no valid token
    * 1: scanning word
    * 2: scanning double quote
    * 3: found two double quotes
    */
    int p = 0,i=0,k = 0;
    int flag=1;
    char buf[BUFFER_SIZE];
    char** words = (char**)malloc(MAX_ARGS_SIZE*sizeof(char*));
    while(1)
    {
        while(line[i] != '\0')
        {
            switch(state)
            {
                case 0: if(line[i] == '"')
                            state = 2;
                        else if(!isspace(line[i]))
                        {
                            state = 1;
                            buf[p++] = line[i];
                        }
                        break;
                case 1: if(!isspace(line[i]))
                        {
                            if(line[i]== '"')
                                state = 2;
                            else
                                buf[p++] = line[i];
                        }
                        else
                        {
                            if(p > 0 && buf[p-1] == '&')
                            {
                                *bg = 1;
                                buf[p-1] = '\0';
                            }
                            state = 0;
                            buf[p++] = '\0';
                            words[k++] = (char*)malloc(p*sizeof(char));
                            strcpy(words[k-1],buf);
                            p = 0;
                        }
                        break;
                case 2: if(line[i] == '"')
                        {
                            state = 3;
                        }
                        else
                        {
                            buf[p++] = line[i];
                        }
                        break;
                case 3:if(line[i] == '"')
                            state = 2;
                        else if(!isspace(line[i]))
                        {
                            state = 1;
                            buf[p++] = line[i];
                        }
                        else
                        {
                            *bg = 0;
                            state = 0;
                            buf[p++] = '\0';
                            words[k++] = (char*)malloc(p*sizeof(char));
                            strcpy(words[k-1],buf);
                            p = 0;
                        }
                        break;
            }
            i++;
        }
        if(p == 0)
        {
            write_history(line,flag);
            break;
        }
        write_history(line,flag);
        flag=0;
        printf(">>");
        line = sh_read_line();
        i = 0;
    }
    *count = k;
    return words;
    
}

void print_prompt()
{
    printf(ANSI_COLOR_GREEN"%s> "ANSI_COLOR_RESET,getenv("PWD"));
}
void simplesh_loop()
{
    char* line,**args;
    int count,is_bg;
    while(1)
    {
        print_prompt();
        line = sh_read_line();
        is_bg = 0;
        count = 0;
        args = parse_command(line,&count,&is_bg);
        if(count == 0)
            continue;
        char *command = args[0];
        //printf("%s\n",command);
        if(strcmp(command,"clear") == 0)
        {
            sh_clear();
        }
        else if(strcmp(command,"env")==0)
        {
            sh_env();
        }
        else if(strcmp(command,"pwd")==0)
        {
            sh_pwd();
        }
        else if(strcmp(command,"cd")==0)
        {
            if(count < 2)
                sh_cd(NULL);
            else
                sh_cd(args[1]);
        }
        else if(strcmp(command,"mkdir")==0)
        {
            if(count < 2)
            {
                printf("mkdir: missing operand\n");
            }
            else
            {
                int r = 1,i;
                for(i=1;i<count;i++)
                {
                    sh_mkdir(args[i]);
                }
            }
        }
        else if(strcmp(command,"rmdir")==0)
        {
            if(count < 2)
            {
                printf("rmdir: missing operand\n");
            }
            else
            {
                int r = 1,i;
                for(i=1;i<count;i++)
                {
                    sh_rmdir(args[i]);
                }
            }
        }
        else if(strcmp(command,"ls")==0)
        {
            if(count < 2)
            {
                sh_ls(NULL,0);
            }
            else if(strcmp(args[1],"-l")==0)
            {
                sh_ls(NULL,1);
            }
        }
        else if(strcmp(command,"history")==0)
        {
            if(count < 2)
            {
                read_history();
            }
            else
            {
                int num = atoi(args[1]);
                read_history_arg(num);
            }
        }
        else if(strcmp(command,"exit")==0)
        {
            exit(0);
        }
        else
        {
            args[count] = NULL;
            execute(command,args,is_bg);
        }
        
    }
}


int main(int argc, char *argv[])
{
    //setbkcolor(BLACK);
    strcpy(history_name,getenv("HOME"));
    strcpy(offset_name,history_name);
    strcat(history_name,"/.myrp_history");
    strcat(offset_name,"/.myrp_offset");
    //printf("%s %s\n",history_name,offset_name);
    sh_clear();
    //printf("%s\n",argv[0]);
    //printf(ANSI_BACK_BLACK);
    simplesh_loop();
    return 0;
}
