#Prerequesite
#pip install grpcio-tools
#

all:
	python3 -m grpc_tools.protoc -I../proto --python_out=. --grpc_python_out=. ../proto/pollux.proto
	python3 -m grpc_tools.protoc -I../proto --python_out=. --grpc_python_out=. ../proto/pollux_payload.proto

clean:
	rm -rf __pycache__ pollux_payload_pb2.py pollux_payload_pb2_grpc.py
	
