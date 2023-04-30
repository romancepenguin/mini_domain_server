#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include<ctype.h>
#include<sys/types.h>
#include<netdb.h>
#include<time.h>

#define BUF_SIZE 256

void * handle_clnt(void * arg);
int send_msg(char * msg, int len,int clnt_sock);
void error_handling(char * msg);
int nowtime(char *now_t,int on);
int today(char *to_d,int on);
int clientInfo(int con_discon,char *d,char *t,char *clnt_addr);
int topDomain(char *msg);
int lowDomain(char *msg);

int msgMake(char *msg);
int get_domain(char *ip_addr);
int get_ip(const char *hostName);
void hit_array();
void hit_plus();

int clnt_cnt=0;

pthread_mutex_t mutx;

char ip_table[9]="table";
char ip_buffer[10][16];
char domain_buffer[256];
char addr_name[256];//별명

char clnt_addr_buffer[100];

int main(int argc, char *argv[])
{
	int serv_sock;
	int clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_sz;
	char t_buf[32];
	char d_buf[32];
	pthread_t t_id;
	if(argc!=2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
  
	pthread_mutex_init(&mutx, NULL);
	serv_sock=socket(PF_INET, SOCK_STREAM, 0); //socket 생성, 반환값을 파일 디스크립터

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET; 
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_adr.sin_port=htons(atoi(argv[1]));
	
	if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1) //프로세스에 소켓 묵기
		error_handling("bind() error");
	if(listen(serv_sock, 5)==-1) //소켓 lsten
		error_handling("listen() error");
	
	while(1)
	{
		clnt_adr_sz=sizeof(clnt_adr);
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr,&clnt_adr_sz); //연결 받아들이기
                //새로운 소켓 생성하여 클라이언트 매핑 시킴
		
		today(d_buf,1);
		nowtime(t_buf,1);

		pthread_mutex_lock(&mutx); //?
		//하나의 쓰레드만 아래의 코드 사용 
		clientInfo(1,d_buf,t_buf,inet_ntoa(clnt_adr.sin_addr));
		strncpy(clnt_addr_buffer,inet_ntoa(clnt_adr.sin_addr),strlen(inet_ntoa(clnt_adr.sin_addr)));
		//pthread_mutex_unlock(&mutx);
	
		pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);
		//pthread_create(thread id, 쓰레드의 특성, 어떤 로직을 작동시킬지, arg)
		pthread_detach(t_id); //해제
		pthread_mutex_unlock(&mutx); //?
		
		printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
	}
	close(serv_sock);
	return 0;
}
	
