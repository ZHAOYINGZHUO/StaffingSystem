#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <string.h>    
#include <unistd.h>    
#include <stdlib.h>    
#include <pthread.h>   
#include <sqlite3.h>   
#include <time.h>      
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
struct newdb{
	sqlite3 *db;
	int newfd;
};

void* do_recv_cli(void * arg);
int do_regsiter(struct agreement reg,struct newdb newfd_db);
int do_login(struct agreement reg,struct newdb newfd_db);
int do_delete(struct agreement reg,struct newdb newfd_db);
int do_delete_info(struct agreement reg,struct newdb newfd_db);
int do_add(struct agreement reg,struct newdb newfd_db);
int do_change(struct agreement reg,struct newdb newfd_db);
int main(int argc,const char *argv[])
{
	if(argc < 3)
	{
		printf("请输入IP和端口号\n");
		return -1;
	}
	//创建流式套接字
	int sfd = socket(AF_INET,SOCK_STREAM,0);
	if(sfd < 0){
		ERR_MSG("socket");
		return -1;
	}
	//允许端口快速重用
	int reuse = 1;
	if(setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse))<0)
	{
		ERR_MSG("setsockopt");
		return -1;
	}
	//填充需要绑定的ip和port
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(atoi(argv[2]));
	sin.sin_addr.s_addr = inet_addr(argv[1]);
	if(bind(sfd,(struct sockaddr*)&sin,sizeof(sin))<0)
	{
		ERR_MSG("bind");
		return -1;
	}
	//将套接字设置为被动监听状态；
	if(listen(sfd,10)<0)
	{
		ERR_MSG("listen");
		return -1;
	}
	//接收客户端的连接，并返回专门用于与客户端进行交互的文件描述符
	struct sockaddr_in cin;
	socklen_t addrlen = sizeof(cin);
	struct newdb newfd_db;
	//创建打开数据库
	if(sqlite3_open("./dict.db",&newfd_db.db)!=0)
	{
		printf("__%d__ %s\n",__LINE__,sqlite3_errmsg(newfd_db.db));
		return -1;
	}
	//创建存储员工id和密码的表
	char sql[128] = "create table if not exists user (id char primary key,password char,flag int)";
	char* errmsg = NULL;
	if(sqlite3_exec(newfd_db.db,sql,NULL,NULL,&errmsg)!=0)
	{
		printf("__%d__ errmsg: %s\n",__LINE__,errmsg);
		return -1;
	}
	//创建储存已经登录用户的表
	bzero(sql,sizeof(sql));
	sprintf(sql,"create table if not exists on_line (id char primary key,password char)");
	if(sqlite3_exec(newfd_db.db,sql,NULL,NULL,&errmsg)!=0)
	{
		printf("__%d__ errmsg: %s\n",__LINE__,errmsg);
		return -1;
	}
	//创建员工信息的表
	bzero(sql,sizeof(sql));
	sprintf(sql,"create table if not exists info(flag int,name char,sex char,old char,id char,phone char,salray char,addr char,wages char)");
	if(sqlite3_exec(newfd_db.db,sql,NULL,NULL,&errmsg)!=0)
	{
		printf("__%d__ errmsg: %s\n",__LINE__,errmsg);
		return -1;
	}

	pthread_t tid;
	while(1)
	{
		newfd_db.newfd=accept(sfd,(struct sockaddr*)&cin,&addrlen);
		if(newfd_db.newfd < 0)
		{
			ERR_MSG("accept");
			return -1;
		}
		//每连接一个客户端，就打开一个分支线程
		if(pthread_create(&tid,NULL,do_recv_cli,&newfd_db)!=0)
		{
			ERR_MSG("pthread_create");
			break;
		}
	}
	//关闭文件描述符
	close(sfd);
	close(newfd_db.newfd);
	return 0;
}
//接收客户端发送的协议
void* do_recv_cli(void * arg)
{
	struct newdb newfd_db = *(struct newdb*)arg;
	struct agreement reg;
	ssize_t res;
	while(1)
	{
		//接收客户端发送的协议
		res = recv(newfd_db.newfd,&reg,sizeof(reg),0);
		if(res < 0)
		{
			ERR_MSG("recv");
			exit(1);
		}
		else if(res == 0)
		{
			//用户退出登录，将退出的用户从on_line表中删除
			do_delete(reg,newfd_db);
			break;
		}
		//提取出协议类型
		switch(reg.type)
		{
		case 'R':
			//注册
			do_regsiter(reg,newfd_db);
			break;
		case 'L':
			//登录
			do_login(reg,newfd_db);
			break;
		default:
			break;
		}

	}
	return NULL;
}
//注册
int do_regsiter(struct agreement reg,struct newdb newfd_db)
{
	//读取协议中的用户名、id和密码
	char sql[128] = "";
	char buf[128] = "";
	char** result = NULL;
	int row,column;
	char* errmsg = NULL;
	//去用户密码表中查找，判断该用户id是否被注册****
#if 1
	sprintf(sql,"select * from user where id=\"%s\"",reg.id);
	if(sqlite3_get_table(newfd_db.db,sql,&result,&row,&column,&errmsg)!=0)
	{
		printf("__%d__ errmsg:%s\n",__LINE__,errmsg);
		return -1;
	}

	if(strcmp(reg.id,result[3])==0)//****
	{
		//该用户id被注册
		strcpy(buf,"已被注册");
		if(send(newfd_db.newfd,buf,sizeof(buf),0)<0)
		{
			ERR_MSG("send");
			return -1;
		}
	}
#endif
	else
	{
#if 1
		//将注册成功的用户写进用户密码表中
		bzero(sql,sizeof(sql));
		strcpy(buf,"注册成功");
		sprintf(sql,"insert into user values(\"%s\",\"%s\",0)",reg.id,reg.password);
		if(sqlite3_exec(newfd_db.db,sql,NULL,NULL,&errmsg)!=0)
		{
			printf("sqlite3_exec:%s __%d__\n",errmsg,__LINE__);
			return -1;
		}

#endif

		//写入信息表
		sprintf(sql,"insert into info(flag,name,id) values(0,\"%s\",\"%s\")",reg.name,reg.id);
		if(sqlite3_exec(newfd_db.db,sql,NULL,NULL,&errmsg)!=0)
		{
			printf("sqlite3_exec:%s __%d__\n",errmsg,__LINE__);
			return -1;
		}

 
		if(send(newfd_db.newfd,buf,sizeof(buf),0)<0)
		{
			ERR_MSG("send");
			return -1;
		}

	}

#if 1
	//释放空间
	sqlite3_free_table(result);
	return 0;
#endif
}

