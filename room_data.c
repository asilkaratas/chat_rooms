#include "room_data.h"

void cleanup(void *arg)
{
	pthread_mutex_unlock((pthread_mutex_t *)arg);
}

struct user_data_t *room_find_user(struct room_data_t *room_data, char *username)
{
	struct user_data_t *user_data = room_data->root_user;
	struct user_data_t *found = NULL;
	while(user_data != NULL) {
		
		printf("find:%s\n", user_data->username);
		if(strcmp(user_data->username, username) == 0) {
			found = user_data;
			printf("found:%s\n", user_data->username);
			break;
		} 
		
		user_data = user_data->next;
	}
	
	return found;
}

void send_chat_message(int socket, char *from, char *to, char *msg)
{
	int total_size = strlen(from) + strlen(msg) + 2;
	char buf[total_size];
	
	sprintf(buf, "*%s*%s", from, msg);
	
	printf("buf:%s\n", buf);
	send_message_to_socket(socket, buf);
}

/*
 * struct user_data_t *user_data = room_find_user(room_data, message_command.username);
	if(user_data != NULL) {
		sprintf(buf, "*%s*%s", room_data->from->username, message_command.msg);
		printf("buf:%s\n", buf);
		send_message_to_socket(user_data->socket, buf);
	} else {
		sprintf(buf, "user \"%s\" is not found in the room \"%s\".", message_command.username, room_data->room_name);
		send_message_to_socket(room_data->from->socket, buf);
	}*/

void communicate_room(struct user_data_t *from, char *msg)
{
	printf("communicate_room:%s!\n", msg);
	
	struct message_command_t message_command;
	if(parse_message_command(msg, &message_command)) {
		
		if(strcmp("all", message_command.username) == 0) {
			struct user_data_t *found = from->room->root_user;
			while(found != NULL) {
				if(found != from)
					send_chat_message(found->socket, from->username , found->username, message_command.msg);
				found = found->next;
			}
		} else {
			struct user_data_t *user_data = room_find_user(from->room, message_command.username);
			if(user_data != NULL) {
				send_chat_message(user_data->socket, from->username , user_data->username, message_command.msg);
			
			} else {
				
			}
			
				
		}
		
	} else {
		printf("Invalid msg!\n");
	}
	
	if(msg != NULL)
		free(msg);
	
}

void *room_broadcast_thread_func(void *arg) 
{
	struct room_data_t *room_data = (struct room_data_t *) arg;
	
	char *msg;
	struct user_data_t *from;
	printf("room:%s is created!\n", room_data->room_name);
	
	int work = 1;
	while (1) {
		pthread_cleanup_push(cleanup, (void *) &room_data->mutex);
		if (pthread_mutex_lock(&room_data->mutex) != 0)
			ERR("pthread_mutex_lock");
		
		while (!room_data->condition && work)
			if (pthread_cond_wait(&room_data->cond, &room_data->mutex) != 0)
				ERR("pthread_cond_wait");
		msg = room_data->msg;
		from = room_data->from;
		room_data->condition = 0;
		if (!work)
			pthread_exit(NULL);
		pthread_cleanup_pop(1);
		
		communicate_room(from, msg);
	}
	
	return NULL;
}

void room_create_broadcast_thread(struct room_data_t *room_data)
{
	pthread_t thread;
	room_data->thread = &thread;
	
	if (pthread_create(&thread, NULL, room_broadcast_thread_func, (void *)room_data) != 0) 
		ERR("pthread_create");
}

void room_add_room(struct room_data_t *root_room, struct room_data_t *room_data)
{
	struct room_data_t *last_room = root_room;
	while(last_room->next != NULL) {
		last_room = last_room->next;
	}
	
	last_room->next = room_data;
	room_data->root_room = root_room;
	
	
}

void room_remove_room(struct room_data_t *root_room, struct room_data_t *room_data)
{
	struct room_data_t *prev_room = root_room->next;
	
	if(prev_room == room_data) {
		root_room->next = prev_room->next;
	} else {
		while(prev_room != NULL && prev_room->next != room_data) {
			prev_room = prev_room->next;
		}
		
		if(prev_room != NULL) {
			prev_room->next = room_data->next;
		}
	}
	
	room_data->next = NULL;
}

void room_add_user(struct room_data_t *room_data, struct user_data_t *user_data)
{
	if(user_data->room == room_data)
		return;
		
	if(room_data->root_user == NULL) {
		room_data->root_user = user_data;
	} else {
		struct user_data_t *last_user = room_data->root_user;
		while(last_user->next != NULL) {
			last_user = last_user->next;
		}
		last_user->next = user_data;
	}
	
	user_data->room = room_data;
	
	room_data->user_count++;
}

