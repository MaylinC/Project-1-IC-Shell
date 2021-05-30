#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#define JOBSLIMIT 20

int flag_his = 0;
char **record;
int status_loop = 1;
int statusExit = 0;
int exit_return = 0;
int count;
int indx;
int job_id = 1; 
int argNumber;
int value; 
char inBG = 'B'; 
char inFG = 'F'; 

struct jobs
{
    int pid;
    int jid;
    int fg;
    int bg;
    int stp; 
    char **command;
};

struct jobs arr_jobs[JOBSLIMIT];

struct jobs *pidFindJob(pid_t pid) { 

    for (int i = 0; i < JOBSLIMIT; i++) {

        if (arr_jobs[i].pid == pid) { // find the job that corresponding with the pid 

            return &arr_jobs[i]; //return that job
        }
    }
    return NULL;
}

struct jobs *jobFindpid(int job_id) { 

    for (int i = 0; i < JOBSLIMIT; i++) {

        if (arr_jobs[i].jid == job_id) { // find the job that corresponding with the pid 

            return &arr_jobs[i]; //return that job
        }
    }
    return NULL;
}

char *line_reader() {
    int flag = 0;
    int c;
    int buffsize = 1024;
    int charac = 0;
    char *buffer = (char *)malloc(buffsize * sizeof(char));

    if (buffer == NULL)
    {
        perror("unsucessfully allocated");
    }

    //read each character from stdin and append it in buffer according to its position

    while ((c = getchar()) != EOF || (c = getchar()) != '\n')
    {

        buffer[charac] = c;
        charac++;

        if (c == '\0' || c == '\n')
        { // if it is the end if stdin then return buffer
            break;
        }

        if (strlen(buffer) >= buffsize)
        { //
            buffsize += 1024;
            buffer = realloc(buffer, buffsize);

            if (buffer == NULL)
            {
                perror("unsucessfully realloced");
            }
        }
    }
    return buffer;
}

char **string_token(char *line)
{
    int splitsize = 1024;
    int charac = 0;
    char **splits = (char **)malloc(splitsize * sizeof(char *));
    char *pre_line;

    if (splits == NULL)
    {
        perror("unsucessfully allocated");
    }

    pre_line = strtok(line, " \t\n");

    while (!(pre_line == NULL))
    {

        splits[charac] = pre_line;
        charac++;

        if ((sizeof(splits) / sizeof(splits[0])) >= splitsize)
        {
            splitsize *= 2;
            splits = realloc(splits, splitsize * sizeof(char *));

            if (splits == NULL)
            {
                perror("unsucessfully realloced");
            }
        }
        pre_line = strtok(NULL, " \t\n");
    }

    splits[charac] = NULL; // let the last charecter equal null to indicate the end of the line

    return splits;
}

void handler_sig()
{
    printf("\n");
    status_loop = 1;
    printf("\n");
}

void echo_command(char **cmd, int idx)
{ 

    int size = (int)sizeof(cmd[0]) / sizeof(char);
    for (int i = idx; i < size; i++)
    { // word
        if (cmd[i] != NULL)
        {
            printf("%s "
                   "",
                   cmd[i]);
        }
    }
    printf("\n");
}

int add_job (int proc_id, char** args, char bgorfg) { 

    int cmd_size = 1024;

    for (int i = job_id-1; i < job_id; i++) {  

        arr_jobs[i].command = (char **)malloc(cmd_size * sizeof(char *)); // malloc each job command

        for (int index = 0; args[index] != NULL; index++) { //0: [sleep] 1: [100] 2:[&]

            // copy each cmd index in command[i]
            arr_jobs[i].command[index] = (char *)malloc(sizeof(args[index]) * sizeof(char)); 
            memcpy(arr_jobs[i].command[index], args[index], strlen(args[index]) + 1); // 1D array
        }

        // printf("%s ", arr_jobs[i].command[1]);  0: [sleep] 1: [100] 2:[&]

        arr_jobs[i].pid = proc_id; 
        arr_jobs[i].jid = job_id; 

        if (bgorfg =='B') {
            arr_jobs[i].bg = 1; 
        }
        else if (bgorfg =='F') {
            arr_jobs[i].fg = 1; 
        }

        job_id++;
         
        return 1; 
    }

    return 1;  
}

