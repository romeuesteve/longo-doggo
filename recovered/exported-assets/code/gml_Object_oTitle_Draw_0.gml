wave1 = Wave(0, 8, 2, 0);
wave2 = Wave(0, 8, 2, 0.1);
draw_sprite_part(sprTitle, 0, 0, 0, 191, 64, x, y + wave1);
draw_sprite_part(sprTitle, 0, 0, 69, 191, 149, x, y + 48 + wave2 + 2);
if (keyboard_check_pressed(vk_anykey))
{
    oTransition.next_lvl = 1;
    oTransition.open_transition = 1;
}
draw_set_font(LongoFontBold);
draw_set_color(make_color_rgb(51, 17, 0));
draw_text(151, 188 + (wave1 * 0.5), "Press Any Key to Start");
draw_set_color(make_color_rgb(255, 235, 204));
draw_text(150, 188 + (wave1 * 0.5), "Press Any Key to Start");