void * handle_clnt(void * arg)
{
	int clnt_sock=*((int*)arg);
	int str_len=0, i;
	char msg[BUF_SIZE];
	char t_buf[32];
	char d_buf[32];
	char cl_addr[100];

	strcpy(cl_addr,clnt_addr_buffer);

	while((str_len=read(clnt_sock, msg, sizeof(msg)))!=0){
		if(str_len<0){
			printf("클라이언트 접속오류");
			close(clnt_sock);
			return NULL;
		}
		//printf("받는 문자길이 : %d\n",strlen(msg));
		//printf("받는 전체 메시지 : %s\n",msg);

		if(!strcmp(msg,"top")) 
		{
			pthread_mutex_lock(&mutx); //읽기 작업만 하므로 mutex 락까지 할 필요가 있나?
			//memset(msg,0,BUF_SIZE);
			topDomain(msg);
			write(clnt_sock, msg, strlen(msg));  //send recv
			//write(파일 디스크립터, 보낼 메시지, 보낼 메시지 크기)
			pthread_mutex_unlock(&mutx);	
		}
		else if(!strcmp(msg,"low")) 
		{
			pthread_mutex_lock(&mutx); //읽기 작업만 하므로 mutex 락까지 할 필요가 있나?
			//memset(msg,0,BUF_SIZE);
			lowDomain(msg);
			write(clnt_sock, msg, strlen(msg)); //send recv
			pthread_mutex_unlock(&mutx);	
		}
		else{ 
			if(send_msg(msg, str_len,clnt_sock)<0){
				printf("클라이언트 접속오류");
				close(clnt_sock);
				return NULL;
			}

		}
		memset(msg,0,BUF_SIZE);

	}
	today(d_buf,0);
	nowtime(t_buf,0);

	pthread_mutex_lock(&mutx);

	clientInfo(0,d_buf,t_buf,cl_addr);
	pthread_mutex_unlock(&mutx);
	
	close(clnt_sock);
	return NULL;
}
int send_msg(char * msg, int len,int clnt_sock)   // send to all
{
	int i;
	pthread_mutex_lock(&mutx);
	//========= critical section ============
	//하나의 쓰레드만 아래 코드 접근 가능
	//쓰레드가 차례대로 실행 됨

	msgMake(msg); // 파일 수정 작업이 
	//hit_array();
	printf("보내는 전체 메시지 : %s\n",msg);
	//for(int i=0;i<strlen(msg)+1;i++)
	//	printf("보내는 메시지 : %c\n",msg[i]);
	if(write(clnt_sock, msg, strlen(msg))<0){  //send recv
		return -1;
	}
	memset(msg,0,BUF_SIZE);
	//========= critical section ============
	pthread_mutex_unlock(&mutx);
	return 1;
}
void error_handling(char * msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

void hit_array(){
	FILE *fcoppy,*ff;

	ff=fopen(ip_table,"r");
	int h=0,h_c,line_num=0,high;
	char line[256];
	char l_token[256];
	char *token;

	char buf[256];
	char l[256];

	while(fgets(line,256,ff)) 
	{
		strcpy(l_token,line);
		token=strtok(l_token," ");
		strcpy(buf,token);
		h_c=atoi(buf);
		h=(h>=h_c)?h:h_c;
		line_num++;
	}
	fclose(ff);
	high=h;

	for(int x=0;x<line_num;x++){
		h=high;
		fseek(ff,0L,SEEK_SET);
		ff=fopen(ip_table,"r");
		fcoppy=fopen("change","w");
		for(int i=0;i<x;i++){fgets(line,256,ff);fputs(line,fcoppy);}
		while(fgets(line,256,ff)) 
		{
			strcpy(l_token,line);
			token=strtok(l_token," ");
			strcpy(buf,token);
			h_c=atoi(buf);
			if(h_c<=h){
				strcpy(l,line);
				h=h_c;
			}
			fputs(line,fcoppy);

		}
	
		fclose(ff);
		fclose(fcoppy);

		ff=fopen(ip_table,"w");
		fcoppy=fopen("change","r");
		fputs(l,ff);
		//fprintf(fp,"%s\n",l);
		while(fgets(line,256,fcoppy))
		{
			if(strcmp(line,l)!=0)
				fputs(line,ff);
		}	

		fclose(fcoppy);
		fclose(ff);

	}


	remove("change");

}

void hit_plus(){
	FILE *fcopy,*fp;

	fp=fopen(ip_table,"r");
	fcopy=fopen("change","w");

	int h,ac,o=0;
	char a_n[100];
	char d[100];
	char ip[10][16];

	char line[256];
	char l_token[256];
	char *token;

	char buf[100];
	
	while(fgets(line,256,fp)) 
	{	

		if(o==0){
			strcpy(l_token,line);
			token=strtok(l_token," ");
			strcpy(buf,token);
			h=atoi(buf);
			token=strtok(NULL," ");
			token=strtok(NULL," ");

			strcpy(buf,token);
			if(strcmp(buf,domain_buffer)==0){
		
				token=strtok(NULL," ");
				strcpy(buf,token);
				ac=atoi(buf);

				strcpy(l_token,line);
				h+=1;	
				sprintf(line,"%d",h);
				//printf("조회수추가 %s\n",line);
				o+=1;

				token=strtok(l_token," ");
				token=strtok(NULL," ");
				for(int i=0;i<3;i++){
					strcpy(buf,token);
					strcat(line," ");
					strcat(line,buf);
					token=strtok(NULL," ");
				}
				for(int i=0;i<ac;i++){
					strcpy(buf,token);
					strcat(line," ");
					strcat(line,buf);
					token=strtok(NULL," ");
				}
				//sprintf(line,"%c",'\n');//방금 추가
				
				line[strlen(line)+1]='\0';
			}	
		}
		
		fprintf(fcopy,"%s",line);
		//fputs(line,fcopy);
	}
	
	fclose(fp);
	fclose(fcopy);

	fp=fopen(ip_table,"w");
	fcopy=fopen("change","r");

	while(fgets(line,256,fcopy))
	{
		fputs(line,fp);
	}	
	fclose(fp);
	fclose(fcopy);
	remove("change");

}

int get_ip(const char *hostName){
	struct in_addr addr;
	struct hostent *host;
	int i;
	
	if((host=gethostbyname(hostName)) == NULL){
		printf("외부로부터 ip주소 획득에 실패하였습니다.\n");
		return -1;
	}
	printf("official name=%s\n",host->h_name);
	i=0;
	if(host->h_aliases[i]==NULL)
		strcpy(addr_name,"none_aliases");

	while(host->h_aliases[i]!=NULL){
		strcpy(addr_name,host->h_aliases[i]);
		printf("aliases=%s\n",host->h_aliases[i]);	
		i++;
	}

	i=0;
	while(host->h_addr_list[i]!=NULL){
		memcpy(&addr.s_addr,host->h_addr_list[i],4);	
		//printf("%s(0x%x)\n",inet_ntoa(addr),ntohl(*(long*)host->h_addr_list[i]));
		strcpy(ip_buffer[i],inet_ntoa(addr));
		i++;
	}
	printf("get_ip : %s\n",addr_name);
	return i;
	/*for(i=0;host->h_addr_list[i];i++){
		
	printf("address=%d(%s)\n",i+1,inet_ntoa(*(struct in_addr*)host->h_addr_list[i]));
	
	}*/
}

int get_domain(char *ip_addr){
	struct hostent *host;
	struct in_addr binary_addr;
	struct in_addr addr;
	int i=0;
        binary_addr.s_addr=inet_addr(ip_addr);

	if((host=gethostbyaddr(&binary_addr,sizeof(binary_addr),AF_INET))<0||host==NULL)
	{
		printf("외부에서 도메인 주소를 가져오는데 실패 하였습니다.\n");
		return -1;
	}

	printf("호스트네임%s\n",host->h_name);
	strncpy(domain_buffer,host->h_name,strlen(host->h_name));
	i=0;
	if(host->h_aliases[i]==NULL)
		strcpy(addr_name,"none_aliases");

	while(host->h_aliases[i]!=NULL){
		strcpy(addr_name,host->h_aliases[i]);
		printf("aliases=%s\n",host->h_aliases[i]);
		i++;
	}
	
	printf("호스트네임%s\n",host->h_name);

	i=0;
	while(host->h_addr_list[i]!=NULL){
		memcpy(&addr.s_addr,host->h_addr_list[i],4);	
		//printf("%s(0x%x)\n",inet_ntoa(addr),ntohl(*(long*)host->h_addr_list[i]));
		strcpy(ip_buffer[i],inet_ntoa(addr));
		i++;
	}
	return 0;
}

int msgMake(char *msg)
{
	FILE *f;
	char buffer[100];
	char input[100];

	int i=0;

	int is_alpha=0;

	char hit_buffer[256];
	int hit;//조회수

	char cnt_buffer[256];
	int addr_count;//ip주소 개수

	char line_buffer[256];
	char *table_data; //split 테이블 데이
	int s,a_i;

	//scanf("%s",input);//주소나 도메인을 입력
	strcpy(input,msg);
	input[strlen(input)]='\0';
	for(int i=0;i<strlen(input)+1;i++)
		printf("받는 메시지 : %c\n",input[i]);	
	if (isalpha(input[0])!=0){is_alpha=1;
				printf("도메인을 입력하셨습니다.\n");
	}
	else printf("ip주소를 입력하셨습니다.\n");

	if((f=fopen(ip_table,"r"))==NULL)
	{
		printf("file is not exist");

	}
	
		while(fgets(line_buffer,256,f)) //table read하는 부분 
		{	
			line_buffer[strlen(line_buffer)-1]='\0';
			table_data=strtok(line_buffer," ");	
			s=0;
			a_i=0;
			while(table_data!=NULL){			
				//printf("%s ",table_data);
				if(s==0){strcpy(hit_buffer,table_data);hit=atoi(hit_buffer);s++;}
				else if(s==1){strcpy(addr_name,table_data);s++;}
				else if(s==2){strcpy(domain_buffer,table_data);s++;}
				else if(s==3){strcpy(cnt_buffer,table_data);addr_count=atoi(cnt_buffer);s++;}
				else if(s==4){strcpy(ip_buffer[a_i],table_data);a_i++;}
				table_data=strtok(NULL," ");

			}
			ip_buffer[a_i+1][0]='\0';
			if (is_alpha==1){
				if(strcmp(domain_buffer,input)==0){	//도메인이면 주소를 검색 아이피이면 도맨인검색 	
					printf("테이블에서 ip주소를 찾았습니다.\n");
					printf("%d %s %s %d\n",++hit,addr_name,domain_buffer,addr_count);
					for(int ix=0;ix<addr_count;ix++){
						printf("%s\n",ip_buffer[ix]);	
					}
					sprintf(msg,"조회수 %d\n가명 %s\n도메인 %s\n주소갯수 %d",hit,addr_name,domain_buffer,addr_count);
					for(int ix=0;ix<addr_count;ix++){
						sprintf(msg,"%s\nip주소 %s",msg,ip_buffer[ix]);	
					}
					
					i++;
					fclose(f);
					hit_plus();
					hit_array();
					//break;
				}
			}
			else{	
				for(int ix=0;ix<addr_count;ix++){

					if(strcmp(ip_buffer[ix],input)==0){
						printf("테이블에서 도메인주소를 찾았습니다.\n"); 
						printf("%d %s %s %d\n",++hit,addr_name,domain_buffer,addr_count);
	
						sprintf(msg,"조회수 %d\n가명 %s\n도메인 %s\n주소갯수 %d",hit,addr_name,domain_buffer,addr_count);
						i++;
						fclose(f);
						hit_plus();
						hit_array();
						for(int i=0;i<addr_count;i++){
							sprintf(msg,"%s\nip주소 %s",msg,ip_buffer[i]);	
						}
						//break;

				
					}
				}	
			}
			
		
		}

		if(i==0)
		{	fclose(f);
			printf("외부에서 데이터를 가져옵니다....\n"); 
			if(is_alpha==1){
				addr_count=get_ip(input);
				if(addr_count<0){sprintf(msg,"%s","데이터를 가져오는데 실패하였습니다.");}
				else {

					f=fopen(ip_table,"a");//쓰기모드로 다시열
					fseek(f,0L,SEEK_END);//커서를 맨뒤로 이동

					fprintf(f,"%d %s %s %d",1,addr_name,input,addr_count); 
					for(int a=0;a<addr_count;a++)
					{
						if(a==addr_count-1) fprintf(f," %s\n",ip_buffer[a]);
						else fprintf(f," %s",ip_buffer[a]);	
						printf("%s\n",ip_buffer[a]);
					}
					//fprintf(f," %c",'\n');
					sprintf(msg,"조회수 %d\n가명 %s\n도메인 %s\n주소갯수 %d",1,addr_name,input,addr_count);
					for(int i=0;i<addr_count;i++){
						sprintf(msg,"%s\nip주소 %s",msg,ip_buffer[i]);	
					}
					//printf("완성된 메시지%s\n",msg);
					fclose(f);
					return 0;
				}
			}
			else{
				if(get_domain(input)<0){sprintf(msg,"%s","데이터를 가져오는데 실패하였습니다.");}
				else{
					//fclose(f);
					f=fopen(ip_table,"a");//쓰기모드로 다시열
					fseek(f,0L,SEEK_END);//커서를 맨뒤로 이동

					fprintf(f,"%d %s %s %d",1,addr_name,domain_buffer,addr_count); 
				
					for(int ix=0;ix<addr_count;ix++){
						if(ix==addr_count-1) fprintf(f," %s\n",ip_buffer[ix]);
						else fprintf(f," %s",ip_buffer[ix]);	
						printf("%s\n",ip_buffer[ix]);
					}
					//fprintf(f," %c",'\n');
					sprintf(msg,"조회수 %d\n가명 %s\n도메인 %s\n주소갯수 %d",1,addr_name,domain_buffer,addr_count);
					for(int i=0;i<addr_count;i++){
						sprintf(msg,"%s\n ip주소 %s",msg,ip_buffer[i]);	
					}
					fclose(f);

					return 0;
				}
				

			}

		}

	return 0;
}
int clientInfo(int con_discon,char *d,char *t,char *clnt_addr)
{
	FILE *f;
	char table[9]="LogTable";

	f=fopen(table,"a");

	fprintf(f,"%d %s %s %s\n",con_discon,d,t,clnt_addr); 
	fclose(f);
}

int today(char *to_d,int on)
{
	time_t ltime;

	    struct tm *today;

	    char SDate[32];


	    time(&ltime);

	    today = localtime(&ltime);

	sprintf(SDate, "%04d-%02d-%02d",

		    today->tm_year + 1900,

		    today->tm_mon + 1, 
		    today->tm_mday);
	strcpy(to_d,SDate);
	if(on==1)printf("접속 날짜:%s\n",SDate);
	else if(on==0)printf("연결종료 날짜:%s\n",SDate);

	return 1;

}

int nowtime(char *now_t,int on)
{
	time_t ltime;

	    struct tm *today;

	    char STime[32];

	    time(&ltime);

	    today = localtime(&ltime);

	sprintf(STime, "%02d:%02d:%02d",

		    today->tm_hour,
		    today->tm_min,
		    today->tm_sec);
	strcpy(now_t,STime);

	if(on==1)printf("접속 시각 : %s \n",STime);
	else if(on==0)printf("연결종료 시각 : %s \n",STime);

	return 1;

}

int topDomain(char *msg)
{
	FILE *f;

	char hit_buffer[256];
	int hit;//조회수
	int top_hit=0;

	char line_buffer[256];
	char line[256];
	char *table_data; //split 테이블 데이

	if((f=fopen(ip_table,"r"))==NULL)
	{
		printf("file is not exist");
	}
	
	while(fgets(line_buffer,256,f)){
		strcpy(line,line_buffer);
		line_buffer[strlen(line_buffer)-1]='\0';

		table_data=strtok(line_buffer," ");	
		strcpy(hit_buffer,table_data);
		hit=atoi(hit_buffer);

		top_hit=(top_hit>hit)?top_hit:hit;
		if(top_hit==hit){
			strcat(msg,"조회수 가명 도메인 주소갯수 ip주소\n");
			strcat(msg,line);
			//strcat(msg,"\n");
		}
		else if(top_hit>hit){
			break;
		}
	}
	fclose(f);
	return 0;
		
}

int lowDomain(char *msg)
{

	FILE *f;

	char hit_buffer[256];
	int hit;//조회수
	int low_hit=0;
	int on=0;

	char line_buffer[256];
	char line[256];
	char *table_data; //split 테이블 데이

	if((f=fopen(ip_table,"r"))==NULL)
	{
		printf("file is not exist");
	}

	while(fgets(line_buffer,256,f)){
		strcpy(line,line_buffer);
		line_buffer[strlen(line_buffer)-1]='\0';

		table_data=strtok(line_buffer," ");	
		strcpy(hit_buffer,table_data);
		hit=atoi(hit_buffer);
		if(on==0){
			low_hit=hit;
			on++;
			memset(msg,0,BUF_SIZE);
			strcat(msg,"조회수 가명 도메인 주소갯수 ip주소\n");
			strcat(msg,line);
		}

		if(low_hit>hit){
			memset(msg,0,BUF_SIZE);
			strcat(msg,"조회수 가명 도메인 주소갯수 ip주소\n");
			strcat(msg,line);

		}
		if(low_hit==hit&&on>0){
			strcat(msg,line);
		}

	}
	fclose(f);

	return 0;
		
}
