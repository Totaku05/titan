#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <netinet/in.h>

#include <string.h>

int main(int argc, char *argv[]) {
	int sockfd, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	   
	char buffer[256];
	char res[1000];
	   
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	   
	if (sockfd < 0) {
		perror("ERROR opening socket");
		exit(1);
	}
	
	server = gethostbyname(argv[1]);
	   
	if (server == NULL) {
		fprintf(stderr,"ERROR, no such host\n");
		exit(0);
	}
	   
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(8080);

	if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR connecting");
		exit(1);
	}
	while(1)
	{	
		printf("Please enter the message: ");
		bzero(buffer,256);
		fgets(buffer,255,stdin);
		   
		n = write(sockfd, buffer, strlen(buffer));
		   
		if (n < 0) {
			perror("ERROR writing to socket");
			exit(1);
		}

		bzero(res,1000);
		n = read(sockfd, res, 1000);
		   
		if (n < 0) {
			perror("ERROR reading from socket");
			exit(1);
		}
	
		printf("%s\n", res);

		bzero(buffer, 256);
		if(recv(sockfd, &buffer, sizeof(buffer), 0) == 0)
			break;
	}
	return 0;
}
