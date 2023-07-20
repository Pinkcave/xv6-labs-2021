#include "kernel/types.h"
#include "user/user.h"

int main(int argc,char* argv[])
{
    if(argc != 2)
    {
        write(2,"paramters didn't match\nUsage:sleep [Time]",sizeof("paramters didn't match\nUsage:sleep [Time]"));
        exit(1);
    }
    
    int clocks = atoi(argv[1]);
    sleep(clocks);
    exit(0);
}