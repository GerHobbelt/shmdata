#! /bin/bash

# starts a writer without a timeout in a separate process
python3 test_writer.py --timeout -1 &
# save its PID to kill it later
WRITER_PID=$!
# connect a reader to it but make the writer segfault in the middle of its read
python3 test_reader.py --crash

# spawn another reader and try to read again
# the default timeout is 4 seconds which should be more than sufficient.
python3 test_reader.py --timeout 3
# sleep 3
# a crash in the reader shouldn't make the writer stop writing
RET=$?

# kill the writer
kill $WRITER_PID
# cleanup dangling shmdata
rm /tmp/some_shmdata

if [ $RET -ne 0 ]; then
    echo "Second reader could not read data from the writer." 1>&2
fi


exit $RET
