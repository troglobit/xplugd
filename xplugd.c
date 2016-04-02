/* xplugd - monitor/keyboard/mouse plug/unplug helper
 *
 * Copyright (C) 2012-2015  Stefan Bolte <portix@gmx.net>
 * Copyright (C) 2013-2015  Andrew Shadura <andrewsh@debian.org>
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
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#define SYSLOG_NAMES
#include <syslog.h>
#include <unistd.h>
#include <sys/wait.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/Xrandr.h>

#define OCNE(X) ((XRROutputChangeNotifyEvent*)X)
#define MSG_LEN 128

static int   loglevel = LOG_NOTICE;
static char *con_actions[] = { "connected", "disconnected", "unknown" };
extern char *__progname;
static char *cmd;

struct pair {
    int key;
    char * value;
};

#define T(x) {x, #x}
#define T_END {0, NULL}


static const struct pair *map(int key, const struct pair *table, bool strict)
{
    if (!table)
	    return NULL;

    while (table->value) {
	    if ((!strict && (table->key & key)) || (table->key == key))
		    return table;

	    table++;
    }

    return NULL;
}

const struct pair device_types[] = {
    T(XIMasterPointer),
    T(XIMasterKeyboard),
    { XISlavePointer,  "pointer" },
    { XISlaveKeyboard, "keyboard" },
    T(XIFloatingSlave),
    T_END
};

const struct pair changes[] = {
    T(XIMasterAdded),
    T(XIMasterRemoved),
    T(XISlaveAdded),
    T(XISlaveRemoved),
    T(XISlaveAttached),
    T(XISlaveDetached),
    { XIDeviceEnabled,  "connected"    },
    { XIDeviceDisabled, "disconnected" },
    T_END
};

static int loglvl(char *level)
{
	for (int i = 0; prioritynames[i].c_name; i++) {
		if (!strcmp(prioritynames[i].c_name, level))
			return prioritynames[i].c_val;
	}

	return atoi(level);
}

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

#define STRINGIFY(x) #x
#define EXPAND_STRINGIFY(x) STRINGIFY(x)
#define UINT_MAX_STRING EXPAND_STRINGIFY(UINT_MAX)

static int handle_device(int id, int type, int flags, char *name)
{
	const struct pair *use = map(type, device_types, true);
	const struct pair *change = map(flags, changes, false);

	if (change) {
		char deviceid[strlen(UINT_MAX_STRING) + 1];
		char *args[] = {
			cmd,
			use ? use->value : "",
			deviceid,
			change->value,
			name ? name : "",
			NULL
		};

		if (type != XISlavePointer && type != XISlaveKeyboard) {
			syslog(LOG_DEBUG, "Skipping dev %d type %s flags %s name %s", id, use ? use->value : "", change->value, name);
			return 0;
		}
		if (flags != XIDeviceEnabled && flags != XIDeviceDisabled) {
			syslog(LOG_DEBUG, "Skipping dev %d type %s flags %s name %s", id, use ? use->value : "", change->value, name);
			return 0;
		}

		snprintf(deviceid, sizeof(deviceid), "%d", id);
		syslog(LOG_DEBUG, "Calling %s %s %s %s %s", cmd, use->value, deviceid, change->value, name ? name : "");
		execv(args[0], args);

		return change->key;
	}

	return -1;
}

static char *get_device_name(Display *display, int deviceid)
{
        XIDeviceInfo *info;
        int i, num_devices;
        char *name = NULL;

        info = XIQueryDevice(display, XIAllDevices, &num_devices);
	if (!info)
		return NULL;

        for (i = 0; i < num_devices; i++) {
		if (info[i].deviceid == deviceid) {
			name = strdup(info[i].name);
			break;
		}
        }
        XIFreeDeviceInfo(info);

        return name;
}

static void parse_event(XIHierarchyEvent *event)
{
	int i;

	for (i = 0; i < event->num_info; i++) {
		int flags = event->info[i].flags;
		int j = 16;

		while (flags && j) {
			int ret = 0;
			char *name;

			name = get_device_name(event->display, event->info[i].deviceid);
			if (!name)
				break;
			
			ret = handle_device(event->info[i].deviceid, event->info[i].use, flags, name);
			free(name);
			if (ret == -1)
				break;

			j--;
			flags -= ret;
		}
	}
}

static void handle_event(Display *dpy, XRROutputChangeNotifyEvent *ev)
{
	char msg[MSG_LEN];
	static char old_msg[MSG_LEN] = "";
	XRROutputInfo *info;
	XRRScreenResources *resources;

	resources = XRRGetScreenResources(ev->display, ev->window);
	if (!resources) {
		syslog(LOG_ERR, "Could not get screen resources");
		return;
	}

	info = XRRGetOutputInfo(ev->display, resources, ev->output);
	if (!info) {
		syslog(LOG_ERR, "Could not get output info");
		XRRFreeScreenResources(resources);
		return;
	}

	/* Check for duplicate plug events */
	snprintf(msg, sizeof(msg), "%s %s", info->name, con_actions[info->connection]);
	if (!strcmp(msg, old_msg)) {
		if (loglevel == LOG_DEBUG)
			syslog(LOG_DEBUG, "Same message as last time, time %lu, skipping ...", info->timestamp);
		goto done;
	}
	strcpy(old_msg, msg);

	if (loglevel == LOG_DEBUG) {
		syslog(LOG_DEBUG, "Event: %s %s", info->name, con_actions[info->connection]);
		syslog(LOG_DEBUG, "Time: %lu", info->timestamp);
		if (info->crtc == 0) {
			syslog(LOG_DEBUG, "Size: %lumm x %lumm", info->mm_width, info->mm_height);
		} else {
			XRRCrtcInfo *crtc;

			syslog(LOG_DEBUG, "CRTC: %lu", info->crtc);

			crtc = XRRGetCrtcInfo(dpy, resources, info->crtc);
			if (crtc) {
				syslog(LOG_DEBUG, "Size: %dx%d", crtc->width, crtc->height);
				XRRFreeCrtcInfo(crtc);
			}
		}
	}

	syslog(LOG_DEBUG, "Calling %s ...", cmd);
	if (!fork()) {
		char *args[] = {
			cmd,
			"display",
			info->name,
			con_actions[info->connection],
			NULL,
		};

		setsid();
		if (dpy)
			close(ConnectionNumber(dpy));

		syslog(LOG_DEBUG, "Calling %s %s %s %s %s", cmd, "display", info->name, con_actions[info->connection], "");
		execv(args[0], args);
		syslog(LOG_ERR, "Failed calling %s: %s", cmd, strerror(errno));
		exit(0);
	}

