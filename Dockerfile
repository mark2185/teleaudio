FROM archlinux:latest

RUN pacman-key --init && pacman -Syu --noconfirm && pacman -S --noconfirm cmake curl zip unzip ninja git gcc pkg-config conan


COPY . /code

WORKDIR /code

RUN conan profile detect && \
    conan install . --output /build && \
    cd /build &&\
    cmake -GNinja -GNinja -DCMAKE_TOOLCHAIN_FILE=/build/Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release /code && \
    cmake --build .

RUN mkdir /audio

EXPOSE 1989

ENTRYPOINT [ "/build/bin/teleaudio", "/audio", "1989" ]
