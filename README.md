srandrd - simple randr daemon 
=============================

srandrd is a simple daemon that executes a command when the xorg output
changes, i.e. a monitor is plugged on or is unplugged.  srandrd forks to
the background by default and exits when the X server exits.


USAGE
-----

    srandrd [option] /path/to/script [optional script args]


OPTIONS
-------

    -h  Show help and exit
    -n  Do not fork to background
    -v  Verbose output, useful while debugging
    -V  Show version info and exit


EXAMPLE
-------

Example script

```shell
    #!/bin/sh
    # If not DOCK'ed output or LAPTOP, then it's likely VGA or Display Port
    # used for presenation.  We assume the primary desktop screen (HDMI3) is
    # placed left of the internal laptop screen, while during presentations
    # the projector is on our right and the laptop screen is the primary.
    
    LAPTOP=LVDS1
    DOCK=HDMI3
    DESKPOS=--left-of
    PRESPOS=--right-of
    
    set ${SRANDRD_ACTION}
    
    if [ "$2" = "disconnected" ]; then
        xrandr --output $1 --off
        exit 0
    fi
    
    if [ "$1" = "${DOCK}" ]; then
        xrandr --output $1 --auto --primary ${DESKPOS} ${LAPTOP}
    elif  [ "$1" != "${LAPTOP}" ]; then
        xrandr --output $1 --auto ${PRESPOS} ${LAPTOP} --primary
    else
        xrandr --auto
    fi
```


Origin & References
-------------------

`srandrd` was originally written by Stefan Bolte, who still maintains
the project at [BitBucket][2].  In 2016 Joachim Nilsson forked the code
at [GitHub][3] for further development.

