#!/usr/bin/env python3

# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation; either version 2.1
# of the License, or (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.

import time
import pyshmdata
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--timeout", type=int, help="time the reader waits before exiting. Put -1 for indefinite waiting", default=4)
args = parser.parse_args()

data = ['all', 'your', 'base']

# reader = pyshmdata.Reader(path="/tmp/some_shmdata", callback=cb, user_data=data)
writer = pyshmdata.Writer(path="/tmp/some_shmdata", datatype="application/x-raw,fun=yes")

start_time = time.monotonic()
while (args.timeout == -1 or time.monotonic() < start_time + args.timeout):
    writer.push(buffer=bytearray("are belong to us", encoding="utf-8"))

reader = None
writer = None
exit(0)
