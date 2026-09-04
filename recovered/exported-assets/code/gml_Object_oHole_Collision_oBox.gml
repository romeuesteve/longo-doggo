if (image_index != 1)
{
    full = 1;
    repeat (7)
    {
        instance_create_depth(x + 4 + random(8), y + 4 + random(8), -200, oSmoke);
    }
    block = 0;
    sprite_index = -4;
    audio_play_sound(snd_poof, 0, false);
    with (other)
    {
        instance_destroy();
    }
}
