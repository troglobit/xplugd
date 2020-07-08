SUBDIRS         = man src
doc_DATA        = README.md LICENSE xplugrc
EXTRA_DIST      = README.md LICENSE xplugrc
DISTCLEANFILES  = *~ DEADJOE semantic.cache *.gdb *.elf core core.* *.d

package:
	@debuild -uc -us -B --lintian-opts --profile debian -i -I --show-overrides

## Target to run when building a release
release: distcheck package
	@for file in $(DIST_ARCHIVES); do	\
		md5sum $$file > ../$$file.md5;	\
	done
	@mv $(DIST_ARCHIVES) ../
	@echo
	@echo "Resulting release files in ../"
	@echo "================================================================="
	@for file in $(DIST_ARCHIVES); do					\
		printf "%-32s Distribution tarball\n" $$file;			\
		printf "%-32s " $$file.md5; cat ../$$file.md5 | cut -f1 -d' ';	\
	done
	@for file in `cd ..; ls $(PACKAGE)_$(VERSION)*`; do			\
		printf "%-32s Debian/Ubuntu file\n" $$file;			\
	done
