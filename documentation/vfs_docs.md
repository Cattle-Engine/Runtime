# Auto mounted folders
| Mount point| Usage | Note |
|------------|-------|------|
| /config    | Uses the [platform-specfic](#platform-specfic-directories) config dir | This mount is writeable |
| /          | The mounted data file, aka your base game data | This mount is not writeable |

# Platform specfic directories
| Mount point | Windows                      | Linux                     |
|-------------|------------------------------|---------------------------|
| /config     | ```%APPDATA%/{Game name}/config``` | ```$HOME/.config/{Game name}``` |