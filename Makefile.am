SUBDIRS         = man src
doc_DATA        = README.md LICENSE xplugrc
EXTRA_DIST      = README.md LICENSE xplugrc
DISTCLEANFILES  = *~ DEADJOE semantic.cache *.gdb *.elf core core.* *.d

## Target to run when building a release
release: distcheck
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
