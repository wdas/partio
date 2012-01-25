#!/usr/bin/env make
#prefix ?= $(CURDIR)/$(shell uname)-$(shell fa.arch -r)-$(shell uname -m)-optimize
prefix ?= $(CURDIR)/$(shell uname)-$(shell fa.arch -r)-$(shell uname -m)
DESTDIR	=
CXX ?= g++
CXXFLAGS ?=

install:
	@echo "Installing $(prefix)"
	scons --prefix="$(DESTDIR)$(prefix)" CXX=$(CXX) CXXFLAGS=$(CXXFLAGS) #"$(DESTDIR)$(prefix)"
	echo "bin" > ${DESTDIR}${prefix}/.release.partio
	echo "lib64" >> ${DESTDIR}${prefix}/.release.partio
	echo "share" >> ${DESTDIR}${prefix}/.release.partio
	echo "include" >> ${DESTDIR}${prefix}/.release.partio

clean:
	scons -c --prefix="$(DESTDIR)$(prefix)" #"$(DESTDIR)$(prefix)"

cleanall: clean
