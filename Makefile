prefix ?= $(CURDIR)/build
flags ?=

all:: install

install:
	jekyll build --destination "$(prefix)" $(flags)
