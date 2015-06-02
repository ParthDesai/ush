#include "common.h"

extern char ** environ;

extern int SavedStdinFileDescriptor;

void ProcessCommandInPipeLine(Cmd command,int ** pipeDescriptors,int * pipeStatus,int * specifiedNiceValue,pid_t * processGroupID);

char * GetEnvMalloc(const char * name,int extraspace);

extern int Flags;

char * GetWorkingDirectory()
{
    int memorySize = 2000;
    char * workingDir = (char *) malloc(sizeof(char) * memorySize);
    workingDir = getcwd(workingDir,memorySize);
    return workingDir;
}

void AddStringIntoArray(char *** stringArray,char * stringToCopy)
{
    int numberOfElements;
    if(*stringArray == NULL)
    {
        numberOfElements = 1;
        *stringArray = (char **) malloc(sizeof(char *) * 2);
    }
    else
    {
        numberOfElements = 1;
        int i = 0;
        for(i = 0 ; (*stringArray)[i] != NULL ; i++)
        {
            numberOfElements++;
        }
        *stringArray = (char **) realloc(*stringArray,(numberOfElements + 1) * sizeof(char *));
    }
    (*stringArray)[numberOfElements - 1] = (char *) malloc(sizeof(char *) * (strlen(stringToCopy) + 1));
    strcpy((*stringArray)[numberOfElements - 1],stringToCopy);
    (*stringArray)[numberOfElements] = NULL;
}

void FreeStringArray(char ** stringArray)
{
    int i = 0 ;
    for(i = 0 ; stringArray[i] != NULL ; i++)
    {
        free(stringArray[i]);
    }
    free(stringArray);
}

char ** ResolveCommands(char * commandName,int resolveOnlyFirst)
{
    char ** commandInstances = NULL;
    if(commandName[0] == '/')
    {
        if(access(commandName,X_OK) == 0)
        {
            AddStringIntoArray(&commandInstances,commandName);
        }
    }
    else
    {
        if(index(commandName,'/') != NULL)
        {
            char * workingDirectoryRelativePath = GetWorkingDirectory();
            workingDirectoryRelativePath = (char *) realloc(workingDirectoryRelativePath,sizeof(char) * (strlen(workingDirectoryRelativePath) +  strlen(commandName) + 2));
            strcat(workingDirectoryRelativePath,"/");
            strcat(workingDirectoryRelativePath,commandName);
            printf("%s\n",workingDirectoryRelativePath);
            if(access(workingDirectoryRelativePath,X_OK) == 0)
            {
                AddStringIntoArray(&commandInstances,workingDirectoryRelativePath);
            }
            free(workingDirectoryRelativePath);
        }
        else
        {
            char * colonDelimitedPath = GetEnvMalloc("PATH",1);
            char * path = NULL;
            char * buffer = NULL;
            while((path  = strtok(colonDelimitedPath,":")) != NULL)
            {
                buffer = (char *) malloc(sizeof(char) * (strlen(path) + strlen(commandName) + 2));
                strcpy(buffer,path);
                strcat(buffer,"/");
                strcat(buffer,commandName);
                if(access(buffer,X_OK) == 0)
                {
                    AddStringIntoArray(&commandInstances,buffer);
                }
                free(buffer);
                colonDelimitedPath = NULL;
            }
            free(colonDelimitedPath);
        }
    }
    return commandInstances;
}

int IsBuiltInCommand(char * commandName)
{
    if(strcasecmp(commandName,ECHO) == 0)
    {
        return 1;
    }
    else if(strcasecmp(commandName,CD) == 0)
    {
        return 1;
    }
    else if(strcasecmp(commandName,NICE) == 0)
    {
        return 1;
    }
    else if(strcasecmp(commandName,PWD) == 0)
    {
        return 1;
    }
    else if(strcasecmp(commandName,SETENV) == 0)
    {
        return 1;
    }
    else if(strcasecmp(commandName,UNSETENV) == 0)
    {
        return 1;
    }
    else if(strcasecmp(commandName,LOGOUT) == 0)
    {
        return 1;
    }
    else if(strcasecmp(commandName,WHERE) == 0)
    {
        return 1;
    }
    else if(strcmp(commandName,END) == 0)
    {
        return 1;
    }
    return 0;
}

