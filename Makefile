SUBDIRS := dumb ebur128 fishsound flac lame mac mpc mpg123 ogg oggz opus sndfile speex taglib tta-cpp vorbis wavpack
export PREFIX ?= $(CURDIR)

all: $(SUBDIRS)
install: $(SUBDIRS)
xz: $(SUBDIRS)
zip: $(SUBDIRS)
clean: $(SUBDIRS)
uninstall: $(SUBDIRS)
.PHONY: all install xz zip clean uninstall

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)
.PHONY: $(SUBDIRS)
