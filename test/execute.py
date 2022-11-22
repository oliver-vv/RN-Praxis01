#!/usr/bin/env python3

import os


EXIT_SUCCESS = 0
EXIT_FAILURE = 1


def main():
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('executable')
    args = parser.parse_args()

    return EXIT_SUCCESS if os.access(args.executable, os.X_OK) else EXIT_FAILURE


if __name__ == '__main__':
    import sys
    sys.exit(main())
