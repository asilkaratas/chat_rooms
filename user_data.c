#include "user_data.h"

void bye(struct user_data_t *user_data)
{
	if(user_data->room != NULL) {
		room_remove_user(user_data->room, user_data);
		printf("user quits:%s!\n", user_data->username);
	}
	
	user_data->work = 0;
}
	
void handle_bye(struct user_data_t *user_data, char *input)
{
	printf("handle_bye\n");
	if(validate_bye_command(input)) {
		bye(user_data);
	} else {
		printf("Invalid bye command!\n");
	}
}

void handle_message(struct user_data_t *user_data, char *msg) {
	printf("handle_message:%s\n", msg);
	printf("handle_message:room:%s\n", user_data->room->room_name);
	
	struct room_data_t *room_data = user_data->room;
	
	if (pthread_mutex_lock(&room_data->mutex) != 0)
		ERR("pthread_mutex_lock");
	
	//strcpy(room_data->msg, msg);
	room_data->msg = msg;
	room_data->from = user_data;
			
	if (pthread_mutex_unlock(&room_data->mutex) != 0)
		ERR("pthread_mutex_unlock");
	
	room_data->condition = 1;
	
	if (pthread_cond_signal(&room_data->cond) != 0)
		ERR("pthread_cond_signal");
}

void handle_connect(struct user_data_t *user_data, char *input) {
	//printf("handle_connect:%s\n", input);
	struct connect_command_t connect_command;
	if(parse_and_validate_connect_command(input, &connect_command)) {
		strcpy(user_data->username, connect_command.username);
		printf("connected:%s\n", user_data->username);
	} else {
		printf("Invalid connect message:%s!\n", input);
	}
}

void handle_open(struct user_data_t *user_data, char *input)
{
	printf("handle_open:%s!\n", input);
	struct room_command_t room_command;
	if(parse_and_validate_room_command(input, &room_command)) {
		printf("open parsed:%s!\n", room_command.room_name);
		room_create(user_data->room->root_room, room_command.room_name, user_data->username);
		//struct room_data_t *room_data = 
		//room_remove_user(user_data->room, user_data);
	} else {
		printf("Invalid open message:%s!\n", input);
	}
}

void handle_close(struct user_data_t *user_data, char *input)
{
	printf("handle_close:%s!\n", input);
	struct room_command_t room_command;
	char buf[MSG_SIZE];
	
	if(parse_and_validate_room_command(input, &room_command)) {
		printf("close parsed:%s!\n", room_command.room_name);
		
		struct room_data_t *room_data = room_get_room(user_data->room->root_room, room_command.room_name);
		if(room_data != NULL && room_data->user_count == 0) {
			if(strcmp(room_data->owner_name, user_data->username) == 0) {
				printf("room closed:%s!\n", room_data->room_name);
				room_remove_room(room_data->root_room, room_data);
			} else {
				sprintf(buf, "Invalid room owner:%s room_owner:%s\n", user_data->username, room_data->owner_name);
				printf(buf);
				send_message_to_socket(user_data->socket, buf);
			}
		}
	} else {
		printf("Invalid open message:%s!\n", input);
	}
}

void handle_enter(struct user_data_t *user_data, char *input)
{
	printf("handle_enter:%s!\n", input);
	struct room_command_t room_command;
	if(parse_and_validate_room_command(input, &room_command)) {
		printf("enter parsed:%s!\n", room_command.room_name);
		
		struct room_data_t *room_data = room_get_room(user_data->room->root_room, room_command.room_name);
		if(room_data != NULL) {
			printf("room entered:%s!\n", room_data->room_name);
			room_remove_user(user_data->room, user_data);
			room_add_user(room_data, user_data);
		}
	} else {
		printf("Invalid enter message:%s!\n", input);
	}
}

void handle_leave(struct user_data_t *user_data, char *input)
{
	printf("handle_leave:%s!\n", input);
	if(validate_leave_command(input)) {
		printf("leave parsed:%s!\n", input);
		
		struct room_data_t *room_data = user_data->room;
		if(room_data != NULL) {
			if(room_data == user_data->room->root_room) {
				bye(user_data);
			} else {
				printf("room leaved:%s!\n", room_data->room_name);
				printf("room entered:%s!\n", room_data->root_room->room_name);
				room_remove_user(room_data, user_data);
				room_add_user(room_data->root_room, user_data);
			}
		}
	} else {
		printf("Invalid enter message:%s!\n", input);
	}
}

void send_room_list(struct user_data_t *user_data) {
	struct room_data_t *room = user_data->room->root_room;
	send_message_to_socket(user_data->socket, "Room List:");
	while(room != NULL) {
		send_message_to_socket(user_data->socket, room->room_name);
		room = room->next;
	}
}

