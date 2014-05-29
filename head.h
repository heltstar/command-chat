#include<stdio.h>
#include<sys/types.h>         
#include<sys/socket.h>
#include<string.h>
#include<strings.h>
#include<stdlib.h>
#include<netdb.h>
#include<errno.h>
#include<string.h>
#include<pthread.h>
#include<unistd.h>
#include<signal.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<fcntl.h>
#include<dirent.h>

#define MAXSIZE 1024
 
/*服务器接收消息后，创建的在线用户列表*/
struct user_info{
	char user_name[20];
	int  id;
	struct sockaddr_in cli_addr;
	struct user_info *next;
};
//分组信息
struct group_info
{
	char group_name[20];
	int  id;
	struct user_info *owner;
	struct group_info *next;  
};

/*客户端给服务器发送的消息*/
struct msg {
	char type;
	char self_name[20];
	char dst_name[20];	
	char data[MAXSIZE];
};
/*消息类型定义*/
enum msg_type{           
	LOG_IN = 1,     //登录
	LOGIN_TOALL,    //登陆提示消息
	REFRESH,         //要求刷新用户在线用户（重新打印在线用户）
	CHAT_PRI,        //私聊消息
	CHAT_ALL,        //群聊消息
	DOWNLOAD,        //从服务器下载文件（下载之前先打印文件列表）
	UPLOAD,         //上传文件到服务器
	OFFLINE,         //下线通知
	OVER,            //服务器发送本次消息结束
	ERROR,           //重复登录
	FILE_NAME,       //发送文件列表
	FILE_ERROR,     //选择文件名失败
	SERVER_EXIT,     //服务器退出信息
	CREATE_GROUP,    //创建组
	JOIN_GROUP,      // 加入组
	LIST_GROUP,      // 列出存在的组
	GROUP_CHAT,     //组内对话
	GROUP_DELETE    //删除组
};
/*服务器总列表*/
struct servmsg
{
	struct msg recvmsg;
	struct sockaddr_in addr;
	struct servmsg *next;
};

/*建立UDP套接字*/
int udp_link(void)
{
	int sock_fd;
	sock_fd = socket(AF_INET,SOCK_DGRAM,0);
	return sock_fd;     
}