int reset_jobs(pid_t pid) {
    
    for (int i = 0; i < JOBSLIMIT; i++) {

        if (arr_jobs[i].pid == pid) {

            arr_jobs[i].pid = 0; 
            arr_jobs[i].jid = 0; 
            arr_jobs[i].fg = 0;
            arr_jobs[i].bg = 0;
            arr_jobs[i].stp = 0; 
            arr_jobs[i].command = NULL;
            return 1; 
        }
    }
    return 0;  
}

void child_handler(int sig) {

    int child_status;
    pid_t pid;

    while ((pid = waitpid(-1, &child_status, WNOHANG | WUNTRACED | WCONTINUED)) > 0)
    {   
        
        if (WIFSIGNALED(child_status)) {
            
            statusExit = WTERMSIG(child_status); 
            printf("\n");
            reset_jobs(pid); 
        }
        else if (WIFEXITED(child_status)) {
            
            statusExit = WEXITSTATUS(child_status);
            if (statusExit == 0) {
                printf("[%d]+ Done            ", pidFindJob(pid)->jid); 
                echo_command(pidFindJob(pid)->command, 0);  // pidFindJob(pid) get that job 
            }
            reset_jobs(pid); 
        }

        else if (WIFCONTINUED(child_status)) {
            printf("");
        }
        
        else if (WIFSTOPPED(child_status)) {
            
            statusExit = WSTOPSIG(child_status);
            printf("\n");
            pidFindJob(pid)->stp = 1; 
            printf("[%d]+ Stopped            ", pidFindJob(pid)->jid);
            echo_command(pidFindJob(pid)->command, 0);  // pidFindJob(pid) get that job  
        }
    }
}

void previous_command(char **cmd)
{
    int c;
    int previous_size = 1024;
    int charac = 0;
    record = (char **)malloc(previous_size * sizeof(char *));

    if (record == NULL)
    {
        perror("unsucessfully allocated");
    }

    int size = (int)sizeof(cmd[0]) / sizeof(char);
    //printf("%d",size);

    for (int i = 0; i < size; i++)
    { // word
        if (cmd[i] != NULL)
        {
            record[i] = (char *)malloc(sizeof(cmd[i]) * sizeof(char));
            memcpy(record[i], cmd[i], strlen(cmd[i]) + 1);
        }
    }
}

int get_index(char **cmd) { 

    int i = 0; 

    while (cmd[i] != NULL) { // word
        i++;  
    }
    return i; 
}

int check_bg(char **args, int idx) {

    int background = 0; 

    if (strcmp(args[idx - 1], "&") == 0) {
        background = 1; 
        args[idx - 1] = NULL; 
    }

    else {
        background = 0; 
    }
    return background; 
}

