var wave = Wave(0, 2, 2, 0);
if ((keyboard_check_pressed(vk_space) || keyboard_check_pressed(vk_enter) || keyboard_check_pressed(ord("E"))) && !oTransition.close_transition)
{
    image_xscale = 0.5;
    image_yscale = 0.5;
    if (i < num)
    {
        i++;
    }
    else
    {
        oDog.play = 1;
        xscale = 0.6;
        yscale = 0.6;
    }
}
var color = make_color_rgb(84, 64, 32);
var color2 = make_color_rgb(255, 196, 101);
draw_set_halign(fa_center);
image_xscale = lerp(image_xscale, xscale, 0.15);
image_yscale = lerp(image_yscale, yscale, 0.15);
draw_sprite_ext(sprite_index, 0, dbox[i][0], dbox[i][1] + wave, image_xscale, image_yscale, 0, c_white, 1);
draw_set_font(LongoFont);
draw_set_color(color2);
draw_text_ext_transformed(dbox[i][0] + 0.5, dbox[i][1] + wave + 0.5, dbox[i][2], 12, 30 * xscale, image_yscale * 0.3, image_yscale * 0.3, 0);
draw_set_color(color);
draw_text_ext_transformed(dbox[i][0], dbox[i][1] + wave, dbox[i][2], 12, 30 * xscale, image_yscale * 0.3, image_yscale * 0.3, 0);
if (image_yscale < 0.65 && i >= num)
{
    instance_destroy();
}
