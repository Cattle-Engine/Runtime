Log.Info("This is an info message")
Log.Debug("This is a debug message")
Log.Warn("This is a warning message")
Log.Error("This is an error message")

Log.Info("Trying to load music")
Audio.Music.Load("music.mp3", "TestMusic")
Log.Info("Loaded music!")
Log.Info("Trying to play music")
Audio.Music.Play("TestMusic")
Log.Info("MUSIC WAS PLAYED!")

function Update()
Textures.Draw("LOL_FROM_LUA_NO_EXISTING_TEXTURE", 100, 200, Color(255, 100, 50, 200))
end