void Executenicely(Cmd cmd,FILE * commandInputStream,FILE * commandOutputStream,FILE * commandErrorStream)
{
    if(cmd->nargs < 2)
    {
        int niceValue = 4;
        nice(niceValue);
    }
    else if(cmd->nargs >= 2)
    {
        int convertedInteger = strtol(cmd->args[1],NULL,0);
        if(convertedInteger == LONG_MAX || convertedInteger <= 0)
        {
            fprintf(commandErrorStream,"Invalid argument.\n");
            fflush(commandErrorStream);
            return;
        }
        if(cmd->nargs == 2)
        {
            nice(convertedInteger);
        }
        else if(cmd->nargs > 2)
        {
            int i;
            int status;
            int * pipeDescriptors = (int *) malloc(sizeof(int *));
            int pipeStatus = STARTING_PHASE;
            pid_t processGroupID = (pid_t)NULL;
            for(i = 2 ; i < cmd->nargs ; i++ )
            {
                cmd->args[i - 2] = cmd->args[i];
            }
            cmd->args[i - 2] = NULL;
            cmd->nargs = cmd->nargs - 2;
            cmd->next = NULL;
            ProcessCommandInPipeLine(cmd,&pipeDescriptors,&pipeStatus,&convertedInteger,&processGroupID);
            while(wait(&status) != -1);
        }

    }

}

void ExecuteBuiltInCommand(Cmd cmd)
{
    FILE * commandInputStream = stdin;
    FILE * commandOutputStream = stdout;
    FILE * commandErrorStream = stderr;

    if(cmd->in == Tin)
    {
        commandInputStream = fopen(cmd->infile,"r");
        if(commandInputStream == NULL)
        {
            commandInputStream = stdin;
        }
    }

    if(cmd->out == Tout || cmd->out == Tapp)
    {
        commandOutputStream = fopen(cmd->outfile,cmd->out == Tout ? "w" : "a");
        if(commandOutputStream == NULL)
        {
            commandOutputStream = stdout;
        }
    }
    else if(cmd->out == ToutErr || cmd->out == TappErr)
    {
        commandOutputStream = fopen(cmd->outfile,cmd->out == ToutErr ? "w" : "a");
        if(commandOutputStream == NULL)
        {
            commandOutputStream = stdout;
            commandErrorStream = stderr;
        }
        else
        {
            commandErrorStream = commandOutputStream;
        }
    }

    if(strcasecmp(cmd->args[0],ECHO) == 0)
    {
        int i;
        for(i = 1 ; i < cmd->nargs ; i++)
        {
            fprintf(commandOutputStream,"%s ",cmd->args[i]);
        }
        fprintf(commandOutputStream,"\n");
        fflush(commandOutputStream);
    }
    else if(strcasecmp(cmd->args[0],PWD) == 0)
    {
        int memorySize = 2000;
        char * workingDir = (char *) malloc(sizeof(char) * memorySize);
        if(workingDir == NULL)
        {
            fprintf(commandErrorStream,"Error while allocating memory for Buffer.");
            fflush(commandErrorStream);
        }
        else
        {
            if(getcwd(workingDir,memorySize) == NULL)
            {
                fprintf(commandErrorStream,"Unable to get current working directory");
                fflush(commandErrorStream);
            }
            fprintf(commandOutputStream,"%s",workingDir);
            fprintf(commandOutputStream,"\n");
            fflush(commandOutputStream);
            free(workingDir);
        }
    }
    else if(strcasecmp(cmd->args[0],LOGOUT) == 0)
    {
        int status;
        while(wait(&status) != -1);
        exit(0);
    }
    else if(strcasecmp(cmd->args[0],CD) == 0)
    {
        if(chdir(cmd->args[1]) == -1)
        {
            if(errno == EACCES || errno == ENOTDIR)
            {
                fprintf(commandErrorStream,"permission denied");
                fprintf(commandErrorStream,"\n");
                fflush(commandErrorStream);
            }
            else if(errno == ENOENT)
            {
                fprintf(commandErrorStream,"no such file or directory.");
                fprintf(commandErrorStream,"\n");
                fflush(commandErrorStream);
            }
        }
    }
    else if(strcasecmp(cmd->args[0],SETENV) == 0)
    {
        if(cmd->nargs == 1)
        {
            int i;
            for(i = 0 ; environ[i] != NULL; i++)
            {
                fprintf(commandOutputStream,"%s",environ[i]);
                fprintf(commandOutputStream,"\n");
                fflush(commandOutputStream);
            }
        }
        else if(cmd->nargs == 2)
        {
            setenv(cmd->args[1],"",1);
        }
        else if(cmd->nargs == 3)
        {
            setenv(cmd->args[1],cmd->args[2],1);
        }
    }
    else if(strcasecmp(cmd->args[0],UNSETENV) == 0)
    {
        if(cmd->nargs > 1)
        {
            unsetenv(cmd->args[1]);
        }
    }
    else if(strcasecmp(cmd->args[0],NICE) == 0)
    {
        Executenicely(cmd,commandInputStream,commandOutputStream,commandErrorStream);
    }
    else if(strcasecmp(cmd->args[0],WHERE) == 0)
    {
        if(IsBuiltInCommand(cmd->args[1]) == 1)
        {
            fprintf(commandOutputStream,"%s : shell built-in command.",cmd->args[1]);
            fflush(commandOutputStream);
        }
        char ** commands = ResolveCommands(cmd->args[1],0);
        if(commands != NULL)
        {

            int i = 0;
            for(i = 0 ; commands[i] != NULL ; i++)
            {
                fprintf(commandOutputStream,"%s\n",commands[i]);
            }
            FreeStringArray(commands);
        }
    }
    else if(strcmp(cmd->args[0],END) == 0)
    {
        if(SavedStdinFileDescriptor == -1)
        {
            int status;
            while(wait(&status) != -1);
            fclose(commandOutputStream);
            fclose(commandInputStream);
            fclose(commandErrorStream);
            exit(0);
        }
        else
        {
            dup2(SavedStdinFileDescriptor,STDIN_FILENO);
            close(SavedStdinFileDescriptor);
            SavedStdinFileDescriptor = -1;
        }
    }

    if(commandErrorStream != stderr || commandOutputStream != stdout)
    {
        fclose(commandOutputStream);
    }
    if(commandInputStream != stdin)
    {
        fclose(commandInputStream);
    }
}

