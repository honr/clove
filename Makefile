CFLAGS=-Wall
# CFLAGS=-g -Wall -pg

UNAME := $(shell uname)
UNAMEM := $(shell uname -m)

LIBEXT := so
ifeq ($(UNAME), Linux)
PLATFORM=linux
LIB_PTHREAD=-lpthread
LIB_HIST=-lhistory -ltermcap
LIBSWITCH=-shared
LIBEXT := so
INCLUDESJAVA := -Ibuild/include -I/usr/include -I/usr/lib/jvm/java-6-openjdk/include -I/usr/lib/jvm/java-6-openjdk/include/linux
else ifeq ($(UNAME), Darwin)
PLATFORM=mac
LIBSWITCH=-dynamiclib
LIBEXT := dylib
INCLUDESJAVA := -Ibuild/include -I/usr/include -I/System//Library/Frameworks/JavaVM.framework/Headers -I/Library/Java/Home/include
endif

ifeq ($(UNAMEM), i686)
UNAMEM := x86
else ifeq ($(UNAMEM), i586)
UNAMEM := x86
else ifeq ($(UNAMEM), i486)
UNAMEM := x86
else ifeq ($(UNAMEM), i386)
UNAMEM := x86
else ifeq ($(UNAMEM), amd64)
UNAMEM := x86_64
endif

ARCHI := $(PLATFORM)-$(UNAMEM)


all: dirs build/bin/$(ARCHI)/clove build/lib/$(ARCHI)/libCloveNet.$(LIBEXT) build/clove.jar

dirs: build build/obj/$(ARCHI) build/bin/$(ARCHI) build/lib/$(ARCHI)
build:
	mkdir -p $@
build/obj/$(ARCHI):
	mkdir -p $@
build/bin/$(ARCHI):
	mkdir -p $@
build/lib/$(ARCHI):
	mkdir -p $@

build/obj/$(ARCHI)/clove-utils.o: src/clove/clove-utils.c src/clove/clove-utils.h
	gcc $(CFLAGS) -c -o $@ $<

build/obj/$(ARCHI)/clove.o: src/clove/clove.c src/clove/clove-utils.h
	gcc $(LIB_PTHREAD) $(CFLAGS) -c -o $@ $<

build/bin/$(ARCHI)/clove: build/obj/$(ARCHI)/clove.o build/obj/$(ARCHI)/clove-utils.o
	gcc $(LIB_PTHREAD) $(CFLAGS) $^ -o $@

build/include/clove_CloveNet.h: src/clove/*.java
	javac -d build -g:none -target 1.6 src/clove/*.java 
# TODO: ^^ find the option to includeJavaRuntime
	javah -d build/include -classpath build clove.CloveNet

build/lib/$(ARCHI)/libCloveNet.$(LIBEXT): build/include/clove_CloveNet.h
	gcc -Wall -fPIC -c $(INCLUDESJAVA) src/clove/CloveNet.c -o build/obj/$(ARCHI)/CloveNet.o
	gcc -o build/lib/$(ARCHI)/libCloveNet.$(LIBEXT) $(LIBSWITCH) build/obj/$(ARCHI)/CloveNet.o

build/clove.jar: build/include/clove_CloveNet.h
	cd build ; jar cf clove.jar clove/*.class
#	jar cf clove.jar -C build clove/*.class
#	stupid jar! Could have an option to strip directory ("build" here)

INSTALLDIR := $(HOME)/.local

install: all
	install -d $(INSTALLDIR)/bin/$(ARCHI)
	install -d $(INSTALLDIR)/lib/$(ARCHI)
	install -d $(INSTALLDIR)/share/java/top-jar
	install build/bin/$(ARCHI)/clove $(INSTALLDIR)/bin/$(ARCHI)
	install build/lib/$(ARCHI)/libCloveNet.$(LIBEXT) $(INSTALLDIR)/lib/$(ARCHI)
	install build/clove.jar $(INSTALLDIR)/share/java/top-jar

	install -d $(INSTALLDIR)/share/clove
	install share/clove/clove.conf $(INSTALLDIR)/share/clove
	install share/clove/clove-clojure $(INSTALLDIR)/share/clove
	install share/clove/clove-clojure.conf $(INSTALLDIR)/share/clove

clean:
	-rm -Rf build
