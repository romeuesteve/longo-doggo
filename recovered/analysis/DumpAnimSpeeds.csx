using System;
using System.IO;
using System.Text;
using System.Reflection;
using UndertaleModLib.Models;

EnsureDataLoaded();

string outputPath = @"C:\Users\romeu\Documents\longo-doggo\recovered\analysis\anim-speeds.txt";
var report = new StringBuilder();

var type = Data.Sprites[0].GetType();
report.AppendLine("UndertaleSprite animation-related properties:");
foreach (var p in type.GetProperties(BindingFlags.Public | BindingFlags.Instance))
{
    if (p.Name.ToLower().Contains("anim") || p.Name.ToLower().Contains("speed") || p.Name.ToLower().Contains("s2"))
        report.AppendLine($"  {p.PropertyType.Name} {p.Name}");
}

for (int i = 0; i < Data.Sprites.Count; i++)
{
    var s = Data.Sprites[i];
    string extra = "";
    foreach (var p in type.GetProperties(BindingFlags.Public | BindingFlags.Instance))
    {
        string ln = p.Name.ToLower();
        if (ln.Contains("anim") || ln.Contains("speed") || ln.Contains("s2"))
        {
            object v = null;
            try { v = p.GetValue(s); } catch {}
            extra += $" {p.Name}={v}";
        }
    }
    report.AppendLine($"SPRITE {i}: {s.Name?.Content} frames={s.Textures.Count}{extra}");
}

File.WriteAllText(outputPath, report.ToString(), Encoding.UTF8);
ScriptMessage($"Wrote {outputPath}");
