# YMake todo.md

```markdown

## TODO

### PRIORITY: pretty stupid mistake. the include dir of a library.src is put in a pool of all includes.
###         this means that a library will try to include itself when building it... (amazing code)
### SOLUTION: put an extra attrib per lib for the include flag. (it's refactoring time.... (who's excited? not me))

- [ ] add multithreading to project's src
    - MT should be added to compiling source files.
    - maybe make the no. of threads user specified?? (need to look into it).

- [ ] add progress bar to compilation.
    - along with compile time estimate and compile time at the end
    - check all source files to be compiled. [% per file = 100 / fileno].

- [ ] update default config

- [ ] add new YMake Command: new --type [exe, shared, static]
    - creates a new project in the current directory.

### THINGS TO THINK ABOUT:

- includes based on OS. (and architecture)
- rebuild command
    - `build --interactive` (hot rebuilding when proj files change)
- `new [type]` (creates a project and configures a simple `YMake.toml` file)
    - type == exe, shared, static, (or lib [aka shared])
- better way of handling packages (aka libs)

```
