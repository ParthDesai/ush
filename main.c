#include "common.h"

#define HOST_NAME_SIZE 255

#define MAIN_INSTANCE 2

#define READING_FROM_INITIALIZATION_FILE 4

int SavedStdinFileDescriptor = -1;

typedef void (*sighandler_t)(int);

void SetSignalHandler(int signum,sighandler_t signalHandler)
{
    struct sigaction act;
    act.sa_handler = signalHandler;
    act.sa_flags = 0;
    sigaction(signum,&act,NULL);
}



void IgnoreSignals()
{
    SetSignalHandler(SIGQUIT,SIG_IGN);
    SetSignalHandler(SIGINT,SIG_IGN);
    SetSignalHandler(SIGTTIN,SIG_IGN);
    SetSignalHandler(SIGTTOU,SIG_IGN);
    SetSignalHandler(SIGCHLD,SIG_IGN);
}

void ResetToDefaultSignals()
{
    SetSignalHandler(SIGQUIT,SIG_DFL);
    SetSignalHandler(SIGINT,SIG_DFL);
    SetSignalHandler(SIGTTIN,SIG_DFL);
    SetSignalHandler(SIGTTOU,SIG_DFL);
    SetSignalHandler(SIGCHLD,SIG_DFL);
}

char * GetEnvMalloc(const char * name,int extraSpace)
{
    char * envVal = getenv(name);
    if(extraSpace < 0)
    {
        return NULL;
    }
    if(envVal == NULL)
    {
        return NULL;
    }
    char * envValCopy = (char *)malloc(sizeof(char) * (strlen(envVal) + 1 + extraSpace));
    if(envValCopy == NULL)
    {
        return NULL;
    }
    strcpy(envValCopy,envVal);
    return envValCopy;
}


void PrintHostName()
{
    static char * name = NULL;
    if(name == NULL)
    {
        name = (char *)malloc(sizeof(char) * 255);
        gethostname(name,HOST_NAME_SIZE);
    }
    printf("%s%%",name);
    fflush(stdout);
}


void ReadFromInitializationFile()
{
    char * initializationFilePath = GetEnvMalloc("HOME",10);
    strcat(initializationFilePath,"/.ushrc");
    int initFD = open(initializationFilePath,O_RDONLY);
    if(initFD == -1)
    {
        free(initializationFilePath);
        return;
    }
    free(initializationFilePath);
    SavedStdinFileDescriptor = dup(STDIN_FILENO);
    dup2(initFD,STDIN_FILENO);
    close(initFD);
    Pipe pipeHead = NULL;
    while(SavedStdinFileDescriptor != -1)
    {
        pipeHead = parse();
        ProcessPipeList(pipeHead);
    }
}


int main()
{
    Pipe pipeHead = NULL;
    IgnoreSignals();
    ReadFromInitializationFile();
    for(;;)
    {
        PrintHostName();
        pipeHead = parse();
        ProcessPipeList(pipeHead);
    }
    return 0;
}
