#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>

#define PORT 1812
#define TRANSMISSION_PACKAGE_SIZE	1024*2
#define TRANS_ACK_SUCCESS	0x55AA00
#define TRANS_ACK_FAILURE	0x55AA01
#define TRANS_RETRY 0x55AA02

#define CMD_NEXT_KEEPGOING	1
#define CMD_NEXT_FINISHED	2

#define SA struct sockaddr

#define JPEG_START_NUM	3000
#define JPEG_END_NUM	JPEG_START_NUM+10

unsigned int checksum(unsigned char *buf, int length)
{
	unsigned int sum = 0;
	
	for(int i=0; i<length; i++)
	{
		sum += (unsigned int)(*(buf+i));
	}
	
	return sum;
}

int sendFile(int sockfd, int section_size, const char *filename)
{
	int file_size;
	unsigned char *file_ptr;
	int retval;
	int transferSize;
	int ack;
	int ret = 0;
	int index = 0;
	
	// get file size
	FILE *fp = fopen(filename, "rb");
	fseek(fp, 0L, SEEK_END);
	file_size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	printf("Read %d bytes\r\n", file_size);
	
	// read file data
	file_ptr = (unsigned char*)malloc(file_size);
	fread(file_ptr, file_size, 1, fp);
	fclose(fp);
	
	// 1. send file size
	transferSize = sizeof(int);
	retval = send(sockfd, &file_size, transferSize, 0);
	if( retval != transferSize )
	{
		printf("Send file size error\r\n");
		ret = -1;
		goto exit;
	}
	// waiting for ack
	transferSize = sizeof(int);
	retval = recv(sockfd, &ack, transferSize, 0);
	if( retval != transferSize || ack != TRANS_ACK_SUCCESS)
	{
		printf("%d@%d, %X\r\n", retval, transferSize, ack);
		printf("receive ack error\r\n");
		ret = -1;
		goto exit;
	}
	
	// 2. send section size	
	transferSize = sizeof(int);
	retval = send(sockfd, &section_size, transferSize, 0);
	if( retval != transferSize )
	{
		printf("Send section size error\r\n");
		ret = -1;
		goto exit;
	}
	// waiting for ack
	transferSize = sizeof(ack);
	retval = recv(sockfd, &ack, transferSize, 0);
	if( retval != transferSize || ack != TRANS_ACK_SUCCESS)
	{
		printf("%d@%d, %X\r\n", retval, transferSize, ack);
		printf("receive ack error\r\n");
		ret = -1;
		goto exit;
	}
	
	// 3. send file data
	printf("checksum = %08X\r\n", checksum(file_ptr, file_size));
	do
	{
		//printf("Sending index = %d\r\n", index);
		transferSize = section_size;
		retval = send(sockfd, file_ptr+index, section_size, 0);
		if( retval != section_size )
		{
			printf("Send data error (%d)\r\n", retval);
			ret = 1;
			break;
		}
		
		// receive ack
		transferSize = sizeof(int);
		retval = recv(sockfd, &ack, transferSize, 0);
		if( retval != transferSize || ack != TRANS_ACK_SUCCESS)
		{
			printf("%d@%d, %X\r\n", retval, transferSize, ack);
			printf("receive ack error\r\n");
			ret = -1;
			goto exit;
		}
		
		index += section_size;
	}while(index < file_size);

exit:	
	free(file_ptr);
	
	return ret;
}

int sendNextCommand(int sockfd, int next_command)
{
	int transferSize = 0;
	int retval = 0;
	int ret = 0;
	int ack = 0;
	
	// 1. send next command
	transferSize = sizeof(int);
	retval = send(sockfd, &next_command, transferSize, 0);
	if( retval != transferSize )
	{
		printf("Send command error\r\n");
		ret = -1;
	}
	
	if( ret >= 0 )
	{
		// waiting for ack
		transferSize = sizeof(int);
		retval = recv(sockfd, &ack, transferSize, 0);
		if( retval != transferSize || ack != TRANS_ACK_SUCCESS)
		{
			printf("%d@%d, %X\r\n", retval, transferSize, ack);
			printf("receive ack error\r\n");
			ret = -1;
		}
	}
	
	return ret;
}

void func(int sockfd, int section_size)
{
	unsigned char *file_ptr;
	int file_size;
	int next_command;
	unsigned int ack = TRANS_ACK_SUCCESS;
	// debug
	char filename[80];
	int ret;

	//for (int i=3000; i<=3641; i++)
	for (int i=JPEG_START_NUM; i<=JPEG_END_NUM; i++)
	{
		sprintf(filename, "%d.jpg", i);
    	printf("Reading: %s\r\n", filename);
    	
		ret = sendFile(sockfd, section_size, "3000.jpg");
		if( ret < 0 )
		{
			break;
		}
		
		if( i < JPEG_END_NUM )
		{
			next_command = CMD_NEXT_KEEPGOING;
		}
		else
		{
			next_command = CMD_NEXT_FINISHED;
		}
		// send next com
		sendNextCommand(sockfd, next_command);
	}
}
   
int main(int argc, char **argv)
{
	if( argc != 4 )
	{
		printf("USAGE: client SERVER_IP PORT SECTION_SIZE(KB)\r\n");
		return EXIT_FAILURE;
	}
	
	int section_size = atoi(argv[3]);
	int port = atoi(argv[2]);
	
    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;
   
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
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    servaddr.sin_port = htons(port);
   
    // connect the client socket to server socket
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }
    else
        printf("connected to the server..\n");
   
    // function for chat
    func(sockfd, section_size*1024);
   
    // close the socket
    close(sockfd);
}

