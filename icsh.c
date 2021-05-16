#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>

int flag_his = 0;
char **record;
int status_loop = 1;
int exit_return = 0;
int count;
int indx;
int argNumber;
struct sigaction sig_act;

char *line_reader()
{
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

            // for (int i = 0; i < strlen(buffer); i++) { // word
            //   printf("print: %c", buffer[i]);
            // }
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

void echo_command(char **cmd)
{

    int i, j;
    int size = (int)sizeof(cmd[0]) / sizeof(char);
    //printf("%d",size);
    for (int i = 1; i < size; i++)
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

void previous_command(char **cmd)
{
    int c;
    int previous_size = 1024;
    int charac = 0;
    record = (char **)malloc(previous_size * sizeof(char **));

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

int external_command(char **args)
{

    int pid;
    int i, j;
    int out; 

    if ((pid = fork()) < 0)
    {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }
    if (!pid)
    {
      int fd0,fd1,i;  //fd0, fd1 is file-descriptor

      sigemptyset(&sig_act.sa_mask);
      sig_act.sa_handler = SIG_DFL;  //return to default becuase the child process also took ignoring signal from parents
      sig_act.sa_flags = 0;
      sigaction(SIGTSTP, &sig_act, NULL);

      int size = (int)sizeof(args[0]) / sizeof(char);

      for (i = 0; i < size; i++) { // word

        if (args[i] != NULL) {

          if (strcmp(args[i], ">") == 0) {

            args[i]=NULL;
            if ((fd1 = open(args[i+1] , O_CREAT | O_TRUNC | O_WRONLY, 0666)) < 0) {
              perror("Couldn't open the output file");
              exit(0);
            }
            dup2(fd1, STDOUT_FILENO); // 1 here can be replaced by STDOUT_FILENO
            close(fd1);
          }

          else if (strcmp(args[i], "<") == 0) {
            args[i]=NULL;
            if ((fd0 = open(args[i+1] , O_RDONLY)) < 0) {
              perror("Couldn't open the output file");
              exit(0);
            }
            //dup2() copies content of fdo in input of preceeding file
            dup2(fd0, 0);
            close(fd0);
          }
        }
      }  

      if (execvp(args[0], args) < 0) {
        printf("bad command");
        printf("\n");
      }
      
      else {
        execvp(args[0], args);
        printf("\n");
      }
    }
    if (pid)
    {
        waitpid(pid, NULL, 0);
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

        previous_command(cmd);
        echo_command(cmd);

        flag_his = 1;
        return 1;
    }

    else if (strcmp(cmd[0], "!!") == 0)
    {

        int size = (int)sizeof(history[0]) / sizeof(char);

        if (flag_his == 1)
        {
            if (history[0] == NULL)
            {

                printf("in Null");
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
    else if (strcmp(cmd[0], "exit") == 0)
    {

        if (cmd[1] == NULL)
        {
            printf("bye \n");
            return 0;
        }

        else
        {

            if (argNumber == 2)
            {

                exit_return = atoi(cmd[1]);
                u_int8_t truncate = cmd[1];

                if (truncate >= 0 || truncate <= 255)
                {
                    return 0;
                }
            }
            else
            {
                exit_return = atoi(cmd[1]);
                u_int8_t truncate = cmd[1];

                if (truncate >= 0 || truncate <= 255)
                {
                    printf("bye \n");
                    return 0;
                }
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

    printf("Starting IC shell \n");
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

void handler_sig()
{
    status_loop = 1;
    printf("\n");
    //printf("stat:%d \n", status_loop);
}

void snhl() {

    sig_act.sa_handler = handler_sig; //ignore signal
    sigaction(SIGTSTP, &sig_act, NULL);
    sigaction(SIGINT, &sig_act, NULL);
}


int main(int argc, char *argv[])
{

    char *path = argv[1];
    argNumber = argc;
    FILE *fp = fopen(path, "r");
    char *each_line = NULL;
    size_t lenght = 0;
    ssize_t read;

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
        snhl(); 

        while (status_loop)
        {
            //printf("in main \n"); 

            //printf("stat:%d \n", status_loop);

            status_loop = command_loop();
          
        }
    }
    free(record); 
    return exit_return;
}



