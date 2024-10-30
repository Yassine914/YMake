# YMake todo.md

```markdown

## TODO

- [x] add multithreading to project's src
    <!-- - MT should be added to compiling source files. -->
    <!-- - maybe make the no. of threads user specified?? (need to look into it). -->

- [x] add progress bar to compilation.
    <!-- - along with compile time estimate and compile time at the end -->
    <!-- - check all source files to be compiled. [% per file = 100 / fileno]. -->

- [ ] update parsing
    - [ ] add workflow with target arch and target architecture for project.

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
