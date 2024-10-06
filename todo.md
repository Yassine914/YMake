# YMake Todo.md

## TODO

- [ ] improve parsing.
    add support for macros defined by me (like CMAKE_DIR etc)
    libraries to either be built or linked from binary
    compiler optimizations specified by the user.

- [ ] build libraries.
    two approaches:
        add a ymake.toml file per library and recursively build.
        include all the project info in the same YMake.toml file.

- [ ] add multithreading.
    MT should be added to compiling source files.
    maybe make the no. of threads user specified?? (need to look into it).

- [ ] add progress bar to compilation.
    along with compile time estimate and compile time at the end
    check all source files to be compiled. [% per file = 100 / fileno].
