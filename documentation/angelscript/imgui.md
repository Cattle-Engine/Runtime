# Description
Minimal immediate-mode UI bindings. Call these only from the optional void imgui()
script function, which runs after the engine starts an ImGui frame and before it renders.
You can also use the event that runs every frame "DrawImgui"


# CE::ImGui

Registration priority: `99`

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

### Checkbox
Return type: `bool`

Signature:
```angelscript
const string& in label, bool checked
```

Draws a checkbox from the provided value and returns the updated checked state.

### InputInt
Return type: `int`

Signature:
```angelscript
const string& in label, int value
```

Draws an integer input field from the provided value and returns the updated value.

### InputFloat
Return type: `float`

Signature:
```angelscript
const string& in label, float value
```

Draws a float input field from the provided value and returns the updated value.

### SliderInt
Return type: `int`

Signature:
```angelscript
const string& in label, int value, int min, int max
```

Draws an integer slider clamped to [min, max] and returns the updated value.

### SliderFloat
Return type: `float`

Signature:
```angelscript
const string& in label, float value, float min, float max
```

Draws a float slider clamped to [min, max] and returns the updated value.

### DragInt
Return type: `int`

Signature:
```angelscript
const string& in label, int value, float speed = 1.0f, int min = 0, int max = 0
```

Draws a draggable integer field and returns the updated value; min >= max means unbounded.

### DragFloat
Return type: `float`

Signature:
```angelscript
const string& in label, float value, float speed = 1.0f, float min = 0.0f, float max = 0.0f
```

Draws a draggable float field and returns the updated value; min >= max means unbounded.

### InputFloat2
Return type: `bool`

Signature:
```angelscript
const string& in label, float x, float y, float &out outX, float &out outY
```

Draws a two-component float input; writes the edited values to outX and outY and returns true when changed.

### InputFloat3
Return type: `bool`

Signature:
```angelscript
const string& in label, float x, float y, float z, float &out outX, float &out outY, float &out outZ
```

Draws a three-component float input; writes the edited values to outX, outY and outZ and returns true when changed.

### DragFloat2
Return type: `bool`

Signature:
```angelscript
const string& in label, float x, float y, float &out outX, float &out outY, float speed = 1.0f, float min = 0.0f, float max = 0.0f
```

Draws a draggable two-component float field; writes the edited values to outX and outY and returns true when changed.

### DragFloat3
Return type: `bool`

Signature:
```angelscript
const string& in label, float x, float y, float z, float &out outX, float &out outY, float &out outZ, float speed = 1.0f, float min = 0.0f, float max = 0.0f
```

Draws a draggable three-component float field; writes the edited values to outX, outY and outZ and returns true when changed.

### SliderFloat2
Return type: `bool`

Signature:
```angelscript
const string& in label, float x, float y, float &out outX, float &out outY, float min, float max
```

Draws a two-component float slider clamped to [min, max]; writes the edited values to outX and outY and returns true when changed.

### SliderFloat3
Return type: `bool`

Signature:
```angelscript
const string& in label, float x, float y, float z, float &out outX, float &out outY, float &out outZ, float min, float max
```

Draws a three-component float slider clamped to [min, max]; writes the edited values to outX, outY and outZ and returns true when changed.

### ColourEdit3
Return type: `Colour`

Signature:
```angelscript
const string& in label, Colour colour
```

Draws an RGB colour editor (alpha is preserved) and returns the updated colour.

### ColourEdit4
Return type: `Colour`

Signature:
```angelscript
const string& in label, Colour colour
```

Draws an RGBA colour editor and returns the updated colour.

### ColourPicker4
Return type: `Colour`

Signature:
```angelscript
const string& in label, Colour colour
```

Draws a full RGBA colour picker and returns the updated colour.

### ColourButton
Return type: `bool`

Signature:
```angelscript
const string& in id, Colour colour
```

Draws a small colour swatch and returns true when clicked.
