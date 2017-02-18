#include "commands.h"


int starts_with(const char *str, const char *pre) 
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? 0 : strncmp(str, pre, lenpre) == 0;
}

int parse_message_command(char *input, struct message_command_t *message_command)
{
	char *username;
	char *msg;
	
	int input_size = strlen(input) + 1;
	char buf[input_size];
	
	//printf("input_size:%d\n", input_size);
	memcpy(buf, input, input_size);
	buf[input_size] = '\0';
	
	username = strtok(buf, "*");
	if(username == NULL)
		return 0;
	
	msg = strtok(NULL, "*");
	if(msg == NULL)
		return 0;
		
	//printf("*username:%s!\n", username);
	//printf("*msg:%s!\n", msg);
	strcpy(message_command->username, username);
	strcpy(message_command->msg, msg);
	
	return 1;
}

//connect
void print_connect_command(struct connect_command_t *connect_command)
{
	printf("connect_command [username:%s address:%s port:%d]\n", connect_command->username, connect_command->address, connect_command->port);
}

int validate_connect_command(struct connect_command_t *connect_command)
{
	int max = 31;
	int min = 6;
	int len = strlen(connect_command->username);
	
	if(len < min || len > max) {
		printf("Invalid username size:%d. It should be between 6-31.\n", len);
		return 0;
	}
	
	if(connect_command->port < 1024 && connect_command->port > 65535) {
		printf("Invalid port number:%d.\n", connect_command->port);
		return 0;
	}
	
	return 1;
}

int parse_connect_command(char *input, struct connect_command_t *connect_command) {
	char *username;
	char *address;
	char *port;
	
	int input_size = strlen(input) + 1;
	char buf[input_size];
	
	//printf("input_size:%d\n", input_size);
	memcpy(buf, input, input_size);
	buf[input_size] = '\0';
	
	//printf("parse:%s!\n", buf);
	
	if(strtok(buf, " @:") == NULL)
		return 0;
		
	username = strtok(NULL, " @:");
	if(username == NULL)
		return 0;
	
	address = strtok(NULL, " @:");
	if(address == NULL)
		return 0;
		
	port = strtok(NULL, " @:");
	if(port == NULL)
		return 0;
		
	if(strtok(NULL, " ") != NULL)
		return 0;
		
	strcpy(connect_command->username, username);
	strcpy(connect_command->address, address);
	connect_command->port = atoi(port);
	return 1;
}

int parse_and_validate_connect_command(char *input, struct connect_command_t *connect_command)
{
	if(parse_connect_command(input, connect_command)) {
		if(validate_connect_command(connect_command)) {
			return 1;
		}
	} 
	return 0;
}
//room
int validate_room_command(struct room_command_t *room_command)
{
	int len = strlen(room_command->room_name);
	int min = 6;
	int max = 32;
	
	if(len < min || len > max) {
		printf("Invalid roomname size:%d. It should be between 6-32.\n", len);
		return 0;
	}
	
	return 1;
}

int parse_room_command(char *input, struct room_command_t *room_command)
{
	char *room_name;
	int input_size = strlen(input) + 1;
	char buf[input_size];
	
	memcpy(buf, input, input_size);
	buf[input_size] = '\0';
	
	if(strtok(buf, " ") == NULL) 
		return 0;
	
	room_name = strtok(NULL, " \n");
	if(room_name == NULL)
		return 0;
		
	if(strtok(NULL, " ") != NULL)
		return 0;
		
	strcpy(room_command->room_name, room_name);
	return 1;
}

int parse_and_validate_room_command(char *input, struct room_command_t *room_command)
{
	if(parse_room_command(input, room_command)) {
		if(validate_room_command(room_command)) {
			return 1;
		}
	}
	return 0;
}


int validate_leave_command(char *input)
{
	if(strcmp(input, "!leave") == 0)
		return 1;
		
	return 0;
}

int validate_bye_command(char *input)
{
	if(strcmp(input, "!bye") == 0)
		return 1;
		
	return 0;
}

