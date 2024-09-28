# YMake Todo.md

- [ ] fix command system (apparently)
        arguments are broken..

- [ ] fix caching system

as far as I know: commands that work:
        ymake
        ymake default
        ymake help
        ymake clean
        ymake setup
        ymake list

functions that need to work:
        ymake build

```cpp

➜ .\ymake.exe build Cube
COMMAND INPUT: 
        Cube
COMMAND ARGS: 
[YMAKE ERROR]: couldn't open cache file to read config file path.

```

- [ ] update default config for new parsing
- [ ] need to add debug logs throughout for debug versions.

## NEED

- [ ] fix some error messsages (especially cache error messages)
- [ ] fix formatting on the listing project thing

### THINKING

- [ ] i'm thinking of redoing the original caching system.
        there are a few too many cache files to keep track of.
        as well as fixing the new one (per project cache)