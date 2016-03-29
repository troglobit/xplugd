srandrd - simple randr daemon 
=============================

`srandrd` is a simple daemon that detects when amonitor is plugged in or
is unplugged.  It is very useful in combination with lightweight setups,
e.g. when running a simple X window manager like [Awesome][1].


Usage
-----

    srandrd [option] /path/to/script [optional script args]


Options
-------

    -h  Show help and exit
    -n  Do not fork to background
    -v  Verbose output, useful while debugging
    -V  Show version info and exit


Example
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

[1]: https://awesome.naquadah.org
[2]: http://bitbucket.org/portix/srandrd
[3]: https://github.com/troglobit/srandard
