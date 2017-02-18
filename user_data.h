#ifndef _USER_DATA_H
#define _USER_DATA_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include "common.h"
#include "commands.h"
#include "room_data.h"
#include <aio.h>

struct file_transfer_t {
	struct aiocb cb;
};

struct user_data_t {
	char username[USER_NAME_SIZE];
	int socket;
	pthread_t *thread;
	struct user_data_t *next;
	struct room_data_t *room;
	volatile sig_atomic_t work;
	
	struct file_transfer_t *file_transfer;
	
};



void communicate(struct user_data_t *user_data);
void *user_thread_func(void *arg);
void user_create_thread(struct user_data_t *user_data);
struct user_data_t *user_create(int socket);

#endif















