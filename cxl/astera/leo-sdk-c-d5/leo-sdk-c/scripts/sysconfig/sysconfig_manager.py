#!/usr/bin/env python3.6

import sys
import json
import argparse
import os
from functools import reduce


class SysconfigConfigManager:
    crc32Table = [
        0x00000000, 0xF26B8303, 0xE13B70F7, 0x1350F3F4,
        0xC79A971F, 0x35F1141C, 0x26A1E7E8, 0xD4CA64EB,
        0x8AD958CF, 0x78B2DBCC, 0x6BE22838, 0x9989AB3B,
        0x4D43CFD0, 0xBF284CD3, 0xAC78BF27, 0x5E133C24,
        0x105EC76F, 0xE235446C, 0xF165B798, 0x030E349B,
        0xD7C45070, 0x25AFD373, 0x36FF2087, 0xC494A384,
        0x9A879FA0, 0x68EC1CA3, 0x7BBCEF57, 0x89D76C54,
        0x5D1D08BF, 0xAF768BBC, 0xBC267848, 0x4E4DFB4B,
        0x20BD8EDE, 0xD2D60DDD, 0xC186FE29, 0x33ED7D2A,
        0xE72719C1, 0x154C9AC2, 0x061C6936, 0xF477EA35,
        0xAA64D611, 0x580F5512, 0x4B5FA6E6, 0xB93425E5,
        0x6DFE410E, 0x9F95C20D, 0x8CC531F9, 0x7EAEB2FA,
        0x30E349B1, 0xC288CAB2, 0xD1D83946, 0x23B3BA45,
        0xF779DEAE, 0x05125DAD, 0x1642AE59, 0xE4292D5A,
        0xBA3A117E, 0x4851927D, 0x5B016189, 0xA96AE28A,
        0x7DA08661, 0x8FCB0562, 0x9C9BF696, 0x6EF07595,
        0x417B1DBC, 0xB3109EBF, 0xA0406D4B, 0x522BEE48,
        0x86E18AA3, 0x748A09A0, 0x67DAFA54, 0x95B17957,
        0xCBA24573, 0x39C9C670, 0x2A993584, 0xD8F2B687,
        0x0C38D26C, 0xFE53516F, 0xED03A29B, 0x1F682198,
        0x5125DAD3, 0xA34E59D0, 0xB01EAA24, 0x42752927,
        0x96BF4DCC, 0x64D4CECF, 0x77843D3B, 0x85EFBE38,
        0xDBFC821C, 0x2997011F, 0x3AC7F2EB, 0xC8AC71E8,
        0x1C661503, 0xEE0D9600, 0xFD5D65F4, 0x0F36E6F7,
        0x61C69362, 0x93AD1061, 0x80FDE395, 0x72966096,
        0xA65C047D, 0x5437877E, 0x4767748A, 0xB50CF789,
        0xEB1FCBAD, 0x197448AE, 0x0A24BB5A, 0xF84F3859,
        0x2C855CB2, 0xDEEEDFB1, 0xCDBE2C45, 0x3FD5AF46,
        0x7198540D, 0x83F3D70E, 0x90A324FA, 0x62C8A7F9,
        0xB602C312, 0x44694011, 0x5739B3E5, 0xA55230E6,
        0xFB410CC2, 0x092A8FC1, 0x1A7A7C35, 0xE811FF36,
        0x3CDB9BDD, 0xCEB018DE, 0xDDE0EB2A, 0x2F8B6829,
        0x82F63B78, 0x709DB87B, 0x63CD4B8F, 0x91A6C88C,
        0x456CAC67, 0xB7072F64, 0xA457DC90, 0x563C5F93,
        0x082F63B7, 0xFA44E0B4, 0xE9141340, 0x1B7F9043,
        0xCFB5F4A8, 0x3DDE77AB, 0x2E8E845F, 0xDCE5075C,
        0x92A8FC17, 0x60C37F14, 0x73938CE0, 0x81F80FE3,
        0x55326B08, 0xA759E80B, 0xB4091BFF, 0x466298FC,
        0x1871A4D8, 0xEA1A27DB, 0xF94AD42F, 0x0B21572C,
        0xDFEB33C7, 0x2D80B0C4, 0x3ED04330, 0xCCBBC033,
        0xA24BB5A6, 0x502036A5, 0x4370C551, 0xB11B4652,
        0x65D122B9, 0x97BAA1BA, 0x84EA524E, 0x7681D14D,
        0x2892ED69, 0xDAF96E6A, 0xC9A99D9E, 0x3BC21E9D,
        0xEF087A76, 0x1D63F975, 0x0E330A81, 0xFC588982,
        0xB21572C9, 0x407EF1CA, 0x532E023E, 0xA145813D,
        0x758FE5D6, 0x87E466D5, 0x94B49521, 0x66DF1622,
        0x38CC2A06, 0xCAA7A905, 0xD9F75AF1, 0x2B9CD9F2,
        0xFF56BD19, 0x0D3D3E1A, 0x1E6DCDEE, 0xEC064EED,
        0xC38D26C4, 0x31E6A5C7, 0x22B65633, 0xD0DDD530,
        0x0417B1DB, 0xF67C32D8, 0xE52CC12C, 0x1747422F,
        0x49547E0B, 0xBB3FFD08, 0xA86F0EFC, 0x5A048DFF,
        0x8ECEE914, 0x7CA56A17, 0x6FF599E3, 0x9D9E1AE0,
        0xD3D3E1AB, 0x21B862A8, 0x32E8915C, 0xC083125F,
        0x144976B4, 0xE622F5B7, 0xF5720643, 0x07198540,
        0x590AB964, 0xAB613A67, 0xB831C993, 0x4A5A4A90,
        0x9E902E7B, 0x6CFBAD78, 0x7FAB5E8C, 0x8DC0DD8F,
        0xE330A81A, 0x115B2B19, 0x020BD8ED, 0xF0605BEE,
        0x24AA3F05, 0xD6C1BC06, 0xC5914FF2, 0x37FACCF1,
        0x69E9F0D5, 0x9B8273D6, 0x88D28022, 0x7AB90321,
        0xAE7367CA, 0x5C18E4C9, 0x4F48173D, 0xBD23943E,
        0xF36E6F75, 0x0105EC76, 0x12551F82, 0xE03E9C81,
        0x34F4F86A, 0xC69F7B69, 0xD5CF889D, 0x27A40B9E,
        0x79B737BA, 0x8BDCB4B9, 0x988C474D, 0x6AE7C44E,
        0xBE2DA0A5, 0x4C4623A6, 0x5F16D052, 0xAD7D5351
    ]
    header_dw_count = 9
    header_pat = 0x5aa55aa5
    trailer_pat = 0xaa55aa55
    mods = []
    mods_edited = 0
    shortlist = [
        ("g_ddr_refresh_mode", 0, "ddr grefresh omde"), 
        ("g_ddr_page_close_mode", 0, "sbr")
    ]

    def __init__(self, fw, cfg, out):
        self.fw = fw
        self.out = out
        self.cfg = self.parse_cfg(cfg)
        self.id_file_data = self.parse_id_file()

    # Load the id file into a dict
    def parse_id_file(self):
        id_file = os.path.join(os.path.dirname(__file__), "leo_system_config_param_id_pub.json")
        with open(id_file) as f:
            return json.load(f)

    # Take a config file and return a dict of only the valid options
    def parse_cfg(self, cfg):
        if cfg is None:
            return {}
        with open(cfg) as f:
            data = json.load(f)

        customcfg = data.copy()

        def is_valid_preboot_option(pair):
            key, value = pair
            if key in ["fw", "out"] or value in [None]:
                return False
            if key in ["temp_threshold", "bw_throttle", "cxl_temp_threshold"]:
                found = False
                for p in value.items():
                    if is_valid_preboot_option(p):
                        found = True
                return found
            return True

        return dict(filter(is_valid_preboot_option, customcfg.items()))

    # Parse config file and set the appropriate bits in returned output file
    def edit_cfg(self):
        mods = []
        data = self.cfg
        # g_ddr_frequency
        if "speed" in data.keys():
            self.set_speed(data["speed"])
            mods.append(str(data["speed"]))

        # g_cxl_compliance
        if "compliance" in data.keys():
            self.set_cxl_compliance(data["compliance"])
            mods.append("compliance")

        # g_ddr_refresh_mode
        if "sbrefresh" in data.keys():
            self.set_sbrefresh(data["sbrefresh"])
            mods.append("refsb")
        
        # g_ddr_2dpc
        if "ddr2dpc" in data.keys():
            self.set_ddr_2dpc()
            mods.append("2dpc")

        # g_cxl_num_link link_0_cxl_link_width link_1_cxl_link_width
        if "cxl2x8" in data.keys():
            self.set_cxl_link_num(2)
            self.set_cxl_link_0_width(8)
            self.set_cxl_link_1_width(8)
            mods.append("x2")

        # g_threshold_temp_[0-2, c]
        if "temp_threshold" in data.keys():
            if self.set_temp_threshold(data["temp_threshold"]):
                mods.append("tmpthrsh")

        # g_threshold_bw_[0-2]
        if "bw_throttle" in data.keys():
            if self.set_bw_throttle(data["bw_throttle"]):
                mods.append("thrbw")

        # g_temp_sample_interval
        if "temp_sample_interval_s" in data.keys():
            self.set_temp_sample_interval(data["temp_sample_interval_s"])
            mods.append("tmpintvl")

        # g_ddr_page_close_mode
        if "ddr_page_close" in data.keys():
            self.set_ddr_page_close(data["ddr_page_close"])
            mods.append("pgcl")

        # link_0_temp_threshold_warning_lo link_0_temp_threshold_warning_hi link_0_temp_threshold_critical_lo link_0_temp_threshold_critical_hi
        # link_1_temp_threshold_warning_lo link_1_temp_threshold_warning_hi link_1_temp_threshold_critical_lo link_1_temp_threshold_critical_hi
        if "cxl_temp_threshold" in data.keys():
            if self.set_cxl_temp_threshold(data["cxl_temp_threshold"]):
                mods.append("cxltmpthrsh")

        # g_cxl_correctable_err_threshold
        if "cxl_correctable_err_threshold" in data.keys():
            self.set_cxl_correctable_err_threshold(data["cxl_correctable_err_threshold"])
            mods.append("cethrsh")

        # g_cxl_mem_size
        if "cxl_mem_size" in data.keys():
            self.set_cxl_mem_size(data["cxl_mem_size"])
            mods.append("cxlmemsz")

        # g_ddr_interleave_ways
        if "ddr_interleave_ways" in data.keys():
            ways = data["ddr_interleave_ways"]
            self.set_ddr_address_interleaving(ways)
            if -1 != ways:
                mods.append(f"{ways}wayddrintlg")

        # legacy name for g_ddr_interleave_ways
        if "ddrinterleaving" in data.keys():
            ways = data["ddrinterleaving"]
            self.set_ddr_address_interleaving(ways)
            if -1 != ways:
                mods.append(f"{ways}wayddrintlg")

        # g_pmic_current_mode
        if "pmic_current_mode" in data.keys():
            pmic_current_mode = data["pmic_current_mode"]
            mode = self.set_pmic_current_mode(pmic_current_mode)
            mods.append(f"pcm{mode}")

        # g_aes_mode
        if "aes_mode" in data.keys():
            mode = data["aes_mode"]
            self.set_aes_mode(mode)
            mods.append(f"aes{mode}")

        # g_ddr_check_guard_rails
        if "ddr_check_guard_rails" in data.keys():
            self.set_ddr_check_guard_rails(data["ddr_check_guard_rails"])
            mods.append("chkguard")

        # g_ddr_rx_margin_width
        if "guard_rx_margin_width" in data.keys():
            guardrail = data["guard_rx_margin_width"]
            self.set_ddr_rx_margin_width(guardrail)
            mods.append(f"rxmargw{guardrail}")

        # g_ddr_rx_margin_height
        if "guard_rx_margin_height" in data.keys():
            guardrail = data["guard_rx_margin_height"]
            self.set_ddr_rx_margin_height(guardrail)
            mods.append(f"rxmargh{guardrail}")

        # g_ddr_tx_margin_height
        if "guard_tx_margin_height" in data.keys():
            guardrail = data["guard_tx_margin_height"]
            self.set_ddr_tx_margin_height(guardrail)
            mods.append(f"txmargh{guardrail}")

        # g_ddr_tx_margin_width
        if "guard_tx_margin_width" in data.keys():
            guardrail = data["guard_tx_margin_width"]
            self.set_ddr_tx_margin_width(guardrail)
            mods.append(f"txmargw{guardrail}")
            
        # link_0_cxl_mb_ready_time, link_1_cxl_mb_ready_time
        if "cxl_mb_ready_time" in data.keys():
            setting = data["cxl_mb_ready_time"]
            self.set_cxl_mb_ready_time(setting)
            mods.append(f"cxlmbrdy{setting}")

        # g_bis_lat
        if "bis_lat" in data.keys():
            setting = data["bis_lat"]
            self.set_bis_lat(setting)
            mods.append(f"bislat-{setting}-ns")

        # g_bis_bw
        if "bis_bw" in data.keys():
            setting = data["bis_bw"]
            self.set_bis_bw(setting)
            mods.append(f"bisbw-{setting}-GBps")

        # g_power_gate_mode
        if "power_gate_mode" in data.keys():
            setting = data["power_gate_mode"]
            self.set_power_gate_mode(setting)
            mods.append(f"pwrgate{setting}")

        # g_perf_mode
        if "perf_mode" in data.keys():
            setting = data["perf_mode"]
            self.set_perf_mode(setting)
            mods.append(f"perf{setting}")

        if not self.mods:
            print("No custom config given, exiting.")
            return

        # main cfg edit
        mem = self.update_cfg(self.fw)

        print(f"Edited {self.mods_edited} config(s)")
        print("mods:\n", mods)

        # decide whether to output file
        if self.out:
            if self.out is not True:  # args.out had a specified value
                filename = self.out
            else:
                mods_str = "_".join(mods)
                filename = f"leo_flash.{mods_str}.mem"
        else:
            if not self.query_yes_no(f"-out option not provided - this will overwrite binary {self.fw}. Continue?"):
                return None
            filename = self.fw

        with open(filename, "w") as f:
            line = []
            line_addr = 0
            for ii, dword in enumerate(mem):
                line.append("%02x" % ((dword >> 24) & 0xff))
                line.append("%02x" % ((dword >> 16) & 0xff))
                line.append("%02x" % ((dword >>  8) & 0xff))
                line.append("%02x" % ( dword        & 0xff))
                if 7 == ii % 8 or len(mem) - ii == 1:
                    f.write(f"@%04x %s \n" % (line_addr, " ".join(line)))
                    line = []
                    line_addr = (ii + 1) * 4

        print(f"wrote new image: {filename}")
        return filename

    #
    # SETTER FUNCTIONS
    #

    def set_cxl_link_num(self, num):
        if (num not in [1, 2]):
            return
        self.mods.append((int(self.id_file_data["g_cxl_num_link"]["id"], 16), num))

    def set_cxl_link_0_width(self, width):
        mapping = None
        for item in self.id_file_data["link_0_cxl_link_width"]["mapping"].items():
            if item[0] == str(width):
                mapping = item[1]
                break
        if mapping is None:
            print(f'Ivalid width ({width})')
            return 1
        self.mods.append((int(self.id_file_data["link_0_cxl_link_width"]["id"], 16), mapping))

    def set_cxl_link_1_width(self, width):
        mapping = None
        for item in self.id_file_data["link_1_cxl_link_width"]["mapping"].items():
            if item[0] == str(width):
                mapping = item[1]
                break
        if mapping is None:
            print(f'Ivalid width ({width})')
            return 1
        self.mods.append((int(self.id_file_data["link_1_cxl_link_width"]["id"], 16), mapping))

    def set_speed(self, speed):
        mapping = None
        for item in self.id_file_data["g_ddr_frequency"]["mapping"].items():
            if item[0] == str(speed):
                mapping = item[1]
                break
        if mapping is None:
            print(f'Ivalid speed ({speed})')
            return 1
        self.mods.append((int(self.id_file_data["g_ddr_frequency"]["id"], 16), mapping))

    def set_ddr_2dpc(self):
        self.mods.append((int(self.id_file_data["g_ddr_2dpc"]["id"], 16), 1))

    def set_cxl_compliance(self, setting):
        self.mods.append((int(self.id_file_data["g_cxl_compliance"]["id"], 16), setting))

    def set_sbrefresh(self, setting):
        self.mods.append((int(self.id_file_data["g_ddr_refresh_mode"]["id"], 16), setting))

    def set_ddr_address_interleaving(self, ways):
        mapping = None
        for item in self.id_file_data["g_ddr_interleave_ways"]["mapping"].items():
            if item[0] == str(ways):
                mapping = item[1]
                break
        if mapping is None:
            print(f'set_ddr_address_interleaving: Ivalid input ({ways})')
            return 1
        self.mods.append((int(self.id_file_data["g_ddr_interleave_ways"]["id"], 16), mapping))

    def set_temp_threshold(self, temp_threshold):
        found = False
        for key, value in temp_threshold.items():
            if value is None:
                continue
            found = True
            param = f"g_threshold_temp_{key}"
            self.mods.append((int(self.id_file_data[param]["id"], 16), value))
        return found

    def set_bw_throttle(self, bw_throttle):
        found = False
        for key, value in bw_throttle.items():
            if value is None:
                continue
            found = True
            param = f"g_threshold_bw_{key}"
            self.mods.append((int(self.id_file_data[param]["id"], 16), value))
        return found

    def set_temp_sample_interval(self, temp_sample_interval):
        self.mods.append((int(self.id_file_data["g_temp_sample_interval"]["id"], 16), temp_sample_interval))

    def set_ddr_page_close(self, setting):
        self.mods.append((int(self.id_file_data["g_ddr_page_close_mode"]["id"], 16), setting))

    def set_cxl_temp_threshold(self, cxl_temp_threshold):
        found = False
        for key, value in cxl_temp_threshold.items():
            if value is None:
                continue
            found = True
            param = f"link_0_temp_threshold_{key}"
            self.mods.append((int(self.id_file_data[param]["id"], 16), value))

            param = f"link_1_temp_threshold_{key}"
            self.mods.append((int(self.id_file_data[param]["id"], 16), value))
        return found

    def set_cxl_correctable_err_threshold(self, correctable_err_threshold):
        self.mods.append((int(self.id_file_data["link_0_correctable_err_threshold"]["id"], 16), correctable_err_threshold))
        self.mods.append((int(self.id_file_data["link_1_correctable_err_threshold"]["id"], 16), correctable_err_threshold))

    def set_cxl_mem_size(self, cxl_mem_size):
        self.mods.append((int(self.id_file_data["g_cxl_mem_size"]["id"], 16), cxl_mem_size))

    def set_aes_mode(self, aes_mode):
        self.mods.append((int(self.id_file_data["g_aes_mode"]["id"], 16), aes_mode))

    def set_ddr_check_guard_rails(self, setting):
        self.mods.append((int(self.id_file_data["g_ddr_check_guard_rails"]["id"], 16), setting))

    def set_ddr_rx_margin_width(self, guardrail):
        self.mods.append((int(self.id_file_data["g_ddr_rx_margin_width"]["id"], 16), guardrail))

    def set_ddr_tx_margin_width(self, guardrail):
        self.mods.append((int(self.id_file_data["g_ddr_tx_margin_width"]["id"], 16), guardrail))

    def set_ddr_rx_margin_height(self, guardrail):
        self.mods.append((int(self.id_file_data["g_ddr_rx_margin_height"]["id"], 16), guardrail))

    def set_ddr_tx_margin_height(self, guardrail):
        self.mods.append((int(self.id_file_data["g_ddr_tx_margin_height"]["id"], 16), guardrail))

    def set_pmic_current_mode(self, mode):
        if mode not in [2, 3]:
            print("invalid value for pmic_current_mode")
            return 0
        self.mods.append((int(self.id_file_data["g_pmic_current_mode"]["id"], 16), mode))
        return mode

    def set_cxl_mb_ready_time(self, setting):
        self.mods.append((int(self.id_file_data["link_0_cxl_mb_ready_time"]["id"], 16), setting))
        self.mods.append((int(self.id_file_data["link_1_cxl_mb_ready_time"]["id"], 16), setting))

    def set_bis_lat(self, setting):
        self.mods.append((int(self.id_file_data["g_bis_lat"]["id"], 16), setting))

    def set_bis_bw(self, setting):
        self.mods.append((int(self.id_file_data["g_bis_bw"]["id"], 16), setting))

    def set_power_gate_mode(self, setting):
        self.mods.append((int(self.id_file_data["g_power_gate_mode"]["id"], 16), setting))

    def set_perf_mode(self, setting):
        self.mods.append((int(self.id_file_data["g_perf_mode"]["id"], 16), setting))

    # Print build information
    def print_build_info(self, mem):
        build_base_addr = (0x24 >> 2)
        asic_version = mem[build_base_addr + 6]
        buildid = mem[build_base_addr + 8]
        builder = mem[build_base_addr + 9]
        fw_version_data = mem[build_base_addr + 10]
        iterations = mem[build_base_addr + 11]

        if 0 == asic_version:
            asic_version = 0xd5

        try:
            builder_str = builder.to_bytes(4, 'big').decode('utf-8')
        except UnicodeDecodeError:
            builder_str = "Unknown"
    
        fw_version_rel = fw_version_data >> 24
        fw_version_major = (fw_version_data >> 12) & 0xfff
        fw_version_minor = (fw_version_data >> 0) & 0xfff
        release_str = "Release" if 0 != fw_version_rel else "Dev"
        sysconfig_version = "Original" if 0 == iterations else str(iterations)

        print()
        print(f"FW Version: %x.%x %s" % (fw_version_major, int(str(fw_version_minor), 16), release_str))
        print(f"BuildId: %d" % buildid)
        print(f"Builder: %s" % builder_str)
        print(f"Leo ASIC Version: %x" % asic_version)
        print(f"Sysconfig Version %s" % sysconfig_version)
        print()

    # Using the original .mem file, update only the sysconfig block with provided config
    def update_cfg(self, filename):
        # read binary and find sys config block
        mem = self.read_mem_binary(filename)
        cfg_block_data, saddr, eaddr = self.find_syscfg_block(mem)

        # increment number of changes
        build_base_addr = (0x24 >> 2)
        mem[build_base_addr + 11] += 1

        self.dump_cfg(short=False)

        # append config option if not found in existing config block
        if len(self.mods) != 0:
            for _, cfg_mod in enumerate(self.mods):
                pos = len(cfg_block_data) - 3
                k, v = cfg_mod
                cfg_block_data.insert(pos, k)
                cfg_block_data.insert(pos + 1, v)
                # 2 dwords
                eaddr += 2

                # increment block size by 8 bytes
                cfg_block_data[4] += 8
                cfg_block_data[5] += 8
                self.mods_edited += 1
                # print(f"add cfg id {hex(k)}: {hex(v)}")

        # calc new crc
        crc = 0
        for _, dw in enumerate(cfg_block_data[2:-3]):
            crc = self.calc_crc32(dw, crc)
        cfg_block_data[-3] = crc

        # update mem with updated and appended sysconfig values
        for addr in range(saddr, eaddr + 3 + 1):
            mem[addr - 3] = cfg_block_data[addr - saddr]

        return mem

    # Map reg name to its display name and value
    def match_id_to_name(self, id, value):
        for name, reg in self.id_file_data.items():
            if type(reg) == str:
                continue
            if id == int(reg['id'], 16):
                reg_display = name
                reg_value = value

                if 'display' in reg.keys():
                    reg_display = reg['display']

                if 'mapping' in reg.keys():
                    found = False
                    for k, v in reg['mapping'].items():
                        if v == value:
                            found = True
                            reg_value = int(k)
                            break
                    if not found:
                        raise KeyError("match_id_to_name: mapping parse error")
                return (reg_display, reg_value, name)
        return (None, None, None)

    # Print all fields configured in a .mem file
    def dump_cfg(self, short):
        mem = self.read_mem_binary(self.fw)
        cfg_block_data, _, _  = self.find_syscfg_block(mem)

        self.print_build_info(mem)

        # remove header, crc, and trailer
        pairs = cfg_block_data[self.header_dw_count:]
        pairs = pairs[:-3]

        # split list into halves
        keys = pairs[::2]
        vals = pairs[1::2]

        # each key should have a corresponding value
        if len(keys) != len(vals):
            raise IndexError("dump_cfg: leo_flash.mem was unable to be parsed during syscfg dump")

        dump_list = []
        print(f"%08s %10s    %s" % ("value(hex)", "value(dec)", "reg name"))
        print("------------------------------------------------------")

        for ii in range(len(keys)):
            match = self.match_id_to_name(keys[ii], vals[ii])

            if None not in match:
                reg_display, value, reg_name = match
                # if item in shortlist, insert to the beginning of dump list
                if [item for item in self.shortlist if item[0] == reg_name]:
                    dump_list.insert(0, (reg_display, value, reg_name))
                    continue
                if not short:
                    dump_list.append((reg_display, value, reg_name))

        unconfigured = ""
        # always dump
        for (reg_name, default_value, _) in self.shortlist:
            if reg_name not in [x[2] for x in dump_list]:
                unconfigured = "\n * Unconfigured. Using default value"
                print(f"0x%08x   %8d    %s*" % (default_value, default_value, self.id_file_data[reg_name]['display']))

        # main dump
        for reg_display, value, reg_name in dump_list:
            print(f"0x%08x   %8d    %s" % (value, value, reg_display))

        print(unconfigured)


    def line_dwords(self, line_data):
        dwords = []
        tmp = []
        for i, byte in enumerate(line_data):
            tmp.append(byte)
            if 3 == (i % 4):
                dwords.append(int(reduce(lambda a,b: a+b, tmp), 16))
                tmp = []
        return dwords
        
    def match_mods(self, key):
        for ii, cfg_mod in enumerate(self.mods):
            if cfg_mod[0] == key:
                val = cfg_mod[1]
                del self.mods[ii]
                return val
        return None

    def read_mem_binary(self, filename):
        mem = []
        with open(filename) as f:
            for line in f:
                line_data = line.split(" ")
                line_data = line_data[1:]  # remove addr token
                if line_data[-1] == '\n':
                    line_data.pop()  # pop \n character
                dwords = self.line_dwords(line_data)
                mem.extend(dwords)
        return mem

    def find_syscfg_block(self, mem):
        in_buf = []
        header = [0x5aa55aa5, 0x5aa55aa5, 0x00000404]
        trailer = [0xaa55aa55, 0xaa55aa55]
        cnt = 0
        saddr = 0xffffff
        eaddr = 0x000000
        found = False
        cfg_block_data = []

        for ii, dw in enumerate(mem):
            # find header + block type
            if in_buf == header:
                saddr = ii
                found = True
                cfg_block_data.extend(in_buf)
                # print(hex((saddr-3) * 4), "block found")
                in_buf = []
                cnt = 3

            if not found:
                in_buf.append(dw)
                if len(in_buf) > 3:
                    in_buf.pop(0)
                continue

            cfg_block_data.append(dw)

            # skip until start of dictionary
            if cnt < self.header_dw_count:
                cnt += 1
                continue

            # stop at trailer
            if dw == trailer[1] and in_buf[-1] == trailer[0]:
                eaddr = ii
                # print(hex(eaddr * 4 + 3), "block end, size ", hex( eaddr - saddr ))
                break

            # accumulate key value pairs
            if len(in_buf) < 2:
                in_buf.append(dw)
                continue

            # set new value for cfg key
            v = self.match_mods(in_buf[0])
            if v is not None:
                cfg_len = len(cfg_block_data)
                cfg_block_data[cfg_len-2] = v
                self.mods_edited += 1
                # print(f"edit cfg id {hex(cfg_block_data[cfg_len-3])}: {hex(cfg_block_data[cfg_len-2])}")

            in_buf = []
            in_buf.append(dw)
        return (cfg_block_data, saddr, eaddr)

    def calc_crc32(self, data, crc):
        idx = (crc ^ data) & 0xff
        crc = self.crc32Table[idx] ^ (crc >> 8)

        idx = (crc ^ (data >> 8)) & 0xff
        crc = self.crc32Table[idx] ^ (crc >> 8)

        idx = (crc ^ (data >> 16)) & 0xff
        crc = self.crc32Table[idx] ^ (crc >> 8)

        idx = (crc ^ (data >> 24)) & 0xff
        crc = self.crc32Table[idx] ^ (crc >> 8)

        # print("calc_crc: data: 0x%x, crc: 0x%0x, crc_in: 0x%x, idx: 0x%x, tbl: 0x%x" % (data, crc, crc_cpy, idx, self.crc32Table [idx]))
        return crc

    def query_yes_no(self, question, default="yes"):
        """Ask a yes/no question via raw_input() and return their answer.

        "question" is a string that is presented to the user.
        "default" is the presumed answer if the user just hits <Enter>.
                It must be "yes" (the default), "no" or None (meaning
                an answer is required of the user).

        The "answer" return value is True for "yes" or False for "no".
        """
        valid = {"yes": True, "y": True, "ye": True, "no": False, "n": False}
        if default is None:
            prompt = " [y/n] "
        elif default == "yes":
            prompt = " [Y/n] "
        elif default == "no":
            prompt = " [y/N] "
        else:
            raise ValueError("invalid default answer: '%s'" % default)

        while True:
            sys.stdout.write(question + prompt)
            choice = input().lower()
            if default is not None and choice == "":
                return valid[default]
            elif choice in valid:
                return valid[choice]
            else:
                sys.stdout.write("Please respond with 'yes' or 'no' " "(or 'y' or 'n').\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Script to alter system configuration for Leo flash image')
    parser.add_argument('fw', action='store', help='Full path to FW image')
    parser.add_argument('-cfg', action='store', help='Full path to pre-boot options config file')
    parser.add_argument('-out', action='store', nargs='?', const=True, help='Path to output .mem binary, or auto-generate a name if no argument is provided')
    parser.add_argument('-dump', action='store_true', help='Dump values of configured options on target flash binary')
    parser.add_argument('-dumps', action='store_true', help='Dump short list of configured options on target flash binary')
    args = parser.parse_args()

    scm = SysconfigConfigManager(args.fw, args.cfg, args.out)

    if (args.dump):
        scm.dump_cfg(short=False)
    elif (args.dumps):
        scm.dump_cfg(short=True)
    else:
        scm.edit_cfg()
