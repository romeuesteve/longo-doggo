var camwidth = camera_get_view_width(view_camera[0]);
var camheight = camera_get_view_height(view_camera[0]);
if (!surface_exists(global.shadow_surf))
{
    global.shadow_surf = surface_create(camwidth, camheight);
}
else
{
    surface_set_target(global.shadow_surf);
    draw_clear_alpha(c_black, 0);
    with (oDogPart)
    {
        if (draw_legs)
        {
            var legs_wave = Wave(-legs_angle, legs_angle, 0.2, 0);
            var dir = (point_direction(xx, yy, follow.xx, follow.yy) - 90) + (180 * first);
            var leglength = 6 + (first * 3);
            draw_line_width_color(xx, yy + 5, xx + lengthdir_x(leglength, -45 + legs_wave + dir), yy + 5 + lengthdir_y(leglength, -45 + legs_wave + dir), 2, c_black, c_black);
            draw_circle_color(xx + lengthdir_x(leglength, -45 + legs_wave + dir), yy + 5 + lengthdir_y(leglength, -45 + legs_wave + dir), 3, c_black, c_black, 0);
            draw_line_width_color(xx, yy + 5, xx + lengthdir_x(leglength, 225 + legs_wave + dir), yy + 5 + lengthdir_y(leglength, 225 + legs_wave + dir), 2, c_black, c_black);
            draw_circle_color(xx + lengthdir_x(leglength, 225 + legs_wave + dir), yy + 5 + lengthdir_y(leglength, 225 + legs_wave + dir), 3, c_black, c_black, 0);
        }
        draw_circle_color(xx - 1, (yy + 5) - 1, 5, c_black, c_black, 0);
        draw_line_width_color(xx - 1, (yy - 1) + 5, follow.xx - 1, (follow.yy + 5) - 1, 10, c_black, c_black);
    }
    with (oDog)
    {
        draw_sprite_ext(sprite_index, image_index, xx, yy + 5, 1, 1, 0, c_black, 1);
    }
    with (oApple)
    {
        draw_sprite_ext(sprite_index, image_index, x, y + 7, 1, 0.6, 0, c_black, 1);
    }
    with (oSkull)
    {
        draw_sprite_ext(sprite_index, image_index, x, y + 7, 1, 0.6, 0, c_black, 1);
    }
    with (oButterfly)
    {
        draw_sprite_ext(sprFly, oApple.image_index, x, y + 16, 1, 0.6, 0, c_black, 1);
    }
    with (oGoal)
    {
        draw_sprite_ext(sprite_index, image_index, x, y + 4, 1, 0.5, 0, c_black, 1);
    }
    with (oBox)
    {
        draw_sprite_ext(sprite_index, 0, x, y + 5, 1, 1, 0, c_black, 1);
    }
    with (oDoor)
    {
        draw_sprite_ext(sprite_index, 0, x, y + 22, 1, -0.4, 0, c_black, 1);
    }
    with (oButton)
    {
        draw_sprite_ext(sprite_index, 0, x, y + 4, 1, 1, 0, c_black, 1);
    }
    with (oTitle)
    {
        draw_sprite_part_ext(sprTitle, 0, 0, 0, 191, 64, x, y + wave1 + 85, 1, 0.5, c_black, 1);
        draw_sprite_part_ext(sprTitle, 0, 0, 69, 191, 149, x, y + 48 + wave2 + 52, 1, 0.5, c_black, 1);
    }
    surface_reset_target();
}
draw_set_alpha(0.2);
if (instance_exists(oDog))
{
    draw_surface(global.shadow_surf, 0, 0);
}
draw_set_alpha(1);
