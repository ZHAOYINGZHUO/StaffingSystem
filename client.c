#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>    
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#define ERR_MSG(msg) do{\
	printf("__%d__ __%s__\n",__LINE__,__func__);\
	perror(msg);\
}while(0)

#define N 128
struct agreement{
	char type;
	char name[20];
	char password[20];
	char word[20];
	char id[20];
	int flag;
};

char id[20] = "";
int sockfd = 0;
int do_regsiter(int sfd);//注册
int do_login(int sfd);//登录

int do_search_info(int sfd);//查信息
int do_delete_info(int sfd);//删用户
int do_add(int sfd);//增加用户
int do_change(int sfd);//修改信息
int do_quit(int sfd,struct agreement reg);//返回上一级
int do_quitt(int sfd);//退出
typedef void(*sighandler_t)(int);
void handler(int sig)
{
	do_quitt(sockfd);
	close(sockfd);
	exit(0);
}


int main(int argc,const char *argv[])
{
	sighandler_t s = signal(2, handler);
	if(argc < 3)
	{
		printf("请输入IP和端口号\n");
		return -1;
	}
	//给 ctrl+c 注册信号处理函数
	if(s == SIG_ERR)
	{
		ERR_MSG("signal");
		return -1;
	}

	//创建套接字
	int sfd = socket(AF_INET,SOCK_STREAM,0);
	if(sfd < 0){
		ERR_MSG("socket");
		return -1;
	}
	printf("创建套接字成功\n");
	//绑定客户端的IP和PORT(非必须绑定)
	//连接服务器
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	//	sin.sin_port = htons(PORT);
	sin.sin_port = htons(atoi(argv[2]));
	//	sin.sin_addr.s_addr = inet_addr(IP);
	sin.sin_addr.s_addr = inet_addr(argv[1]);
	if(connect(sfd,(struct sockaddr*)&sin,sizeof(sin))<0)
	{
		ERR_MSG("connect");
		return -1;
	}
	char choose = 0;
	sockfd = sfd;
	while(1)
	{
		printf("+-------------------+\n");
		printf("+       1.注册      +\n");
		printf("+       2.登录      +\n");
		printf("+       3.清屏      +\n");
		printf("+       4.退出      +\n");
		printf("+-------------------+\n");
		printf("请输入>>>");
		choose = getchar();
		while(getchar()!=10);
		switch(choose)
		{
		case '1':
			//注册
			do_regsiter(sfd);
			break;
		case '2':
			//登录
			do_login(sfd);
			break;
		case '3':
			//清屏
			system("clear");
			break;
		case '4':
			//退出
			goto END;
			break;
		default:
			printf("输入错误，再输入一次\n");
			break;
		}
	}
END:
	close(sfd);
	return 0;
}

//注册
int do_regsiter(int sfd)
{
	struct agreement reg;
	char buf[128] = "";
	//输入用户名和密码
//	reg.flag = 0;
	printf("请输入用户id>>>");
	scanf("%s",reg.id);
	while(getchar()!=10);
	printf("请输入用户名>>>");
	scanf("%s",reg.name);
	while(getchar()!=10);

	printf("请输入密码>>>");
	scanf("%s",reg.password);
	while(getchar()!=10);

	//填充注册协议
	reg.type = 'R';
	//将协议发送给服务器
	if(send(sfd,&reg,sizeof(reg),0)<0)
	{
		ERR_MSG("send");
		return -1;
	}
	//等待服务器回应
	bzero(buf,sizeof(buf));
	ssize_t res = recv(sfd,buf,sizeof(buf),0);
	if(res < 0)
	{
		ERR_MSG("recv");
		return -1;
	}
	else if(res == 0)
	{
		printf("服务器关闭\n");
		return -1;
	}
	//打印注册结果
	printf("-------%s------\n",buf);
	return 0;
}

//返回上一级
int do_quit(int sfd,struct agreement reg)
{
	reg.type = 'E';
	if(send(sfd,&reg,sizeof(reg),0)<0)
	{
		ERR_MSG("send");
		return -1;
	}
	return 0;
}
//查员工信息
int do_search_info(int sfd)
{
	struct agreement reg;
	char buf[128] = "";
	int getgid;
	//判断是否为管理员
	printf("输入你的id>>");
	scanf("%d",&getgid);
	if(getgid==511)
	{
		reg.flag=1;
	}else
	{
		reg.flag=0;
	}
	//输入想要查询的用户id
	printf("请输入想要查询的用户id>>>");
	scanf("%s",reg.id);
	while(getchar()!=10);
	//填充查信息的协议
	reg.type = 'Q';
	//将协议发送给服务器
	if(send(sfd,&reg,sizeof(reg),0)<0)
	{
		ERR_MSG("send");
		return -1;
	}
	//等待服务器回应
	while(1)
	{
		ssize_t res = recv(sfd,buf,sizeof(buf),0);
		if(res<0)
		{
			ERR_MSG("recv");
			return -1;
		}
		//打印查询结果
		if(strcmp(buf,"发送完毕")==0)
		{
			break;
		}
		printf("%s\t\n",buf);
	}
	return 0;
}

//登录
int do_login(int sfd)
{
	struct agreement reg;
	char choose = 0;
	char buf[128] = "";
	//输入用户名和密码
	printf("请输入登id>>>");
	scanf("%s",reg.id);
	while(getchar()!=10);

	printf("请输入密码>>>");
	scanf("%s",reg.password);
	while(getchar()!=10);

	//填充登录协议
	reg.type = 'L';
	//将协议发送给服务器
	if(send(sfd,&reg,sizeof(reg),0)<0)
	{
		ERR_MSG("send");
		return -1;
	}
	//等待服务器回应
	bzero(buf,sizeof(buf));
	ssize_t res = recv(sfd,buf,sizeof(buf),0);
	if(res<0)
	{
		ERR_MSG("recv");
		return -1;
	}
	else if(res ==0)
	{
		printf("服务器关闭");
		return -1;
	}
	char flag=buf[0];//y/n
//	printf("%c\n",flag);
	//打印登录结果
//	printf("-----登陆成功----\n");
	printf("------%s-----\n",buf);
	if(strcmp(buf+1,"登录成功")==0)
	{
		if(flag=='y')
		{
			while(1)
			{
				printf("+-------------------+\n");
				printf("+      1.查信息     +\n");
				printf("+      2.增加信息   +\n");
				printf("+      3.删除信息   +\n");
				printf("+      4.修改信息   +\n");
				printf("+      5.清屏       +\n");
				printf("+     6.返回上一级  +\n");
				printf("+-------------------+\n");
				printf("请输入>>>");
				choose = getchar();
				while(getchar()!=10);
				switch(choose)
				{
				case '1':
					//查信息
					do_search_info(sfd);
					break;
				case '2':
					//增加用户
					do_add(sfd);
					break;
				case '3':
					//删除用户
					do_delete_info(sfd);
					break;
				case '4':
					//修改信息
					do_change(sfd);
					break;

				case '5':
					//清屏
					system("clear");
					break;
				case '6':
					//返回上一级
					do_quit(sfd,reg);
					return -1;
				default:
					printf("输入错误，请重新输入\n");
					break;
				}
			}
		}
		else if(flag=='n')
		{
			while(1)
			{
				printf("+-------------------+\n");
				printf("+      1.查信息     +\n");
				printf("+      4.修改信息   +\n");
				printf("+      5.清屏       +\n");
				printf("+     6.返回上一级  +\n");
				printf("+-------------------+\n");
				printf("请输入>>>");
				choose = getchar();
				while(getchar()!=10);
				switch(choose)
				{
				case '1':
					//查信息
					do_search_info(sfd);
					break;
				case '4':
					//修改信息
					do_change(sfd);
					break;

				case '5':
					//清屏
					system("clear");
					break;
				case '6':
					//返回上一级
					do_quit(sfd,reg);
					return -1;
				default:
					printf("输入错误，请重新输入\n");
					break;
				}
			}
		}
	}
	return 0;
}

