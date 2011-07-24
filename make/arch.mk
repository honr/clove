# defines ARCHI,
# defines and creates $(BUILDDIR) $(DOBJ), $(DBIN), $(DLIB) directories.

UNAME := $(shell uname)
UNAMEM := $(shell uname -m)

LIBEXT := so
ifeq ($(UNAME), Linux)
PLATFORM=linux
# LIB_PTHREAD=-lpthread
# LIB_HIST=-lhistory -ltermcap
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
BUILDDIR := build
DOBJ := $(BUILDDIR)/obj/$(ARCHI)
DBIN := $(BUILDDIR)/bin/$(ARCHI)
DLIB := $(BUILDDIR)/lib/$(ARCHI)

dirs: $(BUILDDIR) $(DOBJ) $(DBIN) $(DLIB)

$(BUILDDIR):
	mkdir -p $@
$(DOBJ):
	mkdir -p $@
$(DBIN):
	mkdir -p $@
$(DLIB):
	mkdir -p $@
