# Description
CE will not auto load bindings. You will have to do it manually.

# CE::Input::Bindings

## Functions
### IsBindingPressed
Return type: `bool`

Signature:
```angelscript
const string& in binding_name
```

Returns true or false if a binding is pressed for this frame

### IsBindingDown
Return type: `bool`

Signature:
```angelscript
const string& in binding_name
```

Returns true or false if a binding is down at the current frame

### IsBindingReleased
Return type: `bool`

Signature:
```angelscript
const string& in binding_name
```

Returns true or false if a key was released this frame

### AddBinding
Return type: `void`

Signature:
```angelscript
const string& in name, const CE::Input::Keyboard::KeyboardKeys& in key
```

Adds a binding using the name and keyboard key

### AddBinding
Return type: `void`

Signature:
```angelscript
const string& in name, const CE::Input::Mouse::MouseButtonss& in key
```

Adds a binding using the name and mouse button

### RemoveBinding
Return type: `void`

Signature:
```angelscript
const string& in name, const CE::Input::Keyboard::KeyboardKeys& in key
```

Removes just one keyboard binding

### RemoveBinding
Return type: `void`

Signature:
```angelscript
const string& in name, const CE::Input::Mouse::MouseButtonss& in key
```

Removes just one button binding

### RemoveEntireBinding
Return type: `void`

Signature:
```angelscript
const string& in binding_name
```

Removes all keys/buttons from a binding

### FlushBindings
Return type: `void`

Signature:
```angelscript
const string& in path
```

Flushes the current bindings to a file.
The path is on the VFS and must be writeable. 
Check the VFS docs to see what mounts are writeable.

### LoadBindings
Return type: `void`

Signature:
```angelscript
const string& in path
```

Loads a binding file from the VFS

### ResetBindings
Return type: `void`

Remove all bindings set
