# Cattle Engine - NO BULLSHIT

THIS IS NOT IN A USABLE STATE AT ALL 

Cattle engine (abbreviated to CE), is a general 2D game engine currently using Raylib (shall be moving to SDL3).
Its techincally possible to make a game in its current state, you use lua ([Docs for it](lua_docs.md)).

Licences for third party stuff can be found in [/thirdparty/README.md](third_party/README.md)

## Example of a lua game
```lua
Textures.Load("player", "textures/player.jpg") -- Load a texture
CE.SetGameState("Main_Menu") -- Set the game state

local playerPosX = 0
local playerPosY = 0

function PlayerUpdate(dt)
    if Input.IsKeyDown("LEFT") then
        playerPosX = playerPosX - 5
    end
    if Input.IsKeyDown("RIGHT") then
        playerPosX = playerPosX + 5
    end
    if Input.IsKeyDown("UP") then
        playerPosY = playerPosY - 5
    end
    if Input.IsKeyDown("DOWN") then
        playerPosY = playerPosY + 5
    end

    Textures.Draw("player", playerPosX, playerPosY)
end

Callbacks.OnUpdate(function(dt) -- Called every frame in all states
    Text.Draw("Press A to set game state to change gamestate to InGame,", 20, 05, 100, Color(0, 0, 0))
    Text.Draw("S to set it to Main_Menu", 20, 5, 120, Color(0, 0, 0))

    if Input.IsKeyPressed("A") then
        CE.SetGameState("InGame")
    end

    if Input.IsKeyPressed("S") then
        CE.SetGameState("Main_Menu")
    end

    Text.Draw("Current state: ", 20, 5, 160, Color(0, 0, 0))
end
)

Callbacks.On("InGame", "Draw", function(state, event) -- This runs when the game state is in 'InGame' every frame
  Text.Draw("Ingame", 20, 165, 160, Color(0,0,0,255))
  PlayerUpdate(Time.Delta())
end)

Callbacks.On("Main_Menu", "Draw", function(state, event) -- This runs when the game state is in 'Main_Menu' every frame
  Text.Draw("Main_Menu", 20, 165, 160, Color(0,0,0,255))
end)
```

## ROADMAP!

- [ ] Core stuff
- [ ] Lua API
- [ ] Make the plugin api
- [ ] Data & texture/audio compression
- [ ] Stabilise everything 