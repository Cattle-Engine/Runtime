# Event list
| **Event**                         | **Note**                                                                      |
| :-------------------------------- | :---------------------------------------------------------------------------- |
| STATE_ENTER                       | Fires when you enter a new state                                              |
| STATE_EXIT                        | Fires when you exit the current state                                         |
| CE_WINDOW_FOCUS_GAINED_FULLSCREEN | Fires when the window focus is lost when in fullscreen window mode            |
| CE_WINDOW_FOCUS_GAINED_BORDERLESS | Fires when the window focus is lost when in borderless window mode            |
| CE_WINDOW_FOCUS_GAINED_WINDOWED   | Fires when the window focus is lost when in normal window mode                |
| CE_WINDOW_FOCUS_GAINED_FULLSCREEN | Fires when we get window focus in fullscreen                                  |
| CE_WINDOW_FOCUS_GAINED_BORDERLESS | Fires when we get window focus in borderless window                           |
| CE_WINDOW_FOCUS_GAINED_WINDOWED   | Fires when we get window focus in normal window                               |
| Update                            | This fires before the renderer has begun a frame. Intended for updating logic |
| Draw3D                            | For drawing 3D stuff. You cannot draw 2D stuff!                   |
| Draw2D                            | For drawing 3D stuff. You cannot draw 3D stuff!                               |