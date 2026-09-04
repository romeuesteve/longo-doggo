if (play)
{
    xmove = sign(((keyboard_check_pressed(vk_right) - keyboard_check_pressed(vk_left)) + keyboard_check_pressed(ord("D"))) - keyboard_check_pressed(ord("A")));
    ymove = sign((-keyboard_check_pressed(vk_up) + keyboard_check_pressed(vk_down) + keyboard_check_pressed(ord("S"))) - keyboard_check_pressed(ord("W")));
    if (keyboard_check_pressed(vk_space))
    {
        audio_play_sound(snd_bark, 0, false);
        var bark = instance_create_depth(xx - lengthdir_x(12, dir + 90), yy - lengthdir_y(12, dir + 90), -500, oBark);
        bark.image_angle = dir + 180;
    }
    if (keyboard_check_pressed(ord("R")) && !oTransition.close_transition)
    {
        if (!instance_exists(oTransition))
        {
            instance_create_depth(0, 0, 0, oTransition);
        }
        oTransition.retry = 1;
        oTransition.open_transition = 1;
    }
}
if (xmove != 0 && key_cooldown)
{
    with (oBox)
    {
        x = xx;
        y = yy;
    }
    if (place_meeting(x + (10 * xmove), y, oBlock))
    {
        if (instance_place(x + (10 * xmove), y, oBlock).block)
        {
            block = 1;
        }
        else
        {
            if (instance_place(x + (8 * xmove), y, oBlock).push)
            {
                instance_place(x + (8 * xmove), y, oBlock).xx += xmove * 16;
                audio_play_sound(snd_pushed, 0, false);
            }
            MoveDogX();
        }
    }
    else
    {
        MoveDogX();
    }
}
else if (ymove != 0 && key_cooldown)
{
    with (oBox)
    {
        x = xx;
        y = yy;
    }
    if (place_meeting(x, y + (10 * ymove), oBlock))
    {
        if (instance_place(x, y + (10 * ymove), oBlock).block)
        {
            block = 1;
        }
        else
        {
            if (instance_place(x, y + (8 * ymove), oBlock).push)
            {
                instance_place(x, y + (8 * ymove), oBlock).yy += ymove * 16;
                audio_play_sound(snd_pushed, 0, false);
            }
            MoveDogY();
        }
    }
    else
    {
        MoveDogY();
    }
}
if (!instance_exists(oTitle))
{
    xx = lerp(xx, x, 0.2);
    yy = lerp(yy, y, 0.2);
}
else
{
    play = 0;
}
image_index = oFlower.image_index;
