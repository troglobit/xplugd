/* See LICENSE for copyright and license details
 * srandrd - simple randr daemon
 */
#include <errno.h>
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

static int help(int status)
{
	printf("Usage: %s [OPTIONS] /path/to/script [optional script args]\n\n"
	       "Options:\n"
	       "   -h  Print this help and exit\n"
	       "   -n  Do not fork to background\n"
	       "   -v  Enable verbose debug output to stdout\n"
	       "   -V  Print version information and exit\n", __progname);
	return status;
}

static int version(void)
{
	printf("    This is : %s\n"
	       "    Version : " VERSION "\n"
	       "  Builddate : " __DATE__ " " __TIME__ "\n"
	       "  Copyright : " COPYRIGHT "\n"
	       "    License : " LICENSE "\n", __progname);
	return 0;
}

int main(int argc, char **argv)
{
	XEvent ev;
	Display *dpy;
	int daemonize = 1, args = 1, verbose = 0;
	char msg[MSG_LEN], old_msg[MSG_LEN] = "";
	uid_t uid;

	if (argc < 2)
		help(1);

	for (args = 1; args < argc && *(argv[args]) == '-'; args++) {
		switch (argv[args][1]) {
		case 'V':
			version();

		case 'n':
			daemonize = 0;
			break;

		case 'v':
			verbose++;
			break;

		case 'h':
			return help(0);

		default:
			return help(1);
		}
	}

	if (argv[args] == NULL)
		help(1);

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
				setenv("SRANDRD_ACTION", msg, False);
				XRRFreeScreenResources(resources);
				XRRFreeOutputInfo(info);
				execvp(argv[args], &(argv[args]));
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
