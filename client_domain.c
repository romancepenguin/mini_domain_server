#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
	
#define BUF_SIZE 256

void * send_msg(void * arg);
void * recv_msg(void * arg);
void error_handling(char * msg);
		
int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_in serv_addr;
	pthread_t snd_thread, rcv_thread;
	void * thread_return;
	if(argc!=3) {
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	 }


	sock=socket(PF_INET, SOCK_STREAM, 0);
	
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_addr.sin_port=htons(atoi(argv[2]));
	  
	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
		error_handling("connect() error");

	printf("메뉴얼을 보려면 m을 입력 하세요\n");
	
	pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);
	pthread_join(snd_thread, &thread_return);
	pthread_join(rcv_thread, &thread_return);
	close(sock);  
	return 0;
}
	
void * send_msg(void * arg)   
{
	char msg[BUF_SIZE];
	int sock=*((int*)arg);
	while(1) 
	{
		memset(msg,0,BUF_SIZE);

		scanf("%s",msg);
		msg[strlen(msg)]='\0';
		//for(int i=0;i<strlen(msg)+1;i++)
		//	printf("보내는 메시지 %c\n",msg[i]);
		//printf("보내는 메시지 길이 %d\n",strlen(msg));
		if(!strcmp(msg,"q")||!strcmp(msg,"Q")) 
		{
			close(sock);
			exit(0);
		}
		else if(!strcmp(msg,"m")||!strcmp(msg,"M")) 
		{
			printf("도메인이나 ip 입력 :시 해당하는 도메인과 ip,조회수를 열람\n");
			printf("top : 조회수가 가장높은 도메인을 열람\n");
			printf("low : 조회수가 가장낮은 도메인을 열람\n");
		}
		else{ 
			if(write(sock, msg, strlen(msg))<0){
				printf("서버로 부터 응답이 없습니다.\n");
				close(sock);
				exit(0);
			}
		}

	}
	return NULL;
}
	
void * recv_msg(void * arg)  
{
	char msg[BUF_SIZE];
	int sock=*((int*)arg);
	int str_len;
	while(1)
	{
		str_len=read(sock, msg, BUF_SIZE-1);

		if(str_len<=0){
			printf("서버로 부터 응답이 없습니다.\n");
			close(sock);
			exit(0);			
		}
		if(str_len==-1) 
			return (void*)-1;
		msg[str_len]='\0';
		//fputs(name_msg, stdout);
		printf("%s\n",msg);
		//for(int i;i<strlen(msg)+1;i++)
		//	printf("받은 메시지 %c\n",msg[i]);
		for(int i=0;i<strlen(msg)+1;i++)
			msg[i]='\0';
	}
	return NULL;
}
	
void error_handling(char *msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