done:
	XRRFreeOutputInfo(info);
	XRRFreeScreenResources(resources);
}

static int usage(int status)
{
	printf("Usage: %s [OPTIONS] script\n\n"
	       "Options:\n"
	       "  -h        Print this help text and exit\n"
	       "  -l LEVEL  Set log level: none, err, info, notice*, debug\n"
	       "  -n        Run in foreground, do not fork to background\n"
	       "  -s        Use syslog, even if running in foreground, default w/o -n\n"
	       "  -v        Show program version\n\n"
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
	int c, log_opts = LOG_CONS | LOG_PID;
	XEvent ev;
	Display *dpy;
	XIEventMask mask;
	int xi_opcode, event, error;
	int background = 1, logcons = 0;
	uid_t uid;

	while ((c = getopt(argc, argv, "hl:nsv")) != EOF) {
		switch (c) {
		case 'h':
			return usage(0);

		case 'l':
			loglevel = loglvl(optarg);
			break;

		case 'n':
			background = 0;
			logcons++;
			break;

		case 's':
			logcons--;
			break;

		case 'v':
			return version();

		default:
			return usage(1);
		}
	}

	if (optind >= argc)
		return usage(1);
	cmd = argv[optind];

	if (((uid = getuid()) == 0) || uid != geteuid()) {
		fprintf(stderr, "%s may not run as root\n", __progname);
		exit(1);
	}

	if ((dpy = XOpenDisplay(NULL)) == NULL) {
		fprintf(stderr, "Cannot open display\n");
		exit(1);
	}

	if (!XQueryExtension(dpy, "XInputExtension", &xi_opcode, &event, &error)) {
		fprintf(stderr, "X Input extension not available\n");
		exit(1);
	}

	if (background) {
		if (daemon(0, 0)) {
			fprintf(stderr, "Failed backgrounding %s: %s", __progname, strerror(errno));
			exit(1);
		}
	}
	if (logcons > 0)
		log_opts |= LOG_PERROR;

	openlog(NULL, log_opts, LOG_USER);
	setlogmask(LOG_UPTO(loglevel));

	signal(SIGCHLD, catch_child);

	mask.deviceid = XIAllDevices;
	mask.mask_len = XIMaskLen(XI_LASTEVENT);
	mask.mask = calloc(mask.mask_len, sizeof(char));
	XISetMask(mask.mask, XI_HierarchyChanged);
	XISelectEvents(dpy, DefaultRootWindow(dpy), &mask, 1);
	XRRSelectInput(dpy, DefaultRootWindow(dpy), RROutputChangeNotifyMask);
	XSync(dpy, False);
	XSetIOErrorHandler((XIOErrorHandler)error_handler);

	while (1) {
		if (!XNextEvent(dpy, &ev)) {
			XGenericEventCookie *cookie = (XGenericEventCookie*)&ev.xcookie;

			if (XGetEventData(dpy, cookie)) {
				if (cookie->type == GenericEvent &&
				    cookie->extension == xi_opcode &&
				    cookie->evtype == XI_HierarchyChanged)
					parse_event(cookie->data);

				XFreeEventData(dpy, cookie);
				continue;
			}

			handle_event(dpy, OCNE(&ev));
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
