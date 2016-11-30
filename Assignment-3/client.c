#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>
#include <grp.h>
#include <stdlib.h>
#define ANSI_COLOR_RED     "\x1b[31;1m"
#define ANSI_COLOR_GREEN   "\x1b[32;1m"
#define ANSI_COLOR_YELLOW  "\x1b[33;1m"
#define ANSI_COLOR_BLUE    "\x1b[34;1m"
#define ANSI_COLOR_MAGENTA "\x1b[35;1m"
#define ANSI_COLOR_CYAN    "\x1b[36;1m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_BACK_BLACK    "\x1b[40;1m"
#define size_of_buff 1024
#define BUFFER_SIZE 1024
#define MAX_ARGS_SIZE 1024
#define SH_RL_BUFSIZE 1024
char history_name[1024], offset_name[1024];
int cpl;
int tml_id, self_pid;
#define BUF_SIZE 6000
typedef struct mymsgbuf {
    long    mtype;          /* Message type */
    char command[1000];
    char msg[6000];
    int pid;
    int terminal;
} mymsgbuf;

void to_exit(int qid, int cpl, char *command)
{
    mymsgbuf msg;
    if (cpl == 1)
    {
        msg.msg[0] = '\0';
        strcpy(msg.command, command);
        printf("%s\n",command);
        msg.pid = self_pid;
        msg.mtype = 2;
        msg.terminal = tml_id;
        if (send_message(qid, &msg) == -1)
        {
            perror("Error while sending message (not uncoupled yet)");
            exit(1);
        }
    }
    exit(1);
}
int send_message( int qid, struct mymsgbuf *qbuf )
{
    int     result, length;

    /* The length is essentially the size of the structure minus sizeof(mtype) */
    length = sizeof(struct mymsgbuf) - sizeof(long);

    if ((result = msgsnd( qid, qbuf, length, 0)) == -1)
    {
        return (-1);
    }
    //printf("%d\n",result);
    return (result);
}
int read_message( int qid, long type, struct mymsgbuf *qbuf )
{
    int     result, length;

    /* The length is essentially the size of the structure minus sizeof(mtype) */
    length = sizeof(struct mymsgbuf) - sizeof(long);

    if ((result = msgrcv( qid, qbuf, length, type,  0)) == -1)
    {
        return (-1);
    }

    return (result);
}
void print_prompt()
{
    printf(ANSI_COLOR_GREEN"%s> "ANSI_COLOR_RESET, getenv("PWD"));
}
void read_history(char *msg)
{
    FILE *fp;
    msg[0] = '\0';
    fp = fopen(history_name, "r");
    if (fp == NULL)
    {
        fp = fopen(history_name, "w");
        //fprintf(fp2,"0");
        fclose(fp);
        fp = fopen(history_name, "r");
        if (fp == NULL)
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
        //sprintf(msg,"%s",line);
        strcat(msg, line);
    }
    fclose(fp);
    if (line)
        free(line);
}

void sh_clear()
{
    if (!printf("\033[2J\033[1;1H"))
    {
        perror("");
    }
}

void read_history_arg(int arg, char *msg)
{
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    msg[0] = '\0';
    if (arg < 0)
    {
        printf("history: %d: invalid option\n", arg);
        printf("history: usage: history or history <argument>\n");
        return;
    }
    FILE *fp;
    int count = 0, i = 0;
    fp = fopen(history_name, "r");
    if (fp == NULL)
    {
        fp = fopen(history_name, "w");
        fclose(fp);
        fp = fopen(history_name, "r");
        if (fp == NULL)
        {
            printf("error while reading from history\n");
            return;
        }
    }
    while ((read = getline(&line, &len, fp)) != -1) {
        count++;
    }
    fclose(fp);
    fp = fopen(history_name, "r");
    while ((read = getline(&line, &len, fp)) != -1) {
        //printf("Retrieved line of length %zu :\n", read);
        if (i >= (count - arg))
        {
            printf("%s", line);
            //sprintf(msg,"%s",line);
            strcat(msg, line);
        }
        i++;
    }
    fclose(fp);
    if (line)
        free(line);
}

