# YMake todo.md

```markdown

## TODO
- [ ] NEW IDEA: design my own file format (since toml is not that great).

- [ ] includes based on architecture/OS (target)
    - [ ] PROJECT_NAME.macos, windows and linux, openBSD, POSIX, etc.
- [ ] custom keys triggered by user.
    (ex: ymake build -k linux_x64, or ymake build --key linux_x64)
    (this triggers the PROJECT_NAME.linux_x65 table)
        read all the custom.tablename as key: tablename, value: struct/vector
        and then check against that key

    THINGS TO CHANGE (to allow platform includes and custom keys)
    - [ ] project class (attributes, serialization, info output, etc)
    - [ ] parsing function (custom keys + platform keys)
    - [ ] preprocess file function.
    - [ ] compile file function.
    - [ ] build cmd function. (with main function) for new command argument.

### THINGS TO THINK ABOUT:

- includes based on OS. (and architecture)

```
