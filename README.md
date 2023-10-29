# Teleaudio

Listening to `.WAV` audio files through the magic of internet.

The files are expected to have only the `RIFF` header along with two subchunks, `fmt` and `data`.

## Build

### Manually

#### Linux

Note: `conan2` is required.

```bash
$> conan profile detect
$> conan install . --output-folder=build --build=missing
$> cmake --preset conan-release
$> cmake --build --preset conan-release
$> ./build/build/Release/bin/teleaudio server <port> test/storage/clean_wavs/
Server listening on 0.0.0.0:<port>
$> # open up a new terminal
$> ./build/build/Release/bin/teleaudio <port> <output-directory>
```

If the instructions aren't working because of presets, use these `cmake` invocations instead:
```bash
$> cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake
$> cmake --build build
```

#### Windows

Note: `conan2` is required.

```bash
$> conan profile detect
$> conan install . --output-folder=build --build=missing
$> cmake         --preset conan-default
$> cmake --build --preset conan-release # conan-release, not conan-default
$> .\build\build\bin\Release\teleaudio server <port> test\storage\clean_wavs\
Server listening on 0.0.0.0:<port>
$> # open up a new terminal
$> .\build\build\bin\Release\teleaudio <port> <output-directory>
```
### Docker variant

Build the image with `docker build -t teleaudio .`

The server will listen on port `1989` by default.

*Note: the `--init` flag is necessary for `<ctrl+c>` to shut the server down.*

```bash
$> docker run --rm --init -p<local_port>:1989 teleaudio
[2023-10-26 00:00:00.123] Server listening on 0.0.0.0:1989
```

As for the client, either use a locally built one:
```bash
$> ./locally-built/bin/teleaudio <local_port> <output_dir>
```

... or mount a `downloads` output directory and tell the client to save there:
```bash
$> docker run --rm -u$(id -u):$(id -g) --network host -v/path/to/local/folder:/output teleaudio <local_port_from_above> /output
```

*Note: the `-u` flag is so the owner of the output file isn't `root`*

*Note: using `--network host` is needed to access the container running the server.*

## Tests

Run the tests with by going into the `build/.../test/` directory and run `ctest -C Release --progress --verbose`.
