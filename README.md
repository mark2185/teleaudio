# Teleaudio

Listening to audio through the magic of internet.

## Build

### Docker variant

Build the image with `docker build -t teleaudio .`

Mount the directory where the `.wav` files are to `/audio` and the server will listen on port `1989` by default.

```bash
$> docker run --rm --init -p<local_port>:1989 teleaudio
[2023-10-26 00:00:00.123] Server listening on 0.0.0.0:1989
$> # in another shell
$> ./build/bin/teleaudio <local_port> <output_dir>
```

*Note: the `--init` flag is necessary for `<ctrl+c>` to shut the server down.*


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
