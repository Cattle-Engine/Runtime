# Description
Minimal immediate-mode UI bindings. Call these only from the optional void imgui()
script function, which runs after the engine starts an ImGui frame and before it renders.
You can also use the event that runs every frame "DrawImgui"

# CE::ImGui

## Functions
### Begin
Return type: `bool`

Signature:
```angelscript
const string& in title
```

Begins a window and returns whether its contents should be drawn; always pair it with End.

### End
Return type: `void`

Ends the current window begun by Begin.

### Text
Return type: `void`

Signature:
```angelscript
const string& in text
```

Draws unformatted text at the current cursor position.

### Button
Return type: `bool`

Signature:
```angelscript
const string& in label
```

Draws a button and returns true on the frame it is clicked.

### Separator
Return type: `void`

Draws a horizontal separator.

### SameLine
Return type: `void`

Places the next item on the current line.

### NewLine
Return type: `void`

Advances the cursor to a new line.

### Spacing
Return type: `void`

Adds vertical spacing between UI items.

### Dummy
Return type: `void`

Signature:
```angelscript
float width, float height
```

Adds an invisible item with the specified width and height.

### Indent
Return type: `void`

Signature:
```angelscript
float width = 0.0f
```

Increases the current indentation by the specified width.

### Unindent
Return type: `void`

Signature:
```angelscript
float width = 0.0f
```

Decreases the current indentation by the specified width.

### BeginChild
Return type: `bool`

Signature:
```angelscript
const string& in id, float width, float height
```

Begins a child region with the specified identifier and size; pair with EndChild.

### EndChild
Return type: `void`

Ends the current child region begun by BeginChild.

### CollapsingHeader
Return type: `bool`

Signature:
```angelscript
const string& in label
```

Draws a collapsible header and returns whether its contents should be displayed.

### TreeNode
Return type: `bool`

Signature:
```angelscript
const string& in label
```

Begins a tree node and returns whether it is expanded; expanded nodes should be paired with TreePop.

### TreePop
Return type: `void`

Ends the current tree node begun by TreeNode.

### Selectable
Return type: `bool`

Signature:
```angelscript
const string& in label, bool selected = false
```

Draws a selectable item and returns true when it is selected or activated.

### BeginCombo
Return type: `bool`

Signature:
```angelscript
const string& in label, const string& in preview
```

Begins a combo box and returns whether its popup contents should be drawn; pair with EndCombo.

### EndCombo
Return type: `void`

Ends the combo box begun by BeginCombo.

### TextDisabled
Return type: `void`

Signature:
```angelscript
const string& in text
```

Draws text using ImGui's disabled text styling.

### BulletText
Return type: `void`

Signature:
```angelscript
const string& in text
```

Draws a bullet followed by the specified text.

### LabelText
Return type: `void`

Signature:
```angelscript
const string& in label, const string& in text
```

Draws a label and associated text on the same line.

### OpenPopup
Return type: `void`

Signature:
```angelscript
const string& in id
```

Opens a popup identified by the specified ID.

### BeginPopup
Return type: `bool`

Signature:
```angelscript
const string& in id
```

Begins a popup and returns whether its contents should be drawn; pair with EndPopup when true.

### EndPopup
Return type: `void`

Ends the current popup begun by BeginPopup.

### BeginPopupModal
Return type: `bool`

Signature:
```angelscript
const string& in title
```

Begins a modal popup and returns whether its contents should be drawn; pair with EndPopup.

### BeginMenu
Return type: `bool`

Signature:
```angelscript
const string& in label
```

Begins a submenu and returns whether its contents should be drawn; pair with EndMenu.

### EndMenu
Return type: `void`

Ends the current submenu begun by BeginMenu.

### MenuItem
Return type: `bool`

Signature:
```angelscript
const string& in label
```

Draws a menu item and returns true when it is activated.
