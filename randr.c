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

int get_monitor_name(const char *name, char *monitor_name, size_t len)
{
	char path[255];
	FILE *fp = NULL;
	size_t result;
	unsigned char edid[128];
	MonitorInfo *info;

	snprintf(path, sizeof(path), "/sys/class/drm/card0-%s/edid", name);
	syslog(LOG_DEBUG, "DRM device sysfs path %s", path);
	fp = fopen(path, "rb");
	if (!fp) {
		syslog(LOG_ERR, "Failed to find sys path for %s", name);
		return -1;
	}

	result = fread(edid, 1, sizeof(edid), fp);
	if (result != 128) {
		syslog(LOG_DEBUG, "No EDID data found at DRM device sysfs path %s", path);
		return -1;
	}

	info = decode_edid(edid);
	if (!info) {
		syslog(LOG_ERR, "decode failure");
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
	char monitor_name[14] = {0};

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
		get_monitor_name(info->name, monitor_name, sizeof(monitor_name));

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
