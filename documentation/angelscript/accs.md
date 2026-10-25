# CE::ACCS

Registration priority: `200`

## ActorHandle
C++ type: `CE::ACCS::ActorHandle`

Flags: `Value`, `POD`, `AppClass`, `AppClassAllInts`

No description given

### Properties
#### index
Type: `uint`

#### generation
Type: `uint`

### Methods
#### IsValid
Return type: `bool`

Const method

No description given


## ComponentID
C++ type: `CE::ACCS::ComponentID`

Flags: `Value`, `POD`, `AppClass`, `AppClassAllInts`

No description given

### Properties
#### index
Type: `uint`

#### generation
Type: `uint`

### Methods
#### IsValid
Return type: `bool`

Const method

No description given


## CE::ACCS
## Enums
### FrameUpdateType
C++ type: `CE::ACCS::FrameUpdateType`

No description given

Values:

- `None`
- `Logic`
- `Render2D`
- `Render3D`
- `ImGui`

## Interfaces
### IComponent
No description given

### ISystem
No description given

#### Methods
```angelscript
void Update(float dt, FrameUpdateType phase)
```

## External Functions
### CreateActor
Return type: `ActorHandle`

No description given

### DestroyActor
Return type: `bool`

Signature:
```angelscript
const ActorHandle &in
```

No description given

### IsActorAlive
Return type: `bool`

Signature:
```angelscript
const ActorHandle &in
```

No description given

### AddComponent
Return type: `ComponentID`

Signature:
```angelscript
const ActorHandle &in, IComponent@
```

No description given

### RemoveComponent
Return type: `bool`

Signature:
```angelscript
const ComponentID &in
```

No description given

### HasComponent
Return type: `bool`

Signature:
```angelscript
const ComponentID &in
```

No description given

### GetComponent
Return type: `IComponent@`

Signature:
```angelscript
const ComponentID &in
```

No description given

### GetActors
Return type: `array<ActorHandle>@`

No description given

### GetComponents
Return type: `array<ComponentID>@`

Signature:
```angelscript
const ActorHandle &in
```

No description given

### RegisterSystem
Return type: `void`

Signature:
```angelscript
FrameUpdateType, ISystem@
```

No description given
