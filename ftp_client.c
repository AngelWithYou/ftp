
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include<fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>


/*
    init_client_socket : 初始化服务端的一个套接字
    @ipstr: ip的点分式字符串
    @port : 端口号
    返回值:
        返回 一个连接的套接字
*/
int init_client_socket(char* ipstr,short port)
{
    //1.创建一个套接字
    int client_sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(client_sockfd == -1)
    {
        perror("socket error");
        return -1;
    }
   

    //2.指定服务器的ip地址 : ip + 端口号
    struct sockaddr_in serv;
    serv.sin_family = AF_INET;//指定协议族
    serv.sin_port = htons(port);//htons(6666);//指定端口号
    serv.sin_addr.s_addr = inet_addr(ipstr);//inet_addr("172.11.0.1");//指定IP

    //连接
    int ret = connect(client_sockfd,(struct sockaddr*)&serv,sizeof(serv));
    if(ret == -1)
    {
        perror("connect error");
        close(client_sockfd);
        return -1;
    }
    printf("connect success!!!\n");

    return client_sockfd;
}

/*
    recv_ls : 接收ls执行的结果,接收文件名
    @client_sock : 连接套接字
        返回值 : 
        失败 返回 -1
*/
int recv_ls(int client_sockfd)
{
    while(1)
    {
        int size;
        char filename[128] = {0};

        //接收文件名大小
        recv(client_sockfd,&size,sizeof(size),0);

        //接收名字
        int ret = recv(client_sockfd,filename,size,0);
        if(ret <= 0)
        {
            perror("recv ls filename error");
            return -1;
        }

        //收到退出条件
        if(strcmp(filename,"OVER") == 0 ) 
        {
            break;
        }
        printf("%s\n",filename);
    }

}

/*
    recv_file : 处理get命令,从服务器接收文件
    @client_sockfd : 套接字
    @filename : 要下载的文件名
        返回值 ：
            失败 返回 -1
*/
int recv_file(int client_sockfd,char* filename)
{
    //先发送文件名,再去接收文件的内容

    //发送文件名的大小
    int name_size = strlen(filename);
    int ret = send(client_sockfd,&name_size,sizeof(name_size),0);
    if(ret <= 0)
    {
        perror("send namesize error");
        return -1;
    }
    printf("name_size = %d\n",name_size);

    //发送文件名
    ret = send(client_sockfd,filename,name_size,0);
    if(ret <= 0)
    {
        perror("send filename error");
        return -1;
    }
    printf("filename = %s\n",filename);

    

    //接收文件的大小
    int file_size = 0;
    ret = recv(client_sockfd,&file_size,sizeof(file_size),0);
    if(ret <= 0)
    {
        perror("send file_size error");
        return -1;
    }
    printf("file_size = %d\n",file_size);
    

    //创建一个文件
    int fd = open(filename,O_RDWR|O_CREAT,0666);
    if(fd == -1)
    {
        perror("create file error");
        return -1;
    }

    //接收文件的内容
    int write_size = 0;
    while(write_size < file_size)
    {
        //先接收,在写入
        char buf[10] = {0};
        int ret = recv(client_sockfd,buf,10,0);
        if(ret > 0)
        {
            write(fd,buf,ret);
            write_size += ret;
        }
        else
        {
            perror("write error");
            break;
        }
    }

    if(write_size == file_size)
    {
        printf("recv over\n");
    }

    //关闭文件
    close(fd);
}


  //上传文件
int put_file(int client_sockfd,char *filename)
{
    //发送文件名的大小
    int name_size = strlen(filename);
    int ret = send(client_sockfd,&name_size,sizeof(name_size),0);
    if(ret <= 0)
    {
        perror("send namesize error");
        return -1;
    }
    printf("name_size = %d\n",name_size);

    //发送文件名
    ret = send(client_sockfd,filename,name_size,0);
    if(ret <= 0)
    {
        perror("send filename error");
        return -1;
    }
    printf("filename = %s\n",filename);

    //发送文件的大小
    struct stat st;
    stat(filename,&st);//获取文件的属性 ---> 文件大小

    int file_size = st.st_size;
    ret = send(client_sockfd,&file_size,sizeof(file_size),0);
    if(ret <= 0)
    {
        perror("send file_size error");
        return -1;
    }
    printf("file_size = %d\n",file_size);

    //读取文件内容,发送数据
    int fd = open(filename,O_RDWR);
    if(fd == -1)
    {
        perror("open file error");
        return -1;
    }

    int readsize = 0;
    while(readsize < file_size)
    {
        //先读取,在发送
        char buf[10] = {0};
        ret = read(fd,buf,10);
        if(ret > 0)
        {
            send(client_sockfd,buf,ret,0);
            readsize += ret;
        }
        else
        {
            perror("read error");
            break;
        }
    }

    if(readsize == file_size)
    {
        printf("put over!\n");
    }

    //关闭文件
    close(fd);

}

