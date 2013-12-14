#include"head.h"
 
static int serv_fd;//主要套接字,用来接收客户端发来的各种消息
 
static struct servmsg *H;    //服务器主线程，将接收的每个消息，建立成链表,H为链表头
static struct sockaddr_in self_addr; //服务器自己的地址
static struct sockaddr_in cli_addr; //客户端地址
static struct msg cli_msgbuf;       //用于存发来消息的结构体

 static struct group_info *head_group;  //

static struct user_info *head_user;  //处理线程建立的用户链表头
static int i=100; //用户id,从100开始（本程序里未使用）
static pthread_t tid1 = -1; //三个线程
static pthread_t tid2 = -1;
static pthread_t tid3 = -1;

////////////////////////////////////////////////////////////////////////////////
void create_group( struct servmsg *user);
void join_group(struct servmsg *user);
void list_group(struct servmsg *user);
void delete_group( struct servmsg *user);
void group_chat(struct servmsg *user);

void send_to(struct msg,struct sockaddr_in user);

void send_to(struct msg buf, struct sockaddr_in user)
{
      int num = sendto(serv_fd,&buf,sizeof(buf),0,(struct sockaddr *)&user,sizeof(struct sockaddr_in));
      if(num < 0){
         perror("sendto");
         exit(1);
      }
}

void create_group( struct servmsg *user)
{
       struct msg mymsg;
       bzero(&mymsg,sizeof(struct msg));
       struct group_info *new_group;

       new_group = (struct group_info *)malloc(sizeof(struct group_info));
       strcpy(new_group->group_name,user->recvmsg.data);
       new_group->id = i++;
       new_group->next = NULL;

	   struct user_info *owner,*new_user;
       new_user = (struct user_info *)malloc(sizeof(struct user_info));
       new_user->cli_addr = user->addr; //结构体整体赋值，客户端端口为5倍进程
       strcpy(new_user->user_name,user->recvmsg.data);
       new_user->id = i++;
       new_user->next = NULL;

	   new_group->owner=new_user;
       struct group_info *head=head_group,*htmp;
       while( head->next != NULL){
				htmp=head;
			   head=head->next;
              if(strcmp(head->group_name,new_group->group_name) == 0)
					break ;
              head = htmp;
       }
 	  if(head->next == NULL)
		{
             head->next = new_group;
	         mymsg.type = CREATE_GROUP;
	        strcpy(mymsg.self_name,"server");
	        strcpy(mymsg.dst_name, user->recvmsg.self_name);
	        strcpy(mymsg.data, new_group->group_name);
	    	strcat(mymsg.data,": 服务器创建好组!\n"); 
         }
		else 
	    {
            mymsg.type = ERROR;
            strcpy(mymsg.data,"该用户已存在,不能重复!\n"); 
 	    }
        send_to(mymsg,user->addr);
       
}



void join_group(struct servmsg *user)
{
       struct msg mymsg;
	   mymsg.type = JOIN_GROUP;
	   strcpy(mymsg.self_name,"server");
	   strcpy(mymsg.dst_name, user->recvmsg.self_name);
	   strcpy(mymsg.data, user->recvmsg.data);

       struct user_info *new_user;
       new_user = (struct user_info *)malloc(sizeof(struct user_info));
       strcpy(new_user->user_name,user->recvmsg.self_name);
       new_user->id = i++;
       new_user->cli_addr = user->addr; 
       new_user->next = NULL; 
// printf("join user name:%s\n",user->recvmsg.self_name);
       struct group_info *head=head_group,*tmph;
       while( head->next != NULL){
              head = head->next;
              if(strcmp(head->group_name,user->recvmsg.data) == 0){
               	   struct user_info *gu_head=head->owner;
			         while( gu_head->next != NULL){
		                 gu_head = gu_head->next;
 		                 if(strcmp(gu_head->user_name,user->recvmsg.self_name) == 0){
 	 	                     mymsg.type = ERROR;
 		                     strcpy(mymsg.data," : 该用户已登录 group,不能重复登录!\n"); 
			                  send_to(mymsg,user->addr);
   	 	              		 free(new_user);
							return ;
  			             }
  			         }
  				     gu_head->next = new_user;			  				     
			    	strcat(mymsg.data,": join in this group ok!\n"); 
		            send_to(mymsg,user->addr);
					return;
              }
       }
   		 free(new_user);
    	strcat(mymsg.data,":组不存在!\n"); 
        send_to(mymsg,user->addr);
}



