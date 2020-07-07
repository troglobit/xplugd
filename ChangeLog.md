Change Log
==========

All notable changes to the project are documented in this file.


[v1.3][] - 20018-02-01
----------------------

### Changes
- Portability, replace `__progname` with a small function
- Make script argument optional, default to `~/.xplugrc`
- Add support for monitor description using EDID, by Magnus Malm

### Fixes
- Fix SynPS/2 matching, `conntected` vs `connected`


[v1.2][] - 20016-06-25
----------------------

### Changes
- Converted to GNU configure and build system
- Add support for Travis-CI

### Fixes
- Issue #1: Use `sigaction()` instead of ambiguous SysV `signal()` API
- Fix missing keyboard/mouse disconnect events
- Portability fix, use `__progname` as identity in `openlog()`, gives
  clearer log messages on non-GLIBC systems


[v1.1][] - 20016-04-03
----------------------

Now with `xinput(1)` support -- one daemon to detect both display and
keyboard/mouse events!  Ideas and code from [inputplug][].


[v1.0][] - 20016-04-01
----------------------

First official release after fork from [srandrd][]

- Handles monitor plug/unplug events
- Can be used to seamlessly dock/undock
- Prepared for future support for xinput(1)

	
[v1.3]: https://github.com/troglobit/xplugd/compare/v1.3...v1.3
[v1.2]: https://github.com/troglobit/xplugd/compare/v1.1...v1.2
[v1.1]: https://github.com/troglobit/xplugd/compare/v1.0...v1.1
[v1.0]: https://github.com/troglobit/xplugd/compare/v0.5...v1.0
[srandrd]:   https://bitbucket.org/portix/srandrd
[inputplug]: https://bitbucket.org/andrew_shadura/inputplug
