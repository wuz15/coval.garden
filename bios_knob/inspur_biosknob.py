import argparse
import sys
import redfish
import time

class InspurBiosKnobOps(object):
	'''
	BIOS knob operations set for Inspur platform (Based on Redfish)
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
		self._log("[INSPUR][BIOS_KNOB_SET] %s" % knobs)
		# Initialize and config the Inspur Redfish
		rf = redfish.redfish_client(base_url="https://%s" % self.ip, username=self.username, password=self.password)
		rf.login()
		# Get Etag before setting bios setting
		url = '/redfish/v1/Systems/1/Bios'
		etag = rf.get(url).getheader('Etag')

		# Starting to set bios knobs
		url = '/redfish/v1/Systems/1/Bios/Settings'
		name_list = []
		value_list = []
		headers = {"If-Match": etag, "Content-Type": "application/json"}
		payload = {}
		knobs = knobs.split(',')

		# Split knobs name and knobs value to two lists
		for index, value in enumerate(knobs):
			temp_list = knobs[index].split('=')[0].strip()
			name_list.append(temp_list)
			temp_list = knobs[index].split('=')[1].strip()
			value_list.append(temp_list)

		# Convert name_list and value_list to payload (a dictionary)
		for key, value in zip(name_list, value_list):
			payload[key] = value

		# Patching BIOS knobs
		resp = rf.patch(url, body=payload, headers=headers)
		self._log("Respone Text: {}".format(resp.text))
		if resp.status not in [200, 202, 204]:
			raise Exception("Failed to send Redfish request with error code {} - {} ".format(resp.status, resp.text))

		# Trigger reboot to make BIOS knob changes take effect
		self._log("Starting to reboot the system.")
		url = '/redfish/v1/Chassis/1/Actions/Chassis.Reset'
		reset_body = {"ResetType": "PowerCycle"}
		resp = rf.post(url, body=reset_body)
		if resp.status not in [200, 202, 204]:
			raise Exception("Failed to send Redfish request with error code {} - {} ".format(resp.status, resp.text))

		return 0

	def read_bios_knobs(self, knobs=None):
		self._log("[INSPUR][BIOS_KNOB_GET] %s" % knobs)
		# Initialize and config the Inspur Redfish
		knobs = knobs.split(',')
		rf = redfish.redfish_client(base_url="https://%s" % self.ip, username=self.username, password=self.password)
		rf.login()
		url = '/redfish/v1/Systems/1/Bios'
		data = rf.get(url)
		data = data.dict

		# Find the BIOS knob attribute whether it has value. If yes, show the value; otherwise, report the error. 
		for current_knob in knobs:
			check_status = False
			current_knob = current_knob.split('=')[0].strip()
			for key, value in data['Attributes'].items():
				if key == current_knob:
					self._log("\n- Current value for attribute \"%s\" is \"%s\"\n" % (current_knob, value))
					check_status = True
					break
				elif isinstance(value, dict) and current_knob in value.keys():
					self._log("\n- Current value for attribute \"%s\" is \"%s\"\n" % (current_knob, value.get(current_knob)))
					check_status = True
					break
			if check_status == False:
				self._log("\n- (ERROR!) Current value for attribute \"%s\" cannot be found.\n" % (current_knob))

		return 0

	def clear_cmos(self):
		self._log("[INSPUR] Clear CMOS via resetting BIOS konbs by default")
		# Initialize and config the Inspur Redfish
		rf = redfish.redfish_client(base_url="https://%s" % self.ip, username=self.username, password=self.password)
		rf.login()
		url = '/redfish/v1/Systems/1/Bios/Actions/Bios.ResetBios'
		# Send BIOS reset request
		resp = rf.post(url)
		if resp.status not in [200, 202, 204]:
			raise Exception("Failed to send Redfish request with error code {} - {} ".format(resp.status, resp.text))

		# Trigger reboot to make ResetBios take effect
		self._log("Starting to reboot the system.")
		url = '/redfish/v1/Chassis/1/Actions/Chassis.Reset'
		reset_body = {"ResetType": "PowerCycle"}
		resp = rf.post(url, body=reset_body)
		if resp.status not in [200, 202, 204]:
			raise Exception("Failed to send Redfish request with error code {} - {} ".format(resp.status, resp.text))

		return 0