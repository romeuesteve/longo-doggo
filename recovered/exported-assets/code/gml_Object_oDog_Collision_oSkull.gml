if (length > 2)
{
    length--;
    repeat (7)
    {
        instance_create_depth(ins[length].x + random_range(-3, 3), ins[length].y + random_range(-3, 3), -1000, oSmoke);
    }
    one = instance_create_depth(x, y - 8, -1000, oOne);
    one.image_index = 1;
    with (ins[length])
    {
        instance_destroy();
    }
    ins[length - 1].block = 0;
    ins[length - 1].draw_legs = 1;
}
else
{
    instance_destroy();
}
audio_play_sound(snd_poof, 0, false);
with (other)
{
    instance_destroy();
}
