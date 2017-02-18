#ifndef _ROOM_DATA_H
#define _ROOM_DATA_H

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
#include <sys/stat.h>
#include "common.h"
#include "user_data.h"
#include "file_data.h"


struct room_data_t {
	char room_name[ROOM_NAME_SIZE];
	pthread_t *thread;
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	int condition;
	
	struct user_data_t *root_user;
	struct room_data_t *next;
	struct room_data_t *root_room;
	char owner_name[USER_NAME_SIZE];
	int user_count;
	
	struct file_data_t *root_file;
	
	struct user_data_t *from;
	char *msg;
};


void *room_broadcast_thread_func(void *arg);
void room_create_broadcast_thread(struct room_data_t *room_data);
void room_add_room(struct room_data_t *root_room,struct room_data_t *room_data);
void room_remove_room(struct room_data_t *root_room,struct room_data_t *room_data);
void room_add_user(struct room_data_t *room_data,struct user_data_t *user_data);
void room_remove_user(struct room_data_t *room_data,struct user_data_t *user_data);
struct room_data_t *room_create(struct room_data_t *entre_room, char *room_name, char *owner_name);
void room_print(struct room_data_t *room_data);
struct room_data_t *room_get_room(struct room_data_t *root_room, char *room_name);

void room_add_file(struct room_data_t *room_data, struct file_data_t *file_data);
void room_remove_file(struct room_data_t *room_data, struct file_data_t *file_data);
struct file_data_t *room_get_file(struct room_data_t *room_data, char *file_name);

#endif










