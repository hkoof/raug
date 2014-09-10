#!/usr/bin/make -f

#DESTDIR = /usr/local
VERSION = 0.2

CC = gcc
CFLAGS = -Wall -O2 -DVERSION=\"$(VERSION)\"
#CFLAGS = -ggdb -Wall -DDEBUG -DVERSION=\"$(VERSION)\"

INST_OWN = root
INST_GRP = root
INST_PERM = 755

#  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

PROGNAME = raug
SOURCES = raug.c
MANPAGE = raug.8
# HEADERS = err.h
OBJECTS = $(PROGNAME).o
TARGETS = $(PROGNAME)

DISTFILES = $(SOURCES) $(HEADERS) $(MANPAGE) Makefile README COPYING debian

# -----------------------------------------------------------------------------------------

DISTDIR = $(PROGNAME)-$(VERSION)
DISTTGZ = $(DISTDIR).tar.gz

# =========================================================================================


.PHONY: default all distcheck dist uninstall install distclean clean realclean

default: $(PROGNAME) $(TARGETS)
all: $(PROGNAME) $(TARGETS) TAGS 

# dependencies

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(PROGNAME): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $(PROGNAME) $(OBJECTS)

clean:
	-rm -f *.o

distclean: clean
	-rm -f $(TARGETS)
	-rm -f TAGS
	-rm -f *~
	-rm -f $(DISTTGZ)
	-rm -rf $(DISTDIR)

realclean: distclean
	debian/rules clean

install: $(PROGNAME)
	install -d $(DESTDIR)/usr/sbin
	install -s -o $(INST_OWN) -g $(INST_GRP) -m $(INST_PERM) $(PROGNAME) $(DESTDIR)/usr/sbin

uninstall:
	rm $(DESTDIR)/usr/sbin/$(PROGNAME)

tags: TAGS

TAGS: $(SOURCES)
	etags $(SOURCES)

dist: $(DISTTGZ)

$(DISTTGZ): $(DISTFILES)
	mkdir $(DISTDIR) ; cd $(DISTDIR) ; \
	for F in $(DISTFILES) ; do ln -s ../"$$F" ; done
	tar czhf $(DISTTGZ) $(DISTDIR)
	rm -rf $(DISTDIR)
	echo -e "\n\n== Package \"$(DISTTGZ)\" created. ==\n\n"

distcheck: dist
	tar xzf $(DISTTGZ)
	cd $(DISTDIR) ; if make ; then  \
	cd .. ; rm -rf $(DISTDIR) ; \
	echo -e "\n\n== Package \"$(DISTTGZ)\" is OK. ==\n\n" ; rm -rf $(DISTDIR) ; \
	else  \
	cd .. ; rm -rf $(DISTTGZ) $(DISTDIR) ; \
	echo -e "\n\n== ERROR building \"$(DISTDIR)\"!\n\"$(DISTTGZ)\" removed.\n\n" ; \
	fi

# --------------- #
