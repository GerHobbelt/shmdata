#!/usr/bin/env python3

# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation; either version 2.1
# of the License, or (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.

import sys
import time
import pyshmdata
import assert_exit_1
import argparse
from sys import argv

parser = argparse.ArgumentParser()
parser.add_argument("--crash", help="causes the reader to segfault in the middle of its read callback", action="store_true")
parser.add_argument("--timeout", type=int, help="time the reader waits before exiting. Put -1 for indefinite waiting", default=4)
args = parser.parse_args()

success = False

def please_segfault():
    """ try to access illegal memory using ctypes in order to cause a segfault.
    Thanks stackoverflow :
    https://codegolf.stackexchange.com/a/22383
    probably doesn't work on windows according to the link.
    """
    import ctypes;ctypes.string_at(1)

def cb(user_data, buffer, datatype, parsed_datatype):
    global success
    if args.crash:
        please_segfault()
    data = buffer.decode(encoding="utf-8")
    print(user_data, data, parsed_datatype)
    # we have been notified, success !
    success = True


data = ['all', 'your', 'base']

reader = pyshmdata.Reader(path="/tmp/some_shmdata", callback=cb, user_data=data)

start_time = time.monotonic()

while (not success and (args.timeout == -1 or time.monotonic() < start_time + args.timeout)):
    pass

reader = None
if not success:
    exit(1)
exit(0)