void list_group(struct servmsg *user)
{
       struct group_info *group_tmp;
       struct msg buf;
       bzero(&buf,sizeof(buf));
       buf.type = LIST_GROUP;//消息类型为
       strcpy(buf.self_name,"server");
       strcpy(buf.dst_name, "client");

       group_tmp = head_group->next;
       if(group_tmp == NULL) {
              strcpy(buf.data,"暂时没有组！");
	          send_to(buf,user->addr);
       }
       else{
              while(group_tmp != NULL){
                     bzero(&buf.data,sizeof(buf.data));
                    strcpy(buf.data,group_tmp->group_name);
	    	          send_to(buf,user->addr);
                     group_tmp = group_tmp->next;
              }
       }
       bzero(&buf,sizeof(buf));
       buf.type = OVER;//结束发送
       send_to(buf,user->addr);

}
void delete_group_and_users(struct group_info *group)
{
         struct user_info *tmp1,*tmp2;
       tmp1 = group->owner;
       tmp2=tmp1->next;

       while( tmp2 != NULL){
			  free(tmp1);
              tmp1 = tmp2;
              tmp2 = tmp2->next;
       }

       free(tmp1);
	   free(group);
       tmp1 = NULL;
		group=NULL;
}

void delete_group(struct servmsg *user)
{
       struct group_info *group_tmp,*tmp_before;
       struct msg buf;
       bzero(&buf,sizeof(buf));
       buf.type = GROUP_DELETE;//消息类型为
       strcpy(buf.self_name,"server");
       strcpy(buf.dst_name, "client");
		tmp_before=head_group;
       group_tmp = head_group->next;
       if(group_tmp == NULL) {
              strcpy(buf.data,"暂时没有组！");
              send_to(buf,user->addr);
       }
       else
           while(group_tmp != NULL){
         		     if(strcmp(group_tmp->group_name,user->recvmsg.data) == 0){
             	        tmp_before->next = group_tmp->next;
		      	        strcpy(buf.data,group_tmp->group_name);
						  strcat(buf.data,":被删除!");
						 delete_group_and_users(group_tmp);
				         send_to(buf,user->addr); 
				         break;              		      
					 }
				tmp_before=group_tmp;
                group_tmp = group_tmp->next;
            }
         bzero(&buf,sizeof(buf));
         buf.type = OVER;//结束发送
		 send_to(buf,user->addr);
}

void send_message_to_all(struct servmsg *user,char ch)
{
       struct user_info *tmp;
       struct msg buf;
       bzero(&buf,sizeof(buf));
       buf.type = ch;//消息类型为
       strcpy(buf.self_name,user->recvmsg.self_name);
       strcpy(buf.dst_name, user->recvmsg.dst_name);
//		strcpy(buf.data,user->recvmsg.self_name);
		strcpy(buf.data,user->recvmsg.data);
         tmp=head_user->next;
		while(tmp != NULL)
        {
	      send_to(buf,tmp->cli_addr);
          tmp=tmp->next;
        }
}
void group_chat(struct servmsg *user)
{
       struct group_info *group_tmp,*tmp_before;
       struct msg buf;
       bzero(&buf,sizeof(buf));
       buf.type = GROUP_CHAT;//消息类型为
       strcpy(buf.self_name,user->recvmsg.self_name);
       strcpy(buf.dst_name, user->recvmsg.dst_name);
       strcpy(buf.data,user->recvmsg.data);

		tmp_before=head_group;
       group_tmp = head_group->next;

       if(group_tmp == NULL) {
              strcpy(buf.data,"暂时没有组！");
		      send_to(buf,user->addr);
			   return ;
       }
       else
        while(group_tmp != NULL)
        {
					
                  if(strcmp(group_tmp->group_name,user->recvmsg.dst_name) == 0)
                  {

                     bzero(&buf.data,sizeof(buf.data));
                     buf.type = GROUP_CHAT;//消息类型为
                     strcpy(buf.data,user->recvmsg.data);
                     struct user_info  *user_tmp=group_tmp->owner;
			         

					while(user_tmp != NULL)
                    {
		    		      send_to(buf,user_tmp->cli_addr);
                         user_tmp=user_tmp->next;
					}
					return;
			      }
				 tmp_before=group_tmp;
                 group_tmp = group_tmp->next;
       }
       
      bzero(&buf,sizeof(buf));
      buf.type = OVER;//结束发送
	  send_to(buf,user->addr);
}


