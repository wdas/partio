#!/usr/bin/env make
FLAVOR ?= optimize
prefix ?= $(CURDIR)/$(shell uname)-$(shell fa.arch -r)-$(shell uname -m)-$(FLAVOR)
DESTDIR	=
CXX ?= g++
CXXFLAGS ?=

install:
	@echo "Installing $(prefix)"
	scons --prefix="$(DESTDIR)$(prefix)" CXX=$(CXX) CXXFLAGS=$(CXXFLAGS)

clean:
	scons -c --prefix="$(DESTDIR)$(prefix)"

cleanall: clean
