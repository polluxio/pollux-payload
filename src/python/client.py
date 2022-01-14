#!/usr/bin/env python3

import sys
import argparse
import logging
import concurrent
import socket
import threading
import time

import grpc
import escher_payload_pb2
import escher_payload_pb2_grpc

timeOut = 10
class EscherPayloadServicer(escher_payload_pb2_grpc.EscherPayloadServicer):
  def Terminate(self, request, context):
    logging.info("Received terminate")
    return escher_payload_pb2.PayloadTerminateResponse()

def sendPayloadDone(stub):
  logging.info("-------------- sendPayloadDone --------------")
  inactiveMessage = escher_payload_pb2.PayloadInactiveMessage(info="I'm hungry !!")
  try:
    response = stub.PayloadInactive(inactiveMessage, timeout=timeOut)
  except grpc.RpcError as rpc_error:
    logging.error('grpc unavailable error: %s', rpc_error)
  logging.info("Received: " + str(response))

def mainLoop(stub):
  logging.info("-------------- Starting main loop --------------")
  #wait for 20 secs
  time.sleep(20)
  #send to master that I'm done
  sendPayloadDone(stub)

def sendPayloadReady(stub, localPort):
  logging.info("-------------- sendPayloadReady --------------")
  readyMessage = escher_payload_pb2.PayloadReadyMessage(info="I'm alive", port=localPort);
  response = stub.PayloadReady(readyMessage)
  logging.info("Sign of life: " + str(response))
  
def run(masterJobPort):
  address = 'localhost:' + masterJobPort
  sock = socket.socket()
  sock.bind(('', 0))
  localPort = sock.getsockname()[1]
  server = grpc.server(concurrent.futures.ThreadPoolExecutor(max_workers=10))
  escher_payload_pb2_grpc.add_EscherPayloadServicer_to_server(EscherPayloadServicer(), server)
  server.add_insecure_port('[::]:' + str(localPort))
  server.start()
  with grpc.insecure_channel(address) as channel:
    stub = escher_payload_pb2_grpc.EscherJobStub(channel)
    sendPayloadReady(stub, localPort)
    with concurrent.futures.ThreadPoolExecutor(max_workers=1) as executor:
      future = executor.submit(mainLoop, stub)
  
  main = threading.Thread(target=mainLoop, args=(address))
  main.start()
  

  server.wait_for_termination()

class ArgumentParser(argparse.ArgumentParser):
  def error(self, message):
    #errorStr = ""
    #self.print_help(errorStr)
    #logging.error(errorStr)
    logging.error(self.prog + ': ' + message)
    self.exit(2, '%s: error: %s\n' % (self.prog, message))

if __name__ == '__main__':
  logging.basicConfig(
            filename='escher_payload.log', filemode='w',
            level=logging.DEBUG,
            format='%(asctime)s %(levelname)-8s %(message)s',
            datefmt='%Y-%m-%d %H:%M:%S')

  logging.info("########################################################")

  parser = ArgumentParser(prog='escher_payload', exit_on_error=False)
  parser.add_argument("--port", required=True, help="escher port")
  parser.add_argument("--id", default=-10, help="escher payload id")

  args = parser.parse_args()

  argStr = "uninitialized"
  if args.id != -1:
    argStr = str(args.id)
  logging.info("escher payload id: " + argStr)
  logging.info("########################################################")

  logging.info("command line arguments:" + ''.join(" " + s  for s in sys.argv[1:]))
  run(args.port)
