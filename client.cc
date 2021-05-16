#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

#define MAXBUFLEN 65535


// ./client input_file eelab5.kaist.ac.kr 12345
// file up to 4MB

int main (int argc, char* argv[]){

    /* Change as you wish */
	int nodefd;
	struct sockaddr_in node_addr;
	struct hostent* node_ip;
	int node_port;
	FILE* file_ptr = fopen(argv[1], "r");
	int read_cnt = 0;
	char send_buf[MAXBUFLEN];
	char recv_buf[MAXBUFLEN];
	char file_size_buf[20];
	FILE* new_fp;
	int flag;
	int file_size = 0;
	char whole_size_buf[10];
	int whole_size = 0;
	int read_cnt2 = 0;
	int whole_read_cnt = 0;
	char* checker;
	int check_cnt = 0;
	// 1: socket
	nodefd = socket(AF_INET, SOCK_STREAM, 0);
	if(nodefd < 0){
		fprintf(stderr, "client// socket: failed\n");
		exit(1);
	}
	// 2: connect
	node_ip = gethostbyname(argv[2]);
	node_port = atoi(argv[3]);
	memset(&node_addr, 0, sizeof(node_addr));
	node_addr.sin_family = AF_INET;
	node_addr.sin_port = htons(node_port);
	node_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)node_ip->h_addr));
	
	if(connect(nodefd, (struct sockaddr*)&node_addr, sizeof(node_addr)) < 0){
		fprintf(stderr, "client// connect: failed\n");
		exit(1);
	}
	
	file_ptr = fopen(argv[1], "r");
	fseek(file_ptr, 0, SEEK_END);
	file_size = ftell(file_ptr);
	printf("file_size: %d\n", file_size);
	fclose(file_ptr);
	// tell file size to supernode
	sprintf(file_size_buf, "%d", file_size);
	write(nodefd, file_size_buf, strlen(file_size_buf));
		
	// 3: write file to supernode
	file_ptr = fopen(argv[1], "r");
	while(feof(file_ptr) == 0){
		memset(send_buf, 0, MAXBUFLEN);
		read_cnt = fread(send_buf, sizeof(char), MAXBUFLEN, file_ptr);
		//send_buf[read_cnt] = '\0';
		write(nodefd, send_buf, strlen(send_buf));
	}
	
	// make socket nonblock
	//flag = fcntl(nodefd, F_GETFL, 0);
	//fcntl(nodefd, F_SETFL, flag | O_NONBLOCK);
	// 4: receive result file from supernode
	new_fp = fopen("translated.txt", "w");
	/*while(read(nodefd, recv_buf, MAXBUFLEN)){
		memset(recv_buf, 0, MAXBUFLEN);
		fwrite(recv_buf, sizeof(char), read_cnt, new_fp); 
	}*/
	//read the whole size 
	memset(recv_buf, 0, MAXBUFLEN);
	read_cnt2 = read(nodefd, recv_buf, MAXBUFLEN);
	printf("recv_buf: %s\n", recv_buf);
	whole_read_cnt += read_cnt2;
	checker = recv_buf;
	while(1){
		if((*checker) == ':') break;
		check_cnt++;
		checker++;
	}
	fwrite(recv_buf + check_cnt + 1, 1, read_cnt2 - check_cnt - 1, new_fp);//
	strncpy(whole_size_buf, recv_buf, check_cnt);
	printf("whole_size_buf: %s, whole_read_cnt: %d\n", whole_size_buf, whole_read_cnt);
	whole_size = atoi(whole_size_buf);
	printf("whole size: %d\n", whole_size);
	while(1){
		if(whole_read_cnt >= whole_size) break;
		printf("whole_read_cnt: %d, whole_size: %d\n", whole_read_cnt, whole_size);
		memset(recv_buf, 0, MAXBUFLEN);
		read_cnt2 = read(nodefd, recv_buf, MAXBUFLEN);//
		whole_read_cnt += read_cnt2;
		printf("recv_buf: %s\n", recv_buf);
		fwrite(recv_buf, 1, read_cnt, new_fp);//
	}
	fclose(new_fp);

	// 5: close connection
	close(nodefd);
     
    return 0;
}
