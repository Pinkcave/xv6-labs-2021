#include"user/user.h"
#define MAXARG 10
#define BUFSIZE 512
int main(int argc,char* argv)
{
    if(argc<2)
    {
        printf("xargs: 2 args at least\n");
    }
    else if(argc>MAXARG)
    {
        printf("xargs: %d args at most\n");
    }

    char* xargv[MAXARG];
    for(int i=1;i<MAXARG;i++)
    {
        xargv[i-1]=argv[i];
    }

    char buf[BUFSIZE];
    int i=0;
    int j=argc-1;
    while(read(0,buf[i],sizeof(char)))
    {
        if(buf[i]==' '||buf[i]=='\n')
        {
            buf[i]='\0';
            xargv[j]=buf;
            j++;
            i=0;
        }
        else
            i++;
        
        if(buf[i]=='\n')
        {
            xargv[j]='\0';
            j=argc-1;
            int pid = fork();
            if(pid<0)
            {
                printf("Fork error");
                exit(1);
            }
            else if(pid>0)
            {
                wait(0);
            }
            else
            {
                exec(xargv[0],xargv);
            }
        }
    }
    exit(0);
}