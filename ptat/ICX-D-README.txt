Intel(R) Power Thermal Utility - Server Edition
Release Date: 03/05/2021
Copyright (C) 2020, Intel Corporation.  All rights reserved.
Intel Confidential


----------------------------------------------------------------------------------------------------------
Version: 2.4
----------------------------------------------------------------------------------------------------------
* Enable turbo test for SSE, AVX-256, AVX-512 
* Update PMax test

----------------------------------------------------------------------------------------------------------
Version: 2.3
----------------------------------------------------------------------------------------------------------
* Enabled ICX-D 
*     Tests Supported: 
*         SSE, AVX256,AVX512 with power levels from 50%-100% 
*         TDP, nTDP Tests
*         PMax test. 

----------------------------------------------------------------------------------------------------------
Version: 2.2 
----------------------------------------------------------------------------------------------------------
* Fixed C1 state.
* Updated AVX-512 and PMax to support higher power delivery.
* Allowed core and thread masks for all CPU tests.
* Fixed the Pmem monitoring data on second socket.
* Do not use cached ref_temp data

----------------------------------------------------------------------------------------------------------
Version: 2.1 
----------------------------------------------------------------------------------------------------------
* Updated dcpmmptu to v1.3, changes included:
    - Support memory BW boost feature at 1-120s time scales (sub 1Hz operation).
* Support Barlow Pass (BPS) individual power measurement (N-DCPPwr, N-12VPwr, N-1.2Pwr). HSD# 2207792944
* Support BPS FW controller and media temperature.
* Support BPS FW thermal throttling (TLog).
* Support BPS FW power management policy- BW boost, PL, TC, thresholds.
* Support ICX DDR-T read/write bandwidth.
* Fixed IMON power report in CPX-4. Total package power is shown in Master die only. Slave die will always be 0.
* Fixed bugs when specifying -logdir option
* Added -logname option to allow custom prefix filename for logging.

----------------------------------------------------------------------------------------------------------
Version: 2.0
----------------------------------------------------------------------------------------------------------
* This new version of PTU is the first release of the Unified Server PTU where  
  monitor (ptumon) and generator (ptugen) functions are integrated into one common binary. 
* This PTU will also span multiple generations of legacy and upcoming CPUs. 
* Support DDR4 read/write bandwidth
• CLI usage has changed. Please see ptu_usage.txt for updated parameters.

----------------------------------------------------------------------------------------------------------
Supported processors are: 
 SKX-(SP, FPGA, W), CLX-(SP, AP), CPX-(4, 6), ICX-(LCC, HCC).
----------------------------------------------------------------------------------------------------------

----------------------------------------------------------------------------------------------------------
Software Installation
----------------------------------------------------------------------------------------------------------
It is recommended to install the PTU in /root/ptu directory.
1) mkdir -p /root/ptu
2) cd /root/ptu
3) tar xvf unified_server_ptu*._gz

For Fedora OS, PTU driver is required to access device sensors such as DIMM temperature, 
PCH temperature and others.
To install the driver, first you need to build it and then install. 
Driver needs to build once only, installing the driver needs to do whenever the system is rebooted.

1) cd /root/ptu/driver/ptusys
2) make clean all
3) make install -> this should install the ptusys.ko file into kernel

NOTES: 
 Make sure that software and kernel development packages are installed.
 yum groupinstall 'Development Tools'
 yum install kernel-devel
 
 If newer kernels, it might not build due to other missing files.
 try:
 yum install "kernel-devel-$(uname -r)"
 dnd update
 yum install flex
 yum install bison
 
 if complaining missing Documentation/Kconfig, do:
 replace SUBDIRS= to M= in the Makefile

----------------------------------------------------------------------------------------------------------
Preparation for SKX-FPGA SKU
----------------------------------------------------------------------------------------------------------
1) Install CentOS 7.3-1611. FPGA software is currently not supported on other Linux distributions.
2) Go to BIOS settings and turn on "Secure Boot"
3) Verify that FPGA platform is upgraded to Blue Bitstream 6.3.0
4) Install SKX-SP/FPGA PTU
   a) Log into your system as root.
   b) PTU package is distributed as tar archive. cd to a <folder>, then untar the file.
      tar xvfz skx_clx_ptu_rev*.tgz
5) Enroll FPGA public key (fpga_public_cert.der) that comes with the package. 
   The key will be added to MOK (Machine Owner Key) list using following steps:
	a) Request addition of the public key by running following command:
		# mokutil --import <Path to FPGA key>. You will be asked to enter and confirm a password 
		  for this MOK enrollment request. Enter any password that you can remember.
	b) Reboot the machine.
	c) MokManager will be launched to allow you to complete the enrollment from the UEFI console. 
	   Choose "Enroll MOK" option from the list in console. You will need to enter the
	   password you previously associated with this request and confirm the enrollment. 
	   Your public key is added to the MOK list, which is persistent.
	d) After the machine reboots, verify that key is enrolled by running following command:
		#mokutil --test-key fpga_cert.der

NOTES:
  If you have existing OPAE drivers installed in the system, you will need to remove them before running PTU.
  In Linux shell, do:

  lsmod | grep fpga

  The fpga drivers will look something like this:

  intel_fpga_fme         51339  0 
  intel_fpga_afu         27203  0 
  fpga_mgr_mod           14693  1 intel_fpga_fme
  intel_fpga_pci         25176  2 intel_fpga_afu,intel_fpga_fme

  To remove each fpga above, you do:

  rmmod intel_fpga_fme
  rmmod intel_fpga_afu
  rmmod fpga_mgr_mod
  rmmod intel_fpga_pci

----------------------------------------------------------------------------------------------------------
How to run PTU in command line mode:
----------------------------------------------------------------------------------------------------------
Please refer to PTU user guide for full usage.

1) Run PTU TDP:
./ptu -ct 1

2) Run PTU with nearTDP enabled:
./ptu -ct 2

3) Run PTU IA/SSE test with power level 80%:
./ptu -ct 3 -cp 80

4) Run PTU using monitor screen mode:
./ptu -mon -scr

5) Run PTU with full monitoring data and long version mode:
./ptu -mon -l 1 -filter 0xff

6) Run PTU monitor mode in level 2 and display only core 0,1:
./ptu -mon -l 2 -moncore 0x3

7) Run BPS (NVDIMM) monitor mode:
,/ptu -mon -scr -l 2 -filter 0x70 -ts

----------------------------------------------------------------------------------------------------------
How to run DCPMM PTU in command line mode:
----------------------------------------------------------------------------------------------------------
Please refer to DCPMMPTU user guide for full usage.

1) Run thermal read test using 8 cores with a name space mounted in /mnt
./dcpmmptu -m thermals -p MAXREAD -t 1-8 -d 1000000 -n /mnt/DCPMMptu1 -s 512G -b 32M

1) Run thermal write test using 8 cores with a name space mounted in /mnt
./dcpmmptu -m thermals -p MAXWRITE -t 1-8 -d 1000000 -n /mnt/DCPMMptu1 -s 512G -b 32M

1) Run thermal read test using 8 cores on DDR4 memory
./dcpmmptu -m thermals -p MAXWRITE -t 1-8 -d 1000000 -n memory -s 512G -b 32M

----------------------------------------------------------------------------------------------------------
Known Issues / Limitations:
----------------------------------------------------------------------------------------------------------
* MCP thermal report for CPX-4 is not fully implemented.

----------------------------------------------------------------------------------------------------------
Troubleshooting:
----------------------------------------------------------------------------------------------------------
* Enable RAPL to see DIMM power.
* Set PCH Thermal Device in BIOS to Enabled in PCI mode for PCH temperature support.
* PCH temperature support requires PCH B0 stepping (SKX).
* Set DIMM thermal config in BIOS to CLTT mode for DIMM temperature support.


----------------------------------------------------------------------------------------------------------
LEGAL / DISCLAIMERS
----------------------------------------------------------------------------------------------------------

INTEL CONFIDENTIAL
Copyright (C) 2010-2018, Intel Corporation.  All rights reserved.

Intel Corporation assumes no responsibility for errors or omissions in this
document. Nor does Intel make any commitment to update the information
contained herein.

* Other product and corporate names may be trademarks of other companies and
* are used only for explanation and to the owners' benefit, without intent to
* infringe.

Disclaimer: Core/Package and DIMM Power meter results are to be used only for
relative power comparisons and not for the TDP power design.

