#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "color.h"

#define DEBUG_TIME

#ifdef DEBUG_TIME
#include <sys/time.h>
#include <unistd.h>
#endif

#define MAX 80
#define PORT 1812
#define TRANSMISSION_PACKAGE_SIZE	1024
#define TRANS_ACK_SUCCESS	0x55AA00
#define TRANS_ACK_FAILURE	0x55AA01

#define CMD_NEXT_KEEPGOING	1
#define CMD_NEXT_FINISHED	2

#define TCP_DELAY	20
#define BUFFER_CACHE_SIZE	64*1024

#define SA struct sockaddr
   
unsigned int checksum(unsigned char *buf, int length)
{
	unsigned int sum = 0;
	
	for(int i=0; i<length; i++)
	{
		sum += (unsigned int)(*(buf+i));
	}
	
	return sum;
}

int show_ip(const char *adapter)
{
	int fd;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	/* I want to get an IPv4 IP address */
	ifr.ifr_addr.sa_family = AF_INET;

	/* I want IP address attached to "eth0" */
	strncpy(ifr.ifr_name, adapter, IFNAMSIZ-1);

	if( ioctl(fd, SIOCGIFADDR, &ifr) < 0 )
	{
		printf("%s is not available\n", adapter);
	}
	else
	{
		printf("Server IP (this board) : %s\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
	}

	close(fd);
}

int recvPackage(int connfd, void *buf, int recv_size)
{
	int ack = TRANS_ACK_SUCCESS;
	int byteLeft;
	int retval;
	
	byteLeft = recv_size;
	int index = 0;
	while (index < recv_size)
	{
		retval = recv(connfd, buf+index, byteLeft, 0);
		
		if( retval == -1 )
		{
			printf("receive packet error\r\n");
			ack = TRANS_ACK_FAILURE;
			break;
		}
		
		index += retval;
		byteLeft -= retval;
	}
	
	// send ack
	byteLeft = sizeof(unsigned int);
	retval = send(connfd, &ack, byteLeft, 0);
	if( retval != byteLeft )
	{
		printf("send ack error\r\n");
		ack = TRANS_ACK_FAILURE;
	}
	
	return (ack == TRANS_ACK_SUCCESS) ? recv_size : -1;
}

// Function designed for chat between client and server.
int jpegProcess(int connfd)
{
    unsigned char *file_ptr;
    int file_size;
    int next_command;
    unsigned int ack = TRANS_ACK_SUCCESS;
    int count=0;
    char filename[80];

	int section_size;
	int ret = 0;
	int retval;
	int transferSize;

	printf(RED "Start receving file\r\n" NONE);
	
	// 1. receive file size
	retval = recvPackage(connfd, (void*)&file_size, sizeof(file_size));
	if( retval < 0 )
	{
		printf("receive file size error\r\n");
		ret = -1;
		goto exit;
	}
	printf("File size is %d\r\n", file_size);

	// 2. receive section size
	retval = recvPackage(connfd, (void*)&section_size, sizeof(section_size));
	if( retval < 0 )
	{
		printf("receive section size error\r\n");
		ret = -1;
		goto exit;
	}
	printf("section size is %d\r\n", section_size);
	
	// 3. receive data loop
	file_ptr = (unsigned char*)malloc(file_size);
	int index = 0;
	do
	{
		int len = ((index+section_size) > file_size) ? (file_size-index) : section_size;
		
		retval = recvPackage(connfd, (void*)(file_ptr+index), len);
		//printf("Received %d bytes\r\n", retval);
		if( retval != len )
		{
			printf("Receive data error\r\n");
			ret = -1;
			goto exit;
		}

		index += section_size;
	}while(index < file_size);	
	
	//printf("checksum = %08X\r\n", checksum(file_ptr, file_size));
	
	// receive next_comamnd
	transferSize = sizeof(next_command);
	retval = recvPackage(connfd, (void*)&next_command, transferSize);
	if( retval != transferSize )
	{
		printf("receive next command error\r\n");
		ret = -1;
		goto exit;
	}
	printf("Next comamnd=%d\r\n", next_command);	
	ret = next_command;
	
exit:
	free(file_ptr);
	return ret;
}
   
// Driver function
int main(int argc, char **argv)
{
#ifdef DEBUG_TIME
	struct timeval start, end;
#endif

	if( argc != 2 )
	{
		printf("USAGE: server PORT_NUMBER\r\n");
		return 0;
	}
	
	show_ip("eth0");

	int sockfd, connfd, len;
	struct sockaddr_in servaddr, cli;
	int port = atoi(argv[1]);

	// socket create and verification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		printf("socket creation failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully created..\n");
	
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);

	int nRecvBuf=BUFFER_CACHE_SIZE;
	setsockopt(sockfd,SOL_SOCKET,SO_RCVBUF,(const char*)&nRecvBuf,sizeof(int));
	int nSendBuf=BUFFER_CACHE_SIZE;
	setsockopt(sockfd,SOL_SOCKET,SO_SNDBUF,(const char*)&nSendBuf,sizeof(int));

	// Binding newly created socket to given IP and verification
	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
		printf("socket bind failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully binded..\n");

	// Now server is ready to listen and verification
	if ((listen(sockfd, 5)) != 0) {
		printf("Listen failed...\n");
		exit(0);
	}
	else
		printf("Server listening..\n");
	
	len = sizeof(cli);

	// Accept the data packet from client and verification
	while( (connfd = accept(sockfd, (SA*)&cli, &len)) >= 0 )
	{
		printf("server accept the client...\n");

		int next_command;
		do
		{
			// Function for chatting between client and server
#ifdef DEBUG_TIME
			gettimeofday(&start, NULL);
#endif			
			next_command = jpegProcess(connfd);
#ifdef DEBUG_TIME
			gettimeofday(&end, NULL);
			
			long long time_es = ((long long)end.tv_sec * 1000000 + end.tv_usec) -
					((long long)start.tv_sec * 1000000 + start.tv_usec);
			printf(RED "Transfer time = %lld.%lld\r\n" NONE , time_es/1000, time_es%1000);
#endif			
		} while(next_command == CMD_NEXT_KEEPGOING);	

		// After chatting close the socket
		close(connfd);
	}
	
	close(sockfd);
}