void room_remove_user(struct room_data_t *room_data, struct user_data_t *user_data)
{
	if(user_data->room != room_data)
		return;
		
	struct user_data_t *prev_user = room_data->root_user;
	if(prev_user == user_data) {
		room_data->root_user = user_data->next;
	} else {
		while(prev_user != NULL && prev_user->next != user_data) {
			prev_user = prev_user->next;
		}
		
		if(prev_user != NULL)
			prev_user->next = user_data->next;
	}
	
	user_data->room = NULL;
	user_data->next = NULL;
	
	room_data->user_count--;
}

void room_create_folder(char *room_name, char *owner_name)
{
	char path[FILE_PATH_SIZE];
	int fd;
	struct stat st = {0};
	
	if(stat("./rooms", &st) == -1) {
		mkdir("./rooms", 0700);
	}
	
	sprintf(path, "./rooms/%s", room_name);
	if(stat(path, &st) == -1) {
		mkdir(path, 0700);
	}
	
	sprintf(path, "./rooms/%s/files", room_name);
	if(stat(path, &st) == -1) {
		mkdir(path, 0700);
	}
	
	if(owner_name != NULL) {
		sprintf(path, "./rooms/%s/owner", room_name);
		if(access(path, F_OK) == -1) {
			if ((fd = TEMP_FAILURE_RETRY(open(path, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR))) < 0)
				ERR("open");
			bulk_write(fd, owner_name, strlen(owner_name));
			if (TEMP_FAILURE_RETRY(close(fd)) < 0)
				ERR("close");
		} else {
			
		}
	}
}

struct room_data_t *room_create(struct room_data_t *entre_room, char *room_name, char *owner_name)
{
	pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	
	struct room_data_t *room_data = (struct room_data_t *)malloc(sizeof(struct room_data_t));
	strcpy(room_data->room_name, room_name);
	room_data->condition = 0;
	room_data->cond = cond;
	room_data->mutex = mutex;
	
	if(owner_name != NULL)
		strcpy(room_data->owner_name, owner_name);
	
	room_create_folder(room_data->room_name, owner_name);
	
	
	room_create_broadcast_thread(room_data);
	
	if(entre_room != NULL) {
		room_data->root_room = entre_room;
		room_add_room(entre_room, room_data);
	} else {
		room_data->root_room = room_data;
	}
	
	return room_data;
}

void room_print(struct room_data_t *room_data)
{
	printf("room:%s\n", room_data->room_name);
	printf("users:\n");
	struct user_data_t *next_user = room_data->root_user;
	while(next_user != NULL) {
		printf("%s\n", next_user->username);
		next_user = next_user->next;
	}
}

struct room_data_t *room_get_room(struct room_data_t *root_room, char *room_name)
{
	struct room_data_t *room = root_room->next;
	
	while(room != NULL) {
		if(strcmp(room->room_name, room_name) == 0) {
			return room;
		}
		room = room->next;
	}
	
	return NULL;
}

//files
void room_add_file(struct room_data_t *room_data, struct file_data_t *file_data)
{
	if(room_data->root_file == NULL) {
		room_data->root_file = file_data;
	} else {
		struct file_data_t *last_file = room_data->root_file;
		while(last_file->next != NULL) {
			last_file = last_file->next;
		}
		last_file->next = file_data;
	}
}

void remove_file_from_disc(char *room_name, char *owner_name, char *file_name) 
{
	char path[FILE_PATH_SIZE];
	
	sprintf(path, "./rooms/%s/files/%s/%s", room_name, owner_name, file_name);
	
	if(TEMP_FAILURE_RETRY(remove(path)) < 0) 
		ERR("remove");
}

void room_remove_file(struct room_data_t *room_data, struct file_data_t *file_data)
{
	struct file_data_t *prev_file = room_data->root_file;
	if(prev_file == file_data) {
		room_data->root_file = file_data->next;
	} else {
		while(prev_file != NULL && prev_file->next != file_data) {
			prev_file = prev_file->next;
		}
		
		if(prev_file != NULL)
			prev_file->next = file_data->next;
	}
	
	file_data->next = NULL;
	
	remove_file_from_disc(room_data->room_name, file_data->owner_name, file_data->file_name);
	
	free(file_data);
}

struct file_data_t *room_get_file(struct room_data_t *room_data, char *file_name)
{
	struct file_data_t *file_data = room_data->root_file;
	struct file_data_t *found = NULL;
	while(file_data != NULL) {
		
		printf("find:%s\n", file_data->file_name);
		if(strcmp(file_data->file_name, file_name) == 0) {
			found = file_data;
			printf("found:%s\n", file_data->file_name);
			break;
		} 
		
		file_data = file_data->next;
	}
	
	return found;
}







