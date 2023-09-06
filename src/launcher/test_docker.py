#!/usr/bin/env python

import sys
import docker

def main() -> int:
    client = docker.from_env()
    client.containers.run("ubuntu", "echo hello world")
    return 0

if __name__ == '__main__':
    sys.exit(main())
