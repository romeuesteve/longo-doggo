dir = 180;
play = 1;
lengthstart = 5;
length = 5;
xx = x;
yy = y;
key_cooldown = 1;
xprev = x;
yprev = y + 16;
xmove = 0;
ymove = 0;
global.shadow_surf = -1;
global.buttons = 0;
surface_resize(application_surface, camera_get_view_width(view_camera[0]), camera_get_view_height(view_camera[0]));
block = 0;
for (i = 0; i < 60; i++)
{
    ins[i] = -1;
}
for (i = 0; i < length; i++)
{
    ins[i] = instance_create_depth(xx, yy + (16 * (i + 1)), 0, oDogPart);
    if (i == 0)
    {
        ins[i].follow = id;
        ins[i].first = 1;
        ins[i].draw_legs = 1;
    }
    else
    {
        ins[i].follow = ins[i - 1];
        if (i >= (length - 1))
        {
            ins[i].block = 0;
            ins[i].draw_legs = 1;
        }
    }
}
instance_create_depth(x, y, 0, oStupidBlock);
depth = 0;