void ProcessCommandInPipeLine(Cmd command,int ** pipeDescriptors,int * pipeStatus,int * specifiedNiceValue,pid_t * processGroupID)
{
    int * newFileDescriptors = NULL;
    switch(*pipeStatus)
    {
        case STARTING_PHASE:
        *pipeDescriptors = (int *)malloc(sizeof(int) * 2);
        pipe(*pipeDescriptors);
        break;
        case INTERMIDEATE_PHASE:
        newFileDescriptors = (int *)malloc(sizeof(int) * 2);
        pipe(newFileDescriptors);
        break;
        case END_PHASE:
        break;
    }

    pid_t childPid = fork();
    if(childPid == (pid_t)NULL)
    {
        SavedStdinFileDescriptor = -1;
        if(*processGroupID == (pid_t)NULL)
        {
            setpgid((pid_t)NULL,(pid_t)NULL);
            *processGroupID = getpgid((pid_t)NULL);
            tcsetpgrp(STDIN_FILENO,*processGroupID);

        }
        switch(command->in)
        {
            case Tpipe:
            case TpipeErr:
            close(STDIN_FILENO);
            dup2((*pipeDescriptors)[0],STDIN_FILENO);
            close((*pipeDescriptors)[0]);
            break;
            case Tin:
            freopen(command->infile,"r",stdin);
            break;
            case Tnil:
            break;
            default:
            printf("Should not get here..");
            break;
        }
        switch(command->out)
        {
            case Tnil:
            if(*pipeStatus == INTERMIDEATE_PHASE)
            {
                if(command->next->in == Tpipe)
                {
                    close(STDOUT_FILENO);
                    dup2(newFileDescriptors[1],STDOUT_FILENO);
                }
                else if(command->next->in == TpipeErr)
                {
                    close(STDOUT_FILENO);
                    close(STDERR_FILENO);
                    dup2(newFileDescriptors[1],STDOUT_FILENO);
                    dup2(newFileDescriptors[1],STDERR_FILENO);
                }
                close(newFileDescriptors[1]);
            }
            else
            {
                if(*pipeStatus == STARTING_PHASE)
                {
                    if(command->next != NULL)
                    {
                        if(command->next->in == Tpipe)
                        {
                            close(STDOUT_FILENO);
                            dup2((*pipeDescriptors)[1],STDOUT_FILENO);
                        }
                        else if(command->next->in == TpipeErr)
                        {
                            close(STDOUT_FILENO);
                            close(STDERR_FILENO);
                            dup2((*pipeDescriptors)[1],STDOUT_FILENO);
                            dup2((*pipeDescriptors)[1],STDERR_FILENO);
                        }
                        close((*pipeDescriptors)[1]);
                    }
                }
            }
            break;
            case Tout:
            {
                int fd = open(command->outfile,O_CREAT | O_WRONLY | O_TRUNC,S_IRWXU | S_IRWXG);
                close(STDOUT_FILENO);
                dup2(fd,STDOUT_FILENO);
                close(fd);
            }
            break;
            case Tapp:
            {
                int fd = open(command->outfile,O_CREAT | O_WRONLY | O_APPEND,S_IRWXU | S_IRWXG);
                close(STDOUT_FILENO);
                dup2(fd,STDOUT_FILENO);
                close(fd);
            }
            break;
            case TappErr:
            {
                int fd = open(command->outfile,O_CREAT | O_WRONLY | O_APPEND,S_IRWXU | S_IRWXG);
                close(STDOUT_FILENO);
                close(STDERR_FILENO);
                dup2(fd,STDOUT_FILENO);
                dup2(fd,STDERR_FILENO);
                close(fd);
            }
            break;
            case ToutErr:
            {
                int fd = open(command->outfile,O_CREAT | O_WRONLY | O_TRUNC,S_IRWXU | S_IRWXG);
                close(STDOUT_FILENO);
                close(STDERR_FILENO);
                dup2(fd,STDOUT_FILENO);
                dup2(fd,STDERR_FILENO);
                close(fd);
            }
            break;
            default:
            printf("Should not get here...");
            break;
        }
        if(specifiedNiceValue != NULL)
        {
            nice(*specifiedNiceValue);
        }
        ResetToDefaultSignals();
        if(IsBuiltInCommand(command->args[0]) == 1)
        {
            ExecuteBuiltInCommand(command);
            exit(0);
        }
        else
        {
            if(execvpe(command->args[0],command->args,environ) == -1)
            {
                if(errno == EACCES ||  errno == EISDIR)
                {
                    fprintf(stderr,STD_ACCESS_DENIDED);
                    fprintf(stderr,"\n");
                }
                else if(errno == ENOENT)
                {
                    fprintf(stderr,STD_COMMAND_NOT_FOUND);
                    fprintf(stderr,"\n");
                }
                close((*pipeDescriptors)[0]);
                close((*pipeDescriptors)[1]);
                killpg(*processGroupID,SIGKILL);
                exit(-1);
            }
        }
    }
    else
    {
        if(*processGroupID == (pid_t)NULL)
        {
            setpgid(childPid,(pid_t)NULL);
            *processGroupID = getpgid(childPid);
            //tcsetpgrp(STDIN_FILENO,*processGroupID);
        }
        switch(*pipeStatus)
        {
            case STARTING_PHASE:
            close((*pipeDescriptors)[1]);
            if(command->next == NULL)
            {
                close((*pipeDescriptors)[0]);
            }
            break;
            case INTERMIDEATE_PHASE:
            close((*pipeDescriptors)[0]);
            free(*pipeDescriptors);
            *pipeDescriptors = newFileDescriptors;
            close(newFileDescriptors[1]);
            break;
            case END_PHASE:
            close((*pipeDescriptors)[0]);
            free(*pipeDescriptors);
            if(newFileDescriptors != NULL)
            {
                close(newFileDescriptors[0]);
                close(newFileDescriptors[1]);
                free(newFileDescriptors);
            }
        }
        if(command->next != NULL)
        {
            if(command->next->next != NULL)
            {
                *pipeStatus = INTERMIDEATE_PHASE;
            }
            else
            {
                *pipeStatus = END_PHASE;
            }
        }
    }
}

void ProcessPipe(Pipe p)
{
    Cmd temp = p->head;
    int * pipeDescriptors = (int *) malloc(sizeof(int *));
    int pipeStatus = STARTING_PHASE;
    pid_t processGroupID = (pid_t)NULL;
    if(temp->next == NULL && (IsBuiltInCommand(temp->args[0]) == 1))
    {
        ExecuteBuiltInCommand(temp);
    }
    else
    {
        while(temp != NULL)
        {
            ProcessCommandInPipeLine(temp,&pipeDescriptors,&pipeStatus,NULL,&processGroupID);
            fflush(stdout);
            temp = temp -> next;
        }

    }
    int status;
    while(wait(&status) != -1);
    tcsetpgrp(STDIN_FILENO,getpgrp());
    freePipe(p);
}

void ProcessPipeList(Pipe pipeHead)
{
    Pipe temp = pipeHead;
    while(temp != NULL)
    {
        ProcessPipe(temp);
        temp = temp -> next;
    }
}

