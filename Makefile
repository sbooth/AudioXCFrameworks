SUBDIRS := dumb flac mac mpc mpg123 ogg opus sndfile speex taglib tta++ vorbis wavpack
export PREFIX ?= $(CURDIR)

all: $(SUBDIRS)
install: $(SUBDIRS)
clean: $(SUBDIRS)
cleaninstall: $(SUBDIRS)
.PHONY: all install clean cleaninstall

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)
.PHONY: $(SUBDIRS)
