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
			struct timeval tv;
			char ctime_storage[26];
			gettimeofday(&tv, NULL);
#if defined(__sun__) && !defined(__clang__)
                	ctime_r((const time_t *) &tv.tv_sec, ctime_storage, 26);
#else
                	ctime_r((const time_t *) &tv.tv_sec, ctime_storage);
#endif
			size_t pos = strlen(ctime_storage);
			ctime_storage[pos-1] = ' ';

			char *full_msg = uwsgi_concat4("\n", ctime_storage,  msg, "\n");
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

static void console_broadcast_alarm_init(struct uwsgi_alarm_instance *uai) {
}

static void console_broadcast_alarm_func(struct uwsgi_alarm_instance *uai, char *msg, size_t len) {
	char *msg2 = NULL;
	if (uai->arg && uai->arg[0] != 0) {
		msg2 = uwsgi_concat2n(uai->arg, strlen(uai->arg), "", 0);
	}
	else {
		msg2 = uwsgi_concat2n(msg, len, "", 0);
	}

	console_broadcast_do(msg2);
	free(msg2);
}

static void console_broadcast_register() {
	uwsgi_register_hook("console_broadcast", console_broadcast_do);
	uwsgi_register_alarm("console_broadcast", console_broadcast_alarm_init, console_broadcast_alarm_func);
}

struct uwsgi_plugin console_broadcast_plugin = {
	.name = "console_broadcast",
	.on_load = console_broadcast_register,
};
