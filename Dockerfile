FROM gcc:latest

WORKDIR /ymake

COPY . /ymake/

RUN g++ -o ymake -std=c++17 -O3 /ymake/src/build/build2.cpp /ymake/src/cache/cache.cpp \
    /ymake/src/cmd/cmd.cpp /ymake/src/core/logger.cpp /ymake/src/toml/parser.cpp /ymake/src/main.cpp \
    -I./src/ -I./lib/tomlplusplus/include/ -lpthread -lstdc++fs


RUN chmod +x /ymake/ymake

CMD ["/ymake/ymake", "help"]