#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include "parse.h"


#define STARTING_PHASE 1

#define INTERMIDEATE_PHASE 2

#define END_PHASE 4

#define ECHO "echo"

#define CD "cd"

#define NICE "nice"

#define PWD "pwd"

#define SETENV "setenv"

#define UNSETENV "unsetenv"

#define LOGOUT "logout"

#define WHERE "where"

#define END "end"

#define STD_COMMAND_NOT_FOUND "command not found"

#define STD_ACCESS_DENIDED "permission denied"

#define HOST_NAME_SIZE 255

#define MAIN_INSTANCE 2

#define READING_FROM_INITIALIZATION_FILE 4
