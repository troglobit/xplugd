bin_PROGRAMS    = xplugd

xplugd_SOURCES  = xplugd.c xplugd.h exec.c input.c randr.c edid.c edid.h
xplugd_CFLAGS   = -W -Wall -Wextra -std=c99
xplugd_CFLAGS  += -D_POSIX_C_SOURCE=200809L -D_BSD_SOURCE -D_DEFAULT_SOURCE
xplugd_CFLAGS  += $(X11_CFLAGS) $(Xi_CFLAGS) $(Xrandr_CFLAGS)
xplugd_LDADD    = $(X11_LIBS) $(Xi_LIBS) $(Xrandr_LIBS)
