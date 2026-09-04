color = make_color_rgb(255, 235, 204);
color2 = make_color_rgb(235, 176, 81);
image_angle += angle;
draw_sprite_ext(sprite_index, 0, x - 1, y, image_xscale, image_yscale, image_angle, color2, 1);
draw_sprite_ext(sprite_index, 0, x + 1, y, image_xscale, image_yscale, image_angle, color2, 1);
draw_sprite_ext(sprite_index, 0, x, y - 1, image_xscale, image_yscale, image_angle, color2, 1);
draw_sprite_ext(sprite_index, 0, x, y + 1, image_xscale, image_yscale, image_angle, color2, 1);
draw_sprite_ext(sprite_index, 0, x, y, image_xscale, image_yscale, image_angle, color, 1);
image_xscale = lerp(image_xscale, 0, 0.04);
image_yscale = lerp(image_yscale, 0, 0.04);
speed = lerp(speed, 0, 0.02);
if (image_xscale < 0.05)
{
    instance_destroy();
}
