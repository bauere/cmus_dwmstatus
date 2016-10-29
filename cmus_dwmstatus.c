#define _DEFAULT_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <X11/Xlib.h>

#define MUSIC_INFO "/usr/bin/cmus-remote -Q"
#define SLEEP_TIME 1000000L

#define PLAY '>'
#define PAUSE '|'

#define STATUS       1
#define ALBUM        2
#define ALBUM_ARTIST 3
#define ARTIST       4
#define TITLE        5
#define TRACK        6
#define OTHER        0

static Display *dpy;

char *smprintf(char *fmt, ...);

char *
nowplaying()
{
	char *ret = NULL;
	char status = NULL; /* playing / paused */

	char *album_artist = NULL;
	char *artist = NULL;
	char *title = NULL;

	char *tmpbuf = NULL;
	size_t len = 0;
	int line = OTHER;

	FILE *music = popen(MUSIC_INFO, "r");
	if (!music)
		return "n/a";
	
	while (getline(&tmpbuf, &len, music) >= 0) {

		if (strncmp(tmpbuf, "status", 6) == 0)
			line = STATUS;
		else if (strncmp(tmpbuf, "tag albumartist", 15) == 0)
			line = ALBUM_ARTIST;
		else if (strncmp(tmpbuf, "tag artist", 10) == 0)
			line = ARTIST;
		else if (strncmp(tmpbuf, "tag title", 9) == 0)
			line = TITLE;

		switch(line) {
			case STATUS:
			if (strcmp(tmpbuf, "status playing\n") == 0)
				status = PLAY;
			else
				status = PAUSE;
			break;

			case ALBUM_ARTIST:
			album_artist = strdup(tmpbuf+16);
			album_artist[strlen(album_artist)-1] = '\0';
			break;

			case ARTIST:
			artist = strdup(tmpbuf+11);
			artist[strlen(artist)-1] = '\0';
			break;

			case TITLE:
			title = strdup(tmpbuf+10);
			title[strlen(title)-1] = '\0';
			break;

			case OTHER:
			break;
		}
		line = OTHER;	
	}

	if (artist)
		ret = smprintf("%s: %s [%c]", artist, title, status);
	else
		ret = smprintf("%s: %s [%c]", album_artist, title, status);

	free(tmpbuf);
	free(album_artist);
	free(artist);
	free(title);
	fclose(music);

	return ret;
}	

char *
smprintf(char *fmt, ...)
{
	va_list fmtargs;
	char *ret;
	int len;

	va_start(fmtargs, fmt);
	len = vsnprintf(NULL, 0, fmt, fmtargs);
	va_end(fmtargs);

	ret = malloc(++len);
	if (ret == NULL) {
		perror("malloc");
		exit(1);
	}

	va_start(fmtargs, fmt);
	vsnprintf(ret, len, fmt, fmtargs);
	va_end(fmtargs);

	return ret;
}

char *
mktimes(char *fmt)
{
	char buf[129];
	time_t tim;
	struct tm *timtm;

	memset(buf, 0, sizeof(buf));
	tim = time(NULL);
	timtm = localtime(&tim);
	if (timtm == NULL) {
		perror("localtime");
		exit(1);
	}

	if (!strftime(buf, sizeof(buf)-1, fmt, timtm)) {
		fprintf(stderr, "strftime == 0\n");
		exit(1);
	}

	return smprintf("%s", buf);
}

void
setstatus(char *str)
{
	XStoreName(dpy, DefaultRootWindow(dpy), str);
	XSync(dpy, False);
}

int
main(void)
{
	char *status;
	char *time;
	char *music;

	struct timespec ts;
	ts.tv_nsec = SLEEP_TIME;
	ts.tv_sec = 0;

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "dwmstatus: cannot open display.\n");
		return 1;
	}

	for (;;nanosleep(&ts, NULL)) {
		time = mktimes("%H:%M");
		music = nowplaying();
		status = smprintf("%s %s", music, time);
		setstatus(status);

		free(time);
		free(music);
		free(status);
	}

	XCloseDisplay(dpy);

	return 0;
}

