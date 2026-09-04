var dark = 10;
var color2 = make_color_rgb(113 - dark, 153 - dark, 61 - dark);
var color = make_color_rgb(141 - dark, 199 - dark, 63 - dark);
if (open_transition)
{
    for (i = 0; i < (camera_get_view_height(view_camera[0]) / 64); i++)
    {
        draw_sprite_ext(sprTransition, 0, x, (i * 64) + 7, 1, 1, 0, color2, 1);
        draw_sprite_ext(sprTransition, 0, x, i * 64, 1, 1, 0, color, 1);
        draw_rectangle_color(x + 32, 0, camera_get_view_width(view_camera[0]), camera_get_view_height(view_camera[0]), color, color, color, color, 0);
    }
    if (x >= -20)
    {
        x = lerp(x, -31, 0.04);
    }
    else
    {
        if (!menu)
        {
            x = camera_get_view_width(view_camera[0]);
        }
        if (!menu)
        {
            close_transition = 1;
        }
        if (retry)
        {
            room_goto(room);
            retry = 0;
        }
        else if (next_lvl)
        {
            room_goto_next();
            next_lvl = 0;
        }
        if (!menu)
        {
            open_transition = 0;
        }
    }
    if (room_num <= 7 && menu == 0)
    {
        text_y = lerp(text_y, (camera_get_view_height(view_camera[0]) / 2) + 4, 0.05);
        draw_set_font(LongoFontBold);
        draw_set_color(c_green);
        draw_text(142, text_y + 2, "LEVEL " + string(room_num));
        draw_set_color(c_white);
        draw_text(140, text_y, "LEVEL " + string(room_num));
    }
    else if (menu)
    {
        text_y = lerp(text_y, (camera_get_view_height(view_camera[0]) / 2) + 4, 0.05);
        draw_set_font(LongoFontBold);
        draw_set_color(c_green);
        draw_text(142, (text_y * 0.2) + 2, "VOLUME <" + string(volume) + ">" + "\nCHOOSE LEVEL <" + string(room_num) + ">");
        draw_set_color(c_white);
        draw_text(140, text_y * 0.2, "VOLUME <" + string(volume) + ">" + "\nCHOOSE LEVEL <" + string(room_num) + ">");
    }
}
else if (close_transition)
{
    for (i = 0; i < (camera_get_view_height(view_camera[0]) / 64); i++)
    {
        draw_sprite_ext(sprTransition, 1, x, (i * 64) + 7, 1, 1, 0, color2, 1);
        draw_sprite_ext(sprTransition, 1, x, i * 64, 1, 1, 0, color, 1);
        draw_rectangle_color(0, 0, x + 32, camera_get_view_height(view_camera[0]), color, color, color, color, 0);
        if (x >= -63)
        {
            x = lerp(x, -64, 0.02);
        }
        else
        {
            close_transition = 0;
        }
    }
    if (room_num <= 7)
    {
        text_y = lerp(text_y, camera_get_view_height(view_camera[0]) + 16, 0.16);
        draw_set_font(LongoFontBold);
        draw_set_color(c_green);
        draw_text(142, text_y + 2, "LEVEL " + string(room_num));
        draw_set_color(c_white);
        draw_text(140, text_y, "LEVEL " + string(room_num));
    }
}
else
{
    x = camera_get_view_width(view_camera[0]);
    text_y = -16;
}
if (menu)
{
}
