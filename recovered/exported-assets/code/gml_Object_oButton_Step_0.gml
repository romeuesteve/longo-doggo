var col = collision_rectangle(x, y, x + 15, y + 15, oBlock, 0, 1);
var stupidcol = collision_rectangle(x, y, x + 15, y + 15, oStupidBlock, 0, 1);
if (col != -4 || stupidcol != -4)
{
    if (!pressed)
    {
        audio_play_sound(snd_button, 0, false);
        global.buttons++;
    }
    pressed = 1;
}
else
{
    if (pressed)
    {
        audio_play_sound(snd_wrong, 0, false);
        global.buttons--;
    }
    pressed = 0;
}
if (pressed)
{
    sprite = 26;
}
else
{
    sprite = 2;
}
