#!/usr/bin/env python3

import contextlib
import random
import re
import subprocess
from http.client import HTTPConnection


class KillOnExit(subprocess.Popen):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def __exit__(self, *args):
        self.kill()
        super().__exit__(*args)


NullContext = contextlib.suppress


EXIT_SUCCESS = 0
EXIT_FAILURE = 1


def main():
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('--port', default=4711, type=int)
    exec_group = parser.add_mutually_exclusive_group(required=True)
    exec_group.add_argument('executable', nargs='?', help="The server to run")
    exec_group.add_argument('--no-spawn', dest='spawn', default=True, action='store_false', help="Do not spawn the server")
    args = parser.parse_args()

    with KillOnExit([args.executable, str(args.port)]) if args.spawn else NullContext():
        conn = HTTPConnection('localhost', args.port, timeout=2)
        conn.connect()

        path = f'/dynamic/{random.randbytes(8).hex()}'
        content = random.randbytes(32).hex().encode()

        conn.request('GET', path)
        response = conn.getresponse()
        payload = response.read()
        if response.status != 404:
            print(f"'{path}' should be missing, but GET was not answered with '404'")
            return EXIT_FAILURE

        conn.request('PUT', path, content)
        response = conn.getresponse()
        payload = response.read()
        if response.status not in {200, 201, 202, 204}:
            print(f"Creation of '{path}' did not yield '201'")
            return EXIT_FAILURE

        conn.request('GET', path)
        response = conn.getresponse()
        payload = response.read()
        if response.status != 200 or payload != content:
            print(f"Content of '{path}' does not match what was passed")
            return EXIT_FAILURE

        conn.request('DELETE', path, content)
        response = conn.getresponse()
        payload = response.read()
        if response.status not in {200, 202, 204}:
            print(f"Deletion of '{path}' did not succeed")
            return EXIT_FAILURE

        conn.request('GET', path)
        response = conn.getresponse()
        payload = response.read()
        if response.status != 404:
            print(f"'{path}' should be missing")
            return EXIT_FAILURE

        return EXIT_SUCCESS


if __name__ == '__main__':
    import sys
    sys.exit(main())
