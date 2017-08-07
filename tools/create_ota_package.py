#!/usr/bin/env python3

"""
@file create_ota_application.py
@brief Create updateable zip file for updating firmware
@author Ran Bao (ran.bao@wirelessguard.co.nz)
@date March, 2017
"""

import os
import zipfile
import json
import subprocess
import hashlib
import argparse

MANIFEST_MAJOR = 1
MANIFEST_MINIOR = 0
MANIFEST_PATCH = 0
MANIFEST_REVISION = 0

class Version:
    major = "major"
    minor = "minor"
    patch = "patch"
    revision = "revision"

    def __init__(self, major, minor, patch, revision):
        self.properties = {}
        self.properties[self.major] = major
        self.properties[self.minor] = minor
        self.properties[self.patch] = patch
        self.properties[self.revision] = revision

    def dump(self):
        return self.properties


class BinaryExecutable:
    description = "description"
    file = "file"
    fingerprint = "fingerprint"
    base = "base"
    version = "version"
    size = "size"

    def __init__(self, description, file, size, fingerprint, base, version):
        self.properties = {}
        self.properties[self.description] = description
        self.properties[self.file] = file
        self.properties[self.size] = size
        self.properties[self.fingerprint] = fingerprint
        self.properties[self.base] = base
        self.properties[self.version] = version.dump()

    def dump(self):
        return self.properties


class Manifest:
    description = "description"
    file = "file"
    fingerprint = "fingerprint"
    version = "version"
    contains = "contains"
    reboot_to_addr = "reboot_to_addr"

    def __init__(self, description, file, fingerprint, reboot_to_addr, contains):
        self.properties = {}
        self.properties[self.description] = description
        self.properties[self.file] = file
        self.properties[self.fingerprint] = fingerprint
        self.properties[self.reboot_to_addr] = reboot_to_addr
        self.properties[self.version] = Version(MANIFEST_MAJOR, MANIFEST_MINIOR, MANIFEST_PATCH, MANIFEST_REVISION).dump()
        self.properties[self.contains] = [b.dump() for b in contains]

    def dump(self):
        return self.properties


def create_zip_package(description, file, output_dir, size, fingerprint, base, reboot_to_addr, version):
    # create binary information
    binary = BinaryExecutable(description, file.split("/")[-1], size, fingerprint, base, version)

    # create manifest
    manifest = Manifest("Hatch Firmware", "manifest.json", "", reboot_to_addr, [binary])

    # create zip
    zip = zipfile.ZipFile("{}/update_{}.zip".format(output_dir, description.lower()), "w", zipfile.ZIP_DEFLATED)

    zip.writestr("manifest.json", json.dumps(manifest.dump(), indent=4))
    zip.write(file, file.split("/")[-1])

    zip.close()


def main(description, file, output_dir, base, reboot_to_addr):
    repo_version = subprocess.check_output(["git", "describe", "--always", "--long"]).decode("utf-8").strip()

    # get major, minor, patch, revision from version string
    part = repo_version[1:].split(".")

    if (len(part) != 3):
        exit(-1)

    major = part[0]
    minor = part[1]
    patch = part[2].split('-')[0]
    revision = part[2].split('-')[1]

    version = Version(major, minor, patch, revision)

    verfile = open(output_dir + "/version.txt","w")
    verfile.write(repo_version+"\n")
    verfile.close()

    # open binary
    fp = open(file, "rb")
    binary = fp.read()
    fp.close()

    # calculate md5 hash of binary file
    m = hashlib.md5()
    m.update(binary)
    md5string = m.hexdigest()

    # get total size for the code
    size = len(binary)

    create_zip_package(description, file, output_dir, size, md5string, base, reboot_to_addr, version)



if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument("-d", "--description", nargs=1, type=str,
        help="description of binary file")

    parser.add_argument("-f", "--file", nargs=1, type=str,
        help="file to be included")

    parser.add_argument("-o", "--output", nargs=1, type=str,
        help="output directory")

    parser.add_argument("-b", "--base", nargs=1, type=int,
        help="base address of the firmware")

    parser.add_argument("-r", "--reboot_to_addr", nargs=1, type=int)

    args = parser.parse_args()

    main(args.description[0], args.file[0], args.output[0], args.base[0], args.reboot_to_addr[0])


