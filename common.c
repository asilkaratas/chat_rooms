#include "common.h"

//IO
ssize_t bulk_read(int fd, char *buf, size_t count)
{
	int c;
	size_t len = 0;

	do {
		c = TEMP_FAILURE_RETRY(read(fd, buf, count));
		if (c < 0)
			return c;
		if (c == 0)
			return len;
		buf += c;
		len += c;
		count -= c;
	} while (count > 0);

	return len;
}

ssize_t bulk_write(int fd, char *buf, size_t count)
{
	int c;
	size_t len = 0;

	do {
		c = TEMP_FAILURE_RETRY(write(fd, buf, count));
		if(c < 0)
			return c;
		buf += c;
		len += c;
		count -= c;
	} while (count > 0);

	return len;
}


//setHandler
void sethandler(void (*f)(int), int sigNo)
{
	struct sigaction act;
	memset(&act, 0x00, sizeof(struct sigaction));
	act.sa_handler = f;

	if (-1 == sigaction(sigNo, &act, NULL))
		ERR("sigaction");
}


int make_socket(void)
{
	int sock;
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		ERR("socket");

	return sock;
}

int add_new_client(int sfd)
{
	int nfd;
	if ((nfd = TEMP_FAILURE_RETRY(accept(sfd, NULL, NULL))) < 0) {
		if (EAGAIN == errno || EWOULDBLOCK == errno)
			return -1;
		ERR("accept");
	}

	return nfd;
}

int bind_tcp_socket(uint16_t port)
{
	struct sockaddr_in addr;
	int socketfd, t=1;

	socketfd = make_socket();
	memset(&addr, 0x00, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t)))
		ERR("setsockopt");
	if (bind(socketfd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
		ERR("bind");
	if (listen(socketfd, BACKLOG) < 0)
		ERR("listen");

	return socketfd;
}

struct sockaddr_in make_address(char *address, uint16_t port)
{
	//printf("make_address:%s!\n", address);
	
	struct sockaddr_in addr;
	struct hostent *hostinfo;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	hostinfo = gethostbyname(address);
	if (hostinfo == NULL)
		HERR("gethostbyname");

	addr.sin_addr = *(struct in_addr *) hostinfo->h_addr;

	return addr;
}

int connect_socket(struct sockaddr_in *addr)
{
	int socketfd;
	socketfd = make_socket();
	if (connect(socketfd, (struct sockaddr *) addr, sizeof(struct sockaddr_in)) < 0) {
		if (errno != EINTR) {
			ERR("connect");
		} else { 
			fd_set wfds;
			int status;
			socklen_t size = sizeof(int);
			FD_ZERO(&wfds);
			FD_SET(socketfd, &wfds);
			
			if (TEMP_FAILURE_RETRY(select(socketfd + 1, NULL, &wfds, NULL, NULL)) < 0)
				ERR("select");
			if (getsockopt(socketfd, SOL_SOCKET, SO_ERROR, &status, &size) < 0)
				ERR("getsockopt");
			if (status != 0)
				ERR("connect");
		}
	}

	return socketfd;
}


//sendMessage
int send_message_to_socket(int socket, char *msg)
{
	char buf[MSG_SIZE_MAX];
	int msgSize;
	
	msgSize = strlen(msg);
	if(msgSize > MSG_SIZE) {
		printf("Invalid message size:%d. It should be max:%d.", msgSize, MSG_SIZE);
		return 0;
	}
	
	if (msg[msgSize - 1] == '\n')
		msgSize -= 1;
	
	memcpy(buf, msg, msgSize);
	
	buf[msgSize] = '\0';
		
	if (bulk_write(socket, (void *) buf, MSG_SIZE_MAX) < 0) 
		ERR("write");
	
	return 1;
}




