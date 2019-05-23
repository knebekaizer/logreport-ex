FROM cpp-build-base:0.1.0 AS build

WORKDIR /src

COPY CMakeLists.txt *.cpp *.h *.hpp ./

RUN cmake . && make -j4

FROM ubuntu:bionic

WORKDIR /opt/dell-test

COPY --from=build /src/iplog* ./
COPY iplog.sh ./

ENTRYPOINT ["./iplog.sh"]
CMD ["/data/customers.txt", "/data/log.txt", "/data/report.txt"]
