#!/usr/bin/env make
#prefix ?= $(CURDIR)/$(shell uname)-$(shell fa.arch -r)-$(shell uname -m)-optimize
prefix ?= $(CURDIR)/$(shell uname)-$(shell fa.arch -r)-$(shell uname -m)
DESTDIR	=

install:
	@echo "Installing $(prefix)"
	scons --prefix="$(DESTDIR)$(prefix)" #"$(DESTDIR)$(prefix)"

clean:
	scons -c --prefix="$(DESTDIR)$(prefix)" #"$(DESTDIR)$(prefix)"

cleanall: clean
