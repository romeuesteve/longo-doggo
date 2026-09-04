if (!instance_exists(oMouse))
{
    if (open)
    {
        image_xscale = lerp(image_xscale, 1.2, 0.1);
        image_yscale = lerp(image_yscale, 0.8, 0.1);
        x = xstart - ((sprite_width * (image_xscale - 1)) / 2);
        y = ystart - (sprite_height * (image_yscale - 1));
        if (image_xscale > 1.15)
        {
            repeat (7)
            {
                instance_create_depth(x + random(16), y + random(16), -1000, oSmoke);
            }
            audio_play_sound(snd_poof, 0, false);
            instance_destroy();
        }
    }
}
else if (global.playing)
{
    if (global.buttons == instance_number(oButton))
    {
        image_xscale = lerp(image_xscale, 1.2, 0.1);
        image_yscale = lerp(image_yscale, 0.8, 0.1);
        x = xstart - ((sprite_width * (image_xscale - 1)) / 2);
        y = ystart - (sprite_height * (image_yscale - 1));
        if (image_xscale > 1.15)
        {
            repeat (7)
            {
                instance_create_depth(x + random(16), y + random(16), -1000, oSmoke);
            }
            audio_play_sound(snd_poof, 0, false);
            instance_destroy();
        }
    }
}
draw_self();
if (global.buttons == instance_number(oButton))
{
    open = 1;
}
