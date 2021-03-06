#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#define COUNT 5

pthread_t threads[COUNT];
pthread_mutex_t mut[COUNT];
int cds[COUNT];

void *func(void *arg)
{
	int id = *((int *)arg);
	pthread_mutex_lock(&mut[id]);
	pthread_mutex_lock(&mut[id]);
	while(1)
	{
		char command[256];
		bzero(command, 256);
		send(cds[id], "hi\0", strlen("hi\0"), 0);
		recv(cds[id], &command, sizeof(command), 0);
		command[254] = '\0';
		if(strstr(command, "stop\0") != NULL)
		{
			send(cds[id], "stop\0", strlen("stop\0"), 0);
			break;
		}
		FILE *process = popen(command, "r");
		if(process == NULL)
		{
			char *message = "Not correct command";
			send(cds[id], message, strlen(message), 0);
		}
		printf("Thread %d execute %s - command of %d client\n", id, command, cds[id]);
		char buf[1000];
		while(fgets(buf, sizeof(buf), process) != NULL){
			printf("%s\n", buf);send(cds[id], buf, strlen(buf), 0);recv(cds[id], &command, 					sizeof(command), 0);}
		pclose(process);
		send(cds[id], "stop\0", strlen("stop\0"), 0);
	}
	close(cds[id]);
	pthread_mutex_unlock(&mut[id]);
}

void initialize()
{
	int i;
	for(i = 0; i < COUNT; i++)
	{
		pthread_mutex_init(&mut[i], NULL);
		pthread_create(&threads[i], NULL, func, (void *) &i);
		sleep(1);
	}
}

int main() {
	int ld = 0;
	int res = 0;
	int cd = 0;
	const int backlog = 10;
	struct sockaddr_in saddr;
	struct sockaddr_in caddr;
	socklen_t size_saddr;
	socklen_t size_caddr;
	
	ld = socket(AF_INET, SOCK_STREAM, 0);
	if (ld == -1) {
		printf("listener create error \n");
	}

	bzero((char *) &saddr, sizeof(saddr));
	
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
	int clilen = sizeof(caddr);

	initialize();

	while (1) {
		cd = accept(ld, (struct sockaddr *)&caddr, &clilen);
		printf("Accept %s\n", strerror(errno));
		if (cd == -1) {
			printf("accept error \n");
		}
		printf("client in %d descriptor. Client addr is %d \n", cd, caddr.sin_addr.s_addr);
		int i;
		for(i = 0; i < COUNT; i++)
		{
			if(pthread_mutex_trylock(&mut[i]))
			{
				cds[i] = cd;
				pthread_mutex_unlock(&mut[i]);
				pthread_mutex_unlock(&mut[i]);
				break;
			}
			else
			{
				pthread_kill(threads[i], 0);
				pthread_mutex_destroy(&mut[i]);
				pthread_mutex_init(&mut[i], NULL);
				pthread_create(&threads[i], NULL, func, (void *) &i);
			}
		}

	}
	return 0;
}