int external_command(char **args) {

    int pid;
    int i, j;
    int exe_stat = 0; 
    int out;
    int is_background = 0;
    int index = get_index(args);  
    
    is_background = check_bg(args,index); 

    if ((pid = fork()) < 0)
    {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }
    if (!pid) {

        int fd0, fd1, i; //fd0, fd1 is file-descriptor
        int size = (int)sizeof(args[0]) / sizeof(char);

        struct sigaction sig_act; 
        sigemptyset(&sig_act.sa_mask);
        sig_act.sa_handler = handler_sig; //return to default becuase the child process also took ignoring signal from parents
        sig_act.sa_flags = 0;
        sigaction(SIGTSTP, &sig_act, NULL); // ^Z
        sigaction(SIGINT, &sig_act, NULL);
        
        pid = getpid();
        setpgid(pid, pid);
        tcsetpgrp(0, pid);

        for (i = 0; i < size; i++) { // word

            if (args[i] != NULL) {

                if (strcmp(args[i], ">") == 0) { //output 

                    args[i] = NULL;  // next command after > , create that file if don't have
                    if ((fd1 = open(args[i + 1], O_CREAT | O_TRUNC | O_WRONLY, 0666)) < 0) // give out the file descriptor 
                    {
                        perror("Couldn't open the output file");
                        exit(0);
                    }
                    dup2(fd1, STDOUT_FILENO); // 1 here can be replaced by STDOUT_FILENO, redirect the descriptor table to particular open file table
                    close(fd1);
                }

                else if (strcmp(args[i], "<") == 0)
        
                {
                    args[i] = NULL; // next command, read only
                    if ((fd0 = open(args[i + 1], O_RDONLY)) < 0)
                    {
                        perror("Couldn't open the input file");
                        exit(0);
                    }
                    //dup2() copies content of fdo in input of preceeding file
                    dup2(fd0, 0);
                    close(fd0);
                }
            }
        }

        if (execvp(args[0], args) < 0){

            printf("bad command");
            printf("\n");
            exit(1); 
        }

        exe_stat = execvp(args[0], args); 
    }

    if (pid) {
       
        if (is_background == 0) {

            setpgid(pid, pid); // setpgrp(pid_t pid, pid_t pgrp);
            // system call sets the process group of the specified process pid to the specified pgrp.
            add_job(pid,args,inFG);
            tcsetpgrp(0, pid); // set fg group id 
            int wstatus;
            value = waitpid(pid, &wstatus, WUNTRACED);
            tcsetpgrp(0, getpid());

            if (value > 0) {

                if (WIFEXITED(wstatus)) {

                    statusExit = WEXITSTATUS(wstatus);
                    if (statusExit == 0) {
                        printf("[%d]+ Done            ", pidFindJob(pid)->jid); 
                        echo_command(pidFindJob(pid)->command, 0);  // pidFindJob(pid) get that job 
                    }
                    reset_jobs(pid); 
                }

                else if (WIFSIGNALED(wstatus)) {

                    statusExit = WTERMSIG(wstatus); 
                    printf("\n");
                    reset_jobs(pid); 
                }

                else if (WIFSTOPPED(wstatus)) {

                    statusExit = WSTOPSIG(wstatus);
                    pidFindJob(pid)->stp = 1; 
                    printf("\n");
                    printf("[%d]+ Stopped            ", pidFindJob(pid)->jid);
                    echo_command(pidFindJob(pid)->command, 0);  // pidFindJob(pid) get that job  
    
                }

                else if (WIFCONTINUED(wstatus)) {
                    printf("Continued\n");
                }
            }
        }

        else if (is_background == 1) {

            args[index - 1] = "&"; 
            add_job(pid,args,inBG); ///////////// not come out  //struct jobs *proc = pidFindJob(arr_jobs,pid);
            printf("[%d] %d\n", pidFindJob(pid)->jid, pidFindJob(pid)->pid);
            sleep(1); 
            tcsetpgrp(0, getpid());
            
        }

        if (exe_stat == -1) { 

            reset_jobs(pid);
        }
    }

    return 1;
}

int get_bg_status(int job_id) {
    
    if (jobFindpid(job_id) == NULL) {
        return -1; 
    }

    return jobFindpid(job_id)->bg; 
}

int get_fg_status(int job_id) {
    
    if (jobFindpid(job_id) == NULL) {
        return -1; 
    }

    return jobFindpid(job_id)->fg; 
}

int get_stp_status(int job_id) {
    
    if (jobFindpid(job_id) == NULL) {
        return -1; 
    }

    return jobFindpid(job_id)->stp; 
}

pid_t get_pid_from_job(int job_id) {
    
    if (jobFindpid(job_id) == NULL) {
        return -1; 
    }

    return jobFindpid(job_id)->pid; 
}

void print_fg(pid_t pid) {

    int size = (int)sizeof(pidFindJob(pid)->command[0]) / sizeof(char);
    
    for (int idx = 0; idx < size; idx++) { // word
        if (pidFindJob(pid)->command[idx] != NULL) {
            if (strcmp(pidFindJob(pid)->command[idx], "&") != 0) {
                printf("%s "
                "",
                   pidFindJob(pid)->command[idx]);
            }
        }
    }
    printf("\n");
}

