# Description
Provides access to the instance's virtual file system. All paths are virtual paths
and resolve through the mounts configured by the engine. File handles are scoped.

# CE::VFS

## File
C++ type: `CE::Scripting::Bindings::ASIFile`

Flags: `Reference`, `Scoped`, `NoCount`

A scoped handle to an open virtual file.

### Behaviours
#### Release
Closes the file and releases its scoped handle.

### Methods
#### IsOpen
Return type: `bool`

Const method

Returns whether the underlying file is still open.

#### Size
Return type: `uint64`

Const method

Gets the file size in bytes.

#### TellRead
Return type: `uint64`

Gets the current read cursor position in bytes.

#### TellWrite
Return type: `uint64`

Gets the current write cursor position in bytes.

#### DateModified
Return type: `int64`

Gets the last-modified Unix timestamp in milliseconds, or zero when unavailable.

#### SeekRead
Return type: `bool`

Signature:
```angelscript
int64 offset, SeekOrigin origin
```

Moves the read cursor by an offset relative to an origin.

#### SeekWrite
Return type: `bool`

Signature:
```angelscript
int64 offset, SeekOrigin origin
```

Moves the write cursor by an offset relative to an origin.

#### ReadString
Return type: `string`

Signature:
```angelscript
uint64 bytes
```

Reads exactly the requested number of bytes as a string; returns an empty string on failure.

#### WriteString
Return type: `bool`

Signature:
```angelscript
const string& in text
```

Writes the bytes of a string at the current write cursor.

#### Flush
Return type: `bool`

Flushes buffered writes to the backing file provider.

#### Eof
Return type: `bool`

Const method

Returns whether the read cursor has reached end-of-file.


## Enums
### SeekOrigin
C++ type: `CE::Common::FS::VFS::SeekOrigin`

Values:

- `Begin`
- `Current`
- `End`

### OpenFlags
C++ type: `CE::Common::FS::VFS::OpenFlags`

Values:

- `None`
- `Read`
- `Write`
- `Create`
- `Append`

## Functions
### FileExists
Return type: `bool`

Signature:
```angelscript
const string& in path
```

Returns whether a virtual path resolves to an existing file.

### GetFileSize
Return type: `uint64`

Signature:
```angelscript
const string& in path
```

Gets a file's size in bytes, or zero if the path cannot be resolved.

### IsWritable
Return type: `bool`

Signature:
```angelscript
const string& in path
```

Returns whether a writable mount can service a virtual path.

### CreateFile
Return type: `bool`

Signature:
```angelscript
const string& in path
```

Creates an empty file in the selected writable mount.

### DeleteFile
Return type: `bool`

Signature:
```angelscript
const string& in path
```

Deletes a file from the writable provider selected for a virtual path.

### MoveFile
Return type: `bool`

Signature:
```angelscript
const string& in from, const string& in to
```

Moves a file when both paths resolve to the same writable provider.

### OpenFile
Return type: `File@`

Signature:
```angelscript
const string& in path, OpenFlags flags
```

Opens a scoped file handle using the requested access flags; returns null on failure.
