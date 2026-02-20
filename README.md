# Prism

PPM P3 image editor written in C using raylib.

## Features

- Load and save PPM P3 images
- Basic drawing tools (brush and fill)
- Color picker

This project is not currently accepting feature requests or contributions, but feel free to fork the repository and make your own improvements!

## Building

### Linux

First, make sure you have [raylib](https://github.com/raysan5/raylib) installed. Then, clone the repository:

```bash
git clone https://github.com/ijo-elaja/prism.git
cd prism
```

Finally, build the project using CMake and Make:

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

Or if you prefer using Ninja:

```bash
mkdir build && cd build
cmake .. -G Ninja
ninja
```

### Windows & MacOS

Prism was only developed for Linux, and it likely will not work properly on Windows or MacOS.

I may or may not add support for non-Linux platforms in the future.

## Usage

To run the program, simply execute the generated binary:

```bash
./prism
```

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

