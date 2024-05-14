###############################################################################
#
# srmtMwaitKM.c
#
# Copyright (C) 2017 Intel Corporation. All rights reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License version
# 2 as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.
#
# Author Name <cjparsons@soft-forge.com>
#
###############################################################################
LKMNAME = srmtMwaitLKM
SRCS   = $(LKMNAME).c
OBJS   = $(SRCS:.c=.o)
BINDIR := ../bin
RM := `which rm`

obj-m += $(OBJS)

ifndef KMDIR
KMDIR := /lib/modules/$(shell uname -r)/build
endif

PWD := $(shell pwd)

all:
	$(MAKE) -C $(KMDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KMDIR) M=$(PWD) clean
