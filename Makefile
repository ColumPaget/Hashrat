
CC = gcc
VERSION = 0.1
CFLAGS = -g -O2
LIBS = 
INSTALL=/bin/install -c
prefix=/usr/local
bindir=$(prefix)${exec_prefix}/bin
FLAGS=$(CFLAGS) -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DPACKAGE_URL=\"\" -DSTDC_HEADERS=1
OBJ=
EXE=hasher

all: $(OBJ) main.c
	@cd libUseful-2.0; $(MAKE)
	gcc -g $(LIBS) -o$(EXE) $(OBJ) main.c libUseful-2.0/libUseful-2.0.a


clean:
	rm -f *.o */*.o */*.a */*.so $(EXE)

install:
	@mkdir -p $(DESTDIR)$(prefix)/sbin
	cp -f $(EXE) $(DESTDIR)$(prefix)/sbin
