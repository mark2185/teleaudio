# Teleaudio

Listening to `.WAV` audio files through the magic of internet.

The files are expected to have only the `RIFF` header along with two subchunks, `fmt` and `data`.

## Build

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
$> ./bin/teleaudio <output-directory> <port>
```

#### Windows

Not yet tested.

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
$> docker run --rm -u$(id -u):$(id -g) --init -v/path/to/local/folder:/output teleaudio /output 1989
```

*Note: the `-u` flag is so the owner of the output file isn't `root`*
