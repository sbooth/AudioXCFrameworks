SUBDIRS := dumb flac lame mac mpc mpg123 ogg opus sndfile speex taglib tta-cpp vorbis wavpack
export PREFIX ?= $(CURDIR)

all: $(SUBDIRS)
install: $(SUBDIRS)
clean: $(SUBDIRS)
uninstall: $(SUBDIRS)
.PHONY: all install clean uninstall

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)
.PHONY: $(SUBDIRS)
