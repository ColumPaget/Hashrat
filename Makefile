CC = gcc
CFLAGS = -g -O2
LIBS = libUseful-5/libUseful.a -lssl -lcrypto -lz  
INSTALL=/bin/install -c
prefix=/usr/local
bindir=$(prefix)${exec_prefix}/bin
FLAGS=$(LDFLAGS) $(CPPFLAGS) $(CFLAGS) -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DPACKAGE_URL=\"\" -DHAVE_STDIO_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_STRINGS_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_UNISTD_H=1 -DSTDC_HEADERS=1 -D_FILE_OFFSET_BITS=64 -DHAVE_LIBZ=1 -DHAVE_LIBCRYPTO=1 -DHAVE_LIBSSL=1
OBJ=common.o encodings.o command-line-args.o ssh.o http.o fingerprint.o include-exclude.o files.o filesigning.o xattr.o check-hash.o find.o otp.o memcached.o frontend.o cgi.o xdialog.o output.o
EXE=hashrat

all: hashrat

hashrat: $(OBJ) main.c libUseful-5/libUseful.a
	$(CC) $(FLAGS) -o$(EXE) $(OBJ) main.c $(LIBS) 

libUseful-5/libUseful.a:
	@cd libUseful-5; $(MAKE)

common.o: common.h common.c
	$(CC) $(FLAGS) -c common.c

encodings.o: encodings.h encodings.c
	$(CC) $(FLAGS) -c encodings.c

fingerprint.o: fingerprint.h fingerprint.c
	$(CC) $(FLAGS) -c fingerprint.c

include-exclude.o: include-exclude.h include-exclude.c
	$(CC) $(FLAGS) -c include-exclude.c

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

http.o: http.h http.c
	$(CC) $(FLAGS) -c http.c

otp.o: otp.h otp.c
	$(CC) $(FLAGS) -c otp.c

frontend.o: frontend.h frontend.c
	$(CC) $(FLAGS) -c frontend.c

output.o: output.h output.c
	$(CC) $(FLAGS) -c output.c

cgi.o: cgi.h cgi.c
	$(CC) $(FLAGS) -c cgi.c

xdialog.o: xdialog.h xdialog.c
	$(CC) $(FLAGS) -c xdialog.c

memcached.o: memcached.h memcached.c
	$(CC) $(FLAGS) -c memcached.c

command-line-args.o: command-line-args.h command-line-args.c
	$(CC) $(FLAGS) -c command-line-args.c

check: hashrat
	@./check.sh

clean:
	-rm -f *.o */*.o */*.a */*.so *.orig *.out $(EXE)
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

test:
	echo "no tests"
