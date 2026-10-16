# CE Scripting Language

CE provides several extensions to its AngelScript-based scripting language. These extensions include a module system, a source preprocessor, and compile-time assertions.

## Implemented Extensions

- **Module system** - Import and export symbols between script files.
- **Preprocessor** - Define macros and conditionally include source code.
- **Compile-time assertions** - Validate constant expressions during semantic analysis using `static_assert`.

## Planned Extensions

- **Type aliases** - Introduce alternative names for types. AngelScript technically supports `typedef`, but its functionality is limited to basic types.

# Preprocessor

CE includes a built-in preprocessor that processes script source code before it is passed to the lexer and parser.

The preprocessor supports macros and conditional compilation, allowing scripts to adapt to different configurations without requiring changes to the source code itself.

Preprocessor directives begin with `#` and must appear at the beginning of a line, apart from optional whitespace.

## Defining Macros

Use `#define` to define a macro:

```angelscript
#define MAX_PLAYERS 16
#define GAME_NAME "My Game"
#define ENABLE_AUDIO
```

When a macro has a replacement value, occurrences of its name in the source code are replaced with that value.

For example:

```angelscript
#define MAX_HEALTH 100

void Initialise() {
    int health = MAX_HEALTH;
}
```

After preprocessing, `MAX_HEALTH` is replaced with `100`.

A macro without a replacement value can still be used to control conditional compilation:

```angelscript
#define DEBUG_BUILD
```

### Macro Replacement Rules

Macro replacement follows these rules:

- Macro names must be valid identifiers. They may contain letters, digits, and underscores, but cannot begin with a digit.
- Replacement is performed on identifier tokens rather than arbitrary substrings.
- Macros are replaced in ordinary source code, but not inside string literals, character literals, or comments.
- Both single-line (`//`) and block (`/* ... */`) comments are recognised when performing macro replacement.
- Replacement values are inserted as written. The preprocessor does not recursively expand macros within replacement values in the same pass.
- Function-like macros and macro parameters are not supported.

For example:

```angelscript
#define VALUE 10

int a = VALUE;       // Replaced with 10.
int b = VALUE_COUNT; // Unchanged.
string s = "VALUE";  // Unchanged.
```

### Redefining Macros

An existing macro can be redefined using another `#define`. Its replacement value is updated.

```angelscript
#define MAX_PLAYERS 8

// Override the previous definition.
#define MAX_PLAYERS 16
```

The second definition replaces the first.

## Conditional Compilation

CE supports conditional compilation through `#ifdef`, `#ifndef`, `#elif`, `#else`, and `#endif`.

These directives allow sections of source code to be included or excluded depending on whether a macro has been defined.

### `#ifdef`

Includes a section when the specified macro is defined.

```angelscript
#define DEBUG_BUILD

#ifdef DEBUG_BUILD
    void DebugLog(string message) {
        print(message);
    }
#endif
```

Because `DEBUG_BUILD` is defined, the enclosed code remains active.

### `#ifndef`

Includes a section when the specified macro has not been defined.

```angelscript
#ifndef RELEASE_BUILD
    void EnableDebugTools() {
        // Development-only functionality.
    }
#endif
```

The enclosed code is active only when `RELEASE_BUILD` is not defined.

### `#elif`

Checks whether another macro is defined when no previous branch in the same conditional chain has been selected.

```angelscript
#ifdef WINDOWS_BUILD
    // Windows-specific code.
#elif LINUX_BUILD
    // Linux-specific code.
#else
    // Default implementation.
#endif
```

`#elif` accepts a macro name, not a general boolean expression. In the example, `LINUX_BUILD` is checked only if the `WINDOWS_BUILD` branch was not selected.

### `#else`

Provides a fallback branch when no preceding branch has been selected.

```angelscript
#ifdef ENABLE_AUDIO
    // Audio-enabled implementation.
#else
    // Audio-disabled implementation.
#endif
```

Only one branch in a conditional chain is active.

### `#endif`

Closes a conditional compilation block.

Every conditional block must have a matching `#endif`. Nested conditional blocks are supported.

```angelscript
#ifdef DEBUG_BUILD
    #ifdef ENABLE_AUDIO
        // Audio debugging code.
    #endif
#endif
```

### Conditional Compilation Rules

- `#ifdef` checks whether a macro exists, regardless of whether its replacement value is empty.
- `#ifndef` checks whether a macro does not exist.
- `#elif` checks whether the specified macro exists and no previous branch has been selected.
- `#else` activates only when no previous branch has been selected.
- Nested conditionals are supported.
- Directives are processed even inside inactive branches so that conditional blocks remain correctly balanced.
- Malformed directives, unmatched conditional blocks, and unknown directives cause preprocessing to fail.

