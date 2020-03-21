#define _GNU_SOURCE
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define ERROR do { \
	return write_error(); \
} while(0)

static int write_error(void) {
	const char *buf = "failed to execute real tmux\n";
	size_t left = strlen(buf);
	while(1) {
		ssize_t n = write(2, buf, left);
		if(n <= 0) return 1;
		left -= n;
		if(left == 0) return 1;
		buf += n;
	}
}

__attribute__((always_inline)) inline static int make_path(char *buf, const char *pfx, size_t len, const char *sfx) {
	size_t slen = strlen(sfx) + 1;
	if(len > PATH_MAX - slen) {
		return 0;
	}
	memcpy(buf, pfx, len);
	memcpy(buf + len, sfx, slen);
	return 1;
}

extern char **environ;

int main(int argc, char **argv) {
	dev_t self_dev;
	ino_t self_ino;
	const char *path;
	char **argv2;
	struct stat st;
	char socket[PATH_MAX];
	char config[PATH_MAX];
	char pbuf[PATH_MAX];

	path = getenv("PATH");
	if(!path || stat("/proc/self/exe", &st) < 0) {
		ERROR;
	}
	self_dev = st.st_dev;
	self_ino = st.st_ino;

	// Assuming the running process is indeed at /tmux somewhere in $PATH,
	// set pathbuf past this so next time we find the real tmux
	while(1) {
		const char *last = strchr(path, ':');
		if(!last || !make_path(pbuf, path, last - path, "/tmux")) {
			ERROR;
		}
		path = last + 1;

		if(stat(pbuf, &st) >= 0 && self_dev == st.st_dev && self_ino == st.st_ino) {
			break;
		}
	}

	// Create XDG base directory paths
	argv2 = calloc(argc + 5, sizeof(char*));
	if(!argv2) {
		ERROR;
	}

	{
		const char *dir;
		if((dir = getenv("XDG_CONFIG_HOME"))) {
			make_path(config, dir, strlen(dir), "/tmux/tmux.conf");
		} else if((dir = getenv("HOME"))) {
			make_path(config, dir, strlen(dir), "/.config/tmux/tmux.conf");
		} else {
			ERROR;
		}
	}

	{
		const char *dir = getenv("XDG_RUNTIME_DIR");
		if(dir) {
			make_path(socket, dir, strlen(dir), "/tmux");
		} else {
			sprintf(socket, "/run/user/%u/tmux", (unsigned)getuid());
		}
	}

	// Construct tmux arguments
	argv2[0] = argv[0];
	argv2[1] = (char*)"-f";	// Specify alternative configuration file
	argv2[2] = config;
	argv2[3] = (char*)"-S";	// Specify alternative server socket path
	argv2[4] = socket;
	for(int i = 1; i <= argc; i++) {
		argv2[4 + i] = argv[i];
	}

	// Find and run the real tmux
	while(1) {
		const char *last = strchr(path, ':');
		if(!last || !make_path(pbuf, path, last ? (size_t)(last - path) : strlen(path), "/tmux")) {
			ERROR;
		}
		execve(pbuf, argv2, environ);
		if(last) {
			path = last + 1;
		} else {
			ERROR;
			return 1;
		}
	}
}

