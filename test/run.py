from subprocess import Popen
import sys

returncode = 0

with Popen(["test/test"]) as server:
    with Popen([sys.argv[1]]) as runner:
        runner.wait()
        returncode = runner.returncode
    server.terminate()

sys.exit(returncode)
