# Description
The signature required for a function subscribed is: ```const string& in state, const string& in event_name```

# CE::GameState

## Functions
### ChangeState
Return type: `void`

Signature:
```angelscript
const string& in state_name
```

Changes the current game state

### Emit
Return type: `void`

Signature:
```angelscript
const string& in event_name
```

Emits an event to happen in the current game state

### GetState
Return type: `string`

Gets the current state name

### IsState
Return type: `bool`

Signature:
```angelscript
const string& in state
```

Checks if the specified state is the current one.

### Subscribe
Return type: `int`

Signature:
```angelscript
const string& in state_name, const string& in event_name, StateEventCallback@ callback
```

Subscribes a script callback to an event in the specified game state. Returns an ID that can be used to unsubscribe.

### Unsubscribe
Return type: `void`

Signature:
```angelscript
int subscription_id
```

Unsubscribes a previously registered state event callback using its subscription ID.
