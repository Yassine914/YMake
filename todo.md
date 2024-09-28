# YMake Todo.md

- [ ] fix command system (apparently)
        arguments are broken..

- [ ] fix caching system

as far as I know: only 4 commands work:
        ymake
        ymake default
        ymake help
        ymake clean

functions that need to work:
        ymake setup
        ymake build
        ymake list

```cpp

➜ .\ymake.exe build Cube
COMMAND INPUT: 
        Cube
COMMAND ARGS: 
[YMAKE ERROR]: couldn't open cache file to read config file path.

```

- [ ] update default config for new parsing
- [ ] need to add debug logs throughout for debug versions.
- [ ] fix some error messsages (especially cache error messages)
- [ ] fix formatting on the listing project thing