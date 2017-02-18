#ifndef _COMMON_H
#define _COMMON_H
 
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <signal.h>
#include <netdb.h>
#include <pthread.h>
#include "constants.h"

#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))

#define HERR(source) (fprintf(stderr, "%s(%d) at %s:%d\n", source, h_errno, __FILE__, __LINE__),\
					  exit(EXIT_FAILURE))

#define BACKLOG 3

ssize_t bulk_read(int fd, char *buf, size_t count);
ssize_t bulk_write(int fd, char *buf, size_t count);

void sethandler(void (*f)(int), int sigNo);

int make_socket(void);
int add_new_client(int sfd);
int bind_tcp_socket(uint16_t port);
struct sockaddr_in make_address(char *address, uint16_t port);
int connect_socket(struct sockaddr_in *addr);
int send_message_to_socket(int socket, char *msg);



#endif



