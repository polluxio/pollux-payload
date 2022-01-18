#Prerequesite
#pip install grpcio-tools
#

all:
	python3 -m grpc_tools.protoc -I../proto --python_out=. --grpc_python_out=. ../proto/escher.proto
	python3 -m grpc_tools.protoc -I../proto --python_out=. --grpc_python_out=. ../proto/escher_payload.proto

clean:
	rm -rf __pycache__ escher_payload_pb2.py escher_payload_pb2_grpc.py
	
