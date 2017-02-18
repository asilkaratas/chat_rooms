#ifndef _FILE_DATA_H
#define _FILE_DATA_H

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

struct file_data_t {
	char file_name[FILE_NAME_SIZE];
	char owner_name[USER_NAME_SIZE];
	struct file_data_t *next;
};

#endif















