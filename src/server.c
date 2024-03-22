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
	
	// 1. waiting for file size
	transferSize = sizeof(file_size);
	retval = recv(connfd, &file_size, sizeof(file_size), 0);
	if( retval != transferSize )
	{
		printf("receive file size error\r\n");
		ret = -1;
		goto exit;
	}
	
	// send ack
	transferSize = sizeof(unsigned int);
	retval = send(connfd, &ack, transferSize, 0);
	if( retval != transferSize )
	{
		printf("send ack error\r\n");
		ret = -1;
		goto exit;
	}
	printf("File size is %d\r\n", file_size);
	
	// 2. waiting for section size
	transferSize = sizeof(file_size);
	retval = recv(connfd, &section_size, transferSize, 0);
	if( retval != transferSize )
	{
		printf("receive section size error\r\n");
		ret = -1;
		goto exit;
	}
	
	// send ack
	transferSize = sizeof(unsigned int);
	retval = send(connfd, &ack, transferSize, 0);
	if( retval != transferSize )
	{
		printf("send ack error\r\n");
		ret = -1;
		goto exit;
	}
	printf("section size is %d\r\n", section_size);
	
	// receive data loop
	file_ptr = (unsigned char*)malloc(file_size);
	int index = 0;
	do
	{
		//printf("Receving index = %d\r\n", index);
		retval = recv(connfd, file_ptr+index, section_size, 0);
		if( retval != section_size )
		{
			printf("Receive data error\r\n");
			ret = -1;
			ack = TRANS_ACK_FAILURE;
		}
		
		// send ack
		transferSize = sizeof(unsigned int);
		retval = send(connfd, &ack, transferSize, 0);
		if( retval != transferSize || ret < 0)
		{
			printf("send ack error\r\n");
			ret = -1;
			goto exit;
		}
		
		index += section_size;
	}while(index < file_size);	
	
	printf("checksum = %08X\r\n", checksum(file_ptr, file_size));
	
	// receive next_comamnd
	transferSize = sizeof(int);
	retval = recv(connfd, &next_command, transferSize, 0);
	if( retval != transferSize )
	{
		printf("receive next command error\r\n");
		ret = -1;
		goto exit;
	}
	
	printf("Next comamnd=%d\r\n", next_command);
	
	// send ack
	transferSize = sizeof(unsigned int);
	retval = send(connfd, &ack, transferSize, 0);
	if( retval != transferSize )
	{
		printf("send ack error\r\n");
		ret = -1;
		goto exit;
	}
	
	ret = next_command;
	
exit:
	free(file_ptr);
	return ret;
}
   
// Driver function
int main(int argc, char **argv)
{
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
			next_command = jpegProcess(connfd);
		} while(next_command == CMD_NEXT_KEEPGOING);	

		// After chatting close the socket
		close(connfd);
	}
	
	close(sockfd);
}

