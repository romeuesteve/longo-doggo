using System;
using System.Reflection;

EnsureDataLoaded();

string Value(object instance, string name)
{
    if (instance == null) return "<null>";
    Type type = instance.GetType();
    PropertyInfo property = type.GetProperty(name,
        BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);
    if (property != null)
    {
        object value = property.GetValue(instance);
        return value == null ? "<null>" : value.ToString();
    }
    FieldInfo field = type.GetField(name,
        BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);
    if (field != null)
    {
        object value = field.GetValue(instance);
        return value == null ? "<null>" : value.ToString();
    }
    return "<missing>";
}

string[] names = {
    "sprDogDown", "sprDogLeft", "sprDogRight", "sprDogUp",
    "sprDogPaw", "sprDogTail", "sprPear", "sprSkull", "sprDialogueBox",
    "sprApple", "sprHouse", "sprDoor", "sprSmoke", "sprBox",
    "sprButton", "sprButtonPressed", "sprHole", "sprFly", "sprFlower",
    "sprTitle", "sprTransition", "sprOldDog", "sprBark", "sprOne", "sprBlock"
};

foreach (string name in names)
{
    var sprite = Data.Sprites.ByName(name);
    Console.WriteLine(string.Join(";", new [] {
        name,
        "OriginX=" + Value(sprite, "OriginX"),
        "OriginY=" + Value(sprite, "OriginY"),
        "Width=" + Value(sprite, "Width"),
        "Height=" + Value(sprite, "Height"),
        "Speed=" + Value(sprite, "PlaybackSpeed"),
        "SpeedType=" + Value(sprite, "SpeedType"),
        "Frames=" + Value(sprite, "Textures")
    }));
}

string[] objectNames = { "oSkull", "oApple", "oPear", "oDog", "oDogPart",
                         "oDialogueBox", "oTutorial", "oBark", "oButterfly",
                         "oFlower", "oSmoke", "oGoal", "oGoalUp", "oBox",
                         "oButton", "oDoor", "oHole", "oTitle", "oTransition" };
foreach (string name in objectNames)
{
    var obj = Data.GameObjects.ByName(name);
    Console.WriteLine("OBJECT;" + name + ";Sprite=" +
                      (obj?.Sprite?.Name?.Content ?? "<null>"));
}
