#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <signal.h>
#include <netdb.h>
#include <pthread.h>
#include <aio.h>
#include "common.h"
#include "commands.h"



struct client_data_t {
	int socket;
	int connected;
};

struct transfer_data_t {
	struct aiocb cb;
	struct client_data_t *client_data;
	int id;
};

volatile sig_atomic_t work = 1;

void usage(char *name) 
{
	fprintf(stderr, "USAGE: %s address port\n", name);
	exit(EXIT_FAILURE);
}

void sig_int_handler(int sig)
{
	work = 0;
}

void sig_pipe_handler(int sig)
{
	printf("sigPipeHandler\n");
	//work = 0;
}

void send_message(struct client_data_t *client_data, char *msg) 
{
	if(client_data->connected)
		send_message_to_socket(client_data->socket, msg);
	else
		printf("Client is not connected!\n");
	
}

#define IO_SIGNAL SIGUSR1

void send_transfer(struct transfer_data_t *transfer_data, int offset, int len, char *buf) 
{
	char msg[MSG_SIZE + 1];
	
	sprintf(msg, "!transfer %d %d %s", offset, len, buf);
	
	//msg[strlen(msg)-1] = '\0';
	send_message(transfer_data->client_data, msg);
	
}

static void aio_sig_handler(int sig, siginfo_t *si, void *ucontext)
{
	struct transfer_data_t *transfer_data;
	
	struct aiocb *cb;
	int s;
	int buf_size = 10;
	char buf[buf_size + 1];
	
	transfer_data = (struct transfer_data_t *)si->si_value.sival_ptr;
	cb = &transfer_data->cb;
	
	if(aio_error(cb) == 0) {
		s = strlen((char *)cb->aio_buf);
		printf("File success!\n");
		printf("buf:%d:%s!\n", s, (char *)cb->aio_buf);
		printf("offset:%lu!\n", cb->aio_offset);
		
		
		
		if(s != 0) {
			memcpy(buf, (void *)cb->aio_buf, buf_size);
			free((void *)cb->aio_buf);
			send_transfer(transfer_data, cb->aio_offset, s, buf);
			cb->aio_offset += s;
			cb->aio_buf = malloc(buf_size + 1);
			
			aio_read(cb);
		} else {
			printf("End of file\n");
			
			//free((void *)cb);
			free((void *)transfer_data);
			/*
			if (TEMP_FAILURE_RETRY(close(cb->aio_fildes)) < 0)
				ERR("close");
				* */
		}
	} 
}

void start_push(struct client_data_t *client_data, struct push_response_t *push_response) 
{
	struct transfer_data_t *transfer_data;
	int fd;
	int s;
	struct sigaction sa;
	int buf_size = 10;
	
	if ((fd = TEMP_FAILURE_RETRY(open(push_response->file_name, O_RDONLY))) < 0)
		ERR("open");
		
	//sethandler(aio_sig_handler, IO_SIGNAL);
	
	sa.sa_flags = SA_RESTART | SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = aio_sig_handler;
	if(sigaction(IO_SIGNAL, &sa, NULL) == -1)
		ERR("sigaction");
	
	//bzero((char *)cb, sizeof(struct aiocb));
	transfer_data = calloc(1, sizeof(struct transfer_data_t));
	if(transfer_data == NULL)
		ERR("calloc");
		
	transfer_data->client_data = client_data;
	transfer_data->cb.aio_fildes = fd;
	transfer_data->cb.aio_offset = 0;
	transfer_data->cb.aio_nbytes = buf_size;
	transfer_data->cb.aio_buf = malloc(buf_size + 1);
	transfer_data->cb.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
	transfer_data->cb.aio_sigevent.sigev_signo = IO_SIGNAL;
	transfer_data->cb.aio_sigevent.sigev_value.sival_ptr = transfer_data;
	s = aio_read(&transfer_data->cb);
	if(s == -1)
		ERR("aio_read");
	printf("read:%d\n", s);
		
}

void handle_push_response(struct client_data_t *client_data, char *input) 
{
	struct push_response_t push_response;
	if(parse_and_validate_push_response(input, &push_response)) {
		printf("valid push response:%s\n", push_response.file_name);
		start_push(client_data, &push_response);
	} else {
		printf("Invalid push reponse:%s!\n", input);
	}
}

