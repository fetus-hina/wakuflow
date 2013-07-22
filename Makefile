SUBDIRS=image source

.PHONY: all clean depend

all: $(SUBDIRS)
clean: $(SUBDIRS)
depend: $(SUBDIRS)

$(SUBDIRS): FORCE
	$(MAKE) -C $@ $(MAKECMDGOALS)

FORCE:
