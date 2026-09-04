using System;
using System.IO;
using System.Linq;
using System.Text;
using UndertaleModLib.Models;

EnsureDataLoaded();

string outputPath = @"C:\Users\romeu\Documents\longo-doggo\recovered\analysis\room-tiles.txt";
string headerPath = @"C:\Users\romeu\Documents\longo-doggo\src\room_tiles.h";
string sourcePath = @"C:\Users\romeu\Documents\longo-doggo\src\room_tiles.c";
var report = new StringBuilder();
var generatedHeader = new StringBuilder();
var generatedSource = new StringBuilder();
var roomNames = new[] { "rm_tutorial", "rm_level1", "rm_level2", "rm_level3", "rm_level4", "rm_level5", "rm_level6", "rm_title_screen", "rm_credits", "rm_editor", "rm_levelbase" };

generatedHeader.AppendLine("#ifndef LONGO_DOGGO_ROOM_TILES_H");
generatedHeader.AppendLine("#define LONGO_DOGGO_ROOM_TILES_H");
generatedHeader.AppendLine();
generatedHeader.AppendLine("#include <stddef.h>");
generatedHeader.AppendLine();
generatedHeader.AppendLine("#define LONGO_ROOM_TILE_WIDTH 38");
generatedHeader.AppendLine("#define LONGO_ROOM_TILE_HEIGHT 26");
generatedHeader.AppendLine("#define LONGO_ROOM_TILE_COUNT 11");
generatedHeader.AppendLine();
generatedHeader.AppendLine("enum {");
for (int roomIndex = 0; roomIndex < roomNames.Length; roomIndex++)
    generatedHeader.AppendLine($"    LONGO_ROOM_{roomNames[roomIndex].Replace("rm_", "").ToUpperInvariant()}_INDEX = {roomIndex},");
generatedHeader.AppendLine("};");
generatedHeader.AppendLine();
generatedHeader.AppendLine("typedef enum LongoTileSet {");
generatedHeader.AppendLine("    LONGO_TILESET_TILESET1 = 0,");
generatedHeader.AppendLine("    LONGO_TILESET_GROUND = 1");
generatedHeader.AppendLine("} LongoTileSet;");
generatedHeader.AppendLine();
generatedHeader.AppendLine("typedef struct LongoTileLayer {");
generatedHeader.AppendLine("    const unsigned int *data;");
generatedHeader.AppendLine("    int width;");
generatedHeader.AppendLine("    int height;");
generatedHeader.AppendLine("    LongoTileSet tileset;");
generatedHeader.AppendLine("    int depth;");
generatedHeader.AppendLine("    int offset_x;");
generatedHeader.AppendLine("    int offset_y;");
generatedHeader.AppendLine("} LongoTileLayer;");
generatedHeader.AppendLine();
generatedHeader.AppendLine("typedef struct LongoRoomTileMap {");
generatedHeader.AppendLine("    const char *room_name;");
generatedHeader.AppendLine("    LongoTileLayer tiles_3;");
generatedHeader.AppendLine("    LongoTileLayer tiles_1;");
generatedHeader.AppendLine("    int background_depth;");
generatedHeader.AppendLine("    int shadows_depth;");
generatedHeader.AppendLine("    int flowers_depth;");
generatedHeader.AppendLine("    int instances_depth;");
generatedHeader.AppendLine("    int blocks_depth;");
generatedHeader.AppendLine("    int gui_depth;");
generatedHeader.AppendLine("} LongoRoomTileMap;");
generatedHeader.AppendLine();
generatedHeader.AppendLine("extern const LongoRoomTileMap longo_room_tile_maps[LONGO_ROOM_TILE_COUNT];");
generatedHeader.AppendLine();
generatedHeader.AppendLine("#endif");

generatedSource.AppendLine("#include \"room_tiles.h\"");
generatedSource.AppendLine();

int LayerDepth(UndertaleRoom room, string name)
{
    var layer = room.Layers.FirstOrDefault(l => l.LayerName?.Content == name);
    if (layer == null)
    {
        Console.WriteLine("Missing layer " + name + " in " + room.Name?.Content);
        return 0;
    }
    return layer.LayerDepth;
}

