xplugd - monitor plug/unplug helper
===================================

`xplugd` is a simple daemon that detects when a monitor is plugged in or
is unplugged.  It is very useful in combination with lightweight setups,
e.g. when running a simple X window manager like [Awesome][1].


Usage
-----

Program usage:

    xplugd [option] /path/to/script
    
    -h        Show help text and exit
    -l LEVEL  Set log level: none, err, info, notice*, debug
    -n        Run in foreground, do not fork to background
    -v        Show version info and exit

The script is called with the following arguments, prepared for future
addition of support for input devices as well as the current support for
output devices:

    sample.script TYPE DEVICE STATUS [Optional Description]
                   |    |      |
                   |    |      `---- connected or disconnected
                   |    `----------- HDMI3, LVDS1, VGA1, etc.
                   `---------------- keyboard, pointer, display

Example how a script is called, notice the last argument "LG Display"
may not be included (reserved for future input device support):

    sample.script display HDMI3 disconnected

or, in the future:

    sample.script keyboard 3 connected "Topre Corporation Realforce 87"

Here keyboard, or pointer, will always be the slave keyboard and pointer
and status encoding will be `XIStatusEnabled` and `XIStatusDisabled` for
connected and disconnected, respectively.


Example
-------

Example script

```shell
    #!/bin/sh
    LAPTOP=LVDS1
    DOCK=HDMI3
    DESKPOS=--left-of
    PRESPOS=--right-of
    
    # Script only supports disply hotplugging atm.
    if [ "$1" = "display" ] || exit 0
    
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


Origin & References
-------------------

`xplugd` was originally written by Stefan Bolte, who still maintains it
as `srandrd` at [BitBucket][2].  In 2016 Joachim Nilsson forked it as
`xplugd` at [GitHub][3] for further development.

[1]: https://awesome.naquadah.org
[2]: http://bitbucket.org/portix/srandrd
[3]: https://github.com/troglobit/xplugd
