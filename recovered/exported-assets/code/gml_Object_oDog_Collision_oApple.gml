length++;
ins[length - 2].block = 1;
ins[length - 2].draw_legs = 0;
ins[length - 1] = instance_create_depth(ins[length - 2].xprev, ins[length - 2].yprev, 0, oDogPart);
ins[length - 1].follow = ins[length - 2];
ins[length - 1].block = 0;
ins[length - 1].draw_legs = 1;
repeat (7)
{
    instance_create_depth(ins[length - 1].x + random_range(-3, 3), ins[length - 1].y + random_range(-3, 3), -1000, oSmoke);
}
instance_create_depth(x, y - 8, -1000, oOne);
audio_play_sound(snd_poof, 0, false);
with (other)
{
    instance_destroy();
}
