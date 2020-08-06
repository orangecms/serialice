##
## SerialICE
##
## Copyright (C) 2009 coresystems GmbH
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; version 2 of the License.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc.
##

VERSION="Oxide_1.7"

export src := $(shell pwd)
export srctree := $(src)
export srck := $(src)/util/kconfig
export obj := $(src)/build
export objk := $(src)/build/util/kconfig

export KERNELVERSION      := $(shell echo $(VERSION) )
export KCONFIG_AUTOHEADER := $(obj)/config.h
export KCONFIG_AUTOCONFIG := $(obj)/auto.conf

CONFIG_SHELL := sh
KBUILD_DEFCONFIG := configs/defconfig
HAVE_DOTCONFIG := $(wildcard .config)
MAKEFLAGS += -rR --no-print-directory

# Make is silent per default, but 'make V=1' will show all compiler calls.
ifneq ($(V),1)
Q := @
endif

AS=as --32
CC=gcc -m32 -march=i386 -ffreestanding -nostdinc -fno-pic -fno-exceptions -fno-stack-protector -fno-omit-frame-pointer
CPP=cpp
OBJCOPY=objcopy
NM=nm

LDFLAGS=-Wa,--divide -nostdlib -nostartfiles -static -T serialice.ld -Wl,--defsym,ALIGNED_ROMSIZE=$(CONFIG_ROMSIZE)

SOURCES = serialice.c serialice.h chipset.c serial.c types.h mainboard/*.c southbridge/*.c superio/*.c

HOSTCC ?= gcc
HOSTCFLAGS := -O2 -Wall -I$(srck) -I$(objk)

INCLUDES = -I$(obj)
CFLAGS := -Wall -Werror -Os $(INCLUDES)
OBJECTS = serialice.o
OBJS    = $(patsubst %,$(obj)/%,$(OBJECTS))

ifeq ($(strip $(HAVE_DOTCONFIG)),)

all: config

else

include $(src)/.config

TARGET-y = $(obj)/serialice.rom
all: $(TARGET-y)

endif

prepare:
	$(Q)mkdir -p $(obj)/util/kconfig/lxdialog

clean:
	$(Q)printf "  CLEAN\n"
	$(Q)rm -rf $(obj)/*.elf $(obj)/*.o
	$(Q)cd $(obj); rm -f romcc serialice.S *.o *.o.s
	$(Q)cd $(obj); rm -f serialice.elf serialice.rom serialice.map
	$(Q)cd $(obj); rm -f serialice-gcc.elf serialice-gcc.rom serialice-gcc.map
	$(Q)cd $(obj); rm -f serialice-gcc.S serialice-pre.s xmmstack serialice-gcc.map

distclean: clean
	$(Q)printf "  DISTCLEAN\n"
	$(Q)rm -rf build
	$(Q)rm -f .config .config.old ..config.tmp .kconfig.d .tmpconfig*

include util/kconfig/Makefile

.PHONY: $(PHONY) prepare clean distclean

$(obj)/serialice.rom: $(obj)/serialice.elf
	$(Q)printf "  OBJCOPY  $(subst $(shell pwd)/,,$(@))\n"
	$(Q)$(OBJCOPY) -O binary $< $@

$(obj)/serialice.elf: $(obj)/serialice.o $(obj)/start.o $(src)/serialice.ld
	$(Q)printf "  LINK     $(subst $(shell pwd)/,,$(@))\n"
	$(Q)$(CC) $(LDFLAGS) -o $@ $(obj)/serialice.o $(obj)/start.o
	$(Q)$(NM) $@ | sort -u > $(obj)/serialice.map

$(obj)/serialice.o: $(SOURCES) $(KCONFIG_AUTOHEADER)
	$(Q)printf "  CC       $(subst $(shell pwd)/,,$(@))\n"
	$(Q)$(CC) $(INCLUDES) -I. -Imainboard -Isouthbridge -Isuperio -DVERSION=\"$(VERSION)\" -o $@ -c $<

# #####################################################################

$(obj)/%.o: $(src)/%.S
	$(Q)printf "  CPP      $(subst $(shell pwd)/,,$(@))\n"
	$(Q)$(CPP) $(INCLUDES) -DVERSION=\"$(VERSION)\" -o $@.s $^
	$(Q)printf "  AS       $(subst $(shell pwd)/,,$(@))\n"
	$(Q)$(AS) -o $@ $@.s

$(obj)/%.o: $(obj)/%.S
	$(Q)printf "  CPP      $(subst $(shell pwd)/,,$(@))\n"
	$(Q)$(CPP) $(INCLUDES) -DVERSION=\"$(VERSION)\" -o $@.s $^
	$(Q)printf "  AS       $(subst $(shell pwd)/,,$(@))\n"
	$(Q)$(AS) -o $@ $@.s

# #####################################################################

dongle: $(TARGET-y)
	dongle.py -v -c /dev/cu.usbserial-00* $(TARGET-y) EOF
