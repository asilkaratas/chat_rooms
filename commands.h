#ifndef _COMMANDS_H
#define _COMMANDS_H

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


struct connect_command_t {
	char username[USER_NAME_SIZE];
	char address[ADDRESS_SIZE];
	int port;
};

struct room_command_t {
	char room_name[ROOM_NAME_SIZE];
};

struct file_command_t {
	char file_name[FILE_NAME_SIZE];
};

struct transfer_command_t {
	int offset;
	int len;
	char data[MSG_SIZE];
};

struct push_response_t {
	int id;
	char file_name[FILE_NAME_SIZE];
};

struct message_command_t {
	char username[USER_NAME_SIZE];
	char msg[MSG_SIZE];
};


enum command_type_t{
	CONNECT,
	BYE,
	ROOMS,
	OPEN,
	CLOSE,
	ENTER,
	LEAVE,
	FILES,
	PUSH_RESPONSE,
	PUSH,
	PULL,
	RM,
	TRANSFER,
	MESSAGE
};


int starts_with(const char *str, const char *pre);
void get_command(char *input, int *command);

void print_connect_command(struct connect_command_t *connect_command);
int validate_connect_command(struct connect_command_t *connect_command);
int parse_connect_command(char *input, struct connect_command_t *connect_command);
int parse_and_validate_connect_command(char *input, struct connect_command_t *connect_command);

int parse_and_validate_room_command(char *input, struct room_command_t *room_command);

int parse_message_command(char *input, struct message_command_t *message_command);

int validate_leave_command(char *input);
int validate_bye_command(char *input);

int validate_rooms_command(char *input);
int validate_files_command(char *input);

int parse_and_validate_file_command(char *input, struct file_command_t *file_command);
int parse_and_validate_push_response(char *input, struct push_response_t *push_response);
int parse_and_validate_transfer_command(char *input, struct transfer_command_t *transfer_command);

#endif
