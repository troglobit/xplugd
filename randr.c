/* x randr handler
 *
 * Copyright (C) 2012-2015  Stefan Bolte <portix@gmx.net>
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

#include "xplugd.h"
#include "edid.h"

static char *con_actions[] = { "connected", "disconnected", "unknown" };

static int get_monitor_name(const char *name, Display *display, XRRScreenResources *res, char *monitor_name, size_t len)
{
	struct monitor_info *info = NULL;
	int i, j, np;
	Atom *p;

	for (i = 0; i < res->noutput; ++i) {
		XRROutputInfo *output_info = XRRGetOutputInfo(display, res,
							      res->outputs[i]);

		if (!output_info)
			continue;

		if (output_info->connection != RR_Connected)
			continue;

		if (strcmp(output_info->name, name)) {
			continue;
		} else {
			p = XRRListOutputProperties(display, res->outputs[i], &np);
			for (j = 0; j < np; ++j) {
				unsigned char *prop;
				int actual_format;
				unsigned long nitems, bytes_after;
				Atom actual_type;

				XRRGetOutputProperty(display, res->outputs[i], p[j], 0, 128,
						     False, False, AnyPropertyType, &actual_type, &actual_format,
						     &nitems, &bytes_after, &prop);

				if (!strcmp(XGetAtomName(display, p[j]), "EDID")) {
					if (nitems >= 128) {
						info = edid_decode(prop);
					} else {
						syslog(LOG_INFO,
						       "Not enough EDID data found. Need at least 128 bytes, got %lu bytes",
						       nitems);
					}
					break;
				}
			}
			break;
		}
	}

	if (!info) {
		syslog(LOG_INFO, "Failed decoding EDID data: %s", strerror(errno));
		return -1;
	}

	syslog(LOG_DEBUG, "MODEL: %s", info->dsc_product_name);
	strncpy(monitor_name, info->dsc_product_name, len);
	free(info);

	return 0;
}

static void handle_event(Display *dpy, XRROutputChangeNotifyEvent *ev)
{
	char msg[MSG_LEN];
	static char old_msg[MSG_LEN] = "";
	XRROutputInfo *info;
	XRRScreenResources *resources;
	char monitor_name[14] = { 0 };

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

	if (!strcmp(con_actions[info->connection], "connected"))
		get_monitor_name(info->name, dpy, resources, monitor_name, sizeof(monitor_name));

	exec("display", info->name, con_actions[info->connection], monitor_name);
 done:
	XRRFreeOutputInfo(info);
	XRRFreeScreenResources(resources);
}

int randr_init(Display *dpy)
{
	XRRSelectInput(dpy, DefaultRootWindow(dpy), RROutputChangeNotifyMask);
	return 0;
}

int randr_event(Display *dpy, XEvent *ev)
{
	handle_event(dpy, (XRROutputChangeNotifyEvent *)ev);
	return 0;
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
