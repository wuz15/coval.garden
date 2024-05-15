import json
import logging
import os
import sys
import IPython
from traitlets.config import Config

'''
Unified interface to call BIOS knobs operation APIs for Co-Val platforms
Supported Co-Val Platform:
	1. Dell
	2. HPE
	3. Lenovo
	4. Inspur
'''
# System Env Variables
os.environ['CPU_PROJECT'] = 'SRF'
os.environ['CUSTOMER_TYPE'] = 'DELL'
os.environ['BMC_TYPE'] = 'OSM'
os.environ['BMC_IP'] = '127.0.0.1'
os.environ['BMC_USERNAME'] = 'root'
os.environ['BMC_PASSWORD'] = 'Dell0penBMC'

# Global Variables
cpu_project = os.environ['CPU_PROJECT']
customer_type = os.environ['CUSTOMER_TYPE']
bmc_type = os.environ['BMC_TYPE']
bmc_ip = os.environ['BMC_IP']
bmc_username = os.environ['BMC_USERNAME']
bmc_password = os.environ['BMC_PASSWORD']

# Logging Configs
logger = logging.getLogger('OOB')
logger.setLevel(logging.DEBUG)
handler = logging.StreamHandler()
formater = logging.Formatter('%(asctime)s - %(levelname)s: %(message)s')
handler.setFormatter(formater)
logger.addHandler(handler)

def update_configuration():
	global cpu_project, customer_type, bmc_type, bmc_ip, bmc_username, bmc_password
	cpu_project = os.environ['CPU_PROJECT']
	customer_type = os.environ['CUSTOMER_TYPE']
	bmc_type = os.environ['BMC_TYPE']
	bmc_ip = os.environ['BMC_IP']
	bmc_username = os.environ['BMC_USERNAME']
	bmc_password = os.environ['BMC_PASSWORD']

def show_configuration():
	update_configuration()
	logger.info("====================System Configuration====================")
	logger.info("CPU Project: {}".format(cpu_project))
	logger.info("CUSTOMER TYPE: {}".format(customer_type))
	logger.info("BMC TYPE: {}".format(bmc_type))
	logger.info("BMC IP ADDRESS: {}".format(bmc_ip))
	logger.info("BMC USERNAME: {}".format(bmc_username))
	logger.info("BMC PASSWORD: {}".format(bmc_password))
	logger.info("=============================END============================")

def get_bios_knob_ops():
	'''
	Unified interface to call BIOS knobs operation APIs for Co-Val platforms
	Supported Co-Val Platform:
		1. Dell
		2. HPE
		3. Lenovo
		4. Inspur
		5. Intel
	'''
	logger.info("====================System Configuration====================")
	logger.info("CPU Project: {}".format(cpu_project))
	logger.info("CUSTOMER TYPE: {}".format(customer_type))
	logger.info("BMC TYPE: {}".format(bmc_type))
	logger.info("BMC IP ADDRESS: {}".format(bmc_ip))
	logger.info("BMC USERNAME: {}".format(bmc_username))
	logger.info("BMC PASSWORD: {}".format(bmc_password))
	logger.info("=============================END============================")
	os.system('pause')
	if customer_type.upper() == 'DELL':
		from dell_biosknob import DellBiosKnobOps
		return DellBiosKnobOps(bmc_ip, bmc_username, bmc_password, logger, bmc_type)
	elif customer_type.upper() == 'HPE':
		from HPE_biosknob import HpeBiosKnobOps
		return HpeBiosKnobOps(bmc_ip, bmc_username, bmc_password, logger)
	elif customer_type == 'LENOVO':
		from lenovo_biosknob import LenovoBiosKnobOps
		return LenovoBiosKnobOps(bmc_ip, bmc_username, bmc_password, logger)
	elif customer_type == 'INSPUR':
		from inspur_biosknob import InspurBiosKnobOps
		return InspurBiosKnobOps(bmc_ip, bmc_username, bmc_password, logger)
	elif customer_type == 'INTEL':
		from intel_biosknob import IntelBiosKnobOps
		return IntelBiosKnobOps(bmc_ip, bmc_username, bmc_password, logger)
	else:
		raise Exception("{} was not supported yet, please check your platform type".format(customer_type))

def write_bios_knobs(knobs=None):
	update_configuration()
	return get_bios_knob_ops().write_bios_knobs(knobs)

def read_bios_knobs(knobs=None):
	update_configuration()
	return get_bios_knob_ops().read_bios_knobs(knobs)

def clear_cmos():
	update_configuration()
	return get_bios_knob_ops().clear_cmos()

if __name__ == "__main__":
	_ipython_config = Config()
	_ipython_config.InteractiveShell.autocall = 2  # so "refresh" behaves the same as "refresh()"
	os.environ['PYTHONINSPECT'] = ''
	IPython.embed(config=_ipython_config, colors="neutral")
