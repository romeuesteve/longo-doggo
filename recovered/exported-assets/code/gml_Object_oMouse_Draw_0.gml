draw_self();
if (mouse_wheel_up())
{
    if (num < 9)
    {
        num++;
    }
    else
    {
        num = 0;
    }
}
else if (mouse_wheel_down())
{
    if (num > 0)
    {
        num--;
    }
    else
    {
        num = 9;
    }
}
draw_sprite_ext(object[num][1], 0, xx, yy, 1, 1, 0, c_white, 0.75);
draw_set_color(c_red);
draw_set_alpha(0.4);
draw_set_halign(fa_left);
if (global.playing)
{
    draw_text(2, 8, "PLAYING");
}
else
{
    draw_text(2, 8, "PAUSED");
}
draw_set_alpha(1);
