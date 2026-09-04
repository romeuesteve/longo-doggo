vspd = clamp(vspd, -0.3, 0.3);
hspd = clamp(hspd, -0.3, 0.3);
if (instance_exists(oMouse))
{
    if (global.playing)
    {
        hspd += lengthdir_x(0.001, dir);
        vspd += lengthdir_y(0.001, dir);
        x += hspd;
        y += vspd;
    }
}
else
{
    hspd += lengthdir_x(0.001, dir);
    vspd += lengthdir_y(0.001, dir);
    x += hspd;
    y += vspd;
}
if (distance_to_object(oDog) < 10 && keyboard_check_pressed(vk_space))
{
    dir = point_direction(oDog.x, oDog.y, x + 8, y + 8);
    hspd = lengthdir_x(0.3, dir);
    vspd = lengthdir_y(0.3, dir);
}