//查员工信息
int do_search_info(struct agreement reg,struct newdb newfd_db)
{
	//读取客户端发来的用户id
	//根据id在员工信息表中查找
	char sql[128] = "";
	char buf[128] = "";
	char** result = NULL;
	int row,column;
	char* errmsg = NULL;
	int i = 0;
	sprintf(sql,"select * from info where id=\"%s\"",reg.id);
	if(sqlite3_get_table(newfd_db.db,sql,&result,&row,&column,&errmsg)!=0)
	{
		printf("__%d__ errmsg:%s\n",__LINE__,errmsg);
		return -1;
	}
	if(reg.flag==1)
	{
		for(i=0;i<(row+1)*column;i+=9)
		{
			sprintf(buf,"%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t",result[i+1],result[i+2],result[i+3],result[i+4],result[i+5],result[i+6],result[i+7],result[i+8]);
			if(send(newfd_db.newfd,buf,sizeof(buf),0)<0)
			{
				ERR_MSG("send");
				return -1;
			}
		}
	}else if(reg.flag==0 )
	{
		for(i=0;i<(row+1)*column;i+=9)
		{
			sprintf(buf,"%s\t%s\t%s\t%s\t%s\t%s\t",result[i+1],result[i+2],result[i+3],result[i+4],result[i+5],result[i+6]);
			if(send(newfd_db.newfd,buf,sizeof(buf),0)<0)
			{
				ERR_MSG("send");
				return -1;
			}
		}

	}
/*
		sprintf(buf,"%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t",result[1],result[2],result[3],result[4],result[5],result[6],result[7],result[8]);
		if(send(newfd_db.newfd,buf,sizeof(buf),0)<0)
		{
			ERR_MSG("send");
			return -1;
		}
*/
	strcpy(buf,"发送完毕");
	if(send(newfd_db.newfd,buf,sizeof(buf),0)<0)
	{
		ERR_MSG("send");
		return -1;
	}
	return 0;
}



