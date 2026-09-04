var camwidth = camera_get_view_width(view_camera[0]);
var camheight = camera_get_view_height(view_camera[0]);
var color = make_color_rgb(153, 108, 53);
var color_outline = make_color_rgb(107, 61, 49);
with (oDogPart)
{
    if (draw_legs)
    {
        var legs_wave = Wave(-legs_angle, legs_angle, 0.2, 0);
        var dir = (point_direction(xx, yy, follow.xx, follow.yy) - 90) + (180 * first);
        var leglength = 6 + (first * 3);
        draw_line_width_color(xx, yy, xx + lengthdir_x(leglength, -45 + legs_wave + dir), yy + lengthdir_y(leglength, -45 + legs_wave + dir), 2, color_outline, color);
        draw_circle_color(xx + lengthdir_x(leglength, -45 + legs_wave + dir), yy + lengthdir_y(leglength, -45 + legs_wave + dir), 3, color, color_outline, 0);
        draw_line_width_color(xx, yy, xx + lengthdir_x(leglength, 225 + legs_wave + dir), yy + lengthdir_y(leglength, 225 + legs_wave + dir), 2, color_outline, color);
        draw_circle_color(xx + lengthdir_x(leglength, 225 + legs_wave + dir), yy + lengthdir_y(leglength, 225 + legs_wave + dir), 3, color, color_outline, 0);
    }
    draw_circle_color(xx - 1, yy - 1, 5, color_outline, color_outline, 0);
    draw_line_width_color(xx - 1, yy - 1, follow.xx - 1, follow.yy - 1, 10, color_outline, color_outline);
    if (first)
    {
        draw_circle_color(oDog.xx, oDog.yy - 1, 5, color_outline, color_outline, 0);
    }
}
with (oDogPart)
{
    draw_circle_color(xx - 1, yy - 1, 4, color, color, 0);
    draw_line_width_color(xx - 1, yy - 1, follow.xx - 1, follow.yy - 1, 8, color, color);
    if (draw_legs && !first)
    {
        draw_sprite(sprDogTail, oFlower.image_index, xx - 1, yy - 3);
    }
}
draw_sprite(sprite_index, image_index, xx, yy);
