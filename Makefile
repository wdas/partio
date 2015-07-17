#!/usr/bin/env make
SH ?= sh
uname_S := $(shell $(SH) -c 'uname -s || system')
uname_R := $(shell $(SH) -c 'uname -r | sed -r "s/-.*//" || release')
uname_M := $(shell $(SH) -c 'uname -m || machine')
FLAVOR ?= optimize
platformdir ?= $(uname_S)-$(uname_R)-$(uname_M)-$(FLAVOR)
builddir ?= $(CURDIR)/build/$(platformdir)

prefix ?= $(CURDIR)/$(platformdir)
#DESTDIR =

# The default target in this Makefile is...
all::

install: all
	$(MAKE) -C $(builddir) DESTDIR=$(DESTDIR) install

$(builddir)/stamp:
	mkdir -p $(builddir)
	cd $(builddir) && cmake -DCMAKE_INSTALL_PREFIX=$(prefix) ../..
	touch $@

all:: $(builddir)/stamp
	$(MAKE) -C $(builddir) $(MAKEARGS) all

clean: $(builddir)/stamp
	$(MAKE) -C $(builddir) $(MAKEARGS) clean
