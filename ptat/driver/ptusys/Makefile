#
# Makefile - ptusys.
# 
# Intel provides a functionality equivalent to /dev/mem kernel module driver without the restriction 
# as a reference source code and potentially as binary driver with PTU tool only for IHV board validation. 
# No upstream or end user release planned or supported for the kernel helper driver.
# 
#
# This file is provided under a GPLv2 license.  When using or
#   redistributing this file, you may do so under the GPLv2 license.
# 
#   GPL LICENSE SUMMARY
# 
#   Copyright(c) 2007-2013 Intel Corporation. All rights reserved.
# 
#   This program is free software; you can redistribute it and/or modify 
#   it under the terms of version 2 of the GNU General Public License as
#   published by the Free Software Foundation.
# 
#   This program is distributed in the hope that it will be useful, but 
#   WITHOUT ANY WARRANTY; without even the implied warranty of 
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
#   General Public License for more details.
# 
#   You should have received a copy of the GNU General Public License 
#   along with this program; if not, write to the Free Software 
#   Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
#   The full GNU General Public License is included in this distribution 
#   in the file called LICENSE.GPL.
# 
#   Contact Information:
#   Intel Corporation
#   

KERNEL_SRC_DIR = /lib/modules/`uname -r`/build
KERNEL_REL_NAME = `uname -r | cut -d '.' -f4`
KERNEL_REL_SHORT = `uname -r | cut -d '.' -f1-3`
obj-m += ptusys.o
ptusys-objs := ptusys_drv.o 

all:	clean ptusys 

ptusys : clean
	make -C $(KERNEL_SRC_DIR) M=`pwd` modules

install:
	insmod ptusys.ko

remove:
	rmmod ptusys

clean : 
	rm -f *.o *.ko *.mod.c Module.symvers modules.order
	rm -f \.*.o.cmd \.*.ko.cmd \.*.o.d
	rm -rf \.tmp_versions
