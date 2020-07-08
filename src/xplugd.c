/* xplugd - monitor/keyboard/mouse plug/unplug helper
 *
 * Copyright (C) 2012-2015  Stefan Bolte <portix@gmx.net>
 * Copyright (C) 2013-2015  Andrew Shadura <andrewsh@debian.org>
 * Copyright (C) 2016-2020  Joachim Nilsson <troglobit@gmail.com>
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

#define SYSLOG_NAMES
#include <alloca.h>
#include <glob.h>
#include "xplugd.h"

int loglevel = LOG_NOTICE;
char *cmd;
char *prognm;

static char *tilde_expand(char *path)
{
	glob_t gl;
	char *rc;
	int flags = GLOB_ERR;

#ifdef GLOB_TILDE
	/* E.g. musl libc < 1.1.21 does not have this GNU LIBC extension  */
	flags |= GLOB_TILDE;
#else
	/* Simple homegrown replacement that at least handles leading ~/ */
	if (!strncmp(path, "~/", 2)) {
		size_t len;
		char *buf, *home;

		home = getenv("HOME");
		if (!home)
			return NULL;

		len = strlen(home) + strlen(path);
		buf = alloca(len);

		snprintf(buf, len, "%s/%s", home, &path[2]);
		path = buf;
	}
#endif

	if (glob(path, flags, NULL, &gl))
		return NULL;

	if (gl.gl_pathc < 1)
		return NULL;

	rc = strdup(gl.gl_pathv[0]);
	globfree(&gl);

	return rc;
}

/*
 * Returns the ~/ expanded path to the rc file.  If no user supplied
 * path exists the following paths are checked:
 *  - $XDG_CONFIG_HOME/xplugrc
 *  - ~/.config/xplugrc
 *  - ~/.xplugrc
 */
static char *rcfile(char *arg)
{
	char *home;

	if (arg)
		return tilde_expand(arg);

	home = getenv("XDG_CONFIG_HOME");
	if (home) {
		size_t len = strlen(home) + 9;
		char *path;

		path = malloc(len);
		if (!path)
			return NULL;

		snprintf(path, len, "%s/xplugrc", home);
		arg = tilde_expand(path);
	}

	if (!arg)
		arg = tilde_expand(XPLUGRC);

	if (!arg)
		arg = tilde_expand(XPLUGRC_FALLBACK);

	return arg;
}

static int loglvl(char *level)
{
	for (int i = 0; prioritynames[i].c_name; i++) {
		if (!strcmp(prioritynames[i].c_name, level))
			return prioritynames[i].c_val;
	}

	return atoi(level);
}

static int error_handler(Display *display)
{
	exit(1);
}

static int usage(int status)
{
	printf("Usage: %s [-hnpsv] [-l LEVEL] [FILE]\n\n"
	       "Options:\n"
	       "  -h        Print this help text and exit\n"
	       "  -l LEVEL  Set log level: none, err, info, notice*, debug\n"
	       "  -n        Run in foreground, do not fork to background\n"
	       "  -p        Probe currently connected outputs and output EDID info\n"
	       "  -s        Use syslog, even if running in foreground, default w/o -n\n"
	       "  -v        Show program version\n"
	       "\n"
	       " FILE       Optional script argument, default $XDG_CONFIG_HOME/xplugrc\n"
	       "            Fallback also checks for ~/.config/xplugrc and ~/.xplugrc\n"
	       "\n"
	       "Copyright (C) 2012-2015  Stefan Bolte\n"
	       "Copyright (C) 2016-2020  Joachim Nilsson\n\n"
	       "Bug report address: %s\n", prognm, PACKAGE_BUGREPORT);
	return status;
}

static int version(void)
{
	printf("v%s\n", PACKAGE_VERSION);
	return 0;
}

static char *progname(char *arg0)
{
	char *nm;

	nm = strrchr(arg0, '/');
	if (nm)
		nm++;
	else
		nm = arg0;

	return nm;
}

int main(int argc, char *argv[])
{
	Display *dpy;
	XEvent ev;
	char *arg = NULL;
	int background = 1;
	int log_opts = LOG_CONS | LOG_PID;
	int logcons = 0;
	int mode = 0;
	int c;

	prognm = progname(argv[0]);
	while ((c = getopt(argc, argv, "hl:npsv")) != EOF) {
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

		case 'p':
			mode = 1;
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

	dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		fprintf(stderr, "Cannot open display\n");
		exit(1);
	}

	if (mode)
		return randr_probe(dpy);

	if (optind < argc)
		arg = argv[optind];

	cmd = rcfile(arg);
	if (!cmd)
		return usage(1);

	if (background) {
		if (daemon(0, 0)) {
			fprintf(stderr, "Failed backgrounding %s: %s", prognm, strerror(errno));
			exit(1);
		}
	}
	if (logcons > 0)
		log_opts |= LOG_PERROR;

	openlog(prognm, log_opts, LOG_USER);
	setlogmask(LOG_UPTO(loglevel));

	exec_init(dpy);
	input_init(dpy);
	randr_init(dpy);
	XSync(dpy, False);
	XSetIOErrorHandler((XIOErrorHandler)error_handler);

	while (1) {
		XNextEvent(dpy, &ev);

		if (is_input_event(dpy, &ev))
			input_event(dpy, &ev);
		else
			randr_event(dpy, &ev);
	}

	return 0;
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
