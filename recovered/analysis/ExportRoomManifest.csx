using System;
using System.IO;
using System.Text;
using System.Linq;
using UndertaleModLib.Models;

EnsureDataLoaded();

string outputPath = @"C:\Users\romeu\Documents\longo-doggo\recovered\analysis\room-manifest.txt";
var report = new StringBuilder();

report.AppendLine("Longo Doggo recovered room manifest");
report.AppendLine("===================================");
report.AppendLine($"Rooms: {Data.Rooms.Count}");
report.AppendLine();

for (int roomIndex = 0; roomIndex < Data.Rooms.Count; roomIndex++)
{
    UndertaleRoom room = Data.Rooms[roomIndex];
    report.AppendLine($"ROOM {roomIndex}: {room.Name?.Content}");
    report.AppendLine($"  size={room.Width}x{room.Height} speed={room.Speed} flags={room.Flags}");
    report.AppendLine($"  backgroundColor={room.BackgroundColor} drawBackgroundColor={room.DrawBackgroundColor}");
    report.AppendLine($"  legacyObjects={room.GameObjects.Count} legacyTiles={room.Tiles.Count} layers={room.Layers.Count}");

    if (room.Views != null)
    {
        for (int viewIndex = 0; viewIndex < room.Views.Count; viewIndex++)
        {
            var view = room.Views[viewIndex];
            report.AppendLine($"  VIEW {viewIndex}: enabled={view.Enabled} view=({view.ViewX},{view.ViewY},{view.ViewWidth},{view.ViewHeight}) port=({view.PortX},{view.PortY},{view.PortWidth},{view.PortHeight})");
        }
    }

    if (room.Layers != null)
    {
        foreach (var layer in room.Layers)
        {
            report.AppendLine($"  LAYER id={layer.LayerId} depth={layer.LayerDepth} type={layer.LayerType} name={layer.LayerName?.Content}");
            if (layer.LayerType == UndertaleRoom.LayerType.Background && layer.BackgroundData != null)
            {
                var background = layer.BackgroundData;
                string backgroundSprite = background.Sprite?.Name?.Content ?? "<null>";
                report.AppendLine($"    BACKGROUND sprite={backgroundSprite} visible={background.Visible} foreground={background.Foreground} tiled=({background.TiledHorizontally},{background.TiledVertically}) stretch={background.Stretch} color={background.Color} frame={background.FirstFrame} animationSpeed={background.AnimationSpeed} animationSpeedType={background.AnimationSpeedType} offset=({background.XOffset},{background.YOffset})");
            }
        }
    }

    foreach (var instance in room.GameObjects.OrderBy(o => o.Y).ThenBy(o => o.X).ThenBy(o => o.InstanceID))
    {
        string objectName = instance.ObjectDefinition?.Name?.Content ?? "<null>";
        report.AppendLine($"  INSTANCE id={instance.InstanceID} object={objectName} x={instance.X} y={instance.Y} scale=({instance.ScaleX},{instance.ScaleY}) rotation={instance.Rotation} imageIndex={instance.ImageIndex} imageSpeed={instance.ImageSpeed} creation={instance.CreationCode?.Name?.Content ?? ""} preCreate={instance.PreCreateCode?.Name?.Content ?? ""}");
    }

    foreach (var tile in room.Tiles)
    {
        string tileName = tile.ObjectDefinition?.Name?.Content ?? "<null>";
        report.AppendLine($"  TILE id={tile.InstanceID} object={tileName} x={tile.X} y={tile.Y} src=({tile.SourceX},{tile.SourceY}) size={tile.Width}x{tile.Height} depth={tile.TileDepth} scale=({tile.ScaleX},{tile.ScaleY})");
    }

    report.AppendLine();
}

File.WriteAllText(outputPath, report.ToString(), Encoding.UTF8);
ScriptMessage($"Wrote {outputPath}");
