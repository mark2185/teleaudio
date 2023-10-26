FROM archlinux:latest

RUN pacman-key --init && \
    pacman -Syu --noconfirm && \
    pacman -S --noconfirm cmake curl zip unzip ninja git gcc pkg-config python python-pip && \
    pip install --break-system-packages conan

COPY . /code

ENV CC=/usr/bin/gcc
ENV CXX=/usr/bin/g++
ENV CMAKE_MAKE_PROGRAM=/usr/bin/ninja
ENV CMAKE_C_COMPILER=$CC
ENV CMAKE_CXX_COMPILER=$CXX

RUN cd /code && \
    conan profile detect && \
    conan install . --output /build --build=missing && \
    cd /build &&\
    cmake -GNinja -GNinja -DCMAKE_TOOLCHAIN_FILE=/build/Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release /code && \
    cmake --build .

RUN mkdir /audio

EXPOSE 1989

WORKDIR /build

COPY ./test/storage/clean_wavs /audio

ENTRYPOINT [ "/build/bin/teleaudio", "/audio", "1989" ]
