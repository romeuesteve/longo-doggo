using System;
using System.IO;
using System.Text;
using System.Reflection;
using UndertaleModLib.Models;

EnsureDataLoaded();

string outputPath = @"C:\Users\romeu\Documents\longo-doggo\recovered\analysis\room-cc.txt";
var report = new StringBuilder();

var roomType = Data.Rooms[0].GetType();
report.AppendLine("UndertaleRoom properties:");
foreach (var p in roomType.GetProperties(BindingFlags.Public | BindingFlags.Instance))
    report.AppendLine($"  {p.PropertyType.Name} {p.Name}");

foreach (var room in Data.Rooms)
{
    report.AppendLine();
    report.AppendLine($"ROOM {room.Name?.Content}:");
    foreach (var p in roomType.GetProperties(BindingFlags.Public | BindingFlags.Instance))
    {
        if (p.Name.Contains("Creation") || p.Name.Contains("Code"))
        {
            object val = null;
            try { val = p.GetValue(room); } catch {}
            report.AppendLine($"  {p.Name} = {(val?.ToString() ?? "<null>")}");
        }
    }
}

// dump any code entries that look like room creation code
report.AppendLine();
report.AppendLine("Code entries matching room/creation patterns:");
foreach (var c in Data.Code)
{
    string n = c?.Name?.Content ?? "";
    if (n.IndexOf("RoomCC", StringComparison.OrdinalIgnoreCase) >= 0 || n.IndexOf("gml_Room", StringComparison.OrdinalIgnoreCase) >= 0)
        report.AppendLine($"  {n}");
    else if (n.StartsWith("gml_") && n.IndexOf("Create", StringComparison.OrdinalIgnoreCase) >= 0 && n.IndexOf("Object") < 0)
        report.AppendLine($"  {n}");
}

File.WriteAllText(outputPath, report.ToString(), Encoding.UTF8);
ScriptMessage($"Wrote {outputPath}");
