bin_PROGRAMS    = xplugd
dist_man1_MANS  = xplugd.1
xplugd_SOURCES  = xplugd.c xplugd.h exec.c input.c randr.c edid-parse.c edid.h
xplugd_CFLAGS   = -W -Wall -Wextra -std=c99
xplugd_CFLAGS  += -D_POSIX_C_SOURCE=200809L -D_BSD_SOURCE -D_DEFAULT_SOURCE
xplugd_CFLAGS  += $(X11_CFLAGS) $(Xi_CFLAGS) $(Xrandr_CFLAGS)
xplugd_LDADD    = $(X11_LIBS) $(Xi_LIBS) $(Xrandr_LIBS)

doc_DATA        = README.md LICENSE xplugd.script
EXTRA_DIST      = README.md LICENSE xplugd.script
DISTCLEANFILES  = *~ DEADJOE semantic.cache *.gdb *.elf core core.* *.d

## Target to run when building a release
release: dist
	@for file in $(DIST_ARCHIVES); do	\
		md5sum $$file > ../$$file.md5;	\
	done
	@mv $(DIST_ARCHIVES) ../
	@echo
	@echo "Resulting release files:"
	@echo "================================================================="
	@for file in $(DIST_ARCHIVES); do					\
		printf "$$file    \tDistribution tarball\n";			\
		printf "$$file.md5\t"; cat ../$$file.md5 | cut -f1 -d' ';	\
	done
