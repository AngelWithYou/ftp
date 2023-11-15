
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>





//选择一个目录作为ftp服务器的目录
#define FTP_PATH "/home/china/ftp"

int client_sockfd;//客户端的套接字

char arr[512]; 



/*
    init_server_socket : 初始化服务端的一个套接字
    @ipstr: ip的点分式字符串
    @port : 端口号
    返回值:
        返回 准备好的套接字
*/
int init_server_socket(char* ipstr,short port)
{
    //1.创建一个套接字
    int server_sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(server_sockfd == -1)
    {
        perror("socket error");
        return -1;
    }
   

    //2.指定服务器(本机)的ip地址 : ip + 端口号
    struct sockaddr_in serv;
    serv.sin_family = AF_INET;//指定协议族
    serv.sin_port = htons(port);//htons(6666);//指定端口号
    serv.sin_addr.s_addr = inet_addr(ipstr);//inet_addr("172.11.0.1");//指定IP

    //设置端口复用
    int n = 1;
    setsockopt(server_sockfd,SOL_SOCKET,SO_REUSEPORT,&n,sizeof(n));

    //绑定
    int ret;
    ret = bind(server_sockfd,(struct sockaddr*)&serv,sizeof(serv));
    if(ret == -1)
    {
        perror("bind error");
        close(server_sockfd);
        return -1;
    }

    //3.进入监听模式
    ret = listen(server_sockfd,10);
    if(ret == -1)
    {
        perror("listen error");
        close(server_sockfd);
        return -1;
    }
    printf("listen success!!!");

    //4.接收 accept
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    client_sockfd = accept(server_sockfd,(struct sockaddr*)&client_addr,&len);
    if(client_sockfd == -1)
    {
        perror("accept error");
        close(server_sockfd);
        return -1;
    }
    printf("accept success!!!");

    printf("client ip : %s\n",inet_ntoa(client_addr.sin_addr));//打印客户端的ip

    //sockfd 一个已经准备好的监听套接字
    return client_sockfd;
}

/*
    send_ls : 执行ls命令,获取当前路径下的  所有的文件名,
            全部发送给客户端
    client_sockfd : 连接套接字
        返回值 ：
            失败 返回 -1
*/
int send_ls(int client_sockfd)
{
    //打开目录
    DIR* dir = opendir(arr);
    if(dir == NULL)
    {
        perror("opendir ftp error");
        return -1;
    }

    //不断的读取目录项
    struct dirent* d = NULL;
    while(d = readdir(dir))
    {
        //过滤 . 和 ..
        if(strcmp(d->d_name,".") == 0 ||strcmp(d->d_name,"..") == 0 )
        {
            continue;
        }

        //发送文件名的长度
        int size = strlen(d->d_name);
        send(client_sockfd,&size,sizeof(size),0); 

        //发送文件名
        send(client_sockfd,d->d_name,size,0); 
    }

    //设定一个退出条件
    char end[] = "OVER";
    int size = strlen(end);
    send(client_sockfd,&size,sizeof(size),0); 
    send(client_sockfd,end,size,0); 

    //关闭目录
    closedir(dir);
}