/*将收到的所有消息插入链表尾部*/
void insert(struct servmsg **last,struct msg buf,struct sockaddr_in cli_addr)
{
       if((*last)->next == NULL ){
              (*last)->next = (struct servmsg *)malloc(sizeof(struct servmsg));
              (*last) = (*last)->next;
              (*last)->next = NULL;
              (*last)->recvmsg.type = buf.type;
              (*last)->addr = cli_addr;
              strcpy((*last)->recvmsg.self_name, buf.self_name);
              strcpy((*last)->recvmsg.dst_name,buf.dst_name);
              strncpy((*last)->recvmsg.data, buf.data, strlen(buf.data)+1);
	           printf("新信息接收完成\n");
       }
}
/*若为登录消息，则将其消息插入用户链表*/
void add_user(struct user_info *head,struct servmsg *user)
{
       struct msg repeat;
       bzero(&repeat,sizeof(struct msg));
       struct user_info *new_user;
       new_user = (struct user_info *)malloc(sizeof(struct user_info));
       strcpy(new_user->user_name,user->recvmsg.self_name);
       new_user->id = i++;
       new_user->cli_addr = user->addr; //结构体整体赋值，客户端端口为5倍进程
       new_user->next = NULL;
 
       while( head->next != NULL){
              head = head->next;
              if(strcmp(head->user_name,new_user->user_name) == 0){
                     repeat.type = ERROR;
                     strcpy(repeat.data,"该用户已登录,不能重复登录!\n");
			    	  send_to(repeat,new_user->cli_addr);
              }
       }
       head->next = new_user;

       printf("新用户登录处理完成\n");
}
/*私聊时，根据消息定位对方用户列表的位置，从而获取其接收套接字 IP和端口*/
struct user_info *locat_des(char *name)
{
       printf("head_user->addr:%p\n",head_user->next);
       if (head_user->next == NULL) return NULL; //无其他用户
       struct user_info *tmp;
       tmp = head_user;
      
       while(tmp != NULL){
              printf("username:%s\n",tmp->user_name);
              if(strcmp(tmp->user_name,name) == 0)
                  return tmp;

             tmp = tmp->next;
       }
       return NULL;
}
/*发送消息至客户端*/
void sendmsg_to_cli(struct user_info *locat_tmp)
{
   	  send_to(H->recvmsg,locat_tmp->cli_addr);
       printf("将消息已发送至客户端\n");
}
/*私聊消息转发*/
void chat_private(void)
{
       char server[]="服务器";
       char off_logo[]="该用户不在线！！";
       int sock_fd;
       sock_fd = serv_fd;
       struct user_info *locat_tmp;
       locat_tmp = locat_des(H->recvmsg.dst_name);         //定位，找出消息目标用户
 
       if(locat_tmp == NULL){
              printf("聊天对象不在线，进入处理。\n");
              locat_tmp = (struct user_info *)malloc(sizeof(struct user_info));
              strcpy(H->recvmsg.self_name,server);
 
              bzero(H->recvmsg.data,sizeof(H->recvmsg.data));
              strcpy(H->recvmsg.data,off_logo);
              H->recvmsg.data[strlen(off_logo)]='\n';
 
              locat_tmp->cli_addr = H->addr;
              printf("消息来自addr   :%s(%d)\n",inet_ntoa(H->addr.sin_addr),ntohs(H->addr.sin_port));
              sendmsg_to_cli(locat_tmp);//对方不在线，发送提示
              free(locat_tmp);
              locat_tmp = NULL;
              return;
       }
       sendmsg_to_cli(locat_tmp);
}