void AppendTileArray(StringBuilder output, string symbol, UndertaleRoom.Layer.LayerTilesData tiles)
{
    output.AppendLine($"static const unsigned int {symbol}[LONGO_ROOM_TILE_WIDTH * LONGO_ROOM_TILE_HEIGHT] = {{");
    for (int y = 0; y < (int)tiles.TilesY; y++)
    {
        output.Append("    ");
        for (int x = 0; x < (int)tiles.TilesX; x++)
        {
            output.Append($"{tiles.TileData[y][x]}u");
            if (x + 1 < (int)tiles.TilesX || y + 1 < (int)tiles.TilesY) output.Append(", ");
        }
        output.AppendLine();
    }
    output.AppendLine("};");
    output.AppendLine();
}

foreach (string roomName in roomNames)
{
    var room = Data.Rooms.First(r => r.Name?.Content == roomName);
    var tiles3 = room.Layers.First(l => l.LayerName?.Content == "Tiles_3").TilesData;
    var tiles1 = room.Layers.First(l => l.LayerName?.Content == "Tiles_1").TilesData;
    string symbol = roomName.Replace("rm_", "longo_");
    AppendTileArray(generatedSource, symbol + "_tiles_3", tiles3);
    AppendTileArray(generatedSource, symbol + "_tiles_1", tiles1);
}

generatedSource.AppendLine("const LongoRoomTileMap longo_room_tile_maps[LONGO_ROOM_TILE_COUNT] = {");
foreach (string roomName in roomNames)
{
    var room = Data.Rooms.First(r => r.Name?.Content == roomName);
    var tiles3Layer = room.Layers.First(l => l.LayerName?.Content == "Tiles_3");
    var tiles1Layer = room.Layers.First(l => l.LayerName?.Content == "Tiles_1");
    int backgroundDepth = LayerDepth(room, "Background");
    int shadowsDepth = LayerDepth(room, "Shadows");
    int flowersDepth = LayerDepth(room, "Flowers");
    int instancesDepth = LayerDepth(room, "Instances");
    int blocksDepth = LayerDepth(room, "Blocks");
    int guiDepth = LayerDepth(room, "GUI");
    string symbol = roomName.Replace("rm_", "longo_");
    generatedSource.AppendLine($"    {{ \"{roomName}\", {{ {symbol}_tiles_3, {tiles3Layer.TilesData.TilesX}, {tiles3Layer.TilesData.TilesY}, LONGO_TILESET_TILESET1, {tiles3Layer.LayerDepth}, {tiles3Layer.XOffset}, {tiles3Layer.YOffset} }}, {{ {symbol}_tiles_1, {tiles1Layer.TilesData.TilesX}, {tiles1Layer.TilesData.TilesY}, LONGO_TILESET_GROUND, {tiles1Layer.LayerDepth}, {tiles1Layer.XOffset}, {tiles1Layer.YOffset} }}, {backgroundDepth}, {shadowsDepth}, {flowersDepth}, {instancesDepth}, {blocksDepth}, {guiDepth} }},");
}
generatedSource.AppendLine("};");
report.AppendLine("Longo Doggo recovered room tile layers");
report.AppendLine("======================================");

foreach (var room in Data.Rooms)
{
    report.AppendLine($"ROOM {room.Name?.Content}");
    foreach (var layer in room.Layers.Where(l => l.LayerType == UndertaleRoom.LayerType.Tiles))
    {
        var tiles = layer.TilesData;
        string background = tiles?.Background?.Name?.Content ?? "<null>";
        report.AppendLine($"LAYER name={layer.LayerName?.Content} depth={layer.LayerDepth} offset=({layer.XOffset},{layer.YOffset}) background={background} size={tiles?.TilesX ?? 0}x{tiles?.TilesY ?? 0}");
        if (tiles?.TileData != null)
        {
            for (int y = 0; y < tiles.TilesY; y++)
            {
                var row = tiles.TileData[y];
                report.AppendLine("ROW " + string.Join(",", Enumerable.Range(0, (int)tiles.TilesX).Select(x => row[x].ToString())));
            }
        }
    }
    report.AppendLine();
}

File.WriteAllText(outputPath, report.ToString(), Encoding.UTF8);
File.WriteAllText(headerPath, generatedHeader.ToString(), Encoding.UTF8);
File.WriteAllText(sourcePath, generatedSource.ToString(), Encoding.UTF8);
ScriptMessage($"Wrote {outputPath}");
ScriptMessage($"Wrote {headerPath}");
ScriptMessage($"Wrote {sourcePath}");
