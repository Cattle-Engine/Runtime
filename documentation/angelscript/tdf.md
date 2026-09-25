# Description
TDF is CE's typed data-file format. Build values with Make functions, store them in a
File, then Save or Load it through the virtual file system. Typed Read functions return
a default value when given a value of the wrong type.

# CE::TDF

## Value
C++ type: `CE::TDF::Value`

Flags: `Value`, `AutoGetFlags`

A typed TDF value used as the contents of a File entry.

### Properties
#### type
Type: `Type`
The stored TDF value type.

### Behaviours
#### Construct
Constructs a null TDF value.

#### Destruct
Destroys the value and its owned data.


## File
C++ type: `CE::TDF::File`

Flags: `Value`, `AutoGetFlags`

An in-memory collection of named TDF values.

### Behaviours
#### Construct
Constructs an empty TDF file.

#### Destruct
Destroys the TDF file and all of its values.

### Methods
#### Has
Return type: `bool`

Signature:
```angelscript
const string& in key
```

Const method

Returns whether an entry with the key exists.

#### Remove
Return type: `bool`

Signature:
```angelscript
const string& in key
```

Removes an entry and returns whether it existed.

#### Set
Return type: `void`

Signature:
```angelscript
const string& in key, const Value& in value
```

Stores or replaces the value associated with a key.


## Functions
### MakeNull
Return type: `Value`

Creates a null TDF value.

### MakeBool
Return type: `Value`

Signature:
```angelscript
bool value
```

Creates a Boolean TDF value.

### MakeInt
Return type: `Value`

Signature:
```angelscript
int value
```

Creates a signed 32-bit integer TDF value.

### MakeUInt
Return type: `Value`

Signature:
```angelscript
uint value
```

Creates an unsigned 32-bit integer TDF value.

### MakeFloat
Return type: `Value`

Signature:
```angelscript
float value
```

Creates a floating-point TDF value.

### MakeString
Return type: `Value`

Signature:
```angelscript
const string& in value
```

Creates a string TDF value.

### ReadBool
Return type: `bool`

Signature:
```angelscript
const Value& in value
```

Reads a Boolean value, or false when the type does not match.

### ReadInt
Return type: `int`

Signature:
```angelscript
const Value& in value
```

Reads a signed integer, or zero when the type does not match.

### ReadUInt
Return type: `uint`

Signature:
```angelscript
const Value& in value
```

Reads an unsigned integer, or zero when the type does not match.

### ReadFloat
Return type: `float`

Signature:
```angelscript
const Value& in value
```

Reads a floating-point value, or zero when the type does not match.

### ReadString
Return type: `string`

Signature:
```angelscript
const Value& in value
```

Reads a string, or an empty string when the type does not match.

### Get
Return type: `Value`

Signature:
```angelscript
const File& in file, const string& in key
```

Gets a copy of a value, or a null value when the key does not exist.

## Enums
### Type
C++ type: `CE::TDF::Type`

Values:

- `Null`
- `Bool`
- `Int32`
- `UInt32`
- `Float`
- `String`
- `Object`
- `ArrBool`
- `ArrInt32`
- `ArrUInt32`
- `ArrFloat`
- `ArrString`
- `ArrObject`

## Functions
### Save
Return type: `bool`

Signature:
```angelscript
const File& in file, const string& in path, uint version
```

Saves a TDF file to a virtual path using the supplied format version.

### Load
Return type: `bool`

Signature:
```angelscript
File& out file, const string& in path
```

Loads a TDF file from a virtual path into an existing File value.