void handle_rooms(struct user_data_t *user_data, char *input)
{
	printf("handle_rooms:%s!\n", input);
	if(validate_rooms_command(input)) {
		send_room_list(user_data);
	} else {
		printf("Invalid rooms message:%s!\n", input);
	}
}

int file_exist(char *room_name, char *file_name)
{
	/*
	 * +2 because of '/' and EOF
	 */
	char file_path[FILE_PATH_SIZE];
	
	sprintf(file_path, "%s/%s", room_name, file_name);
	
	if(access(file_path, F_OK) != -1)
		return 1;
		
	return 0;
}

void send_file_list(struct user_data_t *user_data) {
	struct file_data_t *file = user_data->room->root_file;
	send_message_to_socket(user_data->socket, "File List:");
	while(file != NULL) {
		send_message_to_socket(user_data->socket, file->file_name);
		file = file->next;
	}
}

void handle_files(struct user_data_t *user_data, char *input)
{
	printf("handle_files:%s!\n", input);
	if(validate_files_command(input)) {
		printf("Valid files command!\n");
		send_file_list(user_data);
	} else {
		printf("Invalid files message:%s!\n", input);
	}
}

#define IO_SIGNAL SIGUSR1
static void aio_sig_handler(int sig, siginfo_t *si, void *ucontext)
{
	struct user_data_t *user_data;
	
	struct aiocb *cb;
	int s;
	//int buf_size = 10;
	//char buf[buf_size + 1];
	
	user_data = (struct user_data_t *)si->si_value.sival_ptr;
	cb = &user_data->file_transfer->cb;
	
	if(aio_error(cb) == 0) {
		s = strlen((char *)cb->aio_buf);
		printf("File success!\n");
		printf("buf:%d:%s!\n", s, (char *)cb->aio_buf);
		printf("offset:%lu!\n", cb->aio_offset);
		
		
	} 
}

void create_directory_and_file(struct user_data_t *user_data, char *file_name, int *fd)
{
	struct stat st = {0};
	char path[FILE_PATH_SIZE];
	
	sprintf(path, "./rooms/%s/files/%s", user_data->room->room_name, user_data->username);
	if(stat(path, &st) == -1) {
		mkdir(path, 0700);
	}
	
	sprintf(path, "%s/%s", path, file_name);
	printf("create_directory_and_file:path:%s\n", path);
	if ((*fd = TEMP_FAILURE_RETRY(open(path, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR))) < 0)
			ERR("open");
}

void preapare_file(struct user_data_t *user_data, char *file_name)
{
	struct file_transfer_t *file_transfer;
	int fd;
	//int s;
	struct sigaction sa;
	int buf_size = 10;
	
	sa.sa_flags = SA_RESTART | SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = aio_sig_handler;
	if(sigaction(IO_SIGNAL, &sa, NULL) == -1)
		ERR("sigaction");
		
	create_directory_and_file(user_data, file_name, &fd);
		
	file_transfer = calloc(1, sizeof(struct file_transfer_t));
	if(file_transfer == NULL)
		ERR("calloc");
	user_data->file_transfer = file_transfer;
	
	file_transfer->cb.aio_fildes = fd;
	file_transfer->cb.aio_offset = 0;
	file_transfer->cb.aio_nbytes = buf_size;
	file_transfer->cb.aio_buf = malloc(buf_size + 1);
	file_transfer->cb.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
	file_transfer->cb.aio_sigevent.sigev_signo = IO_SIGNAL;
	file_transfer->cb.aio_sigevent.sigev_value.sival_ptr = user_data;
	
	//printf("Userdata: %d\n", user_data->file_transfer);
	
	/*
	s = aio_write
	if(s == -1)
		ERR("aio_read");
		* */
}

void send_push_response(struct user_data_t *user_data, char *file_name)
{
	struct file_data_t *file_data;
	
	char buf[MSG_SIZE];
	
	printf("sending push response!\n");
	if(user_data->room != NULL) {
		file_data = room_get_file(user_data->room, file_name);
		if(file_data == NULL) {
			preapare_file(user_data, file_name);
			
			sprintf(buf, "!push_response %s\n", file_name);
			
			send_message_to_socket(user_data->socket, buf);	
		} else {
			sprintf(buf, "file:%s is exist!\n", file_name);
			printf(buf);
			send_message_to_socket(user_data->socket, buf);	
		}
	}
	
}

void handle_push(struct user_data_t *user_data, char *input)
{
	printf("handle_push:%s!\n", input);
	struct file_command_t file_command;
	if(parse_and_validate_file_command(input, &file_command)) {
		printf("Valid file command!\n");
		send_push_response(user_data, file_command.file_name);
	} else {
		printf("Invalid push message:%s!\n", input);
	}
}

