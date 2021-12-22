APP = haxdiff
OBJS = haxdiff.o sblist.o

prefix = /usr/local
bindir = $(prefix)/bin

-include config.mak

all: $(APP)

haxdiff: $(OBJS)

clean:
	rm -f $(APP) $(OBJS)

install: $(APP)
	install -Dm 755 $(APP) $(DESTDIR)$(bindir)/$(APP)

check: $(APP)
	cd test ; sh test.sh

.PHONY: all clean install check