//退出
int do_quitt(int sfd)
{
	struct agreement send_msg = {htonl('4')};
	strcpy(send_msg.id, id);
	//发送退出协议
	if(send(sfd, &send_msg, sizeof(send_msg), 0)<0)
	{
		ERR_MSG("send");
		return -1;
	}
	return 0;
}
//删除用户
int do_delete_info(int sfd)
{
	struct agreement reg;
	char buf[128] = "";
	//输入想要删除的用户id
	printf("请输入想要删除的用户id>>>");
	scanf("%s",reg.id);
	while(getchar()!=10);
	//填充删除的协议
	reg.type = 'D';
	//将协议发送给服务器
	if(send(sfd,&reg,sizeof(reg),0)<0)
	{
		ERR_MSG("send");
		return -1;
	}
	//等待服务器回应
	while(1)
	{
		ssize_t res = recv(sfd,buf,sizeof(buf),0);
		if(res<0)
		{
			ERR_MSG("recv");
			return -1;
		}
		//打印删除结果
		if(strcmp(buf,"删除完毕")==0)
		{
			break;
		}
		printf("删除完毕");
	}
	return 0;
}
//增加用户
int do_add(int sfd)
{
	struct agreement reg;
	char buf[128] = "";
	//输入用户名和密码(信息)
	printf("请输入用户id>>>");
	scanf("%s",reg.id);
	while(getchar()!=10);
	printf("请输入用户名>>>");
	scanf("%s",reg.name);
	while(getchar()!=10);

	printf("请输入密码>>>");//0000
	scanf("%s",reg.password);
	while(getchar()!=10);

	//填充增加协议
	reg.type = 'I';
	//将协议发送给服务器
	if(send(sfd,&reg,sizeof(reg),0)<0)
	{
		ERR_MSG("send");
		return -1;
	}
	//等待服务器回应
	bzero(buf,sizeof(buf));
	ssize_t res = recv(sfd,buf,sizeof(buf),0);
	if(res < 0)
	{
		ERR_MSG("recv");
		return -1;
	}
	else if(res == 0)
	{
		printf("服务器关闭\n");
		return -1;
	}
	return 0;
}

#if 1
//修改用户信息
int do_change(int sfd)
{
	struct agreement reg;
	char buf[128] = "";
	int num=0;
	int getgid;
	//判断是否为管理员
	printf("请输入你的id>>");
	scanf("%d",&getgid);
	//输入想要修改信息的用户id
	//如果是管理员可以修改所有人的信息
	//如果不是管理员只能修改自己的信息
	if(getgid==511)
	{
		printf("请输入想要修改的用户id>>>");
	}else
	{
		printf("请再次确认你的id>>");
	}
	scanf("%s",reg.id);
	while(getchar()!=10);
	printf("+-------------------+\n");
	printf("+      1.修改名字   +\n");
	printf("+      2.修改密码   +\n");
	printf("+-------------------+\n");

	printf("请输入想要修改的信息：>>>");
	scanf("%d",&(reg.flag));
	while(getchar()!=10);
	num=reg.flag;
	printf("请输入修改后的信息：>>>");
	if(num==1)
	{
	scanf("%s",reg.name);
	while(getchar()!=10);
	}else if(num==2)
	{
	scanf("%s",reg.password);
	while(getchar()!=10);
	}

	//填充修改的协议
	reg.type = 'C';
	//将协议发送给服务器
	if(send(sfd,&reg,sizeof(reg),0)<0)
	{
		ERR_MSG("send");
		return -1;
	}
	//等待服务器回应
	while(1)
	{
		ssize_t res = recv(sfd,buf,sizeof(buf),0);
		if(res<0)
		{
			ERR_MSG("recv");
			return -1;
		}
		//打印修改结果
		if(strcmp(buf,"修改完毕")==0)
		{
			break;
		}
		printf("修改完毕");
	}
	return 0;
}
#endif
