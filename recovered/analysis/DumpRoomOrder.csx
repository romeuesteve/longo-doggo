using System;
using System.IO;
using System.Text;
using System.Reflection;
using UndertaleModLib.Models;

EnsureDataLoaded();

string outputPath = @"C:\Users\romeu\Documents\longo-doggo\recovered\analysis\room-order.txt";
var report = new StringBuilder();

// find the type exposing RoomOrder
foreach (var asm in AppDomain.CurrentDomain.GetAssemblies())
{
    foreach (var t in asm.GetTypes())
    {
        if (t.GetProperty("RoomOrder") != null)
        {
            report.AppendLine($"RoomOrder found on: {t.FullName}");
            object container = null;
            if (t.IsInstanceOfType(Data)) container = Data;
            else
            {
                var p = Data.GetType().GetProperty("GeneralInfo");
                if (p != null) container = p.GetValue(Data);
            }
            if (container != null)
            {
                var prop = t.GetProperty("RoomOrder");
                var val = prop.GetValue(container) as System.Collections.IEnumerable;
                if (val != null)
                {
                    int i = 0;
                    foreach (var item in val)
                    {
                        // items are UndertaleRoom or wrapper with a Room/Data field
                        string name = "";
                        var pRoom = item.GetType().GetProperty("Room") ?? item.GetType().GetProperty("Target");
                        if (pRoom != null)
                        {
                            var r = pRoom.GetValue(item);
                            if (r is UndertaleRoom ur) name = ur.Name?.Content;
                            else name = r?.ToString();
                        }
                        else if (item is UndertaleRoom r2) name = r2.Name?.Content;
                        else name = item?.ToString();
                        report.AppendLine($"  ORDER {i}: {name}");
                        i++;
                    }
                }
            }
            else report.AppendLine("  (container not resolved on Data)");
        }
    }
}

File.WriteAllText(outputPath, report.ToString(), Encoding.UTF8);
ScriptMessage($"Wrote {outputPath}");
