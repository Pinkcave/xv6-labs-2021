#include "kernel/types.h"
#include "user/user.h"

int main(int argc,char* argv[])
{
   if(argc!=1)
   {
     write(2,"Usage:pingpong",sizeof("Usage:pingpong"));
     exit(1);
   }
   /* user/pingpong.c */
   int pipe1[2],pipe2[2];
   char buf[1] = {0};
   pipe(pipe1);
   pipe(pipe2);
   
   int pid = fork();
   if(pid<0)
   {
      printf("Fork error");
      close(pipe1[0]);
      close(pipe1[1]);
      close(pipe2[0]);
      close(pipe2[1]);
      exit(1);
   }
   else if(pid>0)
   {
      close(pipe2[1]);
      close(pipe1[0]);
      
      if(write(pipe1[1],buf,sizeof(buf))!=sizeof(buf))
      {
           printf("Parent write error");
           exit(4);
      }
      close(pipe1[1]);
      wait(0);
      if(read(pipe2[0],buf,sizeof(buf)!=sizeof(buf)))
      {
           printf("Parent write error");
           exit(5);
      }
      close(pipe2[0]);
      printf("%d:received pong\n",getpid());
      exit(0);
   }
   else
   {
      //子进程部分
      //关闭pipe1写的一端，关闭pipe2读的一端
      close(pipe1[1]);
      close(pipe2[0]);
      //读数据
      if(read(pipe1[0],buf,sizeof(buf)!=sizeof(buf)))
      {
         printf("child read error");
         exit(2);
      }
      close(pipe1[0]);
      printf("%d:received ping\n",getpid());
      //写数据
      if(write(pipe2[1],buf,sizeof(buf))!=sizeof(buf))
      {
          printf("child write error");
          exit(3);
      }
      close(pipe2[1]);
      exit(0);
   }
   
}