void *reciever_func(void *arg)
{
	char buf[MSG_SIZE_MAX];
	int command;
	ssize_t size;
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	
	struct client_data_t *client_data = (struct client_data_t *)arg;

	if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0)
		ERR("pthread_mask");
		
	while (1) {	
		if ((size = TEMP_FAILURE_RETRY(recv(client_data->socket, buf, MSG_SIZE_MAX, MSG_WAITALL))) == -1)
			ERR("read");
		
		if(size == 0) {
			printf("errno:%d!\n", errno);
			break;
		};
		
		get_command(buf, &command);
		//printf("command:%d!\n", command);
		switch(command) {
			case PUSH_RESPONSE:
				handle_push_response(client_data, buf);
			break;
		}
			
		printf("recieved:%s\n", buf);
	}
		
	if (pthread_sigmask(SIG_UNBLOCK, &mask, NULL) != 0)
		ERR("pthread_sigmask");
		
	printf("recieverFunc ends!\n");

	
	return NULL;
}


void print_connect_usage()
{
	printf("Usage:\n");
	printf("!connect <username>@<address>:<port>\n");
}



// handleConnect
void create_reciever_thread(struct client_data_t *client_data) 
{
	pthread_t thread;
	
	if (pthread_create(&thread, NULL, reciever_func, (void *)client_data) != 0)
			ERR("pthread_create");
			
	if (pthread_detach(thread) != 0)
		ERR("pthread_detach");
}

void connect_server(struct client_data_t *client_data, struct connect_command_t *connect_command, char *input)
{
	struct sockaddr_in addr;
	int socket;
	
	addr = make_address(connect_command->address, connect_command->port);
	socket = connect_socket(&addr);
	
	client_data->socket = socket;
	client_data->connected = 1;
	
	create_reciever_thread(client_data);
	send_message(client_data, input);
}

void handle_connect(struct client_data_t *client_data, char *input)
{
	struct connect_command_t connect_command;
	
	if(parse_and_validate_connect_command(input, &connect_command)) {
		connect_server(client_data, &connect_command, input);
	} else {
		print_connect_usage();
	}
}

//handleBye
void print_bye_usage(void) {
	printf("Usage:\n");
	printf("!bye\n");
}

void send_bye(struct client_data_t *client_data) {
	send_message(client_data, "!bye");
}

void handle_bye(struct client_data_t *client_data, char *input)
{
	printf("handle_bye\n");
	if(validate_bye_command(input)) {
		send_bye(client_data);
	} else {
		print_bye_usage();
	}
}

//handleRooms
void print_rooms_usage(void) {
	printf("Usage:\n");
	printf("!rooms\n");
}

void handle_rooms(struct client_data_t *client_data, char *input)
{
	printf("handleRooms\n");
	if(validate_rooms_command(input)) {
		send_message(client_data, input);
	} else {
		print_rooms_usage();
	}
}

//handleOpen
void print_open_usage(void)
{
	printf("Usage:\n");
	printf("!open <roomname>\n");
}

void handle_open(struct client_data_t *client_data, char *input)
{
	printf("handle_open\n");
	struct room_command_t room_command;
	
	if(parse_and_validate_room_command(input, &room_command)) {
		printf("input:%s\n", input);
		send_message(client_data, input);
	} else {
		print_open_usage();
	}
}

//handleClose
void print_close_usage(void)
{
	printf("Usage:\n");
	printf("!open <roomname>\n");
}

void handle_close(struct client_data_t *client_data, char *input)
{
	printf("handle_close\n");
	struct room_command_t room_command;
	
	if(parse_and_validate_room_command(input, &room_command)) {
		printf("input:%s\n", input);
		send_message(client_data, input);
	}
	else {
		print_close_usage();
	}
}

//handleEnter
void print_enter_usage(void)
{
	printf("Usage:\n");
	printf("!enter <roomname>\n");
}

void handle_enter(struct client_data_t *client_data, char *input)
{
	printf("handle_enter\n");
	struct room_command_t room_command;
	
	if(parse_and_validate_room_command(input, &room_command)) {
		printf("input:%s\n", input);
		
		send_message(client_data, input);
	}
	else {
		print_enter_usage();
	}
}

