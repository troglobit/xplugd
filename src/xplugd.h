/* xplugd - monitor/keyboard/mouse plug/unplug helper
 *
 * Copyright (C) 2012-2015  Stefan Bolte <portix@gmx.net>
 * Copyright (C) 2013-2015  Andrew Shadura <andrewsh@debian.org>
 * Copyright (C) 2016-2019  Joachim Nilsson <troglobit@gmail.com>
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

#ifndef XPLUGD_H_
#define XPLUGD_H_

#include "config.h"
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/wait.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/Xrandr.h>

#define MSG_LEN           128
#define XPLUGRC           "~/.config/xplugrc"
#define XPLUGRC_FALLBACK  "~/.xplugrc"

extern int loglevel;
extern char *cmd;
extern char *prognm;

int exec_init      (Display *dpy);
int exec           (char *type, char *device, char *status, char *name);

int input_init     (Display *dpy);
int is_input_event (Display *dpy, XEvent *ev);
int input_event    (Display *dpy, XEvent *ev);

int randr_init     (Display *dpy);
int randr_event    (Display *dpy, XEvent *ev);
int randr_probe    (Display *dpy);

#endif /* XPLUGD_H_ */

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
