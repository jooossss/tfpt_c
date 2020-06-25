
#define DEBUG

int sendReadReq(char *pFilename, struct sockaddr_in *pRemoteAddr);
int sendDataAck(struct sockaddr_in *pPeeraddr,struct TFTPData* pData);
int sockfd = -1 ;

int main(int argc, char **argv)
{
	
	if(argc != 5){	
		printf("Usage :%s host port [get|put] filename\n",argv[0]);
		return 0;	
	}

	char *host = argv[1];
	int port = atoi(argv[2]);
	int opt = strcmp(argv[3],"get") == 0 ? 1 : 2;
	char *filename = argv[4];
	
	printf("DBG: host = %s, port = %d, opt = %d,filename = %s \n",
		host ,port,opt,filename);

	assert(1 == opt);

	struct sockaddr_in remoteaddr;
	remoteaddr.sin_family = PF_INET;
	struct hostent* remoteahost = NULL;
	if((remoteahost = gethostbyname(host)) == NULL){
		perror("gethostbyname()");	
		return -1;
	}
	remoteaddr.sin_addr = *((struct in_addr *) remoteahost->h_addr_list[0]);
	remoteaddr.sin_port = htons(port);
	struct protoent *proto;
	if((proto = getprotobyname("udp")) == NULL){
		fprintf(stderr, "getprotobyname(): failed to find udp info\n");
		return -1;
	}
	if((sockfd = socket(PF_INET,SOCK_DGRAM,proto->p_proto)) < 0){
		perror("socket()");
	}

	sendReadReq(filename,&remoteaddr);

	FILE *fp = fopen(filename ,"wb");
	if(fp == NULL){
		perror("fopen()");
		return 1;
	}
	
	struct sockaddr_in peeraddr;
	memset(&peeraddr,0,sizeof(peeraddr));
	
	while(1){
		int bufferlen = 0;
		while(bufferlen == 0){
			ioctl(sockfd,FIONREAD,&bufferlen);		
		}
		printf("DBG :bufferlen = %d\n",bufferlen);
		
		void *buffer = malloc(bufferlen);
		int recvLen = sizeof(peeraddr);
		int recvlen = recvfrom(sockfd,buffer,bufferlen,0,(struct sockaddr*)&peeraddr,&recvLen);
		if(recvlen < 0){
			perror("recvfrom()");
			return 1;
		}

		struct TFTPHeader *header = (struct TFTPHeader*) buffer;
		if(ntohs(header->opcode) == OPCODE_ERR){
			struct TFTPERR* err = (struct TFTPERR*) buffer;
			printf("Error packet dected with code %d\n",ntohs(err->errcode));
			return 1;		
		}
			
		if(ntohs(header->opcode) == OPCODE_DATA){
			struct TFTPData *data = (struct TFTPData *) buffer;
			printf("Receiving block %d\n",ntohs(data->block));
			fwrite(&data->data, recvlen - sizeof(struct TFTPHeader) - sizeof(short) ,1,fp);
			sendDataAck(&peeraddr,data);		
		
		}
		
		if(recvlen < BLOCKSIZE + sizeof(struct TFTPHeader) + sizeof(short)){
			fclose(fp);
			return 0;
		}
		
		free(buffer);
	}

	return 0;
}



int sendReadReq(char *pFilename, struct sockaddr_in *pRemoteAddr)
{
	struct TFTPHeader header;
	header.opcode = htons(OPCODE_RRQ);
	int filenamelen = strlen(pFilename) + 1;
	int packetsize = sizeof(header) + filenamelen + 5 + 1;
	void *packet = malloc(packetsize);	// mode = "octet"
	memcpy(packet, &header, sizeof(header));
	memcpy(packet + sizeof(header), pFilename, filenamelen);
	char *mode = "octet";
	memcpy(packet + sizeof(header) + filenamelen, mode, strlen(mode) + 1);
 
	if(sendto(sockfd, packet, packetsize, 0,
		(struct sockaddr*)pRemoteAddr, sizeof(struct sockaddr)) < 0){
		perror("sendto()");
		return 1;
	}
	return 0;
}


int sendDataAck(struct sockaddr_in *pPeeraddr,struct TFTPData* pData){

	struct TFTPACK ack;
	ack.header.opcode = htons(OPCODE_ACK);
	ack.block = pData->block;
	if(sendto (sockfd,&ack,sizeof(ack),0,
		(struct sockaddr*)pPeeraddr, sizeof(struct sockaddr_in)) < 0){
		perror("ack send()");
		return 1;
	}
	
	return 0;
}



