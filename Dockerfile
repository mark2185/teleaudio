# it uses gcc-11 for which all the conan packages are prebuilt
FROM ubuntu:22.04 as builder

RUN apt update && apt install -y cmake ninja-build gcc python3-pip && \
    pip3 install conan==2.0.13

COPY conanfile.txt /code/conanfile.txt

RUN cd /code && \
    conan profile detect && \
    conan install . --output /build --build=missing

COPY . /code

RUN cd /build && \
    cmake -GNinja -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release /code && \
    cmake --build .

FROM alpine:3.18.4

COPY --from=builder /build/bin/teleaudio /usr/local/bin/

EXPOSE 1989

COPY test/storage/clean_wavs /audio

ENTRYPOINT [ "/usr/local/bin/teleaudio", "server", "/audio", "1989" ]
