#include "user/user.h"
#include "kernel/syscall.h"
#include "kernel/proc.h"
#define MAXARG 10

int main(int argc,char* argv[])
{
    char* nargv[MAXARG];
    if (trace(atoi(argv[1])) < 0) 
    {
        fprintf(2, "%s: trace failed\n", argv[0]);
        exit(1);
    }

    for(int i = 2; i < argc && i < MAXARG; i++)
    {
        nargv[i-2] = argv[i];
    }
    exec(nargv[0], nargv);
    exit(0);
}