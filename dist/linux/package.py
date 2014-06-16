#!/usr/bin/env python3

import os
import shutil
import sys


if len(sys.argv) > 1: package_name = sys.argv[1]
else:                 package_name = "amanda_3.0-1"

def main():
    # Define some directory constants.
    LINUX_DIR   = os.path.dirname(os.path.abspath(__file__))
    PACKAGE_DIR = os.path.join(LINUX_DIR, package_name)
    BIN_DIR     = os.path.abspath(os.path.join(LINUX_DIR, "..", "..", "bin"))

    # Clean the package tree if it exists.
    try:    shutil.rmtree(os.path.join(PACKAGE_DIR, "usr"))
    except: pass

    # Make the package tree.
    os.makedirs(os.path.join(PACKAGE_DIR, "usr", "bin"))
    os.makedirs(os.path.join(PACKAGE_DIR, "usr", "lib"))

    # Copy the files to the correct locations.
    shutil.copyfile(
        os.path.join(BIN_DIR, "amanda"),
        os.path.join(PACKAGE_DIR, "usr", "bin", "amanda"))
    shutil.copyfile(
        os.path.join(BIN_DIR, "libamanda.so"),
        os.path.join(PACKAGE_DIR, "usr", "lib", "libamanda.so"))
    shutil.copyfile(
        os.path.join(BIN_DIR, "amanda.ini"),
        os.path.join(PACKAGE_DIR, "usr", "lib", "amanda.ini"))

    # Sort out permissions
    os.system("chown -R root:root {}".format(os.path.join(PACKAGE_DIR, "usr")))
    os.system("chmod a+x {}".format(os.path.join(
        PACKAGE_DIR,
        "usr", "bin", "amanda")))

    # Build the .deb package
    os.system("dpkg -b {}".format(PACKAGE_DIR))

if __name__ == "__main__":
    main()
