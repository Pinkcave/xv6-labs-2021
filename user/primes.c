#include "kernel/types.h"
#include "user/user.h"

void primes(int* pipe_left)
{
    close(pipe_left[1]);
    int prime;
    int size = read(pipe_left[0],&prime,sizeof(prime));
    if(size<=0)
    {
        close(pipe_left[0]);
        exit(0);
    }
    
    printf("prime %d\n",prime);

    int num;
    int pipe_right[2];
    pipe(pipe_right);

    int pid = fork();
    if(pid<0)
    {
        printf("Fork error");
        close(pipe_left[0]);
        close(pipe_right[0]);
        close(pipe_right[1]);
        exit(1);
    }
    else if (pid>0)
    {
        close(pipe_right[0]);
        while(read(pipe_left[0],&num,sizeof(num)))
        {
            if(num % prime != 0)
            {
                write(pipe_right[1],&num,sizeof(num));
            }
        }
        close(pipe_left[0]);
        close(pipe_right[1]);
        wait(0);
        exit(0);
    }
    else
    {
        close(pipe_left[0]);
        close(pipe_right[1]);
        primes(pipe_right);
        close(pipe_right[0]);
        exit(0);
    }
}

int main(int argc,char* argv[])
{
    if(argc!=1)
    {
        write(2,"Usage:primes",sizeof("Usage:primes"));
        exit(1);
    }

    int p[2];
    pipe(p);

    int pid = fork();
    if(pid<0)
    {
        printf("Fork error");
        close(p[0]);
        close(p[1]);
        exit(1);
    }
    else if(pid>0)
    {
        close(p[0]);
        for(int i=2;i<=35;i++)
        {
            write(p[1],&i,sizeof(i));
        }
        close(p[1]);
        wait(0);
        exit(0);
    }
    else
    {
        close(p[1]);
        primes(p);
        close(p[0]);
        exit(0);
    }
}