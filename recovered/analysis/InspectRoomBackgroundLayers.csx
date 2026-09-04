using System;
using System.Collections;
using System.Linq;
using System.Reflection;
using UndertaleModLib.Models;

EnsureDataLoaded();

string Text(object value)
{
    if (value == null) return "<null>";
    if (value is UndertaleNamedResource named) return named.Name?.Content ?? "<unnamed>";
    return value.ToString();
}

void PrintMembers(object value, string indent)
{
    if (value == null) return;
    Type type = value.GetType();
    Console.WriteLine(indent + "TYPE=" + type.FullName);

    foreach (PropertyInfo property in type.GetProperties(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance))
    {
        if (property.GetIndexParameters().Length != 0) continue;
        try
        {
            object member = property.GetValue(value);
            Console.WriteLine(indent + property.Name + "=" + Text(member));
        }
        catch (Exception error)
        {
            Console.WriteLine(indent + property.Name + "=<error:" + error.GetType().Name + ">");
        }
    }
}

foreach (var background in Data.Backgrounds)
{
    Console.WriteLine("BACKGROUND_RESOURCE name=" + background.Name?.Content + " type=" + background.GetType().FullName);
    PrintMembers(background, "  ");
}

foreach (var room in Data.Rooms)
{
    foreach (var layer in room.Layers.Where(l => l.LayerType == UndertaleRoom.LayerType.Background || l.LayerName?.Content == "Background"))
    {
        Console.WriteLine("ROOM=" + room.Name?.Content + " LAYER=" + layer.LayerName?.Content + " id=" + layer.LayerId + " depth=" + layer.LayerDepth);
        PrintMembers(layer, "  ");
        Console.WriteLine("  BACKGROUND_DATA");
        PrintMembers(layer.BackgroundData, "    ");
        Console.WriteLine();
    }
}