/*发送在线用户列表至客户端*/
void *send_all_online(struct sockaddr_in addr)
{

       int num = -1;
       int len = -1;
       int i = 0;
       struct user_info *tmp;
       struct msg buf;
       bzero(&buf,sizeof(buf));
       buf.type = REFRESH;//消息类型为刷新
 
       tmp = head_user->next;
       if(tmp == NULL) {
           strcpy(buf.data,"暂时没有其他用户在线！");
           send_to(buf,addr);
       }
       else
       while(tmp != NULL){
           strcpy(buf.data,tmp->user_name);
           send_to(buf,addr);
           tmp = tmp->next;
       }

       bzero(&buf,sizeof(buf));
       buf.type = OVER;//结束发送
       send_to(buf,addr);
       printf("结束发送\n");
}
/*发送文件名*/
void send_dir_allfile(struct sockaddr_in cli_addr)
{
       struct msg file_list;
       int msglen = -1;
       socklen_t addrlen = -1;
       int ret = -1;
 
       bzero(&file_list,sizeof(file_list));
       file_list.type = FILE_NAME;
 
       DIR *dirp;
       struct dirent *E;
       dirp  = opendir("/home/file_center");
       if(dirp == NULL){
              perror("opendir");
              exit(1);
       }
       while((E = readdir(dirp)) != NULL){
              printf("%ld :%s\n",E->d_fileno,E->d_name);
              bzero(&file_list.data, sizeof(file_list.data));
              strcpy(file_list.data, E->d_name);
 
              msglen = sizeof(char)+20+20+strlen(file_list.data);
              addrlen = sizeof(struct sockaddr_in);
              ret = sendto(serv_fd, &file_list,msglen, 0, (struct sockaddr *)&cli_addr, addrlen);   //发送文件列表
              if(ret < 0 ){
                     perror("sendto");
                     exit(1);
              }
       }
 
}
/*若找不到文件，则发送提示消息*/
void send_error_tocli(struct sockaddr_in *cli_addr)
{
       char show[] = "对不起，没有这个选项！";
       char str[] = "服务器";
       int msglen = -1;
       int ret = -1;
       socklen_t addrlen;
       struct msg error_msg;
       bzero(&error_msg,sizeof(error_msg));
       error_msg.type = FILE_ERROR;
       strcpy(error_msg.self_name,str);
       strcpy(error_msg.data,show);
       msglen = sizeof(char)+20+20+strlen(error_msg.data);
       addrlen = sizeof(struct sockaddr_in);
       ret = sendto(serv_fd, &error_msg,msglen, 0, (struct sockaddr *)cli_addr, addrlen);   //发送出错信息
       if(ret < 0 ){
              perror("sendto");
              exit(1);
       }
      
 
}
/*发送下载需要的文件*/
void *download_send(void *file_addr)
{
       int file_fd = -1;
       int send_fd = -1;
       char buf[BUFSIZ];
       int ret = -1;
       int num = -1;
       int len = -1;
 
       char file_path[80]="/home/file_center/";
 
       strcat(file_path, H->recvmsg.data);
 printf("file_path:%s\n",file_path);
       sleep(1);
       file_fd = open(file_path,O_RDONLY); //若打开出错，给客户端发送错误提示消息，客户端结束线程，这样可以保证客户端能都退出死循环。
       if(file_fd < 0){
              perror("open  aaaa");
              send_error_tocli((struct sockaddr_in *)file_addr);
              close(file_fd);
              close(send_fd);
              pthread_exit(NULL);
       }
 
       send_fd = socket(AF_INET,SOCK_STREAM,0);
       if(send_fd < 0){
              perror("socket");
              pthread_exit(NULL);
       }
      
       ((struct sockaddr_in *)file_addr)->sin_family = AF_INET;
       ((struct sockaddr_in *)file_addr)->sin_port = htons(8888);
       ((struct sockaddr_in *)file_addr)->sin_addr.s_addr = inet_addr("127.0.0.1"); 
       printf("\n下载要求来自addr:%s(%d)\n",inet_ntoa(((struct sockaddr_in *)file_addr)->sin_addr),ntohs(((struct sockaddr_in *)file_addr)->sin_port));
//       sleep(1);
       int on = 1;
       ret = setsockopt(send_fd,SOL_SOCKET,SO_REUSEADDR,(void *)&on,sizeof(on));
       if(ret < 0){
              perror("setsockopt to SO_REUSEADDR");
              pthread_exit(NULL);
       }

//       ret = connect(send_fd,(struct sockaddr *)file_addr,sizeof(struct sockaddr_in));
       ret = connect(send_fd,(struct sockaddr *)file_addr,sizeof(struct sockaddr_in));
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
              len = len - num;
              printf("Len = %d\n",len);
       }
       printf("已发送文件<%s>至客户端\n",H->recvmsg.data);
       close(file_fd);
       close(send_fd);
       pthread_exit(NULL);
}
/*接收上传文件*/
void upload_func(void)
{
       char download_path[80] = "/home/file_center/";
 
       int file_fd = -1;
       int recv_fd = -1;
       int new_fd = -1;
       int ret = -1;
       int num = -1;
 
       char buf[BUFSIZ];
       char file_name[64];
       strcpy(file_name,H->recvmsg.data);
 
       printf("文件名：%s\n", file_name);
 
       strcat(download_path, file_name);
       file_fd = open(download_path, O_WRONLY|O_CREAT,0666);
       if(file_fd < 0){
              perror("open rec");
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

     H->addr.sin_family = AF_INET;
     H->addr.sin_addr.s_addr = INADDR_ANY;
       H->addr.sin_port = htons(54321);
 
       printf("\n下载数据至地址addr   :%s(%d)\n",inet_ntoa(H->addr.sin_addr),ntohs(H->addr.sin_port));
      
       ret = bind(recv_fd,(struct sockaddr *)&H->addr, sizeof(struct sockaddr));
       if(ret < 0){
              perror("bind");
              pthread_exit(NULL);
       }
       ret = listen(recv_fd,8);
       if(ret < 0){
              perror("listen");
              pthread_exit(NULL);
       }
      
       new_fd = accept(recv_fd,NULL,NULL);
      
       while(1){
              bzero(&buf,sizeof(buf));
              ret = read(new_fd,buf,sizeof(buf));
              if(ret < 0){
                     perror("read");
                     pthread_exit(NULL);
              }
              printf("ret = %d\n",ret);
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
       printf("<%s>由<%s>传送的文件接收完成\n",file_name,H->recvmsg.self_name);
       
       close(file_fd);
       close(recv_fd);
       close(new_fd);
 
       pthread_exit(NULL);
}
/*用户下线，则将其从链表中删除*/
void  delete_user(char name[])
{
       struct user_info *tmp;
       tmp = head_user->next;
 
       struct user_info *tmp2;
       tmp2 = head_user; //tmp2记住tmp前面的节点
      
       while(strcmp(name,tmp->user_name) != 0){
              tmp2 = tmp;
              tmp = tmp->next;
       }
       tmp2->next = tmp->next;//删除tmp
       free(tmp);
       tmp = NULL;
}
/*解析消息线程*/
void *pthread_func()
{
       int group_fd;     //组播套接字
       group_fd = udp_link();
 
       head_user = (struct user_info *)malloc(sizeof(struct user_info));
       head_user->next = NULL;
      
       struct user_info *tmp; // 循环变量，用于便利在线用户链表将其发送给第一次登录的或者有刷新请求的客户端
       struct  servmsg *free_tmp;
 
       int ret= -1;
       while(1){
              while(H->next != NULL){
                   free_tmp = H;
                     H = H->next; //消息头，处理玩就往后移动
                   free(free_tmp);//将处理完的消息所占内存释放掉
                   free_tmp = NULL;
                    printf("消息来自:%s  IP:PORT:%s:%d\n",H->recvmsg.self_name,inet_ntoa(H->addr.sin_addr),ntohs(H->addr.sin_port));
					
					switch(H->recvmsg.type)
					{
                     case LOG_IN:
                            printf("%s登录消息\n",H->recvmsg.self_name);
                            send_message_to_all(H,LOGIN_TOALL);
                            add_user(head_user,H);
                     		break;
      				case REFRESH:
                            printf("%s刷新消息\n",H->recvmsg.self_name);
                            //usleep(50000);
                            send_all_online(H->addr);
                            break;
                     case CHAT_PRI:
                            printf("%s私聊消息\n",H->recvmsg.self_name);
                            printf("目标：%s\n",H->recvmsg.dst_name);
                            chat_private();
							break;
                     case CHAT_ALL:
                            printf("%s群聊消息\n",H->recvmsg.self_name);
						   send_message_to_all(H,CHAT_ALL);
                       		break;
					case FILE_NAME:
                            printf("%s申请下载\n",H->recvmsg.self_name);
                            send_dir_allfile(H->addr);
                     		break;
					case DOWNLOAD:
                            printf("%s 确认下载\n",H->recvmsg.self_name);
                            pthread_create(&tid2,NULL,download_send,(void *)&H->addr);
                            pthread_detach(tid2);
							break;
                     case  UPLOAD:
                            printf("%s上传文件\n",H->recvmsg.self_name);
                            pthread_create(&tid3,NULL,(void *)&upload_func,NULL);
                            pthread_detach(tid3);
                     		break;
					case OFFLINE:
                            printf("%s下线\n",H->recvmsg.self_name);
                            delete_user(H->recvmsg.self_name);
                     		break;
					case CREATE_GROUP:
                            printf("%s create group\n",H->recvmsg.self_name);
							create_group(H);						
                     		break;
					case  JOIN_GROUP:
                            printf("%s join group\n",H->recvmsg.self_name);
							join_group(H);
                     		break;
					case LIST_GROUP:
                            printf("%s list group\n",H->recvmsg.self_name);
							list_group(H);
                     		break;
					case GROUP_CHAT:
                            printf("%s group chat\n",H->recvmsg.self_name);
							group_chat(H);
                     		break;
					case GROUP_DELETE:
                            printf("%s delete group\n",H->recvmsg.self_name);
							delete_group(H);
                     		break;
					 default:
							printf("msg type error !\n");
							break;
					 }
                     printf("-----------------------------------\n");
              }
       }
}
 
 
int main(void)
{
       void myhandle(int signum);
       signal(SIGINT,myhandle);
 
       serv_fd = udp_link(); 
       H = (struct servmsg *)malloc(sizeof(struct servmsg));
       H->next = NULL;
 
       head_group=(struct group_info *)malloc(sizeof(struct group_info ));
       head_group->next=NULL;

       static struct servmsg *H_recv;
       H_recv = H;
      
       int addrlen= -1;
       int ret = -1;
       int on = 1;
       bzero(&self_addr,sizeof(struct sockaddr));
 
       self_addr.sin_family = AF_INET;
       self_addr.sin_port = htons(7890);
       self_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
 
       ret = setsockopt(serv_fd,SOL_SOCKET,SO_REUSEADDR,(void *)&on,sizeof(on));
       if(ret < 0){
              perror("setsockopt to SO_REUSEADDR");
              exit(1);
       }
 
       if(bind(serv_fd,(struct sockaddr *)&self_addr,sizeof(struct sockaddr)) < 0){
              perror("bind");
              exit(1);
       }
       printf("*******欢迎使用********\n");
       printf("服务器正在运行……\n");
       /*创建解析数据包的线程*/
       if(pthread_create(&tid1, NULL, pthread_func,NULL ) < 0){
              perror("pthread_create");
              exit(1);
       }
       pthread_detach(tid1);
      
       addrlen = sizeof(struct sockaddr_in);
      
       while(1){
              bzero(&cli_msgbuf,sizeof(struct msg));
              ret = recvfrom(serv_fd,&cli_msgbuf,sizeof(cli_msgbuf),0,(struct sockaddr *)&cli_addr,&addrlen);
              if(ret < 0){
                     perror("recvfrom");
                     exit(1);
              }
              printf("有新消息发送到服务器\n");
              /*把收到的消息放入链表等待解析*/
              insert(&H_recv,cli_msgbuf,cli_addr);//H_recv每次改变，将消息每次插入至链表末尾           
      }
      
       return 0;
}

void free_all_user(struct user_info *head_user)
{
   if(head_user == NULL) return;
   struct user_info *head=head_user;
   while(head->next !=NULL)
	{
	   head_user=head->next;
	   free(head);
	   head=head_user;
	}
    free(head);    
}
void free_all_group(struct group_info *head_group)
{
   if(head_group == NULL) return;
   struct group_info *head=head_group;
   while(head->next !=NULL)
	{
	   head_group=head->next;
       free_all_user(head->owner);
	   free(head);
	   head=head_group;
	}
    free(head);    
}
void free_all_message(struct servmsg *h)
{
   if(h == NULL) return;
   struct servmsg  *head=h;
   while(head->next !=NULL)
	{
	   h=head->next;
	   free(head);
	   head=h;
	}
    free(head);  
}

void myhandle(int signum)
{
      struct msg mymsg;
      mymsg.type = SERVER_EXIT;
       strcpy(mymsg.self_name,"server");
      strcpy(mymsg.data," server exit"); 
      struct user_info *tmp=head_user->next;
		while(tmp != NULL)
        {
         printf("user name:%s\n",tmp->user_name);
	      send_to(mymsg,tmp->cli_addr);
          tmp=tmp->next;

        }

       close(serv_fd);
	 free_all_user(head_user);
	 free_all_group(head_group);
	   free_all_message(H);
      printf("\n谢谢使用！再见！\n");
       exit(1);
}
