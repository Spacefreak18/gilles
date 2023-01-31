# Gilles
ncurses based telemetry monitor for racing sims

## Features
- Updates at 120 frames per seconds.
- Modular design for support with various titles

[![gilles1.png](https://i.postimg.cc/JhgrQB8c/gilles1.png)](https://postimg.cc/ns4fFrCC)

## Dependencies
- argtable2
- libconfig
- ncurses
- slog (static)
- [simshmbridge](https://github.com/spacefreak18/simshmbridge)
- [simapi](https://github.com/spacefreak18/simapi)

## Building
This code depends on the shared memory data headers in the simapi [repo](https://github.com/spacefreak18/simapi). When pulling lastest if the submodule does not download run:
```
git submodule sync --recursive
git submodule update --init --recursive
```
Then to compile simply:
```
mkdir build; cd build
cmake ..
make
```

## Usage
```
gilles play --sim=ac -S
```

Omit the -S parameter if you do not wish to send data to your local mqtt server.

## Testing

### Static Analysis
```
    mkdir build; cd build
    make clean
    CFLAGS=-fanalyzer cmake ..
    make
```
### Valgrind
```
    cd build
    valgrind -v --leak-check=full --show-leak-kinds=all --suppressions=../.valgrindrc ./gilles play
```

## ToDo
 - GUI
 - Actual telemetry to compare against self and other drivers
 - much, much more
