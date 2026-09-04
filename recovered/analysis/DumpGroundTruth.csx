using System;
using System.IO;
using System.Text;
using System.Linq;
using UndertaleModLib.Models;

EnsureDataLoaded();

string outputPath = @"C:\Users\romeu\Documents\longo-doggo\recovered\analysis\ground-truth.txt";
var report = new StringBuilder();

report.AppendLine("=== ROOM ORDER (data.win list order; index 0 = game start, room_goto_next follows this) ===");
for (int i = 0; i < Data.Rooms.Count; i++)
{
    var room = Data.Rooms[i];
    report.AppendLine($"ROOM {i}: {room.Name?.Content}");
}

report.AppendLine();
report.AppendLine("=== SPRITES (index: name frames WxH origin) ===");
for (int i = 0; i < Data.Sprites.Count; i++)
{
    var s = Data.Sprites[i];
    int fw = 0, fh = 0;
    if (s.Textures.Count > 0 && s.Textures[0]?.Texture != null)
    {
        fw = s.Textures[0].Texture.SourceWidth;
        fh = s.Textures[0].Texture.SourceHeight;
    }
    report.AppendLine($"SPRITE {i}: {s.Name?.Content} frames={s.Textures.Count} {s.Width}x{s.Height} origin=({s.OriginX},{s.OriginY})");
}

report.AppendLine();
report.AppendLine("=== OBJECTS (index: name sprite persistent visible depth collisionShape parent) ===");
for (int i = 0; i < Data.GameObjects.Count; i++)
{
    var o = Data.GameObjects[i];
    string parent = o.ParentId?.Name?.Content ?? "<none>";
    string sprite = o.Sprite?.Name?.Content ?? "<none>";
    report.AppendLine($"OBJECT {i}: {o.Name?.Content} sprite={sprite} persistent={o.Persistent} visible={o.Visible} depth={o.Depth} solid={o.Solid} parent={parent}");
}

report.AppendLine();
report.AppendLine("=== SOUNDS ===");
for (int i = 0; i < Data.Sounds.Count; i++)
{
    var s = Data.Sounds[i];
    report.AppendLine($"SOUND {i}: {s.Name?.Content} flags={s.Flags}");
}

report.AppendLine();
report.AppendLine("=== FONTS ===");
for (int i = 0; i < Data.Fonts.Count; i++)
{
    var f = Data.Fonts[i];
    report.AppendLine($"FONT {i}: {f.Name?.Content} size={f.EmSize} bold={f.Bold} italic={f.Italic} range=({f.RangeStart}-{f.RangeEnd})");
}

report.AppendLine();
report.AppendLine("=== GAME OPTIONS / GENERAL ===");
report.AppendLine($"generalinfo inferrer: {Data.GeneralInfo?.Name?.Content} major={Data.GeneralInfo?.Major} minor={Data.GeneralInfo?.Minor} build={Data.GeneralInfo?.Build}");
report.AppendLine($"DefaultWindowWidth={Data.GeneralInfo?.DefaultWindowWidth} DefaultWindowHeight={Data.GeneralInfo?.DefaultWindowHeight}");

File.WriteAllText(outputPath, report.ToString(), Encoding.UTF8);
ScriptMessage($"Wrote {outputPath}");