/*
    send_file : 处理get命令,发送文件
    @client_sockfd : 连接套接字
    返回值: 
        失败 返回 -1
*/
int send_file(int client_sockfd)
{
    //改变路径
    chdir(arr);

    //先接收文件名 ,再去发送文件内容

    //接收文件名的大小
    int name_size = 0;
    int ret = recv(client_sockfd,&name_size,sizeof(name_size),0);
    if(ret <= 0)
    {
        perror("recv name_size error");
        return -1;
    }
    printf("name_size = %d\n",name_size);

    //接收文件名
    char filename[128] = {0};
    ret = recv(client_sockfd,filename,name_size,0);
    if(ret <= 0)
    {
        perror("recv filename error");
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
    //如果读取的字节长度比文件自己的长度小，就一直读取
    while(readsize < file_size)
    {
        //先读取,在发送
        char buf[10] = {0};

        //每次读取十个字节
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


    //读取完毕
    if(readsize == file_size)
    {
        printf("recv over!\n");
    }

    //关闭文件
    close(fd);
}

int put_file(int client_sockfd)
{
    //改变路径
    chdir(arr);

    //先接收文件名 ,再去发送文件内容

    //接收文件名的大小
    int name_size = 0;
    int ret = recv(client_sockfd,&name_size,sizeof(name_size),0);
    if(ret <= 0)
    {
        perror("recv name_size error");
        return -1;
    }
    printf("name_size = %d\n",name_size);

     //接收文件名
    char filename[128] = {0};
    ret = recv(client_sockfd,filename,name_size,0);
    if(ret <= 0)
    {
        perror("recv filename error");
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
        printf("put over\n");
    }

    //关闭文件
    close(fd);

}


int rm_file(int client_sockfd)
{
    
    //改变路径
    chdir(arr);
    //接收文件名的大小
    int name_size = 0;
    int ret = recv(client_sockfd,&name_size,sizeof(name_size),0);
    if(ret <= 0)
    {
        perror("recv name_size error");
        return -1;
    }
    printf("name_size = %d\n",name_size);

     //接收文件名
    char filename[128] = {0};
    ret = recv(client_sockfd,filename,name_size,0);
    if(ret <= 0)
    {
        perror("recv filename error");
        return -1;
    }
    printf("filename = %s\n",filename);

    unlink(filename);
    
}

int quit()
{
    printf("断开成功\n");
}


int cd_dir(int client_sockfd)
{
    //接收文件名的大小
    int name_size = 0;
    int ret = recv(client_sockfd,&name_size,sizeof(name_size),0);
    if(ret <= 0)
    {
        perror("recv name_size error");
        return -1;
    }
    printf("name_size = %d\n",name_size);

    //接收文件名
    char filename[128] = {0};
    ret = recv(client_sockfd,filename,name_size,0);
    if(ret <= 0)
    {
        perror("recv filename error");
        return -1;
    }
    printf("filename = %s\n",filename);

    // char buf[512] = {0};
    // sprintf(buf,"%s/%s",FTP_PATH,filename);
    //改变路径
    //printf("%s\n",buf);
    chdir(arr);
    printf("%s\n",arr);
    //判断是否为目录
    struct stat st;
    ret = stat(filename,&st);
    if(ret == -1)
    {
        perror("stat error");
        return -1;
    }
    if(S_ISREG(st.st_mode))
    {
        printf("不是目录，cd 失败\n");
        return -1;
    }
    else if (S_ISDIR(st.st_mode))
    {
        
        //打开目录
        DIR* dir = opendir(filename);
        if(dir == NULL)
        {
            perror("opendir ftp error");
            return -1;
        }
        sprintf(arr,"%s/%s",arr,filename);

        //不断的读取目录项
        struct dirent* d = NULL;
        while(d = readdir(dir))
        {
            //过滤 . 和 ..
            if(strcmp(d->d_name,".") == 0 ||strcmp(d->d_name,"..") == 0 )
            {
                continue;
            }
             //发送文件名的长度
            int size = strlen(d->d_name);
            send(client_sockfd,&size,sizeof(size),0); 
                //发送文件名
            send(client_sockfd,d->d_name,size,0);
        }
            //设定一个退出条件
        char end[] = "OVER";
        int size = strlen(end);
        send(client_sockfd,&size,sizeof(size),0); 
        send(client_sockfd,end,size,0); 

        //关闭目录
        closedir(dir);

    }

    
    
    

}


int main(int argc,char* argv[])
{
    sprintf(arr,"%s",FTP_PATH);
    //初始化服务器的套接字
    int server_sockfd = init_server_socket(argv[1],atoi(argv[2]));

    //数据通信
    while(1)
    {
        //接收命令的大小
        int cmd_size = 0;
        recv(client_sockfd,&cmd_size,sizeof(cmd_size),0);

        //接收命令
        char cmd[128] = {0};
        int ret = recv(client_sockfd,cmd,cmd_size,0);
        if(ret < 0)
        {
            perror("recv cmd error");
            break;
        }
        printf("命令为 : %s\n",cmd);

        //解析 ,根据命令,执行对应的操作
        if(strncmp(cmd,"ls",2) == 0) //ls
        {
            //执行ls命令,把文件名发送过去
            send_ls(client_sockfd);
        }
        else if(strncmp(cmd,"get",3) == 0)//get
        {
            //发送文件
           int r = send_file(client_sockfd);
            if(r = -1)
            {
                continue;
            }
        }
        else if(strncmp(cmd,"put",3) == 0) //put
        {
            //上传文件
            put_file(client_sockfd);
        }
        else if(strncmp(cmd,"remove",6) == 0 ) //remove
        {
            //删除文件
            rm_file(client_sockfd);
        }
        else if(strncmp(cmd,"quit",4) == 0) //quit
        {
            //关闭程序
            quit(client_sockfd);
            break;
        }
        else if(strncmp(cmd,"cd",2) == 0)
        {
            //打开一个目录
           int r =  cd_dir(client_sockfd);
            if(r == -1)
            {
                continue;
            }
        }
        //...
    }

    //关闭套接字
    close(server_sockfd);
    
    return 0;

}