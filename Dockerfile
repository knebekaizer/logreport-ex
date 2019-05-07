FROM cpp-build-base:0.1.0 AS build

WORKDIR /src

COPY CMakeLists.txt *.cpp *.h *.hpp ./

RUN cmake . && make -j4 iplog

FROM ubuntu:bionic

WORKDIR /opt/dell-test

COPY --from=build /src/iplog ./

ENTRYPOINT ["./iplog"]
CMD ["/data/customers.txt", "/data/log.txt", "/data/report.txt"]
