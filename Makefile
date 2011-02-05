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


all: dirs build/bin/clove build/clove.jar

dirs: build build/obj build/bin build/lib
build:
	mkdir -p $@
build/obj:
	mkdir -p $@
build/bin:
	mkdir -p $@
build/lib:
	mkdir -p $@

build/obj/clove-utils.o: src/clove/clove-utils.c src/clove/clove-utils.h
	gcc $(CFLAGS) -c -o $@ $<

build/obj/clove.o: src/clove/clove.c src/clove/clove-utils.h
	gcc $(LIB_PTHREAD) $(CFLAGS) -c -o $@ $<

build/bin/clove: build/obj/clove.o build/obj/clove-utils.o
	gcc $(LIB_PTHREAD) $(CFLAGS) $^ -o $@

build/clove.jar: src/clove/*.java
	javac -d build -g:none -target 1.6 src/clove/*.java 
# TODO: ^^ find the option to includeJavaRuntime

	javah -d build/include -classpath build clove.CloveNet
	gcc -Wall -fPIC -c $(INCLUDESJAVA) src/clove/CloveNet.c -o build/CloveNet.o
	gcc -o build/lib/libCloveNet.$(LIBEXT) $(LIBSWITCH) build/CloveNet.o

	cd build ; jar cf clove.jar clove/*.class
#	jar cf clove.jar -C build clove/*.class
#	stupid jar! Could have an option to strip directory ("build" here)

INSTALLDIR := $(HOME)/.local

install: all
	install -d $(INSTALLDIR)/bin/$(ARCHI)
	install -d $(INSTALLDIR)/lib/$(ARCHI)
	install -d $(INSTALLDIR)/share/java
	install build/bin/clove $(INSTALLDIR)/bin/$(ARCHI)
	install build/lib/libCloveNet.$(LIBEXT) $(INSTALLDIR)/lib/$(ARCHI)
	install build/clove.jar $(INSTALLDIR)/share/java

	install -d $(INSTALLDIR)/share/clove
	install clove.conf $(INSTALLDIR)/share/clove
	install clove-clojure $(INSTALLDIR)/share/clove
	install clove-clojure.conf $(INSTALLDIR)/share/clove

clean:
	-rm -Rf build