//handleLeave
void print_leave_usage(void)
{
	printf("Usage:\n");
	printf("!leave\n");
}

void handle_leave(struct client_data_t *client_data, char *input)
{
	printf("handle_leave\n");
	
	if(validate_leave_command(input)) {
		printf("input:%s!\n", input);
		send_message(client_data, input);
	}
	else {
		print_leave_usage();
	}
}

//handleFiles
void print_files_usage(void)
{
	printf("Usage:\n");
	printf("!files\n");
}

void handle_files(struct client_data_t *client_data, char *input)
{
	printf("handleFiles\n");
	if(validate_files_command(input)) {
		send_message(client_data, input);
	} else {
		print_files_usage();
	}
}

//handlePush
void print_push_usage(void)
{
	printf("Usage:\n");
	printf("!push <filename>\n");
}

int file_exist(char *file_name)
{
	if(access(file_name, F_OK) != -1)
		return 1;
		
	return 0;
}

void handle_push(struct client_data_t *client_data, char *input)
{
	printf("handlePush\n");
	
	struct file_command_t file_command;
	if(parse_and_validate_file_command(input, &file_command)) {
		if(file_exist(file_command.file_name)) {
			printf("File Exist!\n");
			send_message(client_data, input);
		} else {
			printf("file \"%s\" is not found!\n", file_command.file_name);
		}
	} else {
		print_push_usage();
	}
}

//handlePull
void handle_pull(struct client_data_t *client_data, char *input)
{
	printf("handlePull\n");
}

//handleRm
void print_rm_usage(void)
{
	printf("Usage:\n");
	printf("!rm <filename>\n");
}

void handle_rm(struct client_data_t *client_data, char *input)
{
	printf("handleRm\n");
	struct file_command_t file_command;
	if(parse_and_validate_file_command(input, &file_command)) {
		send_message(client_data, input);
	} else {
		print_rm_usage();
	}
}

//handleMessage
void print_message_usage(void)
{
	printf("Usage:\n");
	printf("*<username>*<message>\n");
}

void handle_message(struct client_data_t *client_data, char *input) 
{
	printf("handleMessage\n");
	struct message_command_t message_command;
	if(parse_message_command(input, &message_command)) {
		send_message(client_data, input);
	} else {
		print_message_usage();
	}
}

//doWork
void dowork(void) 
{
	char input[MSG_SIZE_MAX];
	int command;
	struct client_data_t client_data;
	client_data.socket = 0;
	
	while (work) {
		fgets(input, MSG_SIZE_MAX, stdin);
		if(!work)
			break;
		
		if(input[0] == '\n')
			continue;
			
		if(feof(stdin)) {
			send_bye(&client_data);
			work = 0;
			break;
		}
		
		get_command(input, &command);
		//printf("command:%d\n", command);
		switch(command) {
			case CONNECT:
				handle_connect(&client_data, input);
			break;
			
			case BYE:
				handle_bye(&client_data, input);
			break;
			
			case ROOMS:
				handle_rooms(&client_data, input);
			break;
			
			case OPEN:
				handle_open(&client_data, input);
			break;
			
			case CLOSE:
				handle_close(&client_data, input);
			break;
			
			case ENTER:
				handle_enter(&client_data, input);
			break;
			
			case LEAVE:
				handle_leave(&client_data, input);
			break;
			
			case FILES:
				handle_files(&client_data, input);
			break;
			
			case PULL:
				handle_pull(&client_data, input);
			break;
			
			case PUSH:
				handle_push(&client_data, input);
			break;
			
			case RM:
				handle_rm(&client_data, input);
			break;
			
			case MESSAGE:
				handle_message(&client_data, input);
			break;
			
			default:
				printf("Unknown command:%s\n", input);
			break;
		}
	}
	
	if(client_data.socket != 0) {
		if (TEMP_FAILURE_RETRY(close(client_data.socket)) == -1)
			ERR("close");
	}
}


int main(int argc, char **argv) 
{
	sethandler(sig_pipe_handler, SIGPIPE);
	sethandler(sig_int_handler, SIGINT);
	
	struct stat st = {0};
	
	if(stat("./child", &st) == -1) {
		mkdir("./child", 0700);
	}

	dowork();
	
	return EXIT_SUCCESS;
}















