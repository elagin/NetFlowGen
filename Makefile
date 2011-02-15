# Makefile for GNU make
#
# Common Unix flags
EXECUTABLE=netflowgen
CFLAGS=-MMD -D_FILE_OFFSET_BITS=64 -DMT -DLB_MODULE_NAME=\"$(EXECUTABLE)\"

LDFLAGS=/usr/lib/gcc/i486-linux-gnu/4.4.3/libstdc++.a -lm -lpthread -lrt

CC=g++
FOUT=-o
LINK=$(CC) -o $(EXECUTABLE)
STRIP=strip $(EXECUTABLE)
BUILD_DIR=build
DEPS=$(BUILD_DIR)/main.o
VPATH=../../Lblib:../../xmlapi:../api

##### OS detect
OS=$(shell uname)
ifeq ($(OS), Linux)
# Linux stuff
CFLAGS+=-DLINUX
LDFLAGS+=-ldl
MAKE=make
else
ifneq (,$(findstring CYGWIN_NT, $(OS)))
# Windows (Cygwin)
#VPATH+=:../Lblib/win
#CFLAGS=/D"WIN32" /D"MT" /D"_CONSOLE" /D"_USE_32BIT_TIME_T" /D"SNMP_MODULE" /D"LB_MODULE_NAME=\"$(EXECUTABLE)\"" \
#	/FD /EHsc /MT /GS /nologo /I"../Lblib" /I"../sbss" /I"." /I"./common" -I"./device_status" -I"./snmp_protocol"\
#	/I"c:/OpenSSL/include" /I"c:/Program Files/MySQL/MySQL Server 5.0/include"\
#	/I"c:/netsnmp/include" /I"c:/lua/include"
#LDFLAGS=/LIBPATH:"c:/OpenSSL/lib" /LIBPATH:"c:/OpenSSL/lib/VC/static"\
#	/LIBPATH:"c:/Program Files/MySQL/MySQL Server 5.0/lib/opt"\
#	/LIBPATH:"c:/netsnmp/lib"\
#	/LIBPATH:"c:/lua/lib"\
#	/NODEFAULTLIB:"libcd.lib" /NODEFAULTLIB:"libcmtd.lib" /SUBSYSTEM:CONSOLE\
#	/OPT:REF /MACHINE:X86 /INCREMENTAL:NO /NOLOGO\
#	kernel32.lib advapi32.lib user32.lib gdi32.lib\
#	Ws2_32.lib libeay32MT.lib ssleay32MT.lib mysqlclient.lib netsnmp.lib lua_lib.lib
#MAKE=make.exe
#CC=cl.exe /TP
#FOUT=/Fo
#LINK=link.exe /OUT:"$(EXECUTABLE).exe"
#DEPS+=win_threads.o getopt.o read_registry.o service_init.o lbsnmpcd.res
#STRIP=
else
$(error Unknown OS)
#endif
endif
endif
##### end of OS detect

# Add debug flags for 'make debug'
ifdef DEBUG
CFLAGS+=-DDEBUG -DNOCHECKLIC -Wall -g -O0
else
CFLAGS+=-O2 -DNDEBUG -w
ifeq (,$(findstring CYGWIN_NT, $(OS)))
#LDFLAGS+=-static
endif
endif

CFLAGS+=$(LBFLAGS)
export CC
export CFLAGS
export BUILD_DIR
export FOUT

release: all
#	$(STRIP)

.PHONY: subdirs $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

build_dir:
	mkdir -p $(BUILD_DIR)

all: build_dir $(SUBDIRS) $(DEPS)
	$(LINK) $(BUILD_DIR)/*.o $(LDFLAGS)

#.c.o .cpp.o:
#	$(CC) $(CFLAGS) -c $< $(FOUT)$(BUILD_DIR)/$@

$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -MF $(BUILD_DIR)/$*.d $< $(FOUT)$@
	@cp $(BUILD_DIR)/$*.d $(BUILD_DIR)/$*.P; \
		sed -e 's/#.*//' -e 's/^[^:]*: *//' \
			-e 's/ *\\$$//' -e '/^$$/ d' -e 's/$$/ :/' \
			< $(BUILD_DIR)/$*.d >> $(BUILD_DIR)/$*.P; \
		rm -f $(BUILD_DIR)/$*.d

$(BUILD_DIR)/%.o: %.cpp
	$(CC) $(CFLAGS) -c -MF $(BUILD_DIR)/$*.d $< $(FOUT)$@
	@cp $(BUILD_DIR)/$*.d $(BUILD_DIR)/$*.P; \
		sed -e 's/#.*//' -e 's/^[^:]*: *//' \
			-e 's/ *\\$$//' -e '/^$$/ d' -e 's/$$/ :/' \
			< $(BUILD_DIR)/$*.d >> $(BUILD_DIR)/$*.P; \
		rm -f $(BUILD_DIR)/$*.d

-include $(DEPS:%.o=%.P)

lbsnmpcd.res:
	rc.exe /i "c:/Program Files/Microsoft Platform SDK/Include/mfc" /i "C:/Program Files/Microsoft SDKs/Windows/v6.0A/Include" /fo $(BUILD_DIR)/lbsnmpcd.res Lbsnmpcd.rc
debug:
	DEBUG=1 $(MAKE) all

clean:
	rm -rf $(BUILD_DIR) $(EXECUTABLE) $(EXECUTABLE).exe $(EXECUTABLE).exp $(EXECUTABLE).lib vc*.idb
