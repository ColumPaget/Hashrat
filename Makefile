
CC = gcc
VERSION = 
CFLAGS = -g -O2
LIBS = 
INSTALL=/bin/install -c
prefix=/usr/local
bindir=$(prefix)${exec_prefix}/bin
FLAGS=$(LDFLAGS) $(CPPFLAGS) $(CFLAGS) -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DPACKAGE_URL=\"\" -DSTDC_HEADERS=1 -D_FILE_OFFSET_BITS=64
OBJ=common.o command-line-args.o ssh.o fingerprint.o files.o filesigning.o xattr.o cgi.o check-hash.o find.o memcached.o
EXE=hashrat

all: hashrat

hashrat: $(OBJ) main.c
	@cd libUseful-2.5; $(MAKE)
	$(CC) $(FLAGS) -o$(EXE) $(OBJ) main.c libUseful-2.5/libUseful-2.5.a $(LIBS) 

common.o: common.h common.c
	$(CC) $(FLAGS) -c common.c

fingerprint.o: fingerprint.h fingerprint.c
	$(CC) $(FLAGS) -c fingerprint.c

files.o: files.h files.c
	$(CC) $(FLAGS) -c files.c

filesigning.o: filesigning.h filesigning.c
	$(CC) $(FLAGS) -c filesigning.c

find.o: find.h find.c
	$(CC) $(FLAGS) -c find.c

check-hash.o: check-hash.h check-hash.c
	$(CC) $(FLAGS) -c check-hash.c

xattr.o: xattr.h xattr.c
	$(CC) $(FLAGS) -c xattr.c

ssh.o: ssh.h ssh.c
	$(CC) $(FLAGS) -c ssh.c

cgi.o: cgi.h cgi.c
	$(CC) $(FLAGS) -c cgi.c

memcached.o: memcached.h memcached.c
	$(CC) $(FLAGS) -c memcached.c

command-line-args.o: command-line-args.h command-line-args.c
	$(CC) $(FLAGS) -c command-line-args.c

check: hashrat
	@./check.sh

clean:
	-rm -f *.o */*.o */*.a */*.so $(EXE)
	-rm -f config.log config.status */config.log */config.status
	-rm -fr autom4te.cache */autom4te.cache

distclean:
	-rm -f *.o */*.o */*.a */*.so $(EXE)
	-rm -f config.log config.status */config.log */config.status Makefile */Makefile
	-rm -fr autom4te.cache */autom4te.cache


install: hashrat
	-mkdir -p $(DESTDIR)$(prefix)/bin
	cp -f $(EXE) $(DESTDIR)$(prefix)/bin
	-mkdir -p $(DESTDIR)$(prefix)/share/man/man1
	cp -f hashrat.1 $(DESTDIR)$(prefix)/share/man/man1
