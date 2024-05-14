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

def get_bios_knob_ops():
	'''
	Unified interface to call BIOS knobs operation APIs for Co-Val platforms
	Supported Co-Val Platform:
		1. Dell
		2. HPE
		3. Lenovo
		4. Inspur
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
	else:
		raise Exception("{} was not supported yet, please check your platform type".format(customer_type))

def get_mapped_bios_knobs(knobs=None):
	'''
	Used to patch bios_knobs if the bios_knob path being different between RP platform and Co-Val Platform
	'''
	global cpu_project, customer_type
	cpu_project = os.environ['CPU_PROJECT']
	customer_type = os.environ['CUSTOMER_TYPE']
	bmc_type = os.environ['BMC_TYPE']
	bmc_ip = os.environ['BMC_IP']
	bmc_username = os.environ['BMC_USERNAME']
	bmc_password = os.environ['BMC_PASSWORD']

	if cpu_project.upper() == 'SPR':
		mapping_json_file = "coval_biosknob_mapping_spr.json"
	elif cpu_project == 'GNR':
		mapping_json_file = "coval_biosknob_mapping_gnr.json"
	elif cpu_project == 'SRF':
		mapping_json_file = "coval_biosknob_mapping_srf.json"
	else:
		logger.info("Skip Co-Val platform bios_knobs mapping due to {} not supported yet".format(cpu_project))
		return knobs

	delimiter = '='

	mapped_knobs = []
	with open(mapping_json_file) as json_file:
		bios_knobs_mapping = json.load(json_file)
		for knob in knobs.split(','):
			key = knob.split(delimiter)[0].strip()
			value = knob.split(delimiter)[1].strip()
			# Checking whether bios_knob mapping being required
			if key in bios_knobs_mapping.keys() and value in bios_knobs_mapping[key].keys() \
												and customer_type in bios_knobs_mapping[key][value].keys():
				mapped_knob = bios_knobs_mapping[key][value][customer_type]
				# Ignore the bios_knob if it being not supported on Co-Val platform
				if mapped_knob != '':
					mapped_knobs.append(mapped_knob)
			else:
				mapped_knobs.append(knob)
	mapped_knobs = ','.join(mapped_knobs)

	return mapped_knobs

def write_bios_knobs(knobs=None):
	mapped_knobs = get_mapped_bios_knobs(knobs)
	if mapped_knobs != knobs:
		if mapped_knobs == '':
			logger.info("Skip bios_knobs ops due to {} not supported on {} platform".format(knobs, customer_type))
			return 0
		else:
			logger.info("[{}] {} being mapped to {}".format(customer_type, knobs, mapped_knobs))

	return get_bios_knob_ops().write_bios_knobs(mapped_knobs)

def read_bios_knobs(knobs=None):
	mapped_knobs = get_mapped_bios_knobs(knobs)
	if mapped_knobs != knobs:
		if mapped_knobs == '':
			logger.info("Skip bios_knobs ops due to {} not supported on {} platform".format(knobs, customer_type))
			return 0
		else:
			logger.info("[{}] {} being mapped to {}".format(customer_type, knobs, mapped_knobs))

	return get_bios_knob_ops().read_bios_knobs(mapped_knobs)

def clear_cmos(self):
	return get_bios_knob_ops().clear_cmos()

if __name__ == "__main__":
	_ipython_config = Config()
	_ipython_config.InteractiveShell.autocall = 2  # so "refresh" behaves the same as "refresh()"
	os.environ['PYTHONINSPECT'] = ''
	IPython.embed(config=_ipython_config, colors="neutral")
