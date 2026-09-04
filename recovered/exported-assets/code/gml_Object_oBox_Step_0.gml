x = lerp(x, xx, 0.25);
y = lerp(y, yy, 0.25);
depth = -100 - (y / 6);
var fix = 8;
if (collision_line(x + fix, y + fix, x + 16 + fix, y + fix, oDog, false, true) && place_meeting(x - 10, y, oBlock))
{
    if (instance_place(x - 10, y, oBlock) == oDog.ins[oDog.length - 1] || place_meeting(x - 10, y, oHole))
    {
        block = 0;
    }
    else
    {
        block = 1;
    }
}
else if (collision_line(x + fix, y + fix, (x - 16) + fix, y + fix, oDog, false, true) && place_meeting(x + 10, y, oBlock))
{
    if (instance_place(x + 10, y, oBlock) == oDog.ins[oDog.length - 1] || place_meeting(x + 10, y, oHole))
    {
        block = 0;
    }
    else
    {
        block = 1;
    }
}
else if (collision_line(x + fix, y + fix, x + fix, y + 16 + fix, oDog, false, true) && place_meeting(x, y - 10, oBlock))
{
    if (instance_place(x, y - 10, oBlock) == oDog.ins[oDog.length - 1] || place_meeting(x, y - 10, oHole))
    {
        block = 0;
    }
    else
    {
        block = 1;
    }
}
else if (collision_line(x + fix, y + fix, x + fix, (y - 16) + fix, oDog, false, true) && place_meeting(x, y + 10, oBlock))
{
    if (instance_place(x, y + 10, oBlock) == oDog.ins[oDog.length - 1] || place_meeting(x, y + 10, oHole))
    {
        block = 0;
    }
    else
    {
        block = 1;
    }
}
else
{
    block = 0;
}
