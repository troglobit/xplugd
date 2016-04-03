/* x input handler
 *
 * Copyright (C) 2013-2015  Andrew Shadura <andrewsh@debian.org>
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

#define T(x) {x, #x}
#define T_END {0, NULL}

#define STRINGIFY(x) #x
#define EXPAND_STRINGIFY(x) STRINGIFY(x)
#define UINT_MAX_STRING EXPAND_STRINGIFY(UINT_MAX)

struct pair {
    int key;
    char * value;
};

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

static int xi_opcode = -1;

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

static int handle_device(int id, int type, int flags, char *name)
{
	const struct pair *use = map(type, device_types, true);
	const struct pair *change = map(flags, changes, false);

	if (change) {
		char deviceid[strlen(UINT_MAX_STRING) + 1];

		if (type != XISlavePointer && type != XISlaveKeyboard) {
			syslog(LOG_DEBUG, "Skipping dev %d type %s flags %s name %s", id, use ? use->value : "", change->value, name);
			return 0;
		}
		if (flags != XIDeviceEnabled && flags != XIDeviceDisabled) {
			syslog(LOG_DEBUG, "Skipping dev %d type %s flags %s name %s", id, use ? use->value : "", change->value, name);
			return 0;
		}

		snprintf(deviceid, sizeof(deviceid), "%d", id);
		exec(use->value, deviceid, change->value, name);

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

static void handle_event(XIHierarchyEvent *event)
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

int input_init(Display *dpy)
{
	XIEventMask mask;
	int event, error;

	if (!XQueryExtension(dpy, "XInputExtension", &xi_opcode, &event, &error)) {
		syslog(LOG_ERR, "X Input extension not available\n");
		exit(1);
	}

	mask.deviceid = XIAllDevices;
	mask.mask_len = XIMaskLen(XI_LASTEVENT);
	mask.mask     = calloc(mask.mask_len, sizeof(char));
	if (!mask.mask) {
		syslog(LOG_ERR, "Failed initializing X input module: %s", strerror(errno));
		exit(1);
	}

	XISetMask(mask.mask, XI_HierarchyChanged);
	XISelectEvents(dpy, DefaultRootWindow(dpy), &mask, 1);

	return 0;
}

int is_input_event(Display *dpy, XEvent *ev)
{
	XGenericEventCookie *c = (XGenericEventCookie*)&ev->xcookie;

	if (!XGetEventData(dpy, c))
		return 0;

	if (c->type == GenericEvent && c->extension == xi_opcode && c->evtype == XI_HierarchyChanged)
		return 1;

	XFreeEventData(dpy, c);

	return 0;
}

int input_event(Display *dpy, XEvent *ev)
{
	XGenericEventCookie *c = (XGenericEventCookie*)&ev->xcookie;

	handle_event(c->data);
	XFreeEventData(dpy, c);

	return 0;
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
