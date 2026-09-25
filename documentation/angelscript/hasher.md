# Description
Fast 64-bit hashing helpers. Use StreamingHasher when a hash is built from multiple
strings; Finalize returns the hash accumulated so far.

# CE::Utils

## StreamingHasher
C++ type: `CE::Utils::StreamingHasher`

Flags: `Reference`, `Scoped`, `NoCount`

A scoped incremental 64-bit hasher.

### Behaviours
#### Factory
Creates a new empty streaming hasher.

#### Release
Releases the scoped hasher.

### Methods
#### AddString
Return type: `void`

Signature:
```angelscript
const string& in text
```

Adds the bytes of a string to the hash stream.

#### Finalize
Return type: `uint64`

Returns the 64-bit hash of all data added so far.


## Functions
### HashString
Return type: `uint64`

Signature:
```angelscript
const string& in text
```

Returns the 64-bit hash of a string.

### HashToString
Return type: `string`

Signature:
```angelscript
uint64 hash
```

Converts a 64-bit hash to its printable string representation.
