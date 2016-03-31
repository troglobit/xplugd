/* xplugd - monitor plug/unplug helper
 *
 * Copyright (C) 2012-2015  Stefan Bolte <portix@gmx.net>
 * Copyright (C) 2016       Joachim Nilsson <troglobit@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#define OCNE(X) ((XRROutputChangeNotifyEvent*)X)
#define MSG_LEN 128

static char *con_actions[] = { "connected", "disconnected", "unknown" };
extern char *__progname;

static int error_handler(void)
{
	exit(1);
}

static void catch_child(int sig)
{
	(void)sig;
	while (waitpid(-1, NULL, WNOHANG) > 0)
		;
}

static int usage(int status)
{
	printf("Usage: %s [OPTIONS] /path/to/script [optional script args]\n\n"
	       "Options:\n"
	       "   -h  Print this help text and exit\n"
	       "   -n  Run in foreground, do not fork to background\n"
	       "   -v  Enable verbose debug output to stdout\n"
	       "   -V  Print version information and exit\n\n"
	       "Copyright (C) 2012-2015 Stefan Bolte\n"
	       "Copyright (C)      2016 Joachim Nilsson\n\n"
	       "Bug report address: https://github.com/troglobit/xplugd/issues\n\n", __progname);
	return status;
}

static int version(void)
{
	printf("v%s\n", VERSION);
	return 0;
}

int main(int argc, char *argv[])
{
	int c;
	XEvent ev;
	Display *dpy;
	int daemonize = 1, verbose = 0;
	char msg[MSG_LEN], old_msg[MSG_LEN] = "";
	uid_t uid;

	while ((c = getopt(argc, argv, "hnvV")) != EOF) {
		switch (c) {

		case 'V':
			return version();

		case 'n':
			daemonize = 0;
			break;

		case 'v':
			verbose++;
			break;

		case 'h':
			return usage(0);

		default:
			return usage(1);
		}
	}

	if (optind >= argc)
		return usage(1);

	if (((uid = getuid()) == 0) || uid != geteuid()) {
		fprintf(stderr, "%s may not run as root\n", __progname);
		exit(1);
	}

	if ((dpy = XOpenDisplay(NULL)) == NULL) {
		fprintf(stderr, "Cannot open display\n");
		exit(1);
	}

	if (daemonize) {
		if (daemon(0, 0)) {
			fprintf(stderr, "Failed daemonizing: %s\n", strerror(errno));
			exit(1);
		}
	}
	signal(SIGCHLD, catch_child);

	XRRSelectInput(dpy, DefaultRootWindow(dpy), RROutputChangeNotifyMask);
	XSync(dpy, False);
	XSetIOErrorHandler((XIOErrorHandler)error_handler);

	while (1) {
		if (!XNextEvent(dpy, &ev)) {
			XRROutputInfo *info;
			XRRScreenResources *resources;

			resources = XRRGetScreenResources(OCNE(&ev)->display, OCNE(&ev)->window);
			if (!resources) {
				fprintf(stderr, "Could not get screen resources\n");
				continue;
			}

			info = XRRGetOutputInfo(OCNE(&ev)->display, resources, OCNE(&ev)->output);
			if (!info) {
				XRRFreeScreenResources(resources);
				fprintf(stderr, "Could not get output info\n");
				continue;
			}

			/* Check for duplicate plug events */
			snprintf(msg, sizeof(msg), "%s %s", info->name, con_actions[info->connection]);
			if (!strcmp(msg, old_msg)) {
				if (verbose)
					printf("Same message as last time, time %lu, skipping ...\n", info->timestamp);
				XRRFreeScreenResources(resources);
				XRRFreeOutputInfo(info);
				continue;
			}
			strcpy(old_msg, msg);

			if (verbose) {
				printf("Event: %s %s\n", info->name, con_actions[info->connection]);
				printf("Time: %lu\n", info->timestamp);
				if (info->crtc == 0) {
					printf("Size: %lumm x %lumm\n", info->mm_width, info->mm_height);
				} else {
					printf("CRTC: %lu\n", info->crtc);
					XRRCrtcInfo *crtc = XRRGetCrtcInfo(dpy, resources, info->crtc);

					if (crtc != NULL) {
						printf("Size: %dx%d\n", crtc->width, crtc->height);
						XRRFreeCrtcInfo(crtc);
					}
				}
			}

			XRRFreeScreenResources(resources);
			XRRFreeOutputInfo(info);

			if (fork() == 0) {
				if (dpy)
					close(ConnectionNumber(dpy));
				setsid();
				setenv("XPLUG_EVENT", msg, False);
				XRRFreeScreenResources(resources);
				XRRFreeOutputInfo(info);
				execvp(argv[optind], &(argv[optind]));
				exit(0);	/* We only get here if execvp() fails */
			}
		}
	}

	return 0;
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
