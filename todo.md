# YMake Todo.md

caching system now works as intended.

## TODO

- [ ] build libraries.
    two approaches:
        add a ymake.toml file per library and recursively build.
        use a package manager like conan to build the libraries.

- [ ] add multithreading.
    MT should be added to compiling source files.

- [ ] add progress bar to compilation.
    along with compile time estimate and compile time at the end

## FIXME

- [ ] command line parsing is now broken.
- [ ] the preprocessed_metadata.cache has entries for obj files.
    must only have entries for intermediary files.
