# YMake todo.md

```markdown

## TODO

### MAIN TODO: redo the entire build.cpp file. (approach right now is slightly rough)
- FIXME: current system crashes when trying to build libraries.

- [ ] add multithreading.
    - MT should be added to compiling source files.
    - maybe make the no. of threads user specified?? (need to look into it).

- [ ] add progress bar to compilation.
    - along with compile time estimate and compile time at the end
    - check all source files to be compiled. [% per file = 100 / fileno].

### THINGS TO THINK ABOUT:

- includes based on OS. (and arch)
- rebuild command
    - `build --interactive` (hot rebuilding when proj files change)
- `new [type]` (creates a project and configures a simple `YMake.toml` file)
    - type == exe, shared, static, (or lib [aka shared])
- better way of handling packages (aka libs)

```
