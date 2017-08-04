#!/usr/bin/env python3

'''
@file firmwarepackage.py
@brief Load and unpack firmware package
@author Ran Bao (ran.bao@wirelessguard.co.nz)
@date May, 2017

'''

import zipfile
import json
from prettytable import PrettyTable # pip install prettytable
from lib.bootloader import Bootloader

class FirmwarePackage:
    table = PrettyTable(["Description", "Version", "Partition", "MD5 Checksum"])

    def __init__(self, path, block_size=2**7):
        # open update package
        with zipfile.ZipFile(path, 'r') as zip:
            # read package information from manifest.json
            # File manifest.json contains following information as an example
            #
            # @file manifest.json
            # {
            #     "description": "Hatch Firmware",
            #     "file": "manifest.json",
            #     "fingerprint": "",
            #     "reboot_to_region": 0,
            #     "version": {
            #         "major": "0",
            #         "minor": "1",
            #         "patch": "1",
            #         "revision": "376"
            #     },
            #     "contains": [
            #         {
            #             "description": "Hatch_FW_A",
            #             "file": "WG_FIRMWARE_REGION_A.bin",
            #             "fingerprint": "609d826bf8f59cec0f3d108b6e21b577",
            #             "region": 0,
            #             "version": {
            #                 "major": "0",
            #                 "minor": "1",
            #                 "patch": "1",
            #                 "revision": "376"
            #             }
            #         }
            #     ]
            # }
            with zip.open("manifest.json", 'r') as manifest_file:
                self.program_list = []
                manifest = json.loads(manifest_file.read().decode('utf-8'))

                self.reboot_to_region = manifest["reboot_to_region"]

                for file in manifest["contains"]:
                    version_dict = file["version"]
                    version_string = "{}.{}.{}-{}".format(version_dict["major"], version_dict["minor"], version_dict["patch"], version_dict["revision"])
                    description_string = file["description"]
                    partition = int(file["region"])
                    partition_string = Bootloader.region_lookup_table[partition]
                    file_name = file["file"]
                    fingerprint_string = file["fingerprint"]

                    # read binary
                    with zip.open(file_name, 'r') as binary:
                        program = []
                        # read program
                        while True:
                            p = binary.read(block_size)
                            if len(p) == 0:
                                break
                            program.append(p)

                    # add program to program list
                    self.program_list.append((partition, program))

                    # print summary
                    self.table.add_row([description_string, version_string, partition_string, fingerprint_string])

    def __repr__(self):
        return str(self)

    def __str__(self):
        return self.table.__str__()

    def __getitem__(self, item):
        return (self.program_list[item][0], self.program_list[item][1])



if __name__ == "__main__":
    a = FirmwarePackage("../../debug/update_hatch_fw_a.zip")

    for partition, program in a:
        print(partition, program)
