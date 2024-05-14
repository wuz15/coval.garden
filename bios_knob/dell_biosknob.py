import argparse
import sys
import time
import datetime
import requests
import json
import re
import copy
import paramiko

redfish_bios_setting_url = {
	'IDRAC': '/redfish/v1/Systems/System.Embedded.1/Bios/Settings',
	'OSM': '/redfish/v1/Managers/bmc/Actions/Oem/Dell/DellOpenBMCManager.ImportSystemConfiguration',
}

redfish_bios_url = {
	'IDRAC': '/redfish/v1/Systems/System.Embedded.1/Bios',
	'OSM': '/redfish/v1/Managers/bmc/Actions/Oem/Dell/DellOpenBMCManager.ExportSystemConfiguration',
}

redfish_bios_reset_url = {
	'IDRAC': '/redfish/v1/Systems/System.Embedded.1/Bios/Actions/Bios.ResetBios',
	'OSM': '/redfish/v1/Systems/system/Bios/Actions/Bios.ResetBios',
}

redfish_reset_url = {
	'IDRAC': '/redfish/v1/Systems/System.Embedded.1/Actions/ComputerSystem.Reset',
	'OSM': '/redfish/v1/Systems/system/Actions/ComputerSystem.Reset',
}

osm_bios_knob_payload_template = {
	"SystemConfiguration": {
		"Comments": [
			{
				"comment":"Export type is Normal,JSON,Selective"
			}
		],
		"Model": "PowerEdge HS7720",
		"Components": [
			{
				"FQDD":"BIOS.Setup.1-1",
				"Attributes":[
				]
			}
		]
	}
}

