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
#include <fcntl.h>
//Global config defintions for the shell
#define SH_RL_BUFSIZE 1024
#define size_of_buff 1024
#define MAX_ARGS_SIZE 1024
#define BUFFER_SIZE 1024

#define MAX_PIPE_LENGTH 2000

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


typedef struct {
    char** words;
    int count;
}command_main;

typedef struct{
    command_main cmd;
    int is_bg;
    int red_input;
    int red_output;
}command;

char prev[100001];
char comp[100001];

void sh_clear();
void sh_env();
void sh_pwd();
void sh_ls(char *pathname,int option);
void print_file_stat(char *filename);
void convert_mode_to_letters( int mode, char str[]);
char *uid_to_name( uid_t uid );
char *gid_to_name( gid_t gid );
void print_file_info(char *filename, struct stat *info_p);
void sh_cd(char *s);
void sh_mkdir(char *s);
void sh_rmdir(char *s);
void write_history(char *s,int off);
void read_history_arg(int arg);
void execute(char *filename,char *argv[],int back);
char *sh_read_line();
void print_prompt();
command parse_command(char* line);
void simplesh_loop();
void sig_reverse_search(int sig_no);

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


command parse_command(char* line)
{
    int state = 0;
    /*
    * 0: no valid token
    * 1: scanning word
    * 2: scanning double quote
    * 3: found two double quotes
    * 4: inut indirection
    * 5: output redirection
    */

    command cmmd;
    int p = 0,i=0,k = 0;
    int flag=1;
    char buf[BUFFER_SIZE];
    //*isIndirection = 0;
    char** words = (char**)malloc(MAX_ARGS_SIZE*sizeof(char*));
    cmmd.is_bg = 0;
    cmmd.red_output = -1;
    cmmd.red_input = -1;
    while(1)
    {
        while(line[i] != '\0')
        {
            switch(state)
            {
                case 0: if(line[i] == '"')
                            state = 2;
                        else if(line[i] == '<')
                        {
                            cmmd.red_input = k;
                        }
                        else if(line[i] == '>')
                        {
                            cmmd.red_output = k;
                        }
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
                            else if(line[i] == '<')
                            {
                                state = 0;
                                buf[p++] = '\0';
                                words[k++] = (char*)malloc(p*sizeof(char));
                                strcpy(words[k-1],buf);
                                printf("Here");
                                p = 0;
                                cmmd.red_input = k;
                            }
                            else if(line[i] == '>')
                            {
                                state = 0;
                                buf[p++] = '\0';
                                words[k++] = (char*)malloc(p*sizeof(char));
                                strcpy(words[k-1],buf);
                                p = 0;
                                cmmd.red_output = k;
                            }
                            else if(line[i] == '|')
                            {

                            }
                            else
                                buf[p++] = line[i];
                        }
                        else
                        {
                            if(p > 0 && buf[p-1] == '&')
                            {
                                cmmd.is_bg = 1;
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
                            cmmd.is_bg = 1;
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
            
            break;
        }
        //write_history(line,flag);
        flag=0;
        printf(">>");
        line = sh_read_line();
        i = 0;
    }
    cmmd.cmd.count = k;
    cmmd.cmd.words = words;
    return cmmd;
    
}

void print_prompt()
{
    printf(ANSI_COLOR_GREEN"%s> "ANSI_COLOR_RESET,getenv("PWD"));
}

void run_command(char *str_command)
{
    //fprintf(stderr,"in run %s\n",str_command);
    command cmmd = parse_command(str_command);
    int is_bg,count;
    char **args;
    is_bg = cmmd.is_bg;
    count = cmmd.cmd.count;
    args = cmmd.cmd.words;
    int save_stdin,save_stdout;
    int orig_stdout = fileno(stdout),orig_stdin=fileno(stdin);
    int cur_stdin,cur_stdout;
    int read_file,write_file;
    //printf("here count = %d command=%s\n",count,args[0]);
    if(count == 0)
        return;
    int t_count = count;
    if(cmmd.red_output != -1)
    {
        int index = cmmd.red_output;
        //printf("here %s\n",args[index]);
        
        if(index >= count)
        {
            printf("Syntax Error: No file for indrection");
            return;
        }
        write_file = open(args[index],O_WRONLY|O_CREAT,0666);
        if(write_file == -1)
        {
            perror("Error creating file");
            return;
        }
        save_stdout = dup(STDOUT_FILENO);
        dup2(write_file,STDOUT_FILENO);
        close(write_file);
        t_count--;
        //save_stdout = redirect(1,write_file);
        //cur_stdout = dup(write_file);
    }
    if(cmmd.red_input != -1)
    {
        int index = cmmd.red_input;
        //printf("here %s\n",args[index]);
        
        if(index >= count)
        {
            printf("Syntax Error: No file for indrection");
            return;
        }
        read_file = open(args[index],O_RDONLY);
        if(read_file == -1)
        {
            perror("Error opening file");
            return;
        }
        save_stdin = dup(STDIN_FILENO);
        dup2(read_file,STDIN_FILENO);
        close(read_file);
        t_count--;
        //save_stdout = redirect(1,write_file);
        //cur_stdout = dup(write_file);
    }
    count = t_count;
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
    if(cmmd.red_output != -1)
    {
        //flush(fileno(stdout));
        dup2(save_stdout,STDOUT_FILENO);
        //close(save_stdout);
        close(save_stdout);

    }
    if(cmmd.red_input != -1)
    {
        //flush(fileno(stdout));
        dup2(save_stdin,STDIN_FILENO);
        //close(save_stdout);
        close(save_stdin);

    }
}


void rev_history(char*s)
{
    // FILE *fp;
    // //printf("dsd %s\n",s);
    // fp=fopen(history_name,"r");
    // if (fp == NULL)
    // {
    //     printf("error   bjhwhile reading from history\n");
    //     fp=fopen(history_name,"w");
    //     fclose(fp);
    //     fp=fopen(history_name,"r");
    //     if(fp==NULL)
    //     {
    //         printf("error while reading from history\n");
    //         return;
    //     }
    // }
    // int flag=0;
    // while(1)
    // {
    //     char buf[100];
    //     if(fgets(buf, sizeof buf, fp)==NULL)
    //         break; /* expect string like "title: TITLE WITH SPACES" */
    //     //printf("dsad   %s\n",buf);
    //     //sscanf(buf, "%*s %99[^\n]", comp);
    //     int k=0,len=strlen(buf);
    //     if(buf[len-1] == '\n')
    //         buf[len-1] = '\0';
    //     while(!(buf[k]==' '&&buf[k+1]==' '))
    //     {
    //         k++;
    //     }
    //     k=k+2;
    //     int i=0;    
    //     int temp;
    //     //printf("%s --> %s\n",comp,s );
    //     temp=substring(buf+k,s);
    //     if(temp!=0)
    //     {
    //         //printf("dsadsad    %s\n",temp);
    //         strcpy(prev,buf+k);
    //         flag=1;
    //     }
    // }
    // if(flag==1)
    // {
    //     if(prev[0]!='\n')
    //         printf("%s\n",prev);
    // }
    // else
    //     printf("No match found\n");
    // fclose(fp);
     FILE *fp;
     int flag=0;
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
           int k=0,len=read;
        if(line[len-1] == '\n')
            line[len-1] = '\0';
        while(!(line[k]==' '&&line[k+1]==' '))
        {
            k++;
        }
        k=k+2;
        int i=0;    
        int temp;
        //printf("%s --> %s\n",comp,s );
        temp=substring(line+k,s);
        if(temp!=0)
        {
            //printf("dsadsad    %s\n",temp);
            strcpy(prev,line+k);
            flag=1;
        }
       }
    if(flag==1)
    {
        if(prev[0]!='\n')
            printf("%s\n",prev);
    }
    else
        printf("No match found\n");
    fclose(fp);
    if(line)
        free(line);

}

int substring(char* str,char* search)
{
    int i,j,flag = 0;
    int count1 = strlen(str),count2 = strlen(search);
    for(i=0;i<=count1-count2;i++)
    {
        for(j=i;j<i+count2;j++)
        {
            flag=1;
            if (str[j]!=search[j-i])
            {
                flag=0;
               break;
            }
        }
        if (flag==1)
            break;
    }
    return flag;
}

/*void rev_history(char * query)
{
    char found[1024];
    found[0] = '\0';
    FILE *history = fopen(history_name,"r");
    if(history == NULL)
    {
        history = fopen(history_name,"w");
        fclose(history);
        history = fopen(history_name,"r");
    }
    char * line = NULL;
   size_t len = 0;
   ssize_t read;
    while ((read = getline(&line, &len, fp)) != -1) {
       printf("Retrieved line of length %zu :\n", read);
       char temp[1024];
       sscanf(line,"%s",temp);

       printf("%s", line);
   }
}*/

int pipe_exec(int argc, char **argv)
{
    int status;
    int i;

    int pipes[2*(argc-1)];
    for(i=0;i<2*argc-2;i+=2)
    {
        pipe(pipes+i);
    }
    int ind;
    if (fork() == 0)
    {

        dup2(pipes[1], STDOUT_FILENO);

        // close all pipes (very important!); end we're using was safely copied
        int j;
        for(j=0;j<argc;j++)
        {
            close(pipes[j]);
        }
        run_command(argv[0]);
        exit(0);
    }
    else
    {
        // fork second child (to execute grep)
        for(ind=1;ind<(argc-1);ind++)
        {

            if (fork() == 0)
            {
                dup2(pipes[2*(ind-1)], 0);
                dup2(pipes[2*ind+1], 1);
                int j;
                for(j=0;j<argc;j++)
                {
                    close(pipes[j]);
                }
                run_command(argv[ind]);
                exit(0);
                //break;
            }
            else
                continue;
        }

        if(ind==(argc-1))
        {
            if (fork() == 0)
            {
                dup2(pipes[2*(ind-1)], 0);

                int j;
                for(j=0;j<argc;j++)
                {
                    close(pipes[j]);
                }

                run_command(argv[ind]);
                exit(0);
            }
        }
    }

    int j;
    for(j=0;j<argc;j++)
    {
        close(pipes[j]);
    }
    for(i = 0; i < argc; i++)
        wait(&status);
}

void simplesh_loop()
{
    char* line;
    while(1)
    {
        print_prompt();
        line = sh_read_line();
        //printf("The line read:%s",line);
        write_history(line,1);
        char **list_cmd = (char**)malloc(MAX_PIPE_LENGTH*sizeof(char*));
        //printf("here\n");
        //run_command(line);
        int i = 0;
        char *token;
        //printf("here++\n");
        token = list_cmd[i++] = strtok(line,"|");
        while(token != NULL)
        {
            list_cmd[i++] = token = strtok(NULL,"|");
            /*if(list_cmd[i-1]!=NULL)
                printf("%s\n",list_cmd[i-1]);*/
        }
        i--;
       // printf("i = %d\n",i);
        //printf("Here\n");
        if(i < 2)
        {
            run_command(list_cmd[0]);
        }
        else
        {
            /*int n = i;
            int fd[2];
            pipe(fd);
            int save_stdout,save_stdin;
            save_stdout = dup(STDOUT_FILENO);
            save_stdin = dup(STDIN_FILENO);
            dup2(fd[1],STDOUT_FILENO);
            run_command(list_cmd[0]);
            close(fd[1]);
            int fd2[2];
            for(i=1;i<n-1;i++)
            {
                pipe(fd2);
                dup2(fd[0],STDIN_FILENO);
                dup2(fd2[1],STDOUT_FILENO);
                run_command(list_cmd[i]);
                close(fd[0]);
                close(fd2[1]);
                fd[0] = fd2[0];
                fd[1] = fd2[1];
            }
            dup2(fd2[0],STDIN_FILENO);
            run_command(list_cmd[i]);
            close(fd2[0]);
            dup2(save_stdin,STDIN_FILENO);
            //close(save_stdout);
            close(save_stdin);
            dup2(save_stdout,STDOUT_FILENO);
            //close(save_stdout);
            close(save_stdout);*/
            pipe_exec(i,list_cmd);
        }
    }
}




int main(int argc, char *argv[])
{
    //setbkcolor(BLACK);
    signal(SIGQUIT,sig_reverse_search);
    strcpy(history_name,getenv("HOME"));
    strcpy(offset_name,history_name);
    strcat(history_name,"/.myp_history");
    strcat(offset_name,"/.myp_offset");
    //printf("%s %s\n",history_name,offset_name);
    sh_clear();
    //printf("%s\n",argv[0]);
    //printf(ANSI_BACK_BLACK);
    simplesh_loop();
    return 0;
}


void sig_reverse_search(int sig_no)
{
    char buf[1024];
    printf("\n");
    //buf = sh_read_line();
    read(fileno(stdin),buf,1024);
    fflush(stdin);
    int l = strlen(buf);
    buf[l-1] = '\0';
    //printf("%s\n",buf);
    rev_history(buf);
    //printf("%s",buf);
    //print_prompt();
    //fflush(stdin);
}