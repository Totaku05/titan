#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>

int flag = 0;

int wasgrep = 0;

char *filecat = NULL;
char *greppedline = NULL;

void parseFilePath(char *filepath, size_t *len);

void headers(int client, int size, int httpcode) {
	char buf[1024];
	char strsize[20];
	sprintf(strsize, "%d", size);
	if (httpcode == 200) {
		strcpy(buf, "HTTP/1.0 200 OK\r\n");
	}
	else if (httpcode == 404) {
		strcpy(buf, "HTTP/1.0 404 Not Found\r\n");
	}
	else {
		strcpy(buf, "HTTP/1.0 500 Internal Server Error\r\n");
	}
	send(client, buf, strlen(buf), 0);
	strcpy(buf, "Connection: keep-alive\r\n");
	send(client, buf, strlen(buf), 0);
	strcpy(buf, "Content-length: ");
	send(client, buf, strlen(buf), 0);
	strcpy(buf, strsize);
	send(client, buf, strlen(buf), 0);
	strcpy(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
	strcpy(buf, "simple-server");
	send(client, buf, strlen(buf), 0);
	if(!flag)
		sprintf(buf, "Content-Type: text/html\r\n");
	else
		sprintf(buf, "Content-Type: jpeg/jpg\r\n");
	send(client, buf, strlen(buf), 0);
	strcpy(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
}


void parseFileName(char *line, char **filepath, size_t *len) {
	char *start = NULL;
	while ((*line) != '/') line++;
	start = line + 1;
	while ((*line) != ' ') line++;
	(*len) = line - start;
	*filepath = (char*)malloc(*len + 1);
	*filepath = strncpy(*filepath, start, *len);
	(*filepath)[*len] = '\0';
	parseFilePath(*filepath, len);
	printf("%s \n", *filepath);
}

void parseFilePath(char *filepath, size_t *len)
{
	int i = 0;
	int j;
	int k = 0;
	int filepathh;	
	//cat 	
	if(strstr(filepath, "cat") != NULL)
		i = 3;
	//
	char *line = (char*)malloc(strlen(filepath) - i);

	for(j = i; j < *len; j++)
		line[k++] = filepath[j];
	//grep	
	k = 0;
	while((k < strlen(line)) && (line[k++] != "|"));

	if(k != strlen(line))
	{
		int z = 0;
		filepathh = k - 1;
		filecat = malloc(filepathh);
		for(j = 0; j < filepathh; j++)
			filecat[z++] = line[j];		
		
		k++;
		char *grep = (char*)malloc(4);
		grep[0] = line[k++];
		grep[1] = line[k++];
		grep[2] = line[k++];
		grep[3] = line[k++];

		if(strstr(grep, "grep") ! = NULL)
		{
			wasgrep = 1;
			greppedline = (char*)malloc(strlen(line) - k);
			for(j = k; j < strlen(line); j++)
				greppedline[j - k] = line[k];
		}

	}

	
}

void grepp(FILE *file, int cd)
{
	ssize_t len = 0;
	ssize_t read;
	char *line = NULL;
	int res = 0;
	while ((read = getline(&line, &len, file)) != -1)
	{
		if (strstr(line, greppedline) != NULL)
		{
			res = send(cd, line, len, 0);
			if (res == -1) {
				printf("send error \n");
			}
		}
	}
}

int main() {
	int ld = 0;
	int res = 0;
	int cd = 0;
	int filesize = 0;
	const int backlog = 10;
	struct sockaddr_in saddr;
	struct sockaddr_in caddr;
	char *line = NULL;
	size_t len = 0;
	char *filepath = NULL;
	size_t filepath_len = 0;
	int empty_str_count = 0;
	socklen_t size_saddr;
	socklen_t size_caddr;
	FILE *fd;
	FILE *file;

	ld = socket(AF_INET, SOCK_STREAM, 0);
	if (ld == -1) {
		printf("listener create error \n");
	}

	//exp
	bzero((char *) &saddr, sizeof(saddr));
	//endofexp

	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(8080);
	saddr.sin_addr.s_addr = INADDR_ANY;
	res = bind(ld, (struct sockaddr *)&saddr, sizeof(saddr));
	if (res == -1) {
		printf("bind error \n");
	}
	res = listen(ld, backlog);
	if (res == -1) {
		printf("listen error \n");
	}
	//exp
	int clilen = sizeof(caddr);
	//endofexp
	while (1) {
		cd = accept(ld, (struct sockaddr *)&caddr, &clilen /*&size_caddr*/);
		printf("Accept %s\n", strerror(errno));
		if (cd == -1) {
			printf("accept error \n");
		}
		printf("client in %d descriptor. Client addr is %d \n", cd, caddr.sin_addr.s_addr);
		fd = fdopen(cd, "r");
		if (fd == NULL) {
			printf("error open client descriptor as file \n");
		}
		while ((res = getline(&line, &len, fd)) != -1) {
			if (strstr(line, "GET")) {
				parseFileName(line, &filepath, &filepath_len);
			}
			if (strcmp(line, "\r\n") == 0) {
				empty_str_count++;
			}
			else {
				empty_str_count = 0;
			}
			if (empty_str_count == 1) {
				break;
			}
			printf("%s", line);
		}
		//cat		
		printf("cat %s \n", filecat);
		
		file = fopen(filecat, "r");
		if(wasgrep != null)
			grepp(file, cd);	
		else	
		//endcat		
		if (file == NULL) {
			printf("404 File Not Found \n");
			headers(cd, 0, 404);
		}
		else {
			fseek(file, 0L, SEEK_END);
			filesize = ftell(file);
			fseek(file, 0L, SEEK_SET);
			headers(cd, filesize, 200);
			while (getline(&line, &len, file) != -1) {
				res = send(cd, line, len, 0);
				if (res == -1) {
					printf("send error \n");
				}
			}
		}
		close(cd);
	}
	return 0;
}
