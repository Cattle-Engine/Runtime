# CE::ACCS

Registration priority: `10`

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


## Transform
C++ type: `CE::ACCS::Components::Transform`

Flags: `Reference`, `NoCount`

No description given

### Properties
#### transform_3d
Type: `Transform3D`

#### transform_2d
Type: `Transform2D`


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

### LoadResult
C++ type: `CE::ACCS::LoadResult`

No description given

Values:

- `Ok`
- `Partial`
- `Failed`

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

### GetActors
Return type: `array<ActorHandle>@`

No description given

### SetActorName
Return type: `bool`

Signature:
```angelscript
const ActorHandle &in, const string &in
```

No description given

### GetActorName
Return type: `string`

Signature:
```angelscript
const ActorHandle &in
```

No description given

### SetActorActive
Return type: `bool`

Signature:
```angelscript
const ActorHandle &in, bool
```

No description given

### IsActorActive
Return type: `bool`

Signature:
```angelscript
const ActorHandle &in
```

No description given

### AddActorTag
Return type: `bool`

Signature:
```angelscript
const ActorHandle &in, const string &in
```

No description given

### RemoveActorTag
Return type: `bool`

Signature:
```angelscript
const ActorHandle &in, const string &in
```

No description given

### HasActorTag
Return type: `bool`

Signature:
```angelscript
const ActorHandle &in, const string &in
```

No description given

### GetActorTags
Return type: `array<string>@`

Signature:
```angelscript
const ActorHandle &in
```

No description given

### RegisterComponent
Return type: `bool`

Signature:
```angelscript
const string &in name, IComponent@ component
```

No description given

### AddComponent
Return type: `ComponentID`

Signature:
```angelscript
const ActorHandle &in, const string &in
```

No description given

### AddComponent
Return type: `ComponentID`

Signature:
```angelscript
const ActorHandle &in, IComponent@
```

No description given

### HasComponent
Return type: `bool`

Signature:
```angelscript
const ActorHandle &in, const string &in
```

No description given

### HasComponent
Return type: `bool`

Signature:
```angelscript
const ComponentID &in
```

No description given

### RemoveComponent
Return type: `bool`

Signature:
```angelscript
const ActorHandle &in, const string &in
```

No description given

### RemoveComponent
Return type: `bool`

Signature:
```angelscript
const ComponentID &in
```

No description given

### GetComponent
Return type: `ref`

Signature:
```angelscript
const ActorHandle &in, const string &in
```

No description given

### GetComponent
Return type: `ref`

Signature:
```angelscript
const ComponentID &in
```

No description given

### GetComponentName
Return type: `string`

Signature:
```angelscript
const ComponentID &in
```

No description given

### GetComponents
Return type: `array<ComponentID>@`

Signature:
```angelscript
const ActorHandle &in
```

No description given

### SaveWorld
Return type: `bool`

Signature:
```angelscript
const string &in
```

No description given

### LoadWorld
Return type: `LoadResult`

Signature:
```angelscript
const string &in
```

No description given

### RegisterSystem
Return type: `void`

Signature:
```angelscript
FrameUpdateType, ISystem@
```

No description given
