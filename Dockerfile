FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
       build-essential \
       cmake \
       ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

COPY CMakeLists.txt /src/
COPY include /src/include
COPY src /src/src
COPY tests /src/tests
COPY policies /src/policies

RUN cmake -S /src -B /src/build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build /src/build --parallel

FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
       ca-certificates \
       libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /

COPY --from=builder /src/build/sysguardd /usr/local/bin/sysguardd
COPY --from=builder /src/policies/default.policy /etc/sysguardd/default.policy

ENTRYPOINT ["/usr/local/bin/sysguardd"]
CMD ["daemon","--mode","monitor","--policy","/etc/sysguardd/default.policy"]
