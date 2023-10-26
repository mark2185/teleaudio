# Teleaudio

Listening to audio through the magic of internet.

## Build

### Docker variant

Build the image with `docker build -t teleaudio .`

Mount the directory where the `.wav` files are to `/audio` and the server will listen on port `1989` by default.

```bash
$> docker run --rm -p<local_port>:1989 -v/path/to/wav/files:/audio teleaudio
$> ./build/bin/teleaudio <local_port>
```


### Manually

#### Linux

Note: `conan` and `ninja` packages are required.

```bash
$> conan profile detect
$> conan install . --output-folder=build --build=missing
$> cd build
$> cmake -GNinja -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release ../
$> cmake --build .
$> ./bin/teleaudio server ../test/storage/clean_wavs/ <port>
Server listening on 0.0.0.0:<port>
$> # open up a new terminal
$> ./bin/teleaudio <port>
```

#### Windows
