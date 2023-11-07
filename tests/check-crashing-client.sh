#! /bin/bash

# starts a writer without a timeout in a separate process
python ../wrappers/python/example_writer.py --timeout -1 &
# save its PID to kill it later
WRITER_PID=$!
# connect a reader to it but make the writer segfault in the middle of its read
python ../wrappers/python/example_reader.py --crash

# spawn another reader and try to read again
# the default timeout is 4 seconds which should be more than sufficient.
python ../wrappers/python/example_reader.py

# a crash in the reader shouldn't make the writer stop writing
RET=$?

# kill the writer
kill $WRITER_PID

exit $RET
