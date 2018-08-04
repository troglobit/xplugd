xplugd - X plug daemon
======================
[![Travis Status][]][Travis] [![Coverity Status]][Coverity Scan]

`xplugd` is a UNIX daemon that executes a script on X input and RandR
changes, i.e., when a, keyboard, mouse. or a monitor is plugged in or
unplugged.  Useful in combination with lightweight setups, e.g. when
running an X window manager like [Awesome][1], Fluxbox, or similar to
detect when docking or undocking a laptop.


Usage
-----

    xplugd [-hnpsv] [-l LEVEL] [FILE]
    
    -h        Show help text and exit
    -l LEVEL  Set log level: none, err, info, notice*, debug
    -n        Run in foreground, do not fork to background
    -p        Probe currently connected outputs and output EDID info.
    -s        Use syslog, even if running in foreground, default w/o -n
    -v        Show version info and exit
    
    FILE       Optional script file argument, default ~/.xplugrc

When `FILE` is omitted `xplugd` defaults to use `~/.xplugrc`.  This file
is called as a shell script on plug events with the following arguments:

    ~/.xplugrc TYPE DEVICE STATUS ["Optional Description"]
                |    |      |
                |    |       `---- connected or disconnected
                |     `----------- HDMI3, LVDS1, VGA1, etc.
                 `---------------- keyboard, pointer, display

The script may be called like this, notice how the description is not
included for displays:

    ~/.xplugrc display HDMI3 disconnected
    ~/.xplugrc keyboard 3 connected "Topre Corporation Realforce 87"

The keyboard or pointer is always the X slave keyboard or pointer, and
the status encoding for `XIStatusEnabled` and `XIStatusDisabled` is
forwarded to the script as connected and disconnected, respectively.

If EDID data is available from a connected display, the monitor model is
passed in as fourth argument ("Optional Description") to the script.


### Example ~/.xplugrc

```sh
#!/bin/sh
LAPTOP=LVDS1
DOCK=HDMI3
DESKPOS=--left-of
PRESPOS=--right-of

if [ "$1" != "display" ]; then
    case "$1,$3,$4" in
        pointer,conntected,"SynPS/2 Synaptics TouchPad")
            xinput set-prop $2 'Synaptics Off' 1
            ;;
        keyboard,connected,*)
            setxkbmap -option ctrl:nocaps
            ;;
    esac
    exit 0
fi

if [ "$3" = "disconnected" ]; then
    xrandr --output $2 --off
    exit 0
fi

if [ "$2" = "${DOCK}" ]; then
    xrandr --output $2 --auto --primary ${DESKPOS} ${LAPTOP}
elif  [ "$1" != "${LAPTOP}" ]; then
    xrandr --output $2 --auto ${PRESPOS} ${LAPTOP} --primary
else
    xrandr --auto
fi
```


Build & Install
---------------

To build `xplugd` you need the standard libraries and header files for
X11, X11 input, and Xrandr.  On a Debian/Ubuntu system these files can
be installed with:

    sudo apt install libx11-dev libxi-dev libxrandr-dev

Then run the configure script and make:

    ./configure && make

Unless building from the GIT sources, in which case `./autogen.sh` first
must be called to create the configure script.  With relased tarballs this
is not necessary.

To change the default installation prefix from `/usr/local`, use the

    ./configure --prefix=/some/other/path

Followed by

    make all && sudo make install-strip


Origin & References
-------------------

[`xplugd`][2] is composed from pieces of Stefan Bolte's [`srandrd`][3]
and Andrew Shadura's [`inputplug`][4].  Please report bugs and problems
to the `xplugd` project.

[1]: https://awesome.naquadah.org
[2]: https://github.com/troglobit/xplugd
[3]: https://bitbucket.org/portix/srandrd
[4]: https://bitbucket.org/andrew_shadura/inputplug
[Travis]:        https://travis-ci.org/troglobit/xplugd
[Travis Status]: https://travis-ci.org/troglobit/xplugd.png?branch=master
[Coverity Scan]:   https://scan.coverity.com/projects/10739
[Coverity Status]: https://scan.coverity.com/projects/10739/badge.svg

<!--
  -- Local Variables:
  -- mode: markdown
  -- End:
  -->
