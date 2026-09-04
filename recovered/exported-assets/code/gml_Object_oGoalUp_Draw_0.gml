draw_sprite_part_ext(sprHouse, oGoal.image_index, 0, 0, 64, 44, oGoal.x - (32 * oGoal.image_xscale), oGoal.y - (64 * oGoal.image_yscale), oGoal.image_xscale, oGoal.image_yscale, c_white, 1);
var goal_length = 2;
if (instance_exists(oDog))
{
    remain = oDog.length - goal_length;
}
draw_set_font(FontDigits);
draw_set_halign(fa_center);
draw_set_valign(fa_middle);
if (remain > 0)
{
    can_play_sound = 1;
    draw_set_color(c_maroon);
    draw_text(oGoal.x + 1, ((oGoal.y + 1) - 32) + Wave(-count2 / 50, count2 / 50, 0.35, 0), remain);
    draw_set_color(c_red);
    draw_text(oGoal.x, ((oGoal.y + 1) - 32) + Wave(-count2 / 50, count2 / 50, 0.35, 0), remain);
    oGoal.image_index = 0;
    count = 200;
    if (count2 > 0)
    {
        if (count2 > 160)
        {
            instance_create_depth(oGoal.x + random_range(-4, 4), (oGoal.y - 8) + random_range(-4, 4), -1000, oSmoke);
        }
        count2 -= 4;
        oGoal.image_xscale = 1 + Wave(-count2 / 1000, count2 / 1000, 0.35, 0);
        oGoal.image_yscale = 1 - Wave(-count2 / 1000, count2 / 1000, 0.35, 0);
    }
}
else
{
    if (can_play_sound)
    {
        audio_play_sound(snd_win, 0, false);
        can_play_sound = 0;
    }
    if (count > 0)
    {
        if (count > 160)
        {
            instance_create_depth(oGoal.x + random_range(-4, 4), (oGoal.y - 8) + random_range(-4, 4), -1000, oSmoke);
        }
        count -= 4;
        oGoal.image_xscale = 1 + Wave(-count / 1000, count / 1000, 0.35, 0);
        oGoal.image_yscale = 1 - Wave(-count / 1000, count / 1000, 0.35, 0);
    }
    oGoal.image_index = 1;
    count2 = 200;
}
