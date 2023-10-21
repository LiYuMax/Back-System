#include<my_head.h>
#include  <netinet/ip.h>
#include <sys/select.h>
#include <time.h>
#include <sqlite3.h>
typedef struct mseeage{
    char option;
    char name[32];
    char text[256];
}msg_t;
int do_register(int sockfd,msg_t *msg,sqlite3 *db);//声明登录函数
int do_login(int sockfd,msg_t *msg,sqlite3 *db);//声明登录函数
int do_quit(int sockfd,msg_t *msg,sqlite3 *db);//声明退出函数
int query_cmoney(int sockfd,msg_t *msg);
int do_record(int sockfd,msg_t *msg,sqlite3 *db);
msg_t msg;
int num;
int money;
int login_func(void *arg,int columns,char **f_value,char **f_name){
	num++;
	money=atoi(f_value[2]);
	return 0;
}
int record_func(void *arg,int columns,char **f_value,char **f_name){
	printf("我是记录函数\n");
	int sockfd=*(int *)arg;
	msg_t msg;
	//注意这里加*会报错
	//sprintf(msg.text,"[%s--%s]",*f_value[0],*f_value[1]);
	int num=atoi(f_value[2]);
	if(0<num)
	sprintf(msg.text,"您于[%s]存入[%s]",f_value[1],f_value[2]);
	else if(0>num)
	sprintf(msg.text,"您于[%s]取出[%s]",f_value[1],f_value[2]);
	printf("%s\n",msg.text);
	send(sockfd,&msg,sizeof(msg_t),0);
	return 0;
}
int main(int argc, char const *argv[])
{
    sqlite3 *my_db;
	if(sqlite3_open("shark.db",&my_db)!=SQLITE_OK){
        printf("error code:%d\n",sqlite3_errcode(my_db));
        printf("error:%s\n",sqlite3_errmsg(my_db));
        exit(1);
    }
	if(3 != argc){
		printf("Usage : %s <ip> <port>\n", argv[0]);
		exit(1);
	}
	int sockfd = 0;
	if(-1 == (sockfd = socket(AF_INET, SOCK_STREAM, 0)))
		ERR_LOG("socket error");
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(atoi(argv[2]));
	serveraddr.sin_addr.s_addr = inet_addr(argv[1]);
	socklen_t serveraddr_len = sizeof(serveraddr);
	int flag = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
	if(-1 == bind(sockfd, (struct sockaddr *)&serveraddr, serveraddr_len))
		ERR_LOG("bind error");
	if(-1 == listen(sockfd, 5))
		ERR_LOG("listen error");
    fd_set  readfds;
    memset(&readfds,0,sizeof(readfds));
    fd_set readfds_temp;
    memset(&readfds_temp,0,sizeof(readfds_temp));
    FD_SET(sockfd,&readfds);
	int max_fd=0;
	max_fd=max_fd > sockfd ? max_fd : sockfd ;
	int i=0;
	int ret=0;
	int acceptfd;
	while(1){
        readfds_temp=readfds;
		select(max_fd+1,&readfds_temp,NULL,NULL,NULL);
		for(i=3;i<max_fd+1;i++){
			if(FD_ISSET(i,&readfds_temp)){
				if(sockfd==i){
					acceptfd=accept(i,NULL,NULL);
					printf("用户连接成功\n");
					FD_SET(acceptfd,&readfds);
					max_fd=max_fd > acceptfd ? max_fd : acceptfd ;
					continue;
				}
				else {
				ret=recv(i,&msg,sizeof(msg),0);
				if(0==ret){
					printf("用户断开连接\n");
					FD_CLR(i,&readfds);
					close(i);
					continue;
				}
				printf("[%c][%s][%s]\n",msg.option,msg.name,msg.text);
				printf("test\n");
				printf("[%c][%s][%s]\n",msg.option,msg.name,msg.text);
					switch(msg.option){
						case '1':
						do_register(i,&msg,my_db);
						break;
						case '2':
						do_login(i,&msg,my_db);
						break;
						case '3':
						FD_CLR(i,&readfds);
						close(i);
						continue;
						break;
					}
				}
			}
		}
    }
    printf("test2\n");
	return 0;
}

int do_register(int sockfd,msg_t *msg,sqlite3 *db){
	int ret=0;
	char sqlbuff[350]={0};
	sprintf(sqlbuff,"insert into user values( '%s' , '%s' , '%d' )",msg->name,msg->text,0);
	if(SQLITE_OK!=sqlite3_exec(db,sqlbuff,NULL,NULL,NULL)){
		if(19==sqlite3_errcode(db)){
			sprintf(msg->text,"用户名[%s]已经被注册过了,请重新输入...",msg->name);
			send(sockfd,msg,sizeof(msg_t),0);
	}
	}
	sprintf(msg->text,"用户[%s]注册成功",msg->name);
	send(sockfd,msg,sizeof(msg_t),0);
	return 0;
}