int validate_rooms_command(char *input)
{
	if(strcmp(input, "!rooms") == 0)
		return 1;
		
	return 0;
}

int validate_files_command(char *input)
{
	if(strcmp(input, "!files") == 0)
		return 1;
		
	return 0;
}

//push
int validate_file_command(struct file_command_t *file_command)
{
	int len = strlen(file_command->file_name);
	int min = 6;
	int max = 32;
	
	if(len < min || len > max) {
		printf("Invalid filename size:%d. It should be between 6-32.\n", len);
		return 0;
	}
	
	return 1;
}

int parse_file_command(char *input, struct file_command_t *file_command)
{
	char *file_name;
	int input_size = strlen(input) + 1;
	char buf[input_size];
	
	memcpy(buf, input, input_size);
	buf[input_size] = '\0';
	
	if(strtok(buf, " ") == NULL) 
		return 0;
	
	file_name = strtok(NULL, " \n");
	if(file_name == NULL)
		return 0;
		
	if(strtok(NULL, " ") != NULL)
		return 0;
		
	strcpy(file_command->file_name, file_name);
	return 1;
}

int parse_and_validate_file_command(char *input, struct file_command_t *file_command)
{
	if(parse_file_command(input, file_command)) {
		if(validate_file_command(file_command)) {
			return 1;
		}
	}
	return 0;
}

//push response
int parse_push_response(char *input, struct push_response_t *push_response)
{
	char *file_name;
	int input_size = strlen(input) + 1;
	char buf[input_size];
	
	memcpy(buf, input, input_size);
	buf[input_size] = '\0';
	
	if(strtok(buf, " ") == NULL) 
		return 0;
		
	file_name = strtok(NULL, " \n");
	if(file_name == NULL)
		return 0;
		
	if(strtok(NULL, " ") != NULL)
		return 0;
		
	strcpy(push_response->file_name, file_name);
	return 1;
}

int parse_and_validate_push_response(char *input, struct push_response_t *push_response)
{
	if(parse_push_response(input, push_response)) {
		return 1;
	}
	return 0;
}

//transfer
int parse_transfer_command(char *input, struct transfer_command_t *transfer_command)
{
	char *offset;
	char *len;
	char *data;
	
	int input_size = strlen(input) + 1;
	char buf[input_size];
	
	memcpy(buf, input, input_size);
	buf[input_size] = '\0';
	
	if(strtok(buf, " ") == NULL) 
		return 0;
		
	offset = strtok(NULL, " \n");
	if(offset == NULL)
		return 0;
		
	len = strtok(NULL, " \n");
	if(len == NULL)
		return 0;
		
	data = strtok(NULL, "\0");
	if(data == NULL)
		return 0;
	
	strcpy(transfer_command->data, data);
	transfer_command->offset = atoi(offset);
	transfer_command->len = atoi(len);
	
	//printf("parser:offset:%s! len:%s! data:%s!\n", offset, len, data);
	
	return 1;
}

int parse_and_validate_transfer_command(char *input, struct transfer_command_t *transfer_command)
{
	if(parse_transfer_command(input, transfer_command)) {
		return 1;
	}
	return 0;
}


//getCommand
void get_command(char *input, int *command)
{
	if(input[strlen(input) - 1] == '\n') {
		input[strlen(input) - 1] = '\0';
	}
	
	static char *commands[] = {
		"!connect",
		"!bye",
		"!rooms",
		"!open",
		"!close",
		"!enter",
		"!leave",
		"!files",
		"!push_response",
		"!push",
		"!pull",
		"!rm",
		"!transfer"
	};
	
	static int COMMAND_COUNT = sizeof(commands)/sizeof(char *);

	for(int i = 0; i < COMMAND_COUNT; ++i) {
		if(starts_with(input, commands[i])) {
			*command = i;
			return;
		}
	}
	
	if(input[0] == '*') {
		*command = MESSAGE;
		return;
	}	
	
	*command = -1;
}


