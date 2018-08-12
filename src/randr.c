/* x randr handler
 *
 * Copyright (C) 2012-2015  Stefan Bolte <portix@gmx.net>
 * Copyright (C) 2016-2018  Joachim Nilsson <troglobit@gmail.com>
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

static char color_type_names[4][16] = {
	"Undefined",
	"Monochrome",
	"RGB",
	"Other"
};

static char iface_type_names[6][16] = {
	"Undefined",
	"DVI",
	"HDMI-A",
	"HDMI-B",
	"MDDI",
	"Display Port"
};

static struct monitor_info *edid_info(Display *dpy, XID output, Atom prop)
{
	unsigned long nitems, bytes_after;
	unsigned char *data;
	Atom actual_type;
	int actual_format;

	XRRGetOutputProperty(dpy, output, prop, 0, 128, False, False,
			     AnyPropertyType, &actual_type, &actual_format,
			     &nitems, &bytes_after, &data);

	if (nitems < 128) {
		syslog(LOG_INFO,
		       "Not enough EDID data found.  Need at least 128 bytes, got %lu bytes", nitems);
		return NULL;
	}

	return edid_decode(data);
}

static void edid_desc(Display *dpy, XRRScreenResources *res, const char *output, char *buf, size_t len)
{
	struct monitor_info *info = NULL;
	Atom *p;
	int i;

	for (i = 0; i < res->noutput; ++i) {
		XRROutputInfo *output_info;
		int j, np;

		output_info = XRRGetOutputInfo(dpy, res, res->outputs[i]);
		if (!output_info)
			continue;

		if (output_info->connection != RR_Connected)
			continue;

		if (strcmp(output_info->name, output))
			continue;

		p = XRRListOutputProperties(dpy, res->outputs[i], &np);
		for (j = 0; j < np; ++j) {
			if (strcmp(XGetAtomName(dpy, p[j]), "EDID"))
				continue;

			info = edid_info(dpy, res->outputs[i], p[j]);
		}
		break;
	}

	if (!info) {
		syslog(LOG_INFO, "Failed decoding EDID data: %s", strerror(errno));
		return;
	}

	syslog(LOG_DEBUG, "MODEL: %s S/N: %s EXTRA: %s", info->dsc_product_name,
	       info->dsc_serial_number, info->dsc_string);
	strncpy(buf, info->dsc_product_name, len);
	free(info);
}

static void handle_event(Display *dpy, XRROutputChangeNotifyEvent *ev)
{
	static char old_msg[MSG_LEN] = "";
	XRRScreenResources *res;
	XRROutputInfo *info;
	char desc[14] = { 0 };
	char msg[MSG_LEN];

	res = XRRGetScreenResources(ev->display, ev->window);
	if (!res) {
		syslog(LOG_ERR, "Could not get screen resources");
		return;
	}

	info = XRRGetOutputInfo(ev->display, res, ev->output);
	if (!info) {
		syslog(LOG_ERR, "Could not get output info");
		XRRFreeScreenResources(res);
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

			crtc = XRRGetCrtcInfo(dpy, res, info->crtc);
			if (crtc) {
				syslog(LOG_DEBUG, "Size: %dx%d", crtc->width, crtc->height);
				XRRFreeCrtcInfo(crtc);
			}
		}
	}

	if (!strcmp(con_actions[info->connection], "connected"))
		edid_desc(dpy, res, info->name, desc, sizeof(desc));

	exec("display", info->name, con_actions[info->connection], desc);
 done:
	XRRFreeOutputInfo(info);
	XRRFreeScreenResources(res);
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

int randr_probe(Display *dpy)
{
	struct monitor_info *info;
	XRRScreenResources *res;
	Window root;
	Atom *p;
	int i;

	root = RootWindow(dpy, DefaultScreen(dpy));
	res = XRRGetScreenResources(dpy, root);
	if (!res)
		return 1;

	for (i = 0; i < res->noutput; ++i) {
		XRROutputInfo *output_info;
		int j, np;

		output_info = XRRGetOutputInfo(dpy, res, res->outputs[i]);
		if (!output_info)
			continue;

		if (output_info->connection != RR_Connected)
			continue;

		p = XRRListOutputProperties(dpy, res->outputs[i], &np);
		for (j = 0; j < np; ++j) {
			if (strcmp(XGetAtomName(dpy, p[j]), "EDID"))
				continue;

			info = edid_info(dpy, res->outputs[i], p[j]);
			if (!info) {
				printf("No EDID info for output %s\n", output_info->name);
				break;
			}

			print_edid_heading("", output_info->name, "\n", 0);
			print_edid_str("Model", info->dsc_product_name, "\n", 1);
			print_edid_str("Serial Nr.", info->dsc_serial_number, "\n", 1);
			print_edid_integer("Width", info->width_mm, "\n", 1);
			print_edid_integer("Height", info->height_mm, "\n", 1);
			print_edid_double("Aspect Ratio", info->aspect_ratio, "\n", 1);
			print_edid_double("Gamma", info->gamma, "\n", 1);
			print_edid_integer("Prod. Year", info->production_year, "\n", 1);
			print_edid_integer("Prod. Week", info->production_week, "\n", 1);
			print_edid_integer("Model Year", info->model_year, "\n", 1);
			print_edid_str("Extra", info->dsc_string, "\n", 1);

			print_edid_heading("", "DPMS", "\n", 1);
			print_edid_bool("Standby", info->standby, "\n", 2);
			print_edid_bool("Suspend", info->suspend, "\n", 2);
			print_edid_bool("Active Off", info->active_off, "\n", 2);

			if (info->is_digital) {
				print_edid_str("Interface", iface_type_names[info->digital.interface], "\n", 1);
				print_edid_heading("", "Display Type (digital)", "\n", 1);
				print_edid_bool("RGB 4:4:4", info->digital.rgb444, "\n", 2);
				print_edid_bool("YCrCb 4:4:4", info->digital.ycrcb444, "\n", 2);
				print_edid_bool("YCrCb 4:2:2", info->digital.ycrcb422, "\n", 2);
			} else {
				print_edid_heading("", "Display Type (analog)", "\n", 1);
				print_edid_str("", color_type_names[info->analog.color_type], "\n", 2);
			}

			char tmp[10];
			snprintf(tmp, sizeof(tmp), "%d.%d",
				 info->major_version, info->minor_version);
			print_edid_str("EDID Version", tmp, "\n", 1);
			break;
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
