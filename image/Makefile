IMAGES=$(shell ls *.png) 
OBJS=$(IMAGES:.png=.o)
TARGET=libimage.a
HEADER=image.h
OBJCOPY=/usr/bin/objcopy

.PHONY: all clean depend

all: $(OBJS) $(HEADER) $(TARGET)

%.o: %.png
	$(OBJCOPY) -I binary -O elf64-x86-64 -B i386:x86-64 $< $@

$(HEADER): $(OBJS)
	echo "#pragma once" > $(HEADER)
	echo '#ifndef INCLUDED_RESOURCE_IMAGE_H_' >> $(HEADER)
	echo '#define INCLUDED_RESOURCE_IMAGE_H_' >> $(HEADER)
	echo 'extern "C" {' >> $(HEADER)
	nm $^ \
		| awk '{print $$3}' \
		| grep '^_binary_' \
		| grep -E '_(start|end)$$' \
		| sed -e 's/^/    extern const char /' -e 's/$$/[];/' \
		>> $(HEADER)
	echo '}' >> $(HEADER)
	echo '#endif' >> $(HEADER)

$(TARGET): $(OBJS)
	ar rsv $@ $^

clean:
	$(RM) $(OBJS) $(HEADER) $(TARGET)

depend:
