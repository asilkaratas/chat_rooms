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
#include "room_data.h"
#include "user_data.h"
#include <dirent.h>

volatile sig_atomic_t work = 1;

void sig_int_handler(int sig) 
{
	work = 0;
}

void sig_pipe_handler(int sig)
{
	printf("sigPipeHandler\n");
	//work = 0;
}

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s port \n",name);
	exit(EXIT_FAILURE);
}

void read_files(struct room_data_t *room_data, char *owner_name) 
{
	DIR *dir;
	struct dirent *ent;
	char path[ROOM_NAME_SIZE + USER_NAME_SIZE + FILE_NAME_SIZE + 17];
	struct file_data_t *file_data;
	
	sprintf(path, "./rooms/%s/files/%s", room_data->room_name, owner_name);
	if((dir = opendir(path)) != NULL) {
		while((ent = readdir(dir)) != NULL) {
			if(ent->d_type == DT_REG && strlen(ent->d_name) >= FILE_NAME_SIZE_MIN) {
				file_data = malloc(sizeof(struct file_data_t));
				strcpy(file_data->owner_name, owner_name);
				strcpy(file_data->file_name, ent->d_name);
				
				room_add_file(room_data, file_data);
			}
		}
		if(TEMP_FAILURE_RETRY(closedir(dir)) < 0)
			ERR("closedir");
	} else {
		ERR("opendir");
	}
}

void read_file_owners(struct room_data_t *room_data)
{
	DIR *dir;
	struct dirent *ent;
	char path[ROOM_NAME_SIZE + USER_NAME_SIZE + 16];
	//"./rooms/room_name/files/user_name"
	
	sprintf(path, "./rooms/%s/files", room_data->room_name);
	printf("read_file_owners:path:%s\n", path);
	if((dir = opendir(path)) != NULL) {
		while((ent = readdir(dir)) != NULL) {
			if(ent->d_type == DT_DIR && strlen(ent->d_name) >= USER_NAME_SIZE_MIN) {
				printf("read_file_owners:%s\n", ent->d_name);
				read_files(room_data, ent->d_name);
			}
		}
		if(TEMP_FAILURE_RETRY(closedir(dir)) < 0)
			ERR("closedir");
	} else {
		ERR("opendir");
	}
}

void read_rooms(struct room_data_t *root_room) 
{
	DIR *dir;
	struct dirent *ent;
	struct room_data_t *room_data;
	char path[ROOM_NAME_SIZE + 15];
	int size;
	int fd;
	
	read_file_owners(root_room);
	
	if((dir = opendir("./rooms")) != NULL) {
		while((ent = readdir(dir)) != NULL) {
			if(ent->d_type == DT_DIR && strlen(ent->d_name) >= ROOM_NAME_SIZE_MIN) {
				printf("%s\n", ent->d_name);
				if(strcmp(root_room->room_name, ent->d_name) != 0) {
					room_data = room_create(root_room, ent->d_name, NULL);
					
					sprintf(path, "./rooms/%s/owner", room_data->room_name);
					printf("path:%sa!\n", path);
					if ((fd = TEMP_FAILURE_RETRY(open(path, O_RDONLY))) < 0)
						ERR("open");
					
					if((size = bulk_read(fd, room_data->owner_name, USER_NAME_SIZE)) == -1)
						ERR("read");
						
					printf("owner_name:%d:%s\n", fd, room_data->owner_name);
					if (TEMP_FAILURE_RETRY(close(fd)) < 0)
						ERR("close");
						
					read_file_owners(room_data);
				}
			}
		}
		if(TEMP_FAILURE_RETRY(closedir(dir)) < 0)
			ERR("closedir");
	} else {
		ERR("opendir");
	}
}

void dowork(int socket, sigset_t *oldmask)
{
	int user_socket;
	fd_set base_rfds, rfds;
	FD_ZERO(&base_rfds);
	FD_SET(socket, &base_rfds);
	
	struct room_data_t *root_room = room_create(NULL, "entrence", NULL);
	
	read_rooms(root_room);
	
	while (work) {
		rfds = base_rfds;
		if (pselect(socket + 1, &rfds, NULL, NULL, NULL, oldmask) > 0) {
			if ((user_socket = add_new_client(socket)) == -1)
				continue;
			
			struct user_data_t *user_data = user_create(user_socket);
				
			room_add_user(root_room, user_data);
			
				 
		} else {
			if (errno == EINTR)
				continue;
				
			ERR("pselect");
		}
	}
}

void pcleanup(pthread_mutex_t *mutex, pthread_cond_t *cond)
{
	if (pthread_mutex_destroy(mutex) != 0)
		ERR("pthread_mutex_destroy");
	if (pthread_cond_destroy(cond) != 0)
		ERR("pthread_cond_destroy");
}

int main(int argc, char **argv) 
{
	int socket, new_flags;
	sigset_t mask, oldmask;

	if (argc!=2)
		usage(argv[0]);

	sethandler(sig_pipe_handler, SIGPIPE);
	sethandler(sig_int_handler, SIGINT);
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigprocmask(SIG_BLOCK, &mask, &oldmask);
	
	socket = bind_tcp_socket(atoi(argv[1]));
	new_flags = fcntl(socket, F_GETFL) | O_NONBLOCK;
	if (fcntl(socket, F_SETFL, new_flags) == -1)
		ERR("fcntl");
		
	dowork(socket, &oldmask);
	
	if (TEMP_FAILURE_RETRY(close(socket)) < 0)
		ERR("close");
		
	return EXIT_SUCCESS;
}




















