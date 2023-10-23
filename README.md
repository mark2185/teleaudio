# Teleaudio

Listening to audio through the magic of internet.

### Build

#### Docker variant

Build the image with `docker build -t teleaudio-image .`

Mount the directory where the `.wav` files are to `/audio` and the server will listen on port `1989` by default.

```bash
$> docker run --rm -v/path/to/wav/files:/audio teleaudio-image
```


#### Manually

Note: `conan` is required package management
```bash
$> conan profile detect
$> conan install .
$> cmake -S . -B build
$> cmake --build build
$> cd build/bin
$> teleaudio -d
Teleaudio server launching...
$> # open up a new terminal
$> teleaudio ls
$> teleaudio play <filename>
```
