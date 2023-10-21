#include<my_head.h>
#include  <netinet/ip.h>
#include <sys/select.h>
#include <sqlite3.h>
typedef struct mseeage{
    char option;
    char name[32];
    char text[256];
}msg_t;
void print2(void){
    printf("-------------------------------------------------\n");
    printf("1.存钱  2.取钱  3.查看存款  4.查看存取记录  5.退出\n");
    printf("-------------------------------------------------\n");
    printf("请输入您的选择\n");
}
void print1(void){
    printf("------------------------\n");
    printf("1.注册   2.登录   3.退出\n");
    printf("------------------------\n");
    printf("请输入您的选择\n");
}
msg_t msg;
int do_register(int sockfd,msg_t *msg,sqlite3 *db);//声明注册函数
int do_login(int sockfd,msg_t *msg,sqlite3 *db);//声明登录函数
int do_quit(int sockfd,msg_t *msg,sqlite3 *db);//声明退出函数
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
    int ret=0;
    if(-1==(ret=(connect(sockfd,(struct sockaddr *)&serveraddr,serveraddr_len)))){
        ERR_LOG("connect error");
    }
    printf("连接服务器成功\n");
    while(1){
        NEXT2:
        print1();
        scanf("%c",&msg.option);
        getchar();
        if(msg.option>'3' || msg.option<'1'){
            printf("请输入正确的选择\n");
            goto NEXT2;
        }
        switch(msg.option){
            case '1'://注册分支
            do_register(sockfd,&msg,my_db);
            break;
            case '2'://登录分支
            do_login(sockfd,&msg,my_db); 
            break;
            case '3'://退出分支
            do_quit(sockfd,&msg,my_db);
            break;
        }
    }
    return 0;
}
int do_quit(int sockfd,msg_t *msg,sqlite3 *db){
    send(sockfd,msg,sizeof(msg_t),0);
    exit(1);
}
int do_register(int sockfd,msg_t *msg,sqlite3 *db){
    printf("我是注册\n");
    int ret=0;
    printf("请输入注册姓名\n");
    scanf("%s",msg->name);
    getchar();
    printf("请输入注册密码\n");
    scanf("%s",msg->text);
    getchar();
    send(sockfd,msg,sizeof(msg_t),0);
    recv(sockfd,msg,sizeof(msg_t),0);
    printf("%s\n",msg->text);
    return 0;
}
int do_login(int sockfd,msg_t *msg,sqlite3 *db){//登录函数
    printf("我是登录\n");
    char sqlbuff[300]={0};
    char buff[100]={0};
    int ret=99;
    NEXT:
    printf("请输入登录用户名称\n");
    scanf("%s",msg->name);
    getchar();
    printf("请输入登录密码\n");
    scanf("%s",msg->text);
    getchar();
    send(sockfd,msg,sizeof(msg_t),0);
    recv(sockfd,msg,sizeof(msg_t),0);
    printf("%s\n",msg->text);
    sprintf(buff,"用户[%s]登录成功",msg->name);
    if(strcmp(buff,msg->text)){
        goto NEXT;
    }
    while(1){
    NEXT1:
    print2();
    scanf("%c",&msg->option);
    getchar();
    if(msg->option>'5' || msg->option<'1'){
        printf("请输入正确的选择\n");
        goto NEXT1;
    }
    switch(msg->option){
        case '1'://存
        printf("请输入要存入的数目\n");
        scanf("%s",msg->text);
        getchar();
        send(sockfd,msg,sizeof(msg_t),0);
        break;
        case '2'://取
        printf("请输入要取出的数目\n");
        scanf("%s",msg->text);
        getchar();
        send(sockfd,msg,sizeof(msg_t),0);
        break;
        case '3'://查询当前存款
        send(sockfd,msg,sizeof(msg_t),0);
        break;
        case '4'://查询记录
        send(sockfd,msg,sizeof(msg_t),0);
        while(1){
        recv(sockfd,msg,sizeof(msg_t),0);
        if(!strcmp(msg->text,"over")){
            goto NEXT1;
        }
        printf("%s\n",msg->text);
        }    
        break;
        case '5':
        goto NEXT2;
        break;
    }
    recv(sockfd,msg,sizeof(msg_t),0);
    printf("%s\n",msg->text);
    }
    NEXT2:
    return 0;
}
