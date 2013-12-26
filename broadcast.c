#include <uwsgi.h>

/*

	Only Linux is supported:
	/dev/pts is listed, and for every charachter device found
	owned by the current user a "msg" is written

*/

#define PTS_PATH "/dev/pts/"

static int console_broadcast_do(char *msg) {
	uid_t uid = getuid();
	// root is not allowed
	if (uid == 0) return 0;
	DIR *pts = opendir(PTS_PATH);
	if (!pts) {
		uwsgi_error("console_broadcast_do()/opendir()");
		return -1;
	}
	for(;;) {
		struct dirent *de = readdir(pts);
		if (!de) break;
		if (de->d_name[0] == 0 || de->d_name[0] == '.') continue; 
		struct stat st;
		char *filename = uwsgi_concat2(PTS_PATH, de->d_name);
		if (stat(filename, &st)) {
			free(filename);
			continue;
		}
		if (S_ISCHR(st.st_mode) && st.st_uid == uid) {
			int fd = open(filename, O_WRONLY);
			if (fd < 0) {
				uwsgi_error_open(filename);
				free(filename);
				continue;
			}
			char *full_msg = uwsgi_concat3("\n", msg, "\n");
			ssize_t rlen = write(fd, full_msg, strlen(full_msg));
			if (rlen != (ssize_t) strlen(full_msg)) {
				uwsgi_error("console_broadcast_do()/write()");
			};
			free(full_msg);

		}
		free(filename);
	}

	closedir(pts);
	return 0;
}

static void console_broadcast_register() {
	uwsgi_register_hook("console_broadcast", console_broadcast_do);
}

struct uwsgi_plugin console_broadcast_plugin = {
	.name = "console_broadcast",
	.on_load = console_broadcast_register,
};