class DellBiosKnobOps(object):
	'''
	BIOS knob operations set for Dell platform (Based on Redfish)
	Supported Operations:
		1. read_bios_knobs
		2. write_bios_knobs
		3. clear_cmos
	'''
	def __init__(self, ip, username, password, logger, bmc_type):
		self.ip = ip
		self.username = username
		self.password = password
		self.auth = (username, password)
		self.bmc_type = 'IDRAC' if bmc_type not in ['IDRAC', 'OSM'] else bmc_type
		self._log = logger.info
		requests.urllib3.disable_warnings()
		self.session = requests.session()
		self.session.headers['Content-Type'] = 'application/json'

	def wait_for_task_complete(self, task_resp):
		# Check whether the BIOS configuration task being completed within 30 minutes (Only for IDRAC)
		job_location = task_resp.headers["Location"]
		try:
			job_id = re.search("JID.+", job_location).group()
		except Exception:
			raise Exception("Failed to create a BIOS.Setup job")

		url_task = r'https://{0}/redfish/v1/TaskService/Tasks/{1}'.format(self.ip, job_id)
		__start = datetime.datetime.now()
		while (datetime.datetime.now() - __start).seconds < 30 * 60:
			time.sleep(10)
			resp = self.session.get(url_task, auth=self.auth, verify=False)
			try:
				if resp.status_code not in [200, 202, 204]:
					raise Exception("Failed to send Redfish request with error code {} - {} ".format(resp.status_code, resp.text))
			except Exception:
				continue

			if resp.json()['Messages'][0]['Message'] == "Task successfully scheduled.":
				self._log("[DELL][IDRAC_TASK]: BIOS.Setup Job {} was created successfully".format(job_id))
				break

	def do_power_cycle(self):
		# For OSM platform, use ipmitool to do the power cycle instead of redfish
		if self.bmc_type == 'OSM':
			bmc_ssh = paramiko.SSHClient()
			bmc_ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
			try:
				bmc_ssh.connect(self.ip, 22, self.username, self.password)
				bmc_ssh.exec_command("ipmitool power off", timeout=300)
				time.sleep(25)
				bmc_ssh.exec_command("ipmitool power on", timeout=300)
				bmc_ssh.close()
			except Exception as e:
				self._log("Failed to do power cycle via ipmitool due to {}".format(e))
		else:
			url = r'https://{0}'.format(self.ip) + redfish_reset_url[self.bmc_type]
			reset_body = {"ResetType": "PowerCycle"}
			resp = self.session.post(url, data=json.dumps(reset_body), auth=self.auth, verify=False)
			if resp.status_code not in [200, 202, 204]:
				raise Exception("Failed to send Redfish request with error code {} - {} ".format(resp.status_code, resp.text))


	def write_bios_knobs(self, knobs=None):
		self._log("[DELL][BIOS_KNOB_SET] %s" % knobs)
		# Sleep 30 sec to make sure OS monitor thread is closed.
		time.sleep(30)
		# Initialize and config the Dell Redfish
		url = r'https://{0}'.format(self.ip) + redfish_bios_setting_url[self.bmc_type]
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

		# Update BIOS knobs
		if self.bmc_type == 'OSM':
			osm_payload = copy.deepcopy(osm_bios_knob_payload_template)
			for key, value in payload['Attributes'].items():
				osm_bios_attribute = {}
				osm_bios_attribute['Name'] = key
				osm_bios_attribute['Value'] = value
				osm_bios_attribute['Set On Import'] = 'True'
				osm_bios_attribute['Comment'] = 'Read and Write'
				osm_payload['SystemConfiguration']['Components'][0]['Attributes'].append(osm_bios_attribute)

			resp = self.session.post(url, data=json.dumps(osm_payload), auth=self.auth, verify=False)
		else:
			apply_payload = {"@Redfish.SettingsApplyTime": {"ApplyTime": "OnReset"}}
			apply_payload.update(payload)
			resp = self.session.patch(url, data=json.dumps(apply_payload), auth=self.auth, verify=False)
		if resp.status_code not in [200, 202, 204]:
			raise Exception("Failed to send Redfish request with error code {} - {} ".format(resp.status_code, resp.text))

		if self.bmc_type == 'IDRAC':
			self.wait_for_task_complete(resp)

		# Trigger reboot to make BIOS knob changes take effect
		self._log("Starting to reboot the system...")
		self.do_power_cycle()

		return 0

	def read_bios_knobs(self, knobs=None):
		self._log("[DELL][BIOS_KNOB_GET] %s" % knobs)
		# Initialize and config the Dell Redfish
		knobs = knobs.split(',')
		url = r'https://{0}'.format(self.ip) + redfish_bios_url[self.bmc_type]
		if self.bmc_type == 'OSM':
			resp = self.session.post(url, auth=self.auth, verify=False)
		else:
			resp = self.session.get(url, auth=self.auth, verify=False)

		if resp.status_code not in [200, 202, 204]:
			raise Exception("Failed to send Redfish request with error code {} - {} ".format(resp.status_code, resp.text))

		# Reformat resp data from OSM
		if self.bmc_type == 'OSM':
			raw_data = resp.json()['SystemConfiguration']['Components'][0]['Attributes']
			data = {'Attributes': {}}
			for item in raw_data:
				data['Attributes'][item['Name']] = item['Value']
		else:
			data = resp.json()

		# Find the BIOS knob attribute whether it has value. If yes, show the value; otherwise, report the error. 
		for current_knob in knobs:
			check_status = False
			for key, value in data['Attributes'].items():
				if key == current_knob.split('=')[0].strip():
					self._log("\n- Current value for attribute \"%s\" is \"%s\"\n" % (current_knob.split('=')[0].strip(), value))
					check_status = True
					break
			if check_status == False:
				self._log("\n- (ERROR!) Current value for attribute \"%s\" cannot be found.\n" % (current_knob.split('=')[0].strip()))

		return 0

	def get_modified_bios_knobs(self):
		# Get current bios knobs
		url = r'https://{0}'.format(self.ip) + redfish_bios_url[self.bmc_type]
		resp = self.session.post(url, auth=self.auth, verify=False)
		if resp.status_code not in [200, 202, 204]:
			raise Exception("Failed to send Redfish request with error code {} - {} ".format(resp.status_code, resp.text))
		current_bios_knobs = resp.json()['SystemConfiguration']['Components'][0]['Attributes']

		# Get default bios knobs
		with open('tester_instrument/coval_instrument/dell_osm_default_bios_config.json', 'r') as default_bios_config_json_file:
				default_bios_config = json.load(default_bios_config_json_file)
		default_bios_knobs = default_bios_config['SystemConfiguration']['Components'][0]['Attributes']

		# Finalize bios knob list which required to be changed back
		modified_bios_config = copy.deepcopy(osm_bios_knob_payload_template)
		for bios_knob in default_bios_knobs:
			osm_bios_attribute = {}
			for item in current_bios_knobs:
				if item['Name'] == bios_knob['Name'] and item['Value'] != bios_knob['Value']:
					self._log("[DELL][CLEAR_CMOS] Change BIOS_KNOB#{} back to default: {} -> {}".format(bios_knob['Name'], item['Value'], bios_knob['Value']))
					osm_bios_attribute['Name'] = bios_knob['Name']
					osm_bios_attribute['Value'] = bios_knob['Value']
					osm_bios_attribute['Set On Import'] = 'True'
					osm_bios_attribute['Comment'] = 'Read and Write'
					modified_bios_config['SystemConfiguration']['Components'][0]['Attributes'].append(osm_bios_attribute)

		return modified_bios_config

	def clear_cmos(self):
		self._log("[DELL] Clear CMOS via resetting BIOS konbs by default")
		if self.bmc_type == 'OSM':
			# WA for OSM BIOS loading default
			url = r'https://{0}'.format(self.ip) + redfish_bios_setting_url[self.bmc_type]
			modified_bios_knobs = self.get_modified_bios_knobs()
			if not modified_bios_knobs['SystemConfiguration']['Components'][0]['Attributes']:
				print("[DELL] Skip to clear CMOS due to no bios knob changed")
				return 0
			else:
				resp = self.session.post(url, data=json.dumps(modified_bios_knobs), auth=self.auth, verify=False)
		else:
			# Initialize and config the Dell Redfish
			url = r'https://{0}'.format(self.ip) + redfish_bios_reset_url[self.bmc_type]
			# Send BIOS reset request
			data = {}
			resp = self.session.post(url, auth=self.auth, data=json.dumps(data), verify=False)

		if resp.status_code not in [200, 202, 204]:
			raise Exception("Failed to send Redfish request with error code {} - {} ".format(resp.status_code, resp.text))

		# Trigger reboot to make ResetBios take effect
		self._log("Starting to reboot the system...")
		self.do_power_cycle()

		return 0