The current implementation does not support general expressions in conditional directives. For example, conditions such as `#if MAX_PLAYERS > 8` are not supported.

## Configuring Macros from C++

The preprocessor also exposes an API for managing macros externally.

Macros can be added or removed through the `Preprocessor` class, allowing the engine to configure scripts before processing them.

- `AddDefine` adds a macro, returning `false` if the name is invalid or already exists.
- `RemoveDefine` removes a macro by name, returning `false` if no matching macro exists.

Macros supplied through the API can be used by scripts in the same way as macros defined with `#define`.

Unlike a duplicate `AddDefine` call, a `#define` directive in source code can redefine an existing macro.

This is useful for setting engine-wide configuration flags, build modes, or feature switches.

## Preprocessing Errors

The preprocessor reports errors when it encounters invalid or unsupported directives.

Examples include:

- Invalid macro names.
- Invalid `#ifdef` or `#ifndef` arguments.
- `#elif` without a matching conditional block.
- `#elif` after `#else`.
- Duplicate or malformed `#else` directives.
- Unmatched or malformed `#endif` directives.
- Unterminated conditional blocks.
- Unknown preprocessor directives.

If preprocessing fails, the module cannot proceed to the lexer, parser, or semantic analysis.

The preprocessor also preserves newline positions in its output to help maintain useful line numbers in subsequent diagnostics.

# Compile-Time Assertions

CE supports `static_assert` to validate conditions during semantic analysis.

Assertions are evaluated during compilation rather than at script runtime. If an assertion evaluates to `false`, compilation fails with a semantic error. An optional message can explain the failure.

```angelscript
static_assert(1 + 1 == 2);

static_assert(true, "This condition must hold");
```

## Supported Constant Expressions

The compile-time evaluator supports:

- Integer and floating-point literals.
- Boolean literals: `true` and `false`.
- String literals.
- References to `const` global variables whose values can be evaluated at compile time.
- Arithmetic operators: `+`, `-`, `*`, `/`, `%`.
- Comparison operators: `==`, `!=`, `<`, `<=`, `>`, `>=`.
- Logical operators: `&&`, `||`, `!`.
- Alternative logical keywords: `and`, `or`, `not`.
- Parenthesised expressions.
- Unary numeric operators: `+` and `-`.
- String concatenation using `+`.

Integer arithmetic uses checked 64-bit signed values. Integer overflow, division by zero, and modulo by zero produce semantic errors.

The evaluator detects circular dependencies between constants.

Supported constant types are integers, floating-point numbers, booleans, and strings. Handles, references, arrays, and unsupported types cannot be used as compile-time constants.

Example:

```angelscript
const int MAX_ITEMS = 100;
const int ITEMS_PER_GROUP = 10;

static_assert(MAX_ITEMS > 0);
static_assert(
    MAX_ITEMS % ITEMS_PER_GROUP == 0,
    "MAX_ITEMS must be divisible by ITEMS_PER_GROUP"
);
```

The preprocessor and `static_assert` serve different purposes: the preprocessor modifies or excludes source code before parsing, while `static_assert` evaluates constant expressions during semantic analysis.

# Module System

At its core, a module is a script file on disk.

A **symbol** is a function, global variable, or type declared in a module. Symbols can be exported to make them accessible to other modules.

## Importing

You can import the exported symbols from another module using `import`:

```angelscript
import foo;
```

In this example, `foo` refers to a script module on disk.

CE searches for modules using the following paths:

```text
foo.as
foo/module.as
```

An ordinary import makes a module's exported symbols available to the importing module. These imports are not automatically re-exported to other modules that import it.

To re-export an imported module's symbols, use `export import`:

```angelscript
export import foo::bar;
```

### Importing Individual Symbols

You can use `using` to bring a specific exported symbol into scope:

```angelscript
using foo::symbol;
```

This makes the selected symbol available without bringing every exported symbol from the module into scope.

### Namespaces

Symbols declared inside a namespace retain their namespace when imported. Namespace-qualified names can be used to access these symbols.

## Exporting

To make a symbol available to other modules, declare it with the `export` keyword.

For example, to export a type:

```angelscript
export struct MyType {
};
```

Functions and global variables can also be exported.

Only exported symbols are made available through a module's exports, allowing a module to control which parts of its implementation are accessible to other scripts.
