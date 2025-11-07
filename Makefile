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

NINJA_OK := $(shell type ninja >/dev/null 2>&1 && echo 1)
ifeq ($(NINJA_OK),1)
    CMAKE_FLAGS += -G Ninja
endif
# Allow out-of-band customization
-include Makefile.config

ifdef V
    VERBOSE=1
    export VERBOSE
endif

# Installation location: prefix=<path>
CMAKE_FLAGS += -DCMAKE_INSTALL_PREFIX=$(prefix)

# Extra cmake flags: CMAKE_EXTRA_FLAGS=<flags>
ifdef CMAKE_EXTRA_FLAGS
    CMAKE_FLAGS += $(CMAKE_EXTRA_FLAGS)
endif

cmake_ready = $(builddir)/cmake.ready

# The default target in this Makefile is...
all::

install: all
	cmake --build $(builddir) --target install

test: all
	cmake --build $(builddir) --target test

doc: $(cmake_ready)
	cmake --build $(builddir) --target doc

all:: $(cmake_ready)
	cmake --build $(builddir) --target all

clean: $(cmake_ready)
	cmake --build $(builddir) --target clean

$(cmake_ready):
	cmake -S . -B $(builddir) $(CMAKE_FLAGS)
	touch $@
