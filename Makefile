
CC = gcc
VERSION = 0.1
CFLAGS = -g -O2
LIBS = 
INSTALL=/bin/install -c
prefix=/usr/local
bindir=$(prefix)${exec_prefix}/bin
FLAGS=$(CFLAGS) -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DPACKAGE_URL=\"\" -DSTDC_HEADERS=1 -D_LARGEFILE64_SOURCE=1 -D_FILE_OFFSET_BITS=64
OBJ=
EXE=hashrat

all: $(OBJ) main.c
	@cd libUseful-2.0; $(MAKE)
	gcc -g $(FLAGS) $(LIBS) -o$(EXE) $(OBJ) main.c libUseful-2.0/libUseful-2.0.a


clean:
	rm -f *.o */*.o */*.a */*.so $(EXE)

install:
	@mkdir -p $(DESTDIR)$(prefix)/sbin
	cp -f $(EXE) $(DESTDIR)$(prefix)/sbin
