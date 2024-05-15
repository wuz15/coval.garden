import argparse
import sys
import redfish
import time
import json

class IntelBiosKnobOps(object):
	'''
	BIOS knob operations set for Intel platform (Based on Redfish)
	Supported Operations:
		1. read_bios_knobs
		2. write_bios_knobs
		3. clear_cmos
	'''
	def __init__(self, ip, username, password, logger):
		self.ip = ip
		self.username = username
		self.password = password
		self.auth = (username, password)
		self._log = logger.info

	def do_power_cycle(self):
		url = r'https://{0}/redfish/v1/Systems/system/Actions/ComputerSystem.Reset'.format(self.ip)
		reset_body = {"ResetType": "PowerCycle"}
		resp = self.session.post(url, data=json.dumps(reset_body), auth=self.auth, verify=False)
		if resp.status_code not in [200, 202, 204]:
			raise Exception("Failed to send Redfish request with error code {} - {} ".format(resp.status_code, resp.text))

	def write_bios_knobs(self, knobs=None):
		self._log("[INTEL][BIOS_KNOB_SET] %s" % knobs)
		# Initialize and config the HPE Redfish
		url = r'https://{0}/redfish/v1/Systems/system/Bios/Settings'.format(self.ip)
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
		resp = self.session.patch(url, data=json.dumps(payload), auth=self.auth, verify=False)
		if resp.status_code not in [200, 202, 204]:
			raise Exception("Failed to send Redfish request with error code {} - {} ".format(resp.status_code, resp.text))

		# Trigger reboot to make BIOS knob changes take effect
		self._log("Starting to reboot the system...")
		self.do_power_cycle()

		return 0

	def read_bios_knobs(self, knobs=None):
		self._log("[INTEL][BIOS_KNOB_GET] %s" % knobs)
		# Initialize and config the Intel Redfish
		knobs = knobs.split(',')
		url = r'https://{0}'.format(self.ip)

		resp = self.session.get(url, auth=self.auth, verify=False)
		if resp.status_code not in [200, 202, 204]:
			raise Exception("Failed to send Redfish request with error code {} - {} ".format(resp.status_code, resp.text))

		data = resp.json()
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
					break
			if check_status == False:
				self._log("\n- (ERROR!) Current value for attribute \"%s\" cannot be found.\n" % (current_knob.split('=')[0].strip()))
		return 0

	def clear_cmos(self):
		self._log("[INTEL] Clear CMOS via resetting BIOS konbs by default")
		# Initialize and config the Dell Redfish
		url = r'https://{0}/redfish/v1/Systems/system/Bios/Actions/Bios.ResetBios'.format(self.ip)
		# Send BIOS reset request
		data = {}
		resp = self.session.post(url, auth=self.auth, data=json.dumps(data), verify=False)

		if resp.status_code not in [200, 202, 204]:
			raise Exception("Failed to send Redfish request with error code {} - {} ".format(resp.status_code, resp.text))

		# Trigger reboot to make ResetBios take effect
		self._log("Starting to reboot the system...")
		self.do_power_cycle()

		return 0
