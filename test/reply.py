#!/usr/bin/env python3

import contextlib
import subprocess
import socket


class KillOnExit(subprocess.Popen):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def __exit__(self, *args):
        self.kill()
        super().__exit__(*args)


NullContext = contextlib.suppress


EXIT_SUCCESS = 0
EXIT_FAILURE = 1
buffer_size = 1024


def main():
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('--port', default=4711, type=int)
    exec_group = parser.add_mutually_exclusive_group(required=True)
    exec_group.add_argument('executable', nargs='?', help="The server to run")
    exec_group.add_argument('--no-spawn', dest='spawn', default=True, action='store_false', help="Do not spawn the server")
    args = parser.parse_args()

    with KillOnExit([args.executable, str(args.port)]) if args.spawn else NullContext():
        conn = socket.create_connection(('localhost', args.port), timeout=2)
        conn.send('Request\r\n\r\n'.encode())
        reply = conn.recv(1024)
        if (len(reply)):    # SELF CREATED
            print(" Reply: SUCCESS")    # SELF CREATED
        else:
            print(" Reply: FAILED")
        return EXIT_SUCCESS if len(reply) else EXIT_FAILURE


if __name__ == '__main__':
    import sys
    sys.exit(main())
