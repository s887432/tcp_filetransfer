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
#define JPEG_END_NUM	JPEG_START_NUM+1

unsigned int checksum(unsigned char *buf, int length)
{
	unsigned int sum = 0;
	
	for(int i=0; i<length; i++)
	{
		sum += (unsigned int)(*(buf+i));
	}
	
	return sum;
}

int sendPackage(int sockfd, void *buf, int send_size)
{
	int byteLeft;
	int ack = TRANS_ACK_SUCCESS;
	int retval;
	
	byteLeft = send_size;
	
	int index = 0;
	while( index < send_size )
	{
		retval = send(sockfd, buf+index, byteLeft, 0);
		if( retval == -1 )
		{
			printf("Send package error error\r\n");
			ack = TRANS_ACK_FAILURE;	
			break;
		}
		
		index += retval;
		byteLeft -= retval;
	}
	
	// waiting for ack
	byteLeft = sizeof(int);
	retval = recv(sockfd, &ack, byteLeft, 0);
	if( retval != byteLeft || ack != TRANS_ACK_SUCCESS)
	{
		printf("%d@%d, %X\r\n", retval, byteLeft, ack);
		printf("receive ack error\r\n");
	}
	
	return (ack == TRANS_ACK_SUCCESS) ? send_size : -1;	
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

	// read file data
	file_ptr = (unsigned char*)malloc(file_size);
	fread(file_ptr, file_size, 1, fp);
	fclose(fp);
	
	// 1. send file size
	printf("File size %d bytes\r\n", file_size);
	retval = sendPackage(sockfd, (void*)&file_size, sizeof(file_size));
	if( retval != sizeof(file_size) )
	{
		printf("Send file size error\r\n");
		ret = -1;
		goto exit;
	}
	
	// 2. send section size	
	printf("Section size %d bytes\r\n", section_size);
	retval = sendPackage(sockfd, (void*)&section_size, sizeof(section_size));
	if( retval != sizeof(section_size) )
	{
		printf("Send section size error\r\n");
		ret = -1;
		goto exit;
	}
	
	// 3. send file data
	printf("checksum = %08X\r\n", checksum(file_ptr, file_size));
	do
	{
		int len = ((index+section_size) > file_size) ? (file_size-index) : section_size;
		retval = sendPackage(sockfd, (void*)(file_ptr+index), len);
		printf("Sent %d bytes\r\n", retval);
		if( retval != len )
		{
			printf("Receive data error\r\n");
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
	
	printf("Next command=%X\r\n", next_command);
	// 1. send next command
	retval = sendPackage(sockfd, (void*)(&next_command), sizeof(next_command));
	if( retval != sizeof(next_command) )
	{
		printf("Send ack error\r\n");
		return -1;
	}

	return ret;
}

void func(int sockfd, int section_size, const char *listname)
{
	unsigned char *file_ptr;
	int file_size;
	int next_command;
	unsigned int ack = TRANS_ACK_SUCCESS;
	// debug
	char filename[80];
	int ret;

#if 1
	FILE *fp;
	
	if( (fp = fopen(listname, "r")) == NULL )
	{
		printf("Read list file error\r\n");
		return;
	}
	
	while( fgets(filename, 80, fp) )
	{
		int len = strlen(filename);
		for(int i=len-1; i>0; i--)
		{
			if( filename[i] <= 0x20 )
				filename[i] = 0;
		}
		
		printf("Reading [%s]\r\n", filename);
		
		if( strcmp(filename, "EOF") == 0 )
		{
			file_size = -1;
			sendPackage(sockfd, (void*)&file_size, sizeof(file_size));
			
			// end of file
			next_command = CMD_NEXT_FINISHED;
		}
		else
		{
			ret = sendFile(sockfd, section_size, filename);
			if( ret < 0 )
			{
				break;
			}
			
			next_command = CMD_NEXT_KEEPGOING;
			// send next com
			sendNextCommand(sockfd, next_command);
		}	
	}
	
	fclose(fp);
#else
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
#endif
}
   
int main(int argc, char **argv)
{
	if( argc != 5 )
	{
		printf("USAGE: client SERVER_IP PORT SECTION_SIZE(KB) LIST_NAME\r\n");
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
   
   	int nRecvBuf=32*1024;
	setsockopt(sockfd,SOL_SOCKET,SO_RCVBUF,(const char*)&nRecvBuf,sizeof(int));
	int nSendBuf=32*1024;
	setsockopt(sockfd,SOL_SOCKET,SO_SNDBUF,(const char*)&nSendBuf,sizeof(int));
	
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
    func(sockfd, section_size*1024, argv[4]);
   
    // close the socket
    close(sockfd);
}