//删除文件
int rm_file(int client_sockfd,char *filename)
{
   
    //发送文件名的大小
    int name_size = strlen(filename);
    int ret = send(client_sockfd,&name_size,sizeof(name_size),0);
    if(ret <= 0)
    {
        perror("send namesize error");
        return -1;
    }
    printf("name_size = %d\n",name_size);

    //发送文件名
    ret = send(client_sockfd,filename,name_size,0);
    if(ret <= 0)
    {
        perror("send filename error");
        return -1;
    }
    printf("filename = %s\n",filename);

}

int cd_dir(int client_sockfd,char *filename)
{
    //发送文件名的大小
    int name_size = strlen(filename);
    int ret = send(client_sockfd,&name_size,sizeof(name_size),0);
    if(ret <= 0)
    {
        perror("send namesize error");
        return -1;
    }
    printf("name_size = %d\n",name_size);

     //发送文件名
    ret = send(client_sockfd,filename,name_size,0);
    if(ret <= 0)
    {
        perror("send filename error");
        return -1;
    }
    printf("filename = %s\n",filename);


    while (1)
    {
        int size;
        char filename[128] = {0};

        //接收文件名大小
        recv(client_sockfd,&size,sizeof(size),0);

        //接收名字
        int ret = recv(client_sockfd,filename,size,0);
        if(ret <= 0)
        {
            perror("recv ls filename error");
            return -1;
        }

        //收到退出条件
        if(strcmp(filename,"OVER") == 0 ) 
        {
            break;
        }
        printf("%s\n",filename);
    }
    

}


int quit(int client_sockfd)
{
    printf("断开连接\n");
    close(client_sockfd);
}


int main(int argc,char* argv[])
{
    //初始化客户端的套接字
    int client_sockfd = init_client_socket(argv[1],atoi(argv[2]));
    if(client_sockfd == -1)
    {
        return 0;
    }
    //数据通信
    while(1)
    {
        //输入命令
        printf("请输入命令 :\n");
        char buf[128] = {0};
        fgets(buf,128,stdin);

        //解析命令 ----> 命令 + 参数
        char cmd[128] = {0};//命令
        char filename[128] = {0};//参数  (主要是文件名)
        sscanf(buf,"%s %s",cmd,filename);
        printf("cmd ---------->: %s\n",cmd);
        printf("filename ---------->: %s\n",filename);

        //发送命令的大小
        int cmd_size = strlen(cmd);
        send(client_sockfd,&cmd_size,sizeof(cmd_size),0);

        //发送命令
        int ret = send(client_sockfd,cmd,strlen(cmd),0);
        if(ret < 0)
        {
            perror("send cmd error");
            break;
        }

        //根据命令,执行相应的操作
        if(strncmp(cmd,"ls",2) == 0) //ls
        {
            //接收ls执行的结果,接收文件名
            recv_ls(client_sockfd);

        }
        else if(strncmp(cmd,"get",3) == 0)//get
        {
            //接收文件
           int r = recv_file(client_sockfd,filename);
            if(r == -1)
            {
                continue;
            }
        }
        else if(strncmp(cmd,"put",3) == 0) //put
        {
              //上传文件
            put_file(client_sockfd,filename);
        }
        else if(strncmp(cmd,"remove",6) == 0 ) //remove
        {
            //删除文件
            rm_file(client_sockfd,filename);
        }
        else if(strncmp(cmd,"quit",4) == 0) //quit
        {
            //直接程序退出
            quit(client_sockfd);
            break;
        }
        else if(strncmp(cmd,"cd",2) == 0)
        {
            //打开一个目录
            cd_dir(client_sockfd,filename);
        }



    }

    close(client_sockfd);
    return 0;
}