# hbs1 - Host Basic Services - ver. 1
# 
# makefile for Unix-like platforms
#
# Changelog:
#	* 2013/01/06 Costin Ionescu: initial release

N := hbs1
D := HBS1

src_list := src/posix.c src/misc.c
common_hdr_list := $(wildcard include/*.h) $(wildcard include/$(N)/*.h)
sl_obj_list := $(patsubst src/%.c,out/sl/%.o,$(src_list))
dl_obj_list := $(patsubst src/%.c,out/dl/%.o,$(src_list))
hdr_list := $(wildcard include/*.h) $(wildcard include/$(N)/*.h)

ext_sl_libs := -lpthread -lc41
ext_dl_libs := -lpthread -lc41

ifeq ($(PREFIX_DIR),)
PREFIX_DIR:=$(HOME)/.local
endif

.PHONY: all libs install uninstall dlib slib slibcli tests sltest dltest clean tags sclitest dclitest arc

CFLAGS := -fvisibility=hidden -Iinclude -Wall -Wextra -Werror -DNDEBUG $(ext_cflags)
SL_CFLAGS := $(CFLAGS) -D$(D)_STATIC
DL_CFLAGS := $(CFLAGS) -D$(D)_DLIB_BUILD -fPIC

all: libs tests

libs: dlib slib slibcli dlibcli

tests: sltest dltest sclitest dclitest

arc:
	cd .. && tar -Jcvf $(N).txz $(N)/src $(N)/include $(N)/make* $(N)/*.txt

install: libs tests
	mkdir -p $(PREFIX_DIR)/lib
	cp -v out/dl/lib$(N).so $(PREFIX_DIR)/lib/
	[ `whoami` != root ] || ldconfig
	cp -v out/sl/lib$(N).a $(PREFIX_DIR)/lib/
	cp -v out/sl/lib$(N)clis.a $(PREFIX_DIR)/lib/
	cp -v out/dl/lib$(N)clid.a $(PREFIX_DIR)/lib/
	cp -vr include $(PREFIX_DIR)/

uninstall:
	-rm -f $(PREFIX_DIR)/lib/lib$(N).so $(PREFIX_DIR)/lib/lib$(N).a $(PREFIX_DIR)/lib/lib$(N)clis.a $(PREFIX_DIR)/lib/lib$(N)clid.a
	-rm -rf $(PREFIX_DIR)/include/hbs1.h $(PREFIX_DIR)/include/hbs1
	[ `whoami` != root ] || ldconfig

dlib: out/dl/lib$(N).so

slib: out/sl/lib$(N).a

slibcli: out/sl/lib$(N)clis.a

dlibcli: out/dl/lib$(N)clid.a

dltest: out/dl/test dlib
	LD_LIBRARY_PATH=out/dl:$(C41_DL_DIR):$(LD_LIBRARY_PATH) $<

sltest: out/sl/test slib
	$<

clean:
	-rm -rf out

tags:
	ctags -R --fields=+iaS --extra=+q --exclude='.git' .

out out/dl out/sl:
	mkdir -p $@

out/dl/lib$(N).so: $(dl_obj_list) | out/dl
	gcc -shared -o$@ $^ $(ext_dl_libs)

$(dl_obj_list): out/dl/%.o: src/%.c $(lib_hdr_list) | out/dl
	gcc -c $(DL_CFLAGS) $< -o $@

out/dl/test: src/test.c $(hdr_list) | out/dl
	gcc -o$@ $(CFLAGS) src/test.c -Lout/dl -l$(N) $(ext_dl_libs)

out/sl/lib$(N).a: $(sl_obj_list) | out/sl
	ar rcs $@ $^

$(sl_obj_list): out/sl/%.o: src/%.c $(lib_hdr_list) | out/sl
	gcc -c $(SL_CFLAGS) $< -o $@

out/sl/test: src/test.c $(hdr_list) out/sl/lib$(N).a | out/sl
	gcc -o$@ $(CFLAGS) -D$(D)_STATIC src/test.c -static -Lout/sl -l$(N) $(ext_sl_libs)

out/sl/lib$(N)clis.a: out/sl/cli.o | out/sl
	ar rcs $@ $^

out/sl/cli.o: src/cli.c | out/sl
	gcc -c $(CFLAGS) -D$(D)_STATIC -D$(D)CLI_STATIC -DC41_STATIC $< -o $@

sclitest: out/sl/clitest
	$^

out/sl/clitest: src/clitest.c out/sl/lib$(N).a out/sl/lib$(N)clis.a | out/sl
	gcc $(CFLAGS) -D$(D)_STATIC src/clitest.c -o $@ -static -Lout/sl -l$(N)clis -l$(N) $(ext_sl_libs)

dclitest: out/dl/clitest
	LD_LIBRARY_PATH=out/dl:$(C41_DL_DIR):$(LD_LIBRARY_PATH) $^

out/dl/lib$(N)clid.a: out/dl/cli.o | out/dl
	ar rsc $@ $^

out/dl/cli.o: src/cli.c | out/dl
	gcc -c $(CFLAGS) -D$(D)CLI_STATIC $< -o $@

out/dl/clitest: src/clitest.c dlib dlibcli | out/dl
	gcc $(CFLAGS) src/clitest.c -o $@ -Lout/dl -l$(N)clid -l$(N) $(ext_dl_libs)

