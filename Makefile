#!/usr/bin/env make
SH ?= sh
uname_S := $(shell $(SH) -c 'uname -s || echo kernel')
uname_R := $(shell $(SH) -c 'uname -r | cut -d- -f1 || echo release')
uname_M := $(shell $(SH) -c 'uname -m || echo machine')
FLAVOR ?= optimize
platformdir ?= $(uname_S)-$(uname_R)-$(uname_M)-$(FLAVOR)
builddir ?= $(CURDIR)/build/$(platformdir)

prefix ?= $(CURDIR)/$(platformdir)
#DESTDIR =

CMAKE_FLAGS ?= -DCMAKE_INSTALL_PREFIX=$(prefix)
ifdef RP_SeExpr
    CMAKE_FLAGS += -DSEEXPR_BASE=$(RP_SeExpr)
endif

# The default target in this Makefile is...
all::

install: all
	$(MAKE) -C $(builddir) DESTDIR=$(DESTDIR) install

$(builddir)/stamp:
	mkdir -p $(builddir)
	cd $(builddir) && cmake $(CMAKE_FLAGS) ../..
	touch $@

all:: $(builddir)/stamp
	$(MAKE) -C $(builddir) $(MAKEARGS) all

clean: $(builddir)/stamp
	$(MAKE) -C $(builddir) $(MAKEARGS) clean
