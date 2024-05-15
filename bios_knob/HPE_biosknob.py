import argparse
import sys
import redfish
import time

class HpeBiosKnobOps(object):
	'''
	BIOS knob operations set for HPE platform (Based on Redfish)
	Supported Operations:
		1. read_bios_knobs
		2. write_bios_knobs
		3. clear_cmos
	'''
	def __init__(self, ip, username, password, logger):
		self.ip = ip
		self.username = username
		self.password = password
		self._log = logger.info

	def write_bios_knobs(self, knobs=None):
		self._log("[HPE][BIOS_KNOB_SET] %s" % knobs)
		# Initialize and config the HPE Redfish
		rf = redfish.redfish_client(base_url="https://%s" % self.ip, username=self.username, password=self.password)
		rf.login()
		url = '/redfish/v1/systems/1/bios/settings'
		name_list = []
		value_list = []
		payload = {"Attributes":{}}
		knobs = knobs.split(',')

		# Split knobs name and knobs value to two lists
		for index, value in enumerate(knobs):
			temp_list = knobs[index].split('=')[0].strip()
			name_list.append(temp_list)
			temp_list = knobs[index].split('=')[1].strip()
			value_list.append(temp_list)

		# Convert name_list and value_list to payload (a dictionary)
		for key, value in zip(name_list, value_list):
			payload["Attributes"][key] = value

		# Patching BIOS knobs
		resp = rf.patch(url, body=payload)
		# Change the data type to dictionary
		check_message = resp.dict
		# Check the BIOS knob whether success. If yes, the system will reboot; otherwise, report the error. 
		if "SystemResetRequired" in check_message["error"]["@Message.ExtendedInfo"][0]["MessageId"]:
			self._log("Change BIOS knob PASSED!!")
			self._log("Starting to reboot the system.")
			url = '/redfish/v1/Systems/1/Actions/Oem/Hpe/HpeComputerSystemExt.SystemReset/'
			reset_body = {"ResetType":"ColdBoot"}
			resp = rf.post(url, body=reset_body)
		else:
			self._log("Change BIOS knob FAILED !!")
			self._log(check_message)

		return 0

	def read_bios_knobs(self, knobs=None):
		self._log("[HPE][BIOS_KNOB_GET] %s" % knobs)
		# Initialize and config the HPE Redfish
		knobs = knobs.split(',')
		rf = redfish.redfish_client(base_url="https://%s" % self.ip, username=self.username, password=self.password)
		rf.login()
		url = '/redfish/v1/systems/1/bios'
		data = rf.get(url)
		data = data.dict

		# Find the BIOS knob attribute whether it has value. If yes, show the value; otherwise, report the error.
		print("----------------------------------------------------------------------------------------------")
		print("|{:^30s}|{:^30s}|{:^30s}|".format('BIOS_KNOB_NAME', 'CURRENT_VALUE', 'EXPECT_VALUE'))
		print("----------------------------------------------------------------------------------------------")
		for current_knob in knobs:
			check_status = False
			for key, value in data['Attributes'].items():
				if key == current_knob.split('=')[0].strip():
					print("|{:^30s}|{:^30s}|{:^30s}|".format(key, value, current_knob.split('=')[1].strip()))
					print("----------------------------------------------------------------------------------------------")
					check_status = True
			if check_status == False:
				self._log("\n- (ERROR!) Current value for attribute \"%s\" cannot be found.\n" % (current_knob.strip()))

		return 0

	def clear_cmos(self):
		self._log("[HPE] Clear CMOS via resetting BIOS konbs by default")
		# Clear CMOS through the write_bios_knobs function
		self.write_bios_knobs("RestoreDefaults=Yes")

		return 0
