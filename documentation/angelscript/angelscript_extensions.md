# Overview
CE has a few implemented and planned for extensions.
The current implemented ones are:
- Module system

Planned for ones are: 
- Type alias system (angelscript techincally has typedef but this can only be used on basic types)
- Precursor (similar style to the C++ precursor with #define, #ifdef etc)

# Module documentation
At its core a module is a script file on disk.

A symbol is a, function, variable or type.

## Importing
You can import all the stuff that a module has exported by doing this:
```angelscript
import foo;
```
In this case "foo" is the file on disk.
With an import CE will search for an import like so:
```
"foo.as"
"foo/module.as"
```

Imported module(s) will not carry to a module that has imported that module unless you do:
```angelscript
export import foo::bar
```

You can also import singular symbols from a module using the ```using``` keyword:
```angelscript
using foo::symbol;
```
this gives you access to just that

Using an import is easy, if its in a namespace in the imported module, it'll be in a namespace for the current module.

## Exporting
To export something simply do:
```angelscript
export struct symbol {

};
```