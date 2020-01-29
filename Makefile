#!/usr/bin/env make
SH ?= sh
uname_S := $(shell $(SH) -c 'uname -s || echo kernel')
uname_R := $(shell $(SH) -c 'uname -r | cut -d- -f1 || echo release')
uname_M := $(shell $(SH) -c 'uname -m || echo machine')
lib ?= lib64
FLAVOR ?= optimize
platformdir ?= $(uname_S)-$(uname_R)-$(uname_M)-$(FLAVOR)
builddir ?= $(CURDIR)/build/$(platformdir)

prefix ?= $(CURDIR)/$(platformdir)
#DESTDIR =

CMAKE_FLAGS =
# Allow out-of-band customization
-include Makefile.config

ifdef V
    VERBOSE=1
    export VERBOSE
endif

# Installation location: prefix=<path>
CMAKE_FLAGS += -DCMAKE_INSTALL_PREFIX=$(prefix)

# gtest location: RP_gtest=<path>
ifdef RP_gtest
    CMAKE_FLAGS += -DGTEST_LOCATION=$(RP_gtest)
    CMAKE_FLAGS += -DGTEST_ENABLED=1
endif

# Extra cmake flags: CMAKE_EXTRA_FLAGS=<flags>
ifdef CMAKE_EXTRA_FLAGS
    CMAKE_FLAGS += $(CMAKE_EXTRA_FLAGS)
endif

ifdef RP_zlib
    CMAKE_FLAGS += -DZLIB_INCLUDE_DIR=$(RP_zlib)/include
    CMAKE_FLAGS += -DZLIB_LIBRARY_RELEASE=$(RP_zlib)/$(lib)/libz.so
endif

# The default target in this Makefile is...
all::

install: all
	$(MAKE) -C $(builddir) DESTDIR=$(DESTDIR) install

test: all
	$(MAKE) -C $(builddir) DESTDIR=$(DESTDIR) test

$(builddir)/stamp:
	mkdir -p $(builddir)
	cd $(builddir) && cmake $(CMAKE_FLAGS) ../..
	touch $@

all:: $(builddir)/stamp
	$(MAKE) -C $(builddir) $(MAKEARGS) all

clean: $(builddir)/stamp
	$(MAKE) -C $(builddir) $(MAKEARGS) clean
