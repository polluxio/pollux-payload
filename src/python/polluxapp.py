#!/usr/bin/env python3

import sys
import argparse
import logging
import concurrent
import socket
import threading
import time
import random

import grpc
import pollux_pb2
import pollux_payload_pb2
import pollux_payload_pb2_grpc

timeOut = 10
class PolluxPayloadServicer(pollux_payload_pb2_grpc.PolluxPayloadServicer):
  def __init__(self, terminate_event):
    self.terminate_event = terminate_event
    
  def Terminate(self, request, context):
    logging.info("Received terminate")
    self.terminate_event.set()
    return pollux_payload_pb2.PayloadTerminateResponse()

  def PolluxCommunication(self, request, context):
    logging.info("Pollux Message received: origin={}, destinations={}, key={}, value={}".format(request.origin, request.destinations, request.key, request.value))
    return pollux_pb2.PolluxMessageResponse()

def sendPayloadInactive(stub):
  logging.info("-------------- sendPayloadInactive --------------")
  inactiveMessage = pollux_payload_pb2.PayloadInactiveMessage(info="I'm hungry !!")
  try:
    response = stub.PayloadInactive(inactiveMessage, timeout=timeOut)
  except grpc.RpcError as rpc_error:
    logging.error('grpc unavailable error: %s', rpc_error)
  logging.info("Received: " + str(response))

def sendPolluxCommunication(stub, local_id, other_id):
  logging.info("-------------- sendPolluxCommunication --------------")
  logging.info("Send message from {} to {}".format(local_id, other_id))
  polluxMessage = pollux_pb2.PolluxMessage(origin=local_id, destinations=[other_id], key="hello", value="hello")
  try:
    response = stub.PolluxCommunication(polluxMessage, timeout=timeOut)
  except grpc.RpcError as rpc_error:
    logging.error('grpc unavailable error: %s', rpc_error)
  logging.info("Send response: " + str(response))

def mainLoop(stub, local_id, other_ids):
  logging.info("-------------- Starting main loop --------------")
  #wait for 20 secs
  time.sleep(5)
  other_id = random.choice(other_ids)
  sendPolluxCommunication(stub, local_id, other_id)
  time.sleep(5)
  other_id = random.choice(other_ids)
  sendPolluxCommunication(stub, local_id, other_id)
  time.sleep(5)
  other_id = random.choice(other_ids)
  sendPolluxCommunication(stub, local_id, other_id)
  time.sleep(5)
  #send to master that I'm done
  sendPayloadInactive(stub)

def sendPayloadReady(stub, localPort):
  logging.info("-------------- sendPayloadReady --------------")
  readyMessage = pollux_payload_pb2.PayloadReadyMessage(info="I'm alive", port=localPort);
  response = stub.PayloadReady(readyMessage)
  logging.info("Sign of life: " + str(response))
  
def run(masterJobPort, local_id, other_ids):
  address = 'localhost:' + masterJobPort
  sock = socket.socket()
  sock.bind(('', 0))
  localPort = sock.getsockname()[1]
  server = grpc.server(concurrent.futures.ThreadPoolExecutor(max_workers=10))
  terminate_event = threading.Event()
  pollux_payload_pb2_grpc.add_PolluxPayloadServicer_to_server(PolluxPayloadServicer(terminate_event), server)
  server.add_insecure_port('[::]:' + str(localPort))
  server.start()
  with grpc.insecure_channel(address) as channel:
    stub = pollux_payload_pb2_grpc.PolluxJobStub(channel)
    sendPayloadReady(stub, localPort)
    with concurrent.futures.ThreadPoolExecutor(max_workers=1) as executor:
      future = executor.submit(mainLoop, stub, local_id, other_ids)
  
  logging.info("Waiting for terminate_event")
  terminate_event.wait()
  logging.info("Received terminate_event")
  server.stop(5)
  logging.info("server stopped")

class ArgumentParser(argparse.ArgumentParser):
  def error(self, message):
    #errorStr = ""
    #self.print_help(errorStr)
    #logging.error(errorStr)
    logging.error(self.prog + ': ' + message)
    self.exit(2, '%s: error: %s\n' % (self.prog, message))

def main() -> int:
  parser = ArgumentParser(prog='polluxapp', exit_on_error=False)
  parser.add_argument("--port", required=True, help="pollux port")
  parser.add_argument("--id", required=True, help="pollux payload id")

  args = parser.parse_args()

  logName = 'polluxapp' + str(args.id) + '.log' 
  logging.basicConfig(
            filename=logName, filemode='w',
            level=logging.DEBUG,
            format='%(asctime)s %(levelname)-8s %(message)s',
            datefmt='%Y-%m-%d %H:%M:%S')

  logging.info("########################################################")
  logging.info(logName)
  logging.info("########################################################")

  logging.info("command line arguments:" + ''.join(" " + s  for s in sys.argv[1:]))

  other_ids = [0, 1, 2]
  other_ids.remove(int(args.id))
  logging.info("other_ids: " + ''.join(" " + str(i) for i in other_ids))

  run(args.port, int(args.id), other_ids)
  logging.info("payload " + argStr + " terminated")
  return 0

if __name__ == '__main__':
  sys.exit(main())
