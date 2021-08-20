FROM alpine:3.14 AS builder
RUN apk add --no-cache musl-dev gcc cmake make libcap

COPY . /usr/local/src/mdns-reflector
WORKDIR /usr/local/src/mdns-reflector
RUN mkdir -p build \
    && cd build \
    && cmake -DCMAKE_BUILD_TYPE=release .. \
    && make VERBOSE=1 \
    && make install DESTDIR=install

RUN setcap cap_net_raw+ep build/install/usr/local/bin/mdns-reflector

FROM alpine:3.14
COPY --from=builder /usr/local/src/mdns-reflector/build/install/ /
CMD ["/usr/local/bin/mdns-reflector", "-h"]
EXPOSE 5353/udp
USER 1000
