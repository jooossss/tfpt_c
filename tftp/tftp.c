#include "tftp.h"


struct sockaddr_in remoteaddr;
socklen_t remoteaddrlen = sizeof(remoteaddr);

int sockfd = -1;

int main(){
	struct servent *serv;
	if((serv = getservbyname("tftp","udp")) == NULL){
		fprintf(stderr, "getservbyname(): failed to find domain service info.\n");
		return -1;
	}
	
	struct sockaddr_in localaddr;
	localaddr.sin_family = PF_INET;
	if((localaddr.sin_addr.s_addr = INADDR_ANY) == -1){
			perror("inet_addr()");
			return -1;
	}		
	localaddr.sin_port = htons(6969);
	printf("DBG: tffp port = %d\n",ntohs(localaddr.sin_port));
	
	struct protoent *proto;
	if((proto = getprotobyname("udp")) == NULL){
		fprintf(stderr,"getprotobyname(): failed to find udp protocol info\n");
		return -1;
	}
	if((sockfd = socket(PF_INET,SOCK_DGRAM,proto->p_proto)) < 0){
		perror("socket()");
	}
	if(bind(sockfd,(const struct sockaddr*)&localaddr,sizeof(localaddr)) < 0){
		perror("bind()");
	}
	
	int bufferlen = 512 + 2 + 2;
	void *buffer = malloc(bufferlen);
	if(recvfrom(sockfd,buffer,bufferlen,0,(struct sockaddr*)&remoteaddr,&remoteaddrlen) < 0){
		perror("recvfrom()");
	}
	struct TFTPHeader *header = (struct TFTPHeader*)buffer;
	switch(ntohs(header->opcode)){
		case OPCODE_RRQ:
			tftpread(buffer,bufferlen);
			break;
		case OPCODE_WRQ:
			tftpwrite(buffer,bufferlen);
			break;
		default:
			fprintf(stderr,"Incorrect opcode: %d\n",ntohs(header->opcode));
			break;
	}
}


tftpread(void *buffer,int bufferlen){
	printf("DBG:Dected Opcode = RRQ.\n");
	struct TFTPWRRQ request;
	request.filename = (char*)buffer + sizeof(struct TFTPHeader);
	printf("DBG:filename =%s\n",request.filename);
	request.mode = (char*)buffer + sizeof(struct TFTPHeader)+strlen(request.filename)+1;
	printf("DBG:mode =%s\n",request.mode);
	
	if(strcmp(request.mode,"octet") != 0){
		fprintf(stderr,"only binary mode is supported \n");
		return 1;
	}
	
	printf("DBG: remote port = %d\n",ntohs(remoteaddr.sin_port));
	
	struct stat f_stat;
	if(stat(request.filename,&f_stat) < 0){
		perror("stat()");
		return 1;
	}
	
	printf("DBG: i-node number = %d\n",f_stat.st_ino);
	printf("DBG: filesize = %ld\n",f_stat.st_size);
	
	FILE *fp = fopen(request.filename,"rb");
	if(fp == NULL){
		perror("open()");
		return 1;
	}
	
	printf("DBG: file open\n");
	int blockno = f_stat.st_size / BLOCKSIZE + 1;
	int i;	
	for(i = 1; i<=blockno;i++){
		printf("DBG: sending block %d\n",i);
		struct TFTPData data;
		data.header.opcode = htons(OPCODE_DATA);
		data.block = htons(i);
		
		void *filebuffer;
		filebuffer = malloc(BLOCKSIZE);
		int r = fread(filebuffer,1,BLOCKSIZE,fp);
		int datapkgsize = r+2+2;
		void *buffer = malloc(datapkgsize);
		memcpy(buffer,&data,sizeof(data));
		memcpy(buffer + 2 + 2,filebuffer,r);
		
		int ret = sendto(sockfd,buffer,datapkgsize,0,
		(struct sockaddr*)&remoteaddr,sizeof(remoteaddr));
		printf("DBG: sendto() return %d\n",ret);
		struct TFTPACK ack;
		if(recvfrom(sockfd,&ack,sizeof(ack),0,NULL,0) < 0){
			perror("recvfrom");
			return 1;
		}
		if(ntohs(ack.block) != i){
			printf("ack err.\n");
			return 1;
		}else{
			printf("ack checked \n");
		}
	}
	
	
}


tftpwrite(void *buffer,int bufferlen){

	return 1;
}


