#!/usr/bin/env python3

# Place this file in the folder that holds this repository.
# Use `git clone https://github.com/Rubykuby/amanda-resources` to clone all
# other Amanda projects.
# Edit the res_dir and caps variables accordingly.

import os

ama_dir = os.path.join("amanda", "src")
res_dir = os.path.join("amanda-resources", "Amanda205", "Amalib")
caps = True

def main():
    print("ama_dir: {}".format(ama_dir))
    print("res_dir: {}".format(res_dir))

    not_in_ama_dir = []
    not_in_res_dir = []

    # Makes a list (files) of all files in ama_dir.
    for root, _, files in os.walk(ama_dir):
        # Loops through the list of files
        for f in files:
            # Capitalises first letter of target file if that is the case.
            if caps: target_f = "{}{}".format(f[0].upper(), f[1:])
            else:    target_f = f

            # Tests if file actually exists in res_dir
            if not os.path.isfile(os.path.join(res_dir, target_f)):
                not_in_res_dir.append(target_f)
            else:
                cmd = "diff {} {}".format(os.path.join(ama_dir, f),
                                            os.path.join(res_dir, target_f))

                print("$ {}".format(cmd))
                os.system(cmd)

    for root, _, files in os.walk(res_dir):
        for f in files:
            if caps: target_f = "{}{}".format(f[0].lower(), f[1:])
            else:    target_f = f

            if not os.path.isfile(os.path.join(ama_dir, target_f)):
                not_in_ama_dir.append(target_f)

    print()
    print("The following files from {} do not exist in {}:".format(res_dir,
                                                                  ama_dir))
    for f in not_in_ama_dir:
        print(f)
    print()
    print("The following files from {} do not exist in {}:".format(ama_dir,
                                                                  res_dir))
    for f in not_in_res_dir:
        print(f)

if __name__ == "__main__":
    main()

