SUBDIRS=dumb flac mac mpc mpg123 ogg opus sndfile speex taglib tta++ vorbis wavpack

all: $(SUBDIRS)
clean: $(SUBDIRS)
.PHONY: all clean

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)
.PHONY: $(SUBDIRS)
