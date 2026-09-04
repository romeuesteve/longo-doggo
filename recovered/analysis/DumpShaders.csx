using System;
using System.IO;
using System.Text;
using UndertaleModLib.Models;

EnsureDataLoaded();

string outputPath = @"C:\Users\romeu\Documents\longo-doggo\recovered\analysis\shaders.txt";
var report = new StringBuilder();

foreach (var s in Data.Shaders)
{
    report.AppendLine($"===== SHADER {s.Name?.Content} type={s.Type} =====");
    if (s.GLSL != null)
    {
        report.AppendLine("--- GLSL VertexCode ---");
        report.AppendLine(s.GLSL.VertexCode?.Content ?? "<null>");
        report.AppendLine("--- GLSL FragmentCode ---");
        report.AppendLine(s.GLSL.FragmentCode?.Content ?? "<null>");
    }
    if (s.HLSL9 != null)
    {
        report.AppendLine("--- HLSL9 ---");
        report.AppendLine(s.HLSL9.Code?.Content ?? "<null>");
    }
    report.AppendLine();
}

File.WriteAllText(outputPath, report.ToString(), Encoding.UTF8);
ScriptMessage($"Wrote {outputPath}");
