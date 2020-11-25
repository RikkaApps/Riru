#pragma once

#include <sys/types.h>
#include <sys/un.h>

socklen_t setup_sockaddr(struct sockaddr_un *sun, const char *name);
int get_client_cred(int fd, struct ucred *cred);