# Event list
| **Event**                         | **Note**                                                                      |
| :-------------------------------- | :---------------------------------------------------------------------------- |
| STATE_ENTER                       | Fires when you enter a new state                                              |
| STATE_EXIT                        | Fires when you exit the current state                                         |
| CE_WINDOW_FOCUS_LOST_FULLSCREEN   | Fires when the window focus is lost when in fullscreen window mode            |
| CE_WINDOW_FOCUS_LOST_BORDERLESS   | Fires when the window focus is lost when in borderless window mode            |
| CE_WINDOW_FOCUS_LOST_WINDOWED     | Fires when the window focus is lost when in normal window mode                |
| CE_WINDOW_FOCUS_GAINED_FULLSCREEN | Fires when we get window focus in fullscreen                                  |
| CE_WINDOW_FOCUS_GAINED_BORDERLESS | Fires when we get window focus in borderless window                           |
| CE_WINDOW_FOCUS_GAINED_WINDOWED   | Fires when we get window focus in normal window                               |
| Update                            | This fires before the renderer has begun a frame. Intended for updating logic |
| Draw3D                            | For drawing 3D stuff. You cannot draw 2D stuff!                               |
| Draw2D                            | For drawing 2D stuff. You cannot draw 3D stuff!                               |
| DrawImgui                         | Use ImGui functions                                                           |
| CE_WINDOW_SIZE_CHANGE_FULLSCREEN  | Fires when the window size for fullscreen is set                              |
| CE_WINDOW_SIZE_CHANGE_WINDOWED    | Fires when the window size for windowed is set                                |
| CE_WINDOW_MODE_CHANGED_FULLSCREEN | Fires when the window mode is set to fullscreen                               |
| CE_WINDOW_MODE_CHANGED_BORDERLESS | Fires when the window mode is set to borderless                               |
| CE_WINDOW_MODE_CHANGED_WINDOWED   | Fires when the window mode is set to windowed                                 |
| CE_SETTINGS_RELOAD                | Fires before the Update event if a settings reload was asked                  |