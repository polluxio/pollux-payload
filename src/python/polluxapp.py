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

def send_payload_inactive(stub):
  logging.info("-------------- send_payload_inactive --------------")
  inactive_message = pollux_payload_pb2.PayloadInactiveMessage(info="I'm hungry !!")
  try:
    response = stub.PayloadInactive(inactive_message, timeout=timeOut)
  except grpc.RpcError as rpc_error:
    logging.error('grpc unavailable error: %s', rpc_error)
  logging.info("Received: " + str(response))

def send_pollux_communication(stub, local_id, other_id):
  logging.info("-------------- send_pollux_communication --------------")
  logging.info("Send message from {} to {}".format(local_id, other_id))
  pollux_message = pollux_pb2.PolluxMessage(origin=local_id, destinations=[other_id], key="hello", value="hello")
  try:
    response = stub.PolluxCommunication(pollux_message, timeout=timeOut)
  except grpc.RpcError as rpc_error:
    logging.error('grpc unavailable error: %s', rpc_error)
  logging.info("Send response: " + str(response))

def mainLoop(stub, local_id, other_ids):
  logging.info("-------------- Starting main loop --------------")
  #wait for 20 secs
  time.sleep(5)
  other_id = random.choice(other_ids)
  send_pollux_communication(stub, local_id, other_id)
  time.sleep(5)
  other_id = random.choice(other_ids)
  send_pollux_communication(stub, local_id, other_id)
  time.sleep(5)
  other_id = random.choice(other_ids)
  send_pollux_communication(stub, local_id, other_id)
  time.sleep(5)
  #send to master that I'm done
  send_payload_inactive(stub)

def get_available_port():
  sock = socket.socket()
  sock.bind(('', 0))
  return sock.getsockname()[1]

def send_payload_ready(stub, local_port):
  logging.info("-------------- send_payload_ready --------------")
  ready_message = pollux_payload_pb2.PayloadReadyMessage(info="I'm alive", port=local_port);
  response = stub.PayloadReady(ready_message)
  logging.info("Sign of life: " + str(response))
  
def run(master_job_port, local_id, other_ids):
  address = 'localhost:' + master_job_port
  sock = socket.socket()
  sock.bind(('', 0))
  local_port = sock.getsockname()[1]
  server = grpc.server(concurrent.futures.ThreadPoolExecutor(max_workers=10))
  terminate_event = threading.Event()
  pollux_payload_pb2_grpc.add_PolluxPayloadServicer_to_server(PolluxPayloadServicer(terminate_event), server)
  server.add_insecure_port('[::]:' + str(local_port))
  server.start()
#with grpc.insecure_channel(address) as channel:
#    stub = pollux_payload_pb2_grpc.PolluxJobStub(channel)
#    sendPayloadReady(stub, local_port)
#    with concurrent.futures.ThreadPoolExecutor(max_workers=1) as executor:
#      future = executor.submit(mainLoop, stub, local_id, other_ids)
  
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
  parser.add_argument("--partitions", required=True, help="global number of partitions")

  args = parser.parse_args()

  log_name = 'polluxapp' + str(args.id) + '.log' 
  logging.basicConfig(
            filename=log_name, filemode='w',
            level=logging.DEBUG,
            format='%(asctime)s %(levelname)-8s %(message)s',
            datefmt='%Y-%m-%d %H:%M:%S')

  logging.info("########################################################")
  logging.info(log_name)
  logging.info("########################################################")

  logging.info("command line arguments:" + ''.join(" " + s  for s in sys.argv[1:]))

  other_ids = list(range(int(args.partitions)))
  other_ids.remove(int(args.id))

  logging.info("other_ids: " + ''.join(" " + str(i) for i in other_ids))

  local_port = get_available_port()
  address = 'localhost:' + args.port

  with grpc.insecure_channel(address) as channel:
    stub = pollux_payload_pb2_grpc.ZebulonPayloadStub(channel)
    send_payload_ready(stub, local_port)

  while True:
    pass
  #run(args.port, int(args.id), other_ids)
  logging.info("payload " + args.id + " terminated")
  return 0

if __name__ == '__main__':
  sys.exit(main())