//登录
int do_login(struct agreement reg,struct newdb newfd_db)
{
	//读取协议中的用户id和密码
	char sql[128] = "";
	char buf[128] = "";
	
	char** result = NULL;
	char** newresult = NULL;
	char* errmsg = NULL;
	int row,column;
	int res;
	//去用户名密码表中查找，判断用户id是否注册以及用户密码是否正确
	sprintf(sql,"select * from user where id=\"%s\" and password=\"%s\"",reg.id,reg.password);
	if(sqlite3_get_table(newfd_db.db,sql,&result,&row,&column,&errmsg)!=0)
	{
		printf("__%d__ errmsg:%s\n",__LINE__,errmsg);
		return -1;
	}
	if((strcmp(reg.id,result[3])==0)&&(strcmp(reg.password,result[4])==0))//*****
	{
		//去已经登录的用户表中查找，判断用户是否在别处登录
		bzero(sql,sizeof(sql));
		sprintf(sql,"select * from on_line where id=\"%s\"",reg.id);
		if(sqlite3_get_table(newfd_db.db,sql,&newresult,&row,&column,&errmsg)!=0)
		{
			printf("__%d__ errmsg:%s\n",__LINE__,errmsg);
			return -1;
		}
		if(strcmp(reg.id,newresult[2]))
		{
			//登录成功
			//将结果发送给客户端
			if(strcmp("1",result[5])==0){
				buf[0]='y';
			}else{
				buf[0]='n';
			}
			strcpy(buf+1,"登录成功");
			if(send(newfd_db.newfd,buf,sizeof(buf),0)<0)
			{
				ERR_MSG("send");
				return -1;
			}
			//将登录成功的用户名写进on_line表
			bzero(sql,sizeof(sql));
			sprintf(sql,"insert into on_line values(\"%s\",\"%s\")",reg.id,reg.password);
			if(sqlite3_exec(newfd_db.db,sql,NULL,NULL,&errmsg)!=0)
			{
				printf("sqlite3_exec:%s  __%d__\n",errmsg,__LINE__);
				return -1;
			}
			while(1)
			{
				res = recv(newfd_db.newfd,&reg,sizeof(reg),0);
				if(res < 0)
				{
					ERR_MSG("recv");
					return -1;
				}
				else if(res == 0)
				{
					//用户退出登录，将退出的用户从on_line表中删除
					do_delete(reg,newfd_db);
					break;
				}
				//提取出协议中的协议类型
				switch(reg.type)
				{
				case 'Q':
					//查信息
					do_search_info(reg,newfd_db);
					break;
				case 'D':
					//删除用户
					do_delete_info(reg,newfd_db);
					break;
				case 'I':
					//增加用户
					do_add(reg,newfd_db);	
					break;
				case 'C':
					//修改信息
					do_change(reg,newfd_db);
					break;

				default:
					do_delete(reg,newfd_db);
					return -1;
				}
			}
		}
		else
		{
			//登录失败，用户已在别处登录
			strcpy(buf,"重复登录");
			if(send(newfd_db.newfd,buf,sizeof(buf),0)<0)
			{
				ERR_MSG("send");
				return -1;
			}
		}
	}
	else
	{
		//登录失败
		//并将结果发送给客户端
		strcpy(buf,"登录失败，用户密码错误");
		if(send(newfd_db.newfd,buf,sizeof(buf),0)<0)
		{
			ERR_MSG("send");
			return -1;
		}
	}
	//释放查询结果的空间
	sqlite3_free_table(result);
	return 0;
}
//用户退出登录时将该用户从on_line表中删除
int do_delete(struct agreement reg,struct newdb newfd_db)
{
	char* errmsg = NULL;
	char sql[128] = "";
	bzero(sql,sizeof(sql));
	sprintf(sql,"delete from on_line where id=\"%s\"",reg.id);
	if(sqlite3_exec(newfd_db.db,sql,NULL,NULL,&errmsg)!=0)
	{
		printf("sqlite3_exec:%s\n",errmsg);
		return -1;
	}
	return 0;
}
//从info表中删除用户
int do_delete_info(struct agreement reg,struct newdb newfd_db)
{
	//读取客户端发来的用户id
	//根据用户名去信息表中查找

//	char* errmsg = NULL;
	char sql[128] = "";
	char buf[128] = "";
	char** result = NULL;
	int row,column;
	char* errmsg = NULL;
	int i = 0;

//	bzero(sql,sizeof(sql));
	sprintf(sql,"delete from info where id=\"%s\"",reg.id);
	if(sqlite3_exec(newfd_db.db,sql,NULL,NULL,&errmsg)!=0)
	{
		printf("sqlite3_exec:%s\n",errmsg);
		return -1;
	}
	sprintf(sql,"delete from user where id=\"%s\"",reg.id);
	if(sqlite3_exec(newfd_db.db,sql,NULL,NULL,&errmsg)!=0)
	{
		printf("sqlite3_exec:%s\n",errmsg);
		return -1;
	}

	strcpy(buf,"删除完毕");
	if(send(newfd_db.newfd,buf,sizeof(buf),0)<0)
	{
		ERR_MSG("send");
		return -1;
	}

	return 0;
}
//向info表中增加用户
int do_add(struct agreement reg,struct newdb newfd_db)
{
	
	char* errmsg = NULL;
	char sql[128] = "";
	char buf[128] = "";
	char** result = NULL;
	int row,column;
	sprintf(sql,"insert into user values(\"%s\",\"%s\",0)",reg.id,reg.password);
	if(sqlite3_exec(newfd_db.db,sql,NULL,NULL,&errmsg)!=0)
	{
		printf("sqlite3_exec:%s __%d__\n",errmsg,__LINE__);
		return -1;
	}
#if 1
	sprintf(sql,"insert into info(flag,name,id) values(0,\"%s\",\"%s\")",reg.name,reg.id);
	if(sqlite3_exec(newfd_db.db,sql,NULL,NULL,&errmsg)!=0)
	{
		printf("sqlite3_exec:%s __%d__\n",errmsg,__LINE__);
		return -1;
	}
#endif
	if(send(newfd_db.newfd,buf,sizeof(buf),0)<0)
	{
		ERR_MSG("send");
		return -1;
	}
}
#if 1
//修改info表的信息
int do_change(struct agreement reg,struct newdb newfd_db)
{
	
	char* errmsg = NULL;
	char sql[128] = "";
	char buf[128] = "";
	char** result = NULL;
	int row,column;
	int n=reg.flag;
//	sprintf(sql, "update stu set score = %d where id = %d", score, id);
	if(n==2)
	{
		sprintf(sql,"update user set password = \"%s\" where id = \"%s\"",reg.password,reg.id);
		if(sqlite3_exec(newfd_db.db,sql,NULL,NULL,&errmsg)!=0)
		{
			printf("sqlite3_exec:%s __%d__\n",errmsg,__LINE__);
			return -1;
		}
	}else if(n==1)
	{
		sprintf(sql,"update info set name = \"%s\" where id = \"%s\"",reg.name,reg.id);
		if(sqlite3_exec(newfd_db.db,sql,NULL,NULL,&errmsg)!=0)
		{
			printf("sqlite3_exec:%s __%d__\n",errmsg,__LINE__);
			return -1;
		}
	}
	strcpy(buf,"修改完毕");
	if(send(newfd_db.newfd,buf,sizeof(buf),0)<0)
	{
		ERR_MSG("send");
		return -1;
	}
}

#endif