int fg(char **args) {

    int job_id  = 0; 
    pid_t pid; 
    int status = 0; 
    int bg_stat; 
    
    if (args[1][0] == '%') {

        job_id = atoi(args[1]+1); 
        pid = get_pid_from_job(job_id); 
        bg_stat = get_bg_status(job_id); 
        if (pid < 0 || bg_stat != 1) {

            printf("Error: No such job number < %s > or \n", args[1]); 
            printf("The job < %s > is not in the background status \n", args[1]); 
            return -1; 
        }
    }

    print_fg(pid); 
    int index = get_index(pidFindJob(pid)->command);   
    pidFindJob(pid)->command[index - 1] = NULL; 
    tcsetpgrp(0, pid);
    int value = waitpid(pid, &status, WUNTRACED);
    tcsetpgrp(0, getpid());

    if (value > 0) {  

        if (WIFEXITED(status)) {
        
            statusExit = WEXITSTATUS(status);

                if (statusExit == 0) {
                    printf("[%d]+ Done            ", pidFindJob(pid)->jid); 
                    echo_command(pidFindJob(pid)->command, 0);  // pidFindJob(pid) get that job 
                }

            reset_jobs(pid); 

        } else if (WIFSIGNALED(status)) {

            statusExit = WTERMSIG(status); 
            printf("\n");
            //printf("Killed by signal=%d%s\n", statusExit, WCOREDUMP(child_status) ? " (dumped core)" : "");
            reset_jobs(pid);  

        } else if (WSTOPSIG(status)) {

            statusExit = WSTOPSIG(status);
            pidFindJob(pid)->stp = 1; 
            printf("\n");
            printf("[%d]+ Stopped            ", pidFindJob(pid)->jid);
            echo_command(pidFindJob(pid)->command, 0);
        }
    }

    return 1; 
}

int bg(char **args) { 

    int job_id  = 0; 
    pid_t pid; 
    int status = 0; 
    int fg_stat; 
    int stp_stat;

    if (args[1][0] == '%') {

        job_id = atoi(args[1]+1); 
        //printf("%d \n", job_id); 
        pid = get_pid_from_job(job_id); 
        fg_stat = get_fg_status(job_id); 
        stp_stat = get_stp_status(job_id); 
        // printf("fg_stat: %d \n", fg_stat); 
        // printf("pid: %d \n", pid); 

        if (pid < 0 || fg_stat != 1 || stp_stat != 1) {

            if (fg_stat != 1) {
                printf("Error: The job < %s > is not in the foreground status or \n", args[1]);
                printf("No such job number < %s > \n", args[1]);  
            }
            else if (stp_stat != 1) {
                printf("Error: The job < %s > havn't been stopped or \n", args[1]); 
                printf("No such job number < %s > \n", args[1]);  
            }

            return -1; 
        }
        int index = get_index(pidFindJob(pid)->command);   
        printf("[%d]+ ", pidFindJob(pid)->jid); 
        pidFindJob(pid)->command[index + 1] = "&"; 
        echo_command(pidFindJob(pid)->command,0); 
        kill(pid, SIGCONT);
    }
    return 1; 
}

int command_execute(char **cmd, char **history)
{   

    if (cmd[0] == NULL)
    {
        return 1;
    }

    if (strcmp(cmd[0], "echo") == 0)
    {

        if (strcmp(cmd[1], "$?") != 0)
        {

            previous_command(cmd);
            echo_command(cmd,1);
            flag_his = 1;
            return 1;
        }

        else if (strcmp(cmd[1], "$?") == 0)
        {
            printf("%d \n", statusExit);
            return 1;
        }
    }

    else if (strcmp(cmd[0], "!!") == 0) {

        int size = (int)sizeof(history[0]) / sizeof(char);

        if (flag_his == 1)
        {
            if (history[0] == NULL)
            {

                flag_his = 0;
                return 1;
            }
            if (strcmp(history[0], "!!") == 0)
            {
                flag_his = 0;
                return 1;
            }
            if (strcmp(history[0], "!!") != 0)
            {

                if (argNumber == 2)
                {
                    command_execute(history, record);
                }
                else
                {
                    for (int i = 0; i < size; i++)
                    { // word
                        if (history[i] != NULL)
                        {
                            printf("%s "
                                   "",
                                   history[i]);
                        }
                    }
                    printf("\n");
                    command_execute(history, record);
                }
            }
        }
        else
        {
            command_execute(history, record);
        }
    }
    else if (strcmp(cmd[0], "jobs") == 0) {

        for (int i = 0; i < JOBSLIMIT; i++) {
            if (arr_jobs[i].pid != 0)  {

                if (arr_jobs[i].stp == 1) {
                    printf("[%d] Stopped            ", arr_jobs[i].jid); 
                    echo_command(arr_jobs[i].command, 0); 
                }
                else if (arr_jobs[i].stp == 0) {
                    printf("[%d] Running            ", arr_jobs[i].jid); 
                    echo_command(arr_jobs[i].command, 0); 
                }
            }  
        }
    }

    else if (strcmp(cmd[0], "fg") == 0){
        fg(cmd); 
    }

    else if (strcmp(cmd[0], "bg") == 0){
        bg(cmd); 
    }

    else if (strcmp(cmd[0], "exit") == 0)
    {

        if (cmd[1] == NULL)
        {
            printf("bye \n");
            return 0;
        }
        
        else {

            if (argNumber == 2) {

                exit_return = atoi(cmd[1]); 
                return 0;   
        
            }
        }
    }

    else
    {
        previous_command(cmd);
        external_command(cmd);
        return 1;
    }

    return 1;
}