void remove_file(struct user_data_t *user_data, char *file_name)
{
	struct file_data_t *file_data;
	char buf[MSG_SIZE];
	
	if(user_data->room != NULL) {
		file_data = room_get_file(user_data->room, file_name);
		if(file_data != NULL) {
			if(strcmp(user_data->username, file_data->owner_name) == 0) {
				room_remove_file(user_data->room, file_data);
			} else {
				sprintf(buf, "file:%s is not owned by user:%s! owner:%s!\n", file_name, user_data->username, file_data->owner_name);
				printf(buf);
				send_message_to_socket(user_data->socket, buf);
			}
		} else {
			sprintf(buf, "file:%s is not found in room:%s!\n", file_name, user_data->room->room_name);
			printf(buf);
			send_message_to_socket(user_data->socket, buf);
		}
	}
}

void handle_rm(struct user_data_t *user_data, char *input)
{
	printf("handle_rm:%s!\n", input);
	struct file_command_t file_command;
	if(parse_and_validate_file_command(input, &file_command)) {
		printf("Valid file command!\n");
		remove_file(user_data, file_command.file_name);
	} else {
		printf("Invalid rm message:%s!\n", input);
	}
}

void write_data(struct user_data_t *user_data, struct transfer_command_t *transfer_command)
{
	struct aiocb *cb;
	int buf_size = 10;
	
	if(user_data->file_transfer == NULL) {
		cb = &user_data->file_transfer->cb;
		cb->aio_offset = transfer_command->offset;
		cb->aio_nbytes = transfer_command->len;
		cb->aio_buf = malloc(buf_size + 1);
		
		
		memcpy((void *)cb->aio_buf, transfer_command->data , buf_size);
		
		aio_write(cb);
		
	} else {
		printf("User does not have file transfer!\n");
	}
}

void handle_transfer(struct user_data_t *user_data, char *input)
{
	printf("handle_transfer:%s!\n", input);
	
	struct transfer_command_t transfer_command;
	if(parse_and_validate_transfer_command(input, &transfer_command)) {
		write_data(user_data, &transfer_command);
	} else {
		printf("Invalid transfer command!\n");
	}
}



//user
void communicate(struct user_data_t *user_data)
{
	ssize_t size;
	int command;
	char *input = (char *)malloc(sizeof(char) * MSG_SIZE_MAX);

	//char input[MSG_SIZE_MAX];
	
	while(user_data->work) {
		
		if ((size = TEMP_FAILURE_RETRY(recv(user_data->socket, input, MSG_SIZE_MAX, MSG_WAITALL))) == -1)
			ERR("read");
		
		if(size == 0) {
			printf("errno:%d!\n", errno);
			break;
		};
			
		printf("communicate:%s\n", input);
		
		get_command(input, &command);
		
		switch(command) {
			case CONNECT:
				handle_connect(user_data, input);
			break;
			
			case MESSAGE:
				handle_message(user_data, input);
				input = (char *)malloc(sizeof(char) * MSG_SIZE_MAX);
				//allacote new buf
			break;
			
			case ROOMS:
				handle_rooms(user_data, input);
			break;
			
			case OPEN:
				handle_open(user_data, input);
			break;
			
			case CLOSE:
				handle_close(user_data, input);
			break;
			
			case ENTER:
				handle_enter(user_data, input);
			break;	
			
			case LEAVE:
				handle_leave(user_data, input);
			break;
			
			case FILES:
				handle_files(user_data, input);
			break;
			
			case PUSH:
				handle_push(user_data, input);
			break;
			
			case RM:
				handle_rm(user_data, input);
			break;
			
			case TRANSFER:
				handle_transfer(user_data, input);
			break;
			
			case BYE:
				handle_bye(user_data, input);
			break;
			
			default:
				printf("Unknown command!\n");
			break;
		}
	}
	
	free(input);
	
	if (TEMP_FAILURE_RETRY(close(user_data->socket)) < 0)
		ERR("close");
}

void *user_thread_func(void *arg)
{
	struct user_data_t *user_data = (struct user_data_t *)arg;

	communicate(user_data);
	
	free(user_data);

	return NULL;
}

void user_create_thread(struct user_data_t *user_data)
{
	pthread_t thread;
	user_data->thread = &thread;
	
	if (pthread_create(&thread, NULL, user_thread_func, (void *)user_data) != 0) 
		ERR("pthread_create");
}

struct user_data_t *user_create(int socket)
{
	struct user_data_t *user_data = (struct user_data_t *)malloc(sizeof(struct user_data_t));
	user_data->work = 1;
	user_data->socket = socket;
	
	user_create_thread(user_data);
	
	
	
	return user_data;
}

















