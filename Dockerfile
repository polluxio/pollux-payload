FROM ubuntu:latest AS builder

# Update Ubuntu Software repository
RUN apt-get update && apt-get -y install \
    cmake \
    make  \
    g++ \
    libboost-dev \
    protobuf-compiler-grpc \
    libgrpc++-dev

WORKDIR /polluxapp
COPY cmake cmake
COPY src src
COPY thirdparty thirdparty
COPY CMakeLists.txt .
RUN cmake . && make -j$(nproc)

FROM pollux:latest AS polluxapp
RUN apt-get update && apt-get -y install libgrpc++ openssh-server
RUN mkdir /var/run/sshd
RUN sed -ri 's/^#?PermitRootLogin\s+.*/PermitRootLogin yes/' /etc/ssh/sshd_config

ENV GRPC_VERBOSITY=debug
ENV GRPC_TRACE=server_channel,client_channel_call
WORKDIR /root
COPY --from=builder /polluxapp/src/c++/polluxapp ./
EXPOSE 22 
#For Pollux Communication
EXPOSE 50000