int command_loop()
{

    char *line;
    char **cmd;
    int status;

    printf("icsh $ ");
    line = line_reader();
    cmd = string_token(line);
    if (flag_his == 0)
    {
        previous_command(cmd);
    }
    flag_his = 1;
    status = command_execute(cmd, record);

    free(line);
    free(cmd);
    return status;
}

int check_file(char *each_line, char *path)
{

    char *line;
    char **cmd;
    int status = 0;
    char *token;

    //printf("path: %s \n",path);

    if (count == 0)
    {

        token = strtok(each_line, " \t\n");

        //printf("token: %s \n",token);
        while (token != NULL)
        {

            if (indx == 0)
            {
                if ((strcmp(token, "##") == 0))
                {
                    indx++;
                }
                else
                {
                    printf("No file specification ex: ## ... \n");
                }
            }
            if (indx == 2)
            {
                if ((strcmp(token, path) == 0))
                {
                    count++;
                }
                else
                {
                    printf("Wrong File Name \n");
                }
            }
            indx++;
            token = strtok(NULL, " \t\n");
        }
    }

    else

    {
        cmd = string_token(each_line);
        if (flag_his == 0)
        {
            previous_command(cmd);
        }
        flag_his = 1;
        status = command_execute(cmd, record);

        free(line);
        free(cmd);
    }
    return status;
}

void sghl(){

    struct sigaction sig_act; 
    sig_act.sa_handler = SIG_IGN;
    sigaction(SIGINT, &sig_act, NULL);
    sigaction(SIGTSTP, &sig_act, NULL);
    sigaction(SIGTTOU, &sig_act, NULL);
    sigaction(SIGTTIN, &sig_act, NULL);

}

void chdhl() {

    struct sigaction action;
    sigemptyset(&action.sa_mask);
    action.sa_handler = child_handler;
    action.sa_flags = 0;
    sigaction(SIGCHLD, &action, NULL);

}

int main(int argc, char *argv[])
{
    char *path = argv[1];
    argNumber = argc;
    FILE *fp = fopen(path, "r");
    char *each_line = NULL;
    size_t lenght = 0;
    ssize_t read;

    printf("Starting IC shell \n");
    
    sghl(); 
    chdhl();  

    for (int i = 0; i < JOBSLIMIT; i++) {
        arr_jobs[i].pid = 0; 
        arr_jobs[i].jid = 0; 
        arr_jobs[i].fg = 0;
        arr_jobs[i].bg = 0;
        arr_jobs[i].stp = 0; 
        arr_jobs[i].command = NULL;
    }

    if (argNumber == 2)
    {
        int count = 0;
        int indx = 0;

        if (fp == NULL)
        {
            perror("open file unsucessfully");
        }
        while ((read = getline(&each_line, &lenght, fp)) != -1)
        {
            status_loop = check_file(each_line, path);
        }
        fclose(fp);
    }
    else
    {

        while (status_loop)

        {
            status_loop = command_loop();
        }
    }
    free(record);
    return exit_return;
}