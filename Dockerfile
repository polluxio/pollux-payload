FROM ubuntu:latest AS builder

# Update Ubuntu Software repository
RUN apt-get update && apt-get -y install \
    cmake \
    make  \
    g++ \
    protobuf-compiler-grpc \
    libgrpc++-dev

WORKDIR /polluxapp
COPY cmake cmake
COPY src src
COPY CMakeLists.txt .
RUN cmake . && make -j$(nproc)

FROM pollux-zebulon:latest AS polluxapp
RUN apt-get update && apt-get -y install libgrpc++
WORKDIR /root/
COPY --from=builder /polluxapp/src/c++/polluxapp ./
EXPOSE 50000
ENTRYPOINT [ "./zebulon" ] 
