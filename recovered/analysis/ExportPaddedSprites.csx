using System;
using System.IO;
using UndertaleModLib.Util;

EnsureDataLoaded();

string outputRoot = Path.GetFullPath(Path.Combine(
    Environment.CurrentDirectory, ".scratch", "padded-assets"));
string[] names = {
    "sprDogDown", "sprDogLeft", "sprDogRight", "sprDogUp",
    "sprDogPaw", "sprDogTail", "sprPear", "sprSkull", "sprDialogueBox",
    "sprApple", "sprHouse", "sprDoor", "sprSmoke", "sprFly", "sprFlower",
    "sprButton", "sprHole", "sprHouse", "sprTitle", "sprTransition",
    "sprOldDog", "sprBark", "sprOne", "sprBlock"
};

foreach (string name in names)
{
    var sprite = Data.Sprites.ByName(name);
    if (sprite == null || sprite.Textures == null) continue;
    string folder = Path.Combine(outputRoot, name);
    Directory.CreateDirectory(folder);
    for (int i = 0; i < sprite.Textures.Count; ++i)
    {
        if (sprite.Textures[i]?.Texture == null) continue;
        string path = Path.Combine(folder, name + "_" + i + ".png");
        using (TextureWorker worker = new TextureWorker())
        {
            worker.ExportAsPNG(sprite.Textures[i].Texture, path, null, true);
        }
    }
}