int save(int sockfd,sqlite3 *db,int *money,msg_t *msg){
	int now_money=*money;
	int save_money=0;
	char save[300]={0};
	sprintf(save,"+%s",msg->text);
	save_money=atoi(msg->text);
	now_money=now_money+save_money;
	char sqlbuff[350]={0};
	sprintf(sqlbuff,"update user set money = '%d' where name = '%s'",now_money,msg->name);
	sqlite3_exec(db,sqlbuff,NULL,NULL,NULL);
	sprintf(msg->text,"成功存入[%d],现有余额[%d]",save_money,now_money);
	send(sockfd,msg,sizeof(msg_t),0);
	printf("用户[%s]存入成功\n",msg->name);
	*money=now_money;
	time_t now_time;
	time(&now_time);
	struct tm *now_t=localtime(&now_time);
	char time_buff[128]={0};
	sprintf(time_buff,"[%d/%d/%d--%d:%d:%d]",now_t->tm_year+1900,now_t->tm_mon+1,now_t->tm_mday,\
	now_t->tm_hour,now_t->tm_min,now_t->tm_sec);
	sprintf(sqlbuff,"insert into record values('%s','%s','%s' )",msg->name,time_buff,save);
	sqlite3_exec(db,sqlbuff,NULL,NULL,NULL);
	return 0;
}

int draw(int sockfd,sqlite3 *db,int *money,msg_t *msg){
	printf("我是取\n");
	int now_money=*money;
	char draw[300]={0};
	sprintf(draw,"-%s",msg->text);
	int draw_money=0;
	draw_money=atoi(msg->text);
	now_money=now_money-draw_money;
	char sqlbuff[350]={0};
	sprintf(sqlbuff,"update user set money = '%d' where name = '%s'",now_money,msg->name);
	sqlite3_exec(db,sqlbuff,NULL,NULL,NULL);
	sprintf(msg->text,"成功取出[%d],现有余额[%d]",draw_money,now_money);
	send(sockfd,msg,sizeof(msg_t),0);
	printf("用户[%s]取出成功\n",msg->name);
	*money=now_money;
	time_t now_time;
	time(&now_time);
	char time_buff[128]={0};
	struct tm *now_t=localtime(&now_time);
	sprintf(time_buff,"[%d/%d/%d--%d:%d:%d]",now_t->tm_year+1900,now_t->tm_mon+1,now_t->tm_mday,\
	    now_t->tm_hour,now_t->tm_min,now_t->tm_sec);
	sprintf(sqlbuff,"insert into record values('%s','%s','%s' )",msg->name,time_buff,draw);
	sqlite3_exec(db,sqlbuff,NULL,NULL,NULL);
	return 0;
}
int do_login(int sockfd,msg_t *msg,sqlite3 *db){
    char sqlbuff[350]={0};
	num=0;
	sprintf(sqlbuff,"select * from user where name = '%s'",msg->name);
	sqlite3_exec(db,sqlbuff,login_func,NULL,NULL);
    if(1==num){
	num=0;
    sprintf(sqlbuff,"select * from user where name = '%s' and password ='%s'",msg->name,msg->text);
    sqlite3_exec(db,sqlbuff,login_func,NULL,NULL);
    if(1==num){
        sprintf(msg->text,"用户[%s]登录成功",msg->name);
        send(sockfd,msg,sizeof(msg_t),0);
    }else if(0==num){
        sprintf(msg->text,"用户[%s]密码错误,请重新输入",msg->name);
        send(sockfd,msg,sizeof(msg_t),0);
		return -1;
    }
    }else if(0==num){
        sprintf(msg->text,"用户[%s]尚未注册，请先注册后再尝试登录",msg->name);
        send(sockfd,msg,sizeof(msg_t),0);
		return -1;
    }
	printf("test1\n");
	while(1){
	if(0==(recv(sockfd,msg,sizeof(msg_t),0)))
		return -1;
	switch(msg->option){
		case '1':
		save(sockfd,db,&money,msg);
		break;
		case '2':
		printf("[%c][%s][%s]\n",msg->option,msg->name,msg->text);
		draw(sockfd,db,&money,msg);
		break;
		case '3':
		query_cmoney(sockfd,msg);
		break;
		case '4':
		do_record(sockfd,msg,db);
		break;
	}
	}
	printf("test2\n");
	return 0;
}
int query_cmoney(int sockfd,msg_t *msg){
	sprintf(msg->text,"查询成功,您当前的存款为[%d]",money);
	send(sockfd,msg,sizeof(msg_t),0);
}
int do_record(int sockfd,msg_t *msg,sqlite3 *db){
	printf("我是记录\n");
	printf("%s\n",msg->name);
	char sqlbuff[350]={0};
	sprintf(sqlbuff,"select * from record where name = '%s'",msg->name);
	sqlite3_exec(db,sqlbuff,record_func,&sockfd,NULL);
	sprintf(msg->text,"%s","over");
	send(sockfd,msg,sizeof(msg_t),0);
	return 0;
} 