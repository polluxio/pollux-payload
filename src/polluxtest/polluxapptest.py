#!/usr/bin/env python3

import argparse
import logging
import os
import shutil
import sys

def run(partitions, commandLine):
  dirName = 'polluxtest'
  #create polluxtest dir, erase it first if it exists
  if os.path.exists(dirName):
    shutil.rmtree(dirName)
  os.mkdir(dirName)

  topYamlName = 'pollux.yaml'
  topYamlPath = os.path.join(dirName, topYamlName)
  topYaml = open(topYamlPath, 'w')
  topYaml.write('subdirs:\n')

  for i in range(partitions):
    #create dir for this partition
    partName = 'part' + str(i)
    topYaml.write('  - part' + str(i) + '\n')
    partPath = os.path.join(dirName, partName)
    os.mkdir(partPath)
    partYamlName = 'pollux.yaml'
    partYamlPath = os.path.join(partPath, partYamlName)
    partYaml = open(partYamlPath, 'w')
    partYaml.write('id: ' + str(i) + '\n')
    partYaml.write('name: p' + str(i) + '\n')
    partYaml.write('payload:\n')
    partYaml.write('  command: ' + commandLine + '\n')
    partYaml.write('  args: [--partitions, ' + str(partitions) + ']\n')

class ArgumentParser(argparse.ArgumentParser):
  def error(self, message):
    #errorStr = ""
    #self.print_help(errorStr)
    #logging.error(errorStr)
    logging.error(self.prog + ': ' + message)
    self.exit(2, '%s: error: %s\n' % (self.prog, message))

def main() -> int:
  parser = ArgumentParser(prog='polluxapptest', exit_on_error=False)
  parser.add_argument("--partitions", required=True, help="number of partitions")
  parser.add_argument("--command", required=True, help="payload command line")

  args = parser.parse_args()

  logName = 'polluxapptest.log'
  logging.basicConfig(
            filename=logName, filemode='w',
            level=logging.DEBUG,
            format='%(asctime)s %(levelname)-8s %(message)s',
            datefmt='%Y-%m-%d %H:%M:%S')

  logging.info("########################################################")
  logging.info(logName)
  logging.info("########################################################")

  logging.info("command line arguments:" + ''.join(" " + s  for s in sys.argv[1:]))

  run(int(args.partitions), args.command)
  logging.info("polluxapptest terminated")
  return 0

if __name__ == '__main__':
  sys.exit(main())