void write_history(char *s, int off)
{
    if (s[0] == '\n' && off == 1)
        return;
    FILE *fp, *fp2;
    fp = fopen(history_name, "a");
    fp2 = fopen(offset_name, "r");
    if (fp == NULL)
    {
        printf("error while writing to history file\n");
        return;
    }
    if (fp2 == NULL)
    {
        fp2 = fopen(offset_name, "w");
        fprintf(fp2, "0");
        fclose(fp2);
        fp2 = fopen(offset_name, "r");
        if (fp2 == NULL)
        {
            printf("error while retreiving offset\n");
            return;
        }
    }
    int offset;
    fscanf(fp2, "%d", &offset);
    char buf[size_of_buff];
    buf[0] = '\0';
    char str[15];
    if (off == 1)
    {
        offset += 1;
        sprintf(str, " %d", offset);
        strcat(buf, str);
        strcat(buf, "  ");
    }
    strcat(buf, s);
    fprintf(fp, "%s", buf);
    fclose(fp2);
    fp2 = fopen(offset_name, "w");
    fprintf(fp2, "%d", offset);
    fclose(fp);
    fclose(fp2);
}

void sh_cd(char *s)
{
    int flag;
    if (s == NULL)
    {
        s = getenv("HOME");
    }
    flag = chdir(s);
    if (flag == -1)
    {
        char buf[size_of_buff];
        buf[0] = '\0';
        strcat(buf, "cd: ");
        strcat(buf, s);
        perror(buf);
    }
    else
    {
        char *oldpwd = getenv("PWD");
        char buff[SH_RL_BUFSIZE];
        char *pwd = getcwd(buff, SH_RL_BUFSIZE);
        if (pwd == NULL)
        {
            perror("cd");
        }
        else
        {
            setenv("PWD", pwd, 1);
            setenv("OLDPWD", oldpwd, 1);
        }
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
char** parse_command(char* line, int *count, int *bg)
{
    int state = 0;
    int p = 0, i = 0, k = 0;
    int flag = 1;
    char buf[BUFFER_SIZE];
    char** words = (char**)malloc(MAX_ARGS_SIZE * sizeof(char*));
    while (1)
    {
        while (line[i] != '\0')
        {
            switch (state)
            {
            case 0: if (line[i] == '"')
                    state = 2;
                else if (!isspace(line[i]))
                {
                    state = 1;
                    buf[p++] = line[i];
                }
                break;
            case 1: if (!isspace(line[i]))
                {
                    if (line[i] == '"')
                        state = 2;
                    else
                        buf[p++] = line[i];
                }
                else
                {
                    if (p > 0 && buf[p - 1] == '&')
                    {
                        *bg = 1;
                        buf[p - 1] = '\0';
                    }
                    state = 0;
                    buf[p++] = '\0';
                    words[k++] = (char*)malloc(p * sizeof(char));
                    strcpy(words[k - 1], buf);
                    p = 0;
                }
                break;
            case 2: if (line[i] == '"')
                {
                    state = 3;
                }
                else
                {
                    buf[p++] = line[i];
                }
                break;
            case 3: if (line[i] == '"')
                    state = 2;
                else if (!isspace(line[i]))
                {
                    state = 1;
                    buf[p++] = line[i];
                }
                else
                {
                    *bg = 0;
                    state = 0;
                    buf[p++] = '\0';
                    words[k++] = (char*)malloc(p * sizeof(char));
                    strcpy(words[k - 1], buf);
                    p = 0;
                }
                break;
            }
            i++;
        }
        if (p == 0)
        {
            write_history(line, flag);
            break;
        }
        write_history(line, flag);
        flag = 0;
        printf(">>");
        line = sh_read_line();
        i = 0;
    }
    *count = k;
    return words;

}
void *oup(int qid)
{
    mymsgbuf msg;
    while (1)
    {
        while (read_message(qid, self_pid, &msg) == -1)
        {
            perror("Error while reading message");
            char str[10] = "";
            to_exit(qid, cpl, str);
        }
        if (msg.pid == -1)
        {
            tml_id = msg.terminal;
            printf(ANSI_COLOR_BLUE"ID :%d \n"ANSI_COLOR_RESET, msg.terminal);
        }
        else {
            printf(ANSI_COLOR_MAGENTA"\nTerminal %d: "ANSI_COLOR_RESET, msg.terminal);
            printf("%s", msg.command);
            printf("%s", msg.msg);
        }
        print_prompt();
        fflush(stdout);
    }
    pthread_exit(NULL);
}
void *inp(int qid)
{
    cpl = 0;
    mymsgbuf msg;
    char command[100];
    strcpy(history_name, getenv("HOME"));
    strcpy(offset_name, history_name);
    char str[100];
    sprintf(str, "%d", self_pid);
    strcat(history_name, "/.myrp_history_");
    strcat(offset_name, "/.myrp_offset_");
    strcat(history_name, str);
    strcat(offset_name, str);
    int ext = 0;
    int flag = 0;
    while (1)
    {
        if (flag == 1)
        {
            flag = 0;
        }
        else
        {
            print_prompt();
        }

        fgets(command, 100, stdin);
        //printf("Yes1\n");
        int is_bg = 0;
        int count = 0;
        char **args = parse_command(command, &count, &is_bg);
        if (count == 0)
            continue;
        char *command1 = args[0];
        //printf("Yes\n");
        if (strcmp(command1, "couple") == 0)
        {
            if (cpl != 0)
            {
                printf("Already Coupled\n");
                continue;
            }
            msg.pid = self_pid;
            msg.mtype = 1;
            //printf("Yes\n");
            if (send_message(qid, &msg) == -1)
            {
                perror("Error while sending message");
                to_exit(qid, cpl, command);
            }
            cpl = 1;
            flag = 1;
            continue;
        }
        //printf("%s\n",command1);
        if (strcmp(command1, "cd") == 0)
        {
            if (count < 2)
                sh_cd(NULL);
            else
                sh_cd(args[1]);
            if (cpl == 1)
            {
                strcpy(msg.command, command);
                msg.msg[0] = '\0';
                msg.mtype = 3;
                msg.terminal = tml_id;
                if (send_message(qid, &msg) == -1)
                {
                    perror("Error while sending message");
                    to_exit(qid, cpl, command);
                }
            }
            continue;
        }
        if (strcmp(command1, "clear") == 0)
        {
            sh_clear();
            if (cpl == 1)
            {
                strcpy(msg.command, command);
                msg.msg[0] = '\0';
                msg.mtype = 3;
                msg.terminal = tml_id;
                if (send_message(qid, &msg) == -1)
                {
                    perror("Error while sending message");
                    to_exit(qid, cpl, command);
                }
            }
            continue;
        }
        //printf("Yes\n");
        if (strcmp(command1, "exit") == 0)
        {
            int self_pid;
            msg.pid = self_pid;
            msg.mtype = 3;
            msg.terminal = tml_id;
            if (cpl == 1)
            {
                strcpy(msg.command, command);
                msg.msg[0] = '\0';
                msg.mtype = 2;
                msg.terminal = tml_id;
                if (send_message(qid, &msg) == -1)
                {
                    perror("Error while sending message");
                    to_exit(qid, cpl, command);
                }
            }
            cpl = 0;
            exit(0);
        }
        if (strcmp(command1, "history") == 0)
        {
            if (count < 2)
            {
                read_history(msg.msg);
            }
            else
            {
                int num = atoi(args[1]);
                read_history_arg(num, msg.msg);
            }
            if (cpl == 1)
            {
                strcpy(msg.command, command);
                msg.mtype = 3;
                msg.terminal = tml_id;
                if (send_message(qid, &msg) == -1)
                {
                    perror("Error while sending message");
                    to_exit(qid, cpl, command);
                }
            }
            continue;
        }
        if (strcmp(command1, "uncouple") == 0)
        {
            if(cpl==0)
            {
                printf("Already Uncoupled\n");
            }
            else
            {
                int self_pid;
                msg.pid = self_pid;
                msg.mtype = 2;
                msg.terminal = tml_id;
                msg.msg[0]='\0';
                strcpy(msg.command, command);
                if (send_message(qid, &msg) == -1)
                {
                    perror("Error while sending message");
                    to_exit(qid, cpl, command);
                }
                cpl = 0;
            }   
            continue;
        }
        //printf("Yes\n");
        int pipes[2];
        pipe(pipes);
        if (fork() == 0)
        {
            dup2(pipes[1], STDOUT_FILENO);
            dup2(pipes[1], STDERR_FILENO);
            close(pipes[1]);
            close(pipes[0]);
            system(command);
            //printf("Hello\n");
            exit(0);
        }
        //printf("%d\n",is_bg);
        int status;
        wait(&status);
        close(pipes[1]);
        //printf("No\n");

        //printf("Yes\n");
        if (is_bg == 0)
        {
            char buf[BUF_SIZE] = {'\0'};
            int numRead = read(pipes[0], buf, BUF_SIZE);
            close(pipes[0]);
            //printf("%d\n",numRead);
            //printf("No\n");
            if (numRead == -1)
                perror("read");
            if (numRead == 0)
            {
                msg.msg[0] = '\0';
            }
            else
            {
                strcpy(msg.msg, buf);
            }
            //if(numRead)
            printf("%s", buf);
            strcpy(msg.command, command);
            //printf("Yesb\n");
            msg.terminal = tml_id;
            msg.pid = self_pid;
            msg.mtype = 3;
            if (cpl == 1)
            {
                if (send_message(qid, &msg) == -1)
                {
                    perror("Error while sending message");
                    to_exit(qid, cpl, command);
                }
            }
        }
        else
        {
            if (fork() == 0)
            {
                strcpy(msg.command, command);
                msg.msg[0] = '\0';
                //printf("Yesb\n");
                msg.terminal = tml_id;
                msg.pid = getppid();
                msg.mtype = 3;
                if (cpl == 1)
                {
                    if (send_message(qid, &msg) == -1)
                    {
                        perror("Error while sending message");
                        to_exit(qid, cpl, command);
                    }
                }
                char buf[BUF_SIZE] = {'\0'};
                int numRead = read(pipes[0], buf, BUF_SIZE);
                close(pipes[0]);
                //printf("%d\n",numRead);
                //printf("No\n");
                if (numRead == -1)
                    perror("read");
                if (numRead == 0)
                {
                    msg.msg[0] = '\0';
                }
                else
                {
                    strcpy(msg.msg, buf);
                }
                //if(numRead)
                printf("%s", buf);
                if (numRead > 0)
                {   msg.command[0] = '\0';
                    //printf("Yesb\n");
                    msg.terminal = tml_id;
                    msg.pid = getppid();
                    msg.mtype = 3;
                    if (cpl == 1)
                    {
                        if (send_message(qid, &msg) == -1)
                        {
                            perror("Error while sending message");
                            to_exit(qid, cpl, command);
                        }
                    }
                }
                exit(0);
            }
        }
        //printf("Yes\n");
    }
    pthread_exit(NULL);
}

int substring(char* str, char* search)
{
    int i, j, flag = 0;
    int count1 = strlen(str), count2 = strlen(search);
    for (i = 0; i <= count1 - count2; i++)
    {
        for (j = i; j < i + count2; j++)
        {
            flag = 1;
            if (str[j] != search[j - i])
            {
                flag = 0;
                break;
            }
        }
        if (flag == 1)
            break;
    }
    return flag;
}

void rev_history(char*s)
{
    FILE *fp;
    int flag = 0;
    fp = fopen(history_name, "r");
    if (fp == NULL)
    {
        fp = fopen(history_name, "w");
        //fprintf(fp2,"0");
        fclose(fp);
        fp = fopen(history_name, "r");
        if (fp == NULL)
        {
            printf("error while reading from history\n");
            return;
        }
    }
    char * line = NULL;
    char prev[1024];
    size_t len = 0;
    ssize_t read;
    while ((read = getline(&line, &len, fp)) != -1) {
        //printf("Retrieved line of length %zu :\n", read);
        int k = 0, len = read;
        if (line[len - 1] == '\n')
            line[len - 1] = '\0';
        while (!(line[k] == ' ' && line[k + 1] == ' '))
        {
            k++;
        }
        k = k + 2;
        int i = 0;
        int temp;
        //printf("%s --> %s\n",comp,s );
        temp = substring(line + k, s);
        if (temp != 0)
        {
            //printf("dsadsad    %s\n",temp);
            strcpy(prev, line + k);
            flag = 1;
        }
    }
    if (flag == 1)
    {
        if (prev[0] != '\n')
        {
            printf("%s\n", prev);

        }
        //fprintf(stdin, "%s\n",prev);
    }
    else
        printf("No match found\n");
    fclose(fp);
    if (line)
        free(line);
}


void sig_reverse_search(int sig_no);

int main()
{
    signal(SIGQUIT, sig_reverse_search);
    pthread_t threads[2];
    self_pid = getpid();
    int rc;
    key_t key = 1234;
    int qid = msgget(key, IPC_CREAT | 0666);
    printf("qid: %d\n", qid);
    long i;
    i = 0;
    rc = pthread_create(&threads[i], NULL,
                        inp, qid);
    i = 1;
    if (rc) {
        printf("Error:unable to create thread, %d\n", rc);
        exit(-1);
    }
    rc = pthread_create(&threads[i], NULL,
                        oup, qid);
    if (rc) {
        printf("Error:unable to create thread, %d\n", rc);
        exit(-1);
    }
    pthread_exit(NULL);
    return 0;
}


void sig_reverse_search(int sig_no)
{
    char buf[1024];
    printf("\n");
    signal(SIGQUIT, sig_reverse_search);
    //buf = sh_read_line();
    read(fileno(stdin), buf, 1024);
    fflush(stdin);
    int l = strlen(buf);
    buf[l - 1] = '\0';
    //printf("%s\n",buf);
    rev_history(buf);
    //printf("%s",buf);
    //print_prompt();
    //fflush(stdin);
}