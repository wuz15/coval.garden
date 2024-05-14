# BIOS Knob Manager Usage
#### 1. Preparation
```
$ git clone https://github.com/wuz15/coval.garden.git
$ cd coval.garden/bios_knob
$ python start_biosknob_manager.py
Example:
D:\workspace\coval.garden\bios_knob>python start_biosknob_manager.py
Python 3.8.19 (default, Mar 19 2024, 11:19:14) [MSC v.1900 64 bit (AMD64)]
Type 'copyright', 'credits' or 'license' for more information
IPython 8.11.0 -- An enhanced Interactive Python. Type '?' for help.

In [1]:
```

#### 2. System Configuration
##### a. Modify CPU Project
```
In [1]: os.environ['CPU_PROJECT']
Out[1]: 'SRF'

In [2]: os.environ['CPU_PROJECT'] = 'GNR'

In [3]: os.environ['CPU_PROJECT']
Out[3]: 'GNR'
```
##### b. Modify Customer Type
```
In [4]: os.environ['CUSTOMER_TYPE']
Out[4]: 'DELL'

In [5]: os.environ['CUSTOMER_TYPE'] = 'HPE'

In [6]: os.environ['CUSTOMER_TYPE']
Out[6]: 'HPE'
```
##### c. Modify BMC Type
```
In [10]: os.environ['BMC_TYPE']
Out[10]: 'OSM'

In [11]: os.environ['BMC_TYPE'] = 'IDRAC'

In [12]: os.environ['BMC_TYPE']
Out[12]: 'IDRAC'
```
##### d. Modify BMC IP Address
```
In [7]: os.environ['BMC_IP']
Out[7]: '127.0.0.1'

In [8]: os.environ['BMC_IP'] = '10.89.92.150'

In [9]: os.environ['BMC_IP']
Out[9]: '10.89.92.150'
```
##### e. Modify BMC Username
```
In [13]: os.environ['BMC_USERNAME']
Out[13]: 'root'

In [14]: os.environ['BMC_USERNAME'] = 'admin'

In [15]: os.environ['BMC_USERNAME']
Out[15]: 'admin'
```
##### f. Modify BMC Password
```
In [16]: os.environ['BMC_PASSWORD']
Out[16]: 'Dell0penBMC'

In [17]: os.environ['BMC_PASSWORD'] = 'intel@123'

In [18]: os.environ['BMC_PASSWORD']
Out[18]: 'intel@123'
```
##### g. Show System Configuration
```
In [3]: show_configuration
------> show_configuration()
2024-05-14 23:37:54,323 - INFO: ====================System Configuration====================
2024-05-14 23:37:54,323 - INFO: CPU Project: SRF
2024-05-14 23:37:54,323 - INFO: CUSTOMER TYPE: DELL
2024-05-14 23:37:54,323 - INFO: BMC TYPE: OSM
2024-05-14 23:37:54,323 - INFO: BMC IP ADDRESS: 10.89.92.150
2024-05-14 23:37:54,323 - INFO: BMC USERNAME: root
2024-05-14 23:37:54,323 - INFO: BMC PASSWORD: Dell0penBMC
2024-05-14 23:37:54,323 - INFO: =============================END============================
```

#### 3. BIOS Knob Operation
##### a. Read BIOS Knobs
```
In [2]: read_bios_knobs(knobs='ProcCStates=Enabled,ProcTurboMode=Enabled')
2024-05-14 23:12:01,529 - INFO: ====================System Configuration====================
2024-05-14 23:12:01,529 - INFO: CPU Project: SRF
2024-05-14 23:12:01,529 - INFO: CUSTOMER TYPE: DELL
2024-05-14 23:12:01,529 - INFO: BMC TYPE: OSM
2024-05-14 23:12:01,529 - INFO: BMC IP ADDRESS: 10.89.92.150
2024-05-14 23:12:01,529 - INFO: BMC USERNAME: root
2024-05-14 23:12:01,529 - INFO: BMC PASSWORD: Dell0penBMC
2024-05-14 23:12:01,529 - INFO: =============================END============================
Press any key to continue . . .
2024-05-14 23:12:08,341 - INFO: [DELL][BIOS_KNOB_GET] ProcCStates=Enabled,ProcTurboMode=Enabled
2024-05-14 23:12:10,154 - INFO:
- Current value for attribute "ProcCStates" is "Enabled"

2024-05-14 23:12:10,154 - INFO:
- Current value for attribute "ProcTurboMode" is "Enabled"
```
##### b. Write BIOS Knobs
```
In [3]: os.environ['BMC_IP'] = '10.89.92.202'

In [4]: write_bios_knobs(knobs='ProcPwrPerf=MaxPerf')
2024-05-14 23:14:14,342 - INFO: ====================System Configuration====================
2024-05-14 23:14:14,342 - INFO: CPU Project: SRF
2024-05-14 23:14:14,342 - INFO: CUSTOMER TYPE: DELL
2024-05-14 23:14:14,342 - INFO: BMC TYPE: OSM
2024-05-14 23:14:14,342 - INFO: BMC IP ADDRESS: 10.89.92.202
2024-05-14 23:14:14,342 - INFO: BMC USERNAME: root
2024-05-14 23:14:14,342 - INFO: BMC PASSWORD: Dell0penBMC
2024-05-14 23:14:14,342 - INFO: =============================END============================
Press any key to continue . . .
2024-05-14 23:14:16,361 - INFO: [DELL][BIOS_KNOB_SET] ProcPwrPerf=MaxPerf
2024-05-14 23:14:48,078 - INFO: Starting to reboot the system...
```
##### c. Clear CMOS
```
In [2]: clear_cmos()
2024-05-14 23:19:32,511 - INFO: ====================System Configuration====================
2024-05-14 23:19:32,511 - INFO: CPU Project: SRF
2024-05-14 23:19:32,511 - INFO: CUSTOMER TYPE: DELL
2024-05-14 23:19:32,511 - INFO: BMC TYPE: OSM
2024-05-14 23:19:32,511 - INFO: BMC IP ADDRESS: 10.89.88.166
2024-05-14 23:19:32,511 - INFO: BMC USERNAME: root
2024-05-14 23:19:32,511 - INFO: BMC PASSWORD: Dell0penBMC
2024-05-14 23:19:32,511 - INFO: =============================END============================
Press any key to continue . . .
2024-05-14 23:19:34,137 - INFO: [DELL] Clear CMOS via resetting BIOS konbs by default
[DELL] Skip to clear CMOS due to no bios knob changed
Out[2]: 0
```
