#include"head.h"
#include <time.h>
 
static int cli_fd = -1; //主要套接字，用于接受服务器各种消息
static int sock_fd = -1; //组播套接字，用于群聊
static struct sockaddr_in serv_addr; //用于存储服务器IP和端口
static char myname[20]; //用于存储用户名
static char chat_data[MAXSIZE];
static pthread_t tid4 = -1; //将其定义为全局变量的原因是，接收线程会将其取消

void send_sig(char myname[],char desname[],char data[],struct sockaddr_in serv_addr,char ch); 

void show_opt();

////////////////////////////////////////////////////////////////////////////////
void send_group_message(char *myname, char *group_name,char ch)
{
	  if(group_name == NULL)return;
	   send_sig(myname,"server",group_name,serv_addr,ch);
}

////////////////////////////////////////////////////////////////////////////////
/*用于给服务其发送数据的函数*/
void send_sig(char myname[],char desname[],char data[],struct sockaddr_in serv_addr,char ch)
{
       struct msg mymsg;
       mymsg.type = ch;
       if(myname != NULL)
              strcpy(mymsg.self_name,myname);

 
       if(desname != NULL)
              strcpy(mymsg.dst_name, desname);
       if(data != NULL)
              strcpy(mymsg.data, data);
 
       if(sendto(cli_fd,&mymsg,sizeof(struct msg),0,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0){
                     perror("sendto");
                     exit(1);
       }
}
 

/*接收消息函数，接收各种由服务器发来的数据，根据数据类型分别处理*/
void *recv_chat_func()
{
     struct timeval tv;
     char mytime[20] = "";
       int ret = -1;
       int i = 0;
       struct msg rcv_buf;
       while(1){
              bzero(&rcv_buf,sizeof(rcv_buf));
              ret = recvfrom(cli_fd,&rcv_buf,sizeof(rcv_buf),0,NULL,NULL);
              if(ret < 0){
                     perror("recvfrom");
                     exit(1);
              }

              rcv_buf.data[ret - sizeof(rcv_buf.type)-sizeof(rcv_buf.self_name)-sizeof(rcv_buf.dst_name)] = '\0';
   			 switch(rcv_buf.type)
			{
			 case LOGIN_TOALL:  
                    printf("\n%s",rcv_buf.data);
				    show_opt();
					break;
			 case REFRESH:
                     printf("@%s\t",rcv_buf.data);
					break;             
			case OVER: /*发送结束消息*/
                     printf("\n");
					break;
			case ERROR:              /*用户名重复提示消息*/
                     printf("登录失败！\n");
                     printf("%s",rcv_buf.data);
                     exit(1);
             case FILE_ERROR:              /*下载时文件不存在提示消息*/
                     printf("<文件传输>提示信息：%s\n",rcv_buf.data);
                     pthread_cancel(tid4); //如若输入文件名出错，取消线程，退出监听
					break;
		    case FILE_NAME:              /*文件名列表消息*/           
                    printf("<%s>\t\t",rcv_buf.data);
                     i++;
                     if(i%4 == 0)
                          printf("\n");
                     fflush(NULL);
					break;
		    case CREATE_GROUP:              
             	  gettimeofday(&tv,NULL);
				  strftime(mytime,sizeof(mytime),"%Y-%m-%d %T",localtime(&tv.tv_sec));
		          printf("\t\t\t\t来自%s(%s):\n\t\t\t\t%s",rcv_buf.self_name,mytime,rcv_buf.data);
					break;
		    case JOIN_GROUP:                     
             	  gettimeofday(&tv,NULL);
				  strftime(mytime,sizeof(mytime),"%Y-%m-%d %T",localtime(&tv.tv_sec));
		          printf("\t\t\t\t来自%s(%s):\n\t\t\t\t%s",rcv_buf.self_name,mytime,rcv_buf.data);
					break;
		    case LIST_GROUP:                       
             	  gettimeofday(&tv,NULL);
				  strftime(mytime,sizeof(mytime),"%Y-%m-%d %T",localtime(&tv.tv_sec));
		          printf("\t\t\t\t来自%s(%s):\n当前group：%s",rcv_buf.self_name,mytime,rcv_buf.data);
					break;
		    case GROUP_CHAT:                        
             	  gettimeofday(&tv,NULL);
				  strftime(mytime,sizeof(mytime),"%Y-%m-%d %T",localtime(&tv.tv_sec));
     			  printf("\t\t\t\t来自%s(%s):\n\t\t\t\t%s",rcv_buf.self_name,mytime,rcv_buf.data);				  
					break;
		    case GROUP_DELETE:                      
             	  gettimeofday(&tv,NULL);
				  strftime(mytime,sizeof(mytime),"%Y-%m-%d %T",localtime(&tv.tv_sec));				  
			      printf("\t\t\t\t来自%s(%s):\n\t\t\t\t%s",rcv_buf.self_name,mytime,rcv_buf.data);		
					break;
            case SERVER_EXIT:       	  
				  gettimeofday(&tv,NULL);
				  strftime(mytime,sizeof(mytime),"%Y-%m-%d %T",localtime(&tv.tv_sec));
     			  printf("\n\t\t\t\t来自%s(%s):\n\t\t\t\t%s",rcv_buf.self_name,mytime,rcv_buf.data);	
                  fflush(NULL);
				  break;
             default:              /*正常聊天消息（私聊）*/
             	  gettimeofday(&tv,NULL);
				  strftime(mytime,sizeof(mytime),"%Y-%m-%d %T",localtime(&tv.tv_sec));
		          printf("\t\t\t\t来自%s(%s):\n\t\t\t\t%s",rcv_buf.self_name,mytime,rcv_buf.data);
                   //  printf("(若要与%s聊天，请退回主界面重新选择私聊对象。)\n",rcv_buf.self_name);
					break;
              }
       }
}
 
/*私聊函数,参数均为全局变量*/
void chat_private(void)
{
       char peer_name[20];
       char chat_data[MAXSIZE];
      
       bzero(&chat_data,sizeof(chat_data));
       bzero(&peer_name,sizeof(peer_name));
      
       printf("请选择聊天对象：");
       scanf("%s",peer_name);
 
       while(getchar() != '\n'); 
       //usleep(100000);//延时下
       printf("------------正在与《%s》聊天,输入quit退回主界面----------\n",peer_name);
       while(1){
           //   printf("请输入聊天内容（按回车键发送--输入quit退回主界面--）：\n");
              fgets(chat_data,sizeof(chat_data),stdin);
              if(strncmp(chat_data,"quit",4) == 0)
                            break;
              send_sig(myname,peer_name,chat_data,serv_addr,CHAT_PRI);      
       }
}

void chat_group(void)
{
       char group_name[20];
       char chat_data[MAXSIZE];
      
       bzero(&chat_data,sizeof(chat_data));
       bzero(&group_name,sizeof(group_name));
      
       printf("请选择聊天group name：");
       scanf("%s",group_name);
 
       while(getchar() != '\n'); 
    printf("------------正在group《%s》聊天,输入quit退回主界面----------\n",group_name);
       while(1){
           //   printf("请输入聊天内容（按回车键发送--输入quit退回主界面--）：\n");
              fgets(chat_data,sizeof(chat_data),stdin);
              if(strncmp(chat_data,"quit",4) == 0)
                            break;
              send_sig(myname,group_name,chat_data,serv_addr,GROUP_CHAT);      
       }
	return;
}


/*线程：接收群聊消息（广播）*/

void *chat_toall_recv()
{
       int ret = -1;
       int num = -1;
//    sock_fd = cli_fd;
 
       struct msg rcv_buf;
       /*获取本机IP 并分配一个端口*/

       struct hostent *h_info;
       struct in_addr **p_addr;
       h_info = gethostbyname("laptop");
       p_addr = ((struct in_addr **)(h_info->h_addr_list));
 
       struct sockaddr_in self_addr;
       memset(&self_addr,0,sizeof(self_addr));
       self_addr.sin_family = AF_INET;
       self_addr.sin_port = htons(17891);
       self_addr.sin_addr.s_addr = htonl(INADDR_ANY);
       struct ip_mreq group;
       bzero(&group,sizeof(group));
       group.imr_multiaddr.s_addr = inet_addr("224.100.100.100");
       group.imr_interface = *(*p_addr);
       /*允许地址重用*/

       ret = setsockopt(sock_fd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&group,sizeof(group));
       if(ret < 0){
              perror("setsockopt to ADD_MEMBERSHIP");
              exit(1);
       }
       ret = bind(sock_fd,(struct sockaddr *)&self_addr, sizeof(self_addr));
       if(ret < 0){
              perror("bind");
              exit(1);
       }
       while(1){
              //printf("等待接收群聊消息\n");
              ret = recvfrom(sock_fd,&rcv_buf,sizeof(rcv_buf),0,NULL,NULL);
              //printf("新的群聊消息\n");
              if(ret < 0){
                     perror("recvfrom");
                     exit(1);
              }
              rcv_buf.data[ret - sizeof(rcv_buf.type)-sizeof(rcv_buf.self_name)-sizeof(rcv_buf.dst_name)] = '\0';
              printf("=====《群聊》消息来自<%s>:\n",rcv_buf.self_name);
              printf("--------------------------\n");
              printf("消息内容：%s",rcv_buf.data);
             
       }
 
}

/*发送群聊消息，其实跟饲料消息基本一样的，消息类型不一样而已*/
void chat_toall(void)
{
       char chat_data[MAXSIZE];  
       bzero(&chat_data,sizeof(chat_data));

       printf("---------------<正在与所有在线用户聊天>-----------------\n");
       while(getchar() != '\n'); 
       while(1){
 //             printf("请输入聊天内容（按回车键发送--输入quit退回主界面--）：\n");
              fgets(chat_data,sizeof(chat_data),stdin); 
              if(strncmp(chat_data,"quit",4) == 0)
                            break; 
              send_sig(myname,NULL,chat_data,serv_addr,CHAT_ALL);
       }
 
}
void chat_togroup(void)
{
       char chat_data[MAXSIZE];
		char g_name[20];  
       bzero(&chat_data,sizeof(chat_data));
	   printf("输入组名称:");
		scanf("%s",g_name);
       printf("---------------<正在in group 聊天>-----------------\n");
       while(getchar() != '\n'); 
       while(1){
 //             printf("请输入聊天内容（按回车键发送--输入quit退回主界面--）：\n");
              fgets(chat_data,sizeof(chat_data),stdin); 
              if(strncmp(chat_data,"quit",4) == 0)
                            break; 
              send_sig(myname,g_name,chat_data,serv_addr,GROUP_CHAT);
       }
 
}


void *recv_file(void *file_name)
{
       char download_path[80] = "/home/file_download/";
 
       int file_fd = -1;
       int recv_fd = -1; 
       int new_fd = -1;
       int ret = -1;
       int num = -1;
       char buf[BUFSIZ];
       struct sockaddr_in file_addr;

struct sockaddr_in their_addr; 

       printf("文件名：%s\n",(char *)file_name);
 
       strcat(download_path,(char *)file_name);
       file_fd = open(download_path, O_WRONLY|O_CREAT,0666);
       if(file_fd < 0){
              perror("open");
              pthread_exit(NULL);
       }
 
       recv_fd = socket(AF_INET,SOCK_STREAM,0);
       if (recv_fd < 0){
              perror("socket");
              pthread_exit(NULL);
       }
       int on = 1;
       ret = setsockopt(recv_fd,SOL_SOCKET,SO_REUSEADDR,(void *)&on,sizeof(on));
       if(ret < 0){
              perror("setsockopt to SO_REUSEADDR");
              pthread_exit(NULL);
       }
 
       struct hostent *h_info;
       struct in_addr **p_addr;
 
       bzero(&file_addr,sizeof(struct sockaddr));
       file_addr.sin_family = AF_INET;
       file_addr.sin_port = htons(8888);
       file_addr.sin_addr.s_addr = INADDR_ANY;
       
       ret = bind(recv_fd,(struct sockaddr *)&file_addr, sizeof(struct sockaddr));
       if(ret < 0){
              perror("bind");
              pthread_exit(NULL);
       }
  printf("listen \n");
       ret = listen(recv_fd,8);
       if(ret < 0){
              perror("listen");
              pthread_exit(NULL);
       }
      int sin_size = sizeof(struct sockaddr_in); 
       new_fd = accept(recv_fd,(struct sockaddr *)&their_addr,&sin_size);
  printf("accept \n");
       while(1){
              bzero(&buf,sizeof(buf));
              ret = read(new_fd,buf,sizeof(buf));
              if(ret < 0){
                     perror("read");
                     pthread_exit(NULL);
              }
              if(ret == 0){
                     break;
              }
              if(ret > 0){
                     num = write(file_fd,buf,ret);
                     if(num < 0){
                            perror("write");
                            pthread_exit(NULL);
                     }
 
              }
       }
       printf("<%s>下载完成\n",(char *)file_name);

       close(file_fd);
       close(recv_fd);
       close(new_fd);
 
       pthread_exit(NULL);
}
/*发出下载请求并创建接收数据线程*/
void download()
{
       char file_name[64];
       printf("当前资源中心所有文件:\n");
       usleep(100000);
       /*此次接收文件列表*/
       printf("\n请选择一个文件下载：");
       scanf("%s",file_name);
printf("file_name:%s\n",file_name);
       send_sig(myname,NULL,file_name,serv_addr,DOWNLOAD);
       if(pthread_create(&tid4,NULL,recv_file,(void *)file_name)){
              perror("pthread_create2");
              exit(1);
       }
       pthread_join(tid4,NULL);
      
}
/*发送文件*/
void *send_file( void *file_name)
{
       int file_fd = -1;
       int send_fd = -1;
       char buf[BUFSIZ];
       int ret = -1;
       int num = -1;
       int len = -1;
       struct  sockaddr_in file_addr;
 
       char file_path[80]="/home/file_download/";
 
       strcat(file_path, (char *)file_name);
 
       file_fd = open(file_path,O_RDONLY);
       if(file_fd < 0){
              perror("open");
              printf("对不起，您选择有误！请重新进入上传模式。\n");
              pthread_exit(NULL);
       }
 
       send_fd = socket(AF_INET,SOCK_STREAM,0);
       if(send_fd < 0){
              perror("socket");
              pthread_exit(NULL);
       }
       struct hostent *h_info;
       struct in_addr **p_addr;
 
       bzero(&file_addr,sizeof(struct sockaddr));
       file_addr.sin_family = AF_INET;
       file_addr.sin_port = htons(54321);
       file_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
       usleep(200000);
       ret = connect(send_fd,(struct sockaddr *)&file_addr,sizeof(struct sockaddr_in));
       if(ret < 0){
              perror("connect");
              pthread_exit(NULL);
       }
 
       len = lseek(file_fd,0L,SEEK_END);
       lseek(file_fd,0L,SEEK_SET);
 
       while(len > 0){
              bzero(&buf,sizeof(buf));
              ret = read(file_fd,buf,sizeof(buf));
              if(ret < 0){
                     perror("read");
                     pthread_exit(NULL);
              }
              num = write(send_fd,buf,ret);
              if(num < 0){
                     perror("write");
                     pthread_exit(NULL);
              }
       }
       printf("文件上传成功!\n");
       close(file_fd);
       close(send_fd);
       pthread_exit(NULL);
}
 
/*下载文件*/
void upload(void)
{
       char download_path[80] = "/home/file_download/";
       char file_name[64];
       DIR *dirp;
       struct dirent *E;
       pthread_t tid5 = -1;
 
       dirp = opendir(download_path);
       printf("您的文件夹：");
 
       while( (E = readdir(dirp) )!= NULL)
              printf("<%s>\t",E->d_name);
       printf("\n");
 
       printf("选择您要上传的文件：");
       scanf("%s",file_name);
 
       send_sig(myname,NULL,file_name,serv_addr,UPLOAD);
       if(pthread_create(&tid5,NULL,send_file,(void *)&file_name)){
              perror("pthread_create5");
              exit(1);
       }
       pthread_join(tid5,NULL);
 
}
/*打印主菜单*/
void show_opt()
{
	   printf("\n********************************************************\n");
       printf("********************%s 用户主界面**********************\n",myname);
       printf("********************************************************\n");
       printf("《A：@私聊模式》\t《B：@群聊模式》\n《C：@上传文件》\t《D：@下载文件》\n《E：@刷新列表》\t《F：@下线离开》\n《G：@创建组》\t《H：@加入组》\n《I：@列出组》\t《J：@组内聊天》\n《K：@删除组》\n");
       printf("请选择（如：A）：");
		fflush(NULL);
}
 void log_in()
{
       char log[] = "上线了.\n";
       system("clear");
       printf("********************************************************\n");
       printf("***********************欢迎使用**************************\n");
       printf("********************************************************\n");
       printf("\n======================用户登录======================\n");
       printf("请输入您的用户名：");
       scanf("%s",myname);
       strcpy(chat_data,myname);
       strcat(chat_data,log);
       send_sig(myname,"all"," 登陆~~~ \n",serv_addr,LOG_IN); 
       printf("提示：登录成功！\n");
       printf("========================================================\n");     
}
int main(void)
{
       void myhandle(int signum);
       signal(SIGINT,myhandle);
       struct msg mymsg;
	   char gname[20];
       char ch;
       int on = 1;
       int ret = -1;
       char file_name[20];

       bzero(&chat_data,sizeof(chat_data));
       pthread_t tid1;
       pthread_t tid2;
       pthread_t tid3;
 
       bzero(&serv_addr,sizeof(struct sockaddr));
       serv_addr.sin_family = AF_INET;
       serv_addr.sin_port = htons(7890);
       serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
 
       cli_fd = udp_link();
       sock_fd = udp_link();//组播套接字
       ret = setsockopt(sock_fd,SOL_SOCKET,SO_REUSEADDR,(void *)&on,sizeof(on));
       if(ret < 0){
              perror("setsockopt to SO_REUSEADDR");
              exit(1);
       }
	   log_in();
       if(pthread_create(&tid1,NULL,recv_chat_func,NULL) < 0){
              perror("pthread_create1");
              exit(1);
       }
       pthread_detach(tid1);
       if(pthread_create(&tid2,NULL,chat_toall_recv,NULL) < 0){
              perror("pthread_create2");
              exit(1);
       }

       pthread_detach(tid2);
option:   
	   show_opt();
       scanf(" %c",&ch);
       printf("--------------------------------------------------------\n");
       switch(ch)
       {
       case 'A':
              if(pthread_create(&tid3,NULL,(void *)chat_private,NULL) < 0){
                     perror("pthread_create3");
                     exit(1);
              }
              break;
       case 'B':
			chat_toall();
			break;
       case 'C': 
			upload();
			break;
       case 'D':
//			scanf("%s",file_name);
			send_sig(myname,NULL,NULL,serv_addr,FILE_NAME);                       
            download();
			goto option;
			break; 
       case 'E': 
			printf("当前在线用户：\n");
            send_sig(myname,NULL,NULL,serv_addr,REFRESH);
            goto option;
			break;  //发送刷新消息 
       case 'F': 
			send_sig(myname,NULL,NULL,serv_addr,OFFLINE);
            printf("下线成功！谢谢使用！再见\n");
            exit(1);
			break;
       case 'G':   //create group
			printf("输入组名称:");
			scanf("%s",gname);
	    	send_group_message(myname, gname,CREATE_GROUP);
			usleep(200000);
			goto option;
			break;
       case 'H':  //join group
			printf("输入组名称:");
			scanf("%s",gname);
			send_group_message(myname, gname,JOIN_GROUP);
			usleep(200000);
			goto option;
			break;
       case 'I':  //list group
		    send_group_message(myname, "group_list",LIST_GROUP);
			usleep(200000);
			goto option;
			break;
       case 'J':  // chat group
             chat_togroup();
			goto option;
			break;
       case 'K':  // delete group 
			printf("输入组名称:");
			scanf("%s",gname);
			send_group_message(myname, gname,GROUP_DELETE);
			usleep(200000);
			goto option;
			break;
       default :
           printf("没有这个选项，请重新选择：");
           while(getchar() != '\n');
                goto option;       
       }
       pthread_join(tid3,NULL);
  //     pthread_join(tid5,NULL);
       goto option;
       return 0;
}
void myhandle(int signum)
{
       send_sig(myname,NULL,NULL,serv_addr,OFFLINE);
      printf("谢谢使用！再见！\n");
       close(cli_fd);
      close(sock_fd);
       exit(1);
}
