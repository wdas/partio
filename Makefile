prefix ?= $(CURDIR)/build
flags ?=

JEKYLL ?= jekyll

-include config.mak

all:: install

install:
	$(JEKYLL) build --destination "$(prefix)" $(flags)
