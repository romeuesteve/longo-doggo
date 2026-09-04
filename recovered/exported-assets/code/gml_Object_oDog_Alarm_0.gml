randomize();
if (random(1) < 0.2)
{
    audio_play_sound(snd_bark, 0, false);
    var bark = instance_create_depth(x - lengthdir_x(12, dir + 90), y - lengthdir_y(12, dir + 90), -500, oBark);
    bark.image_angle = dir + 180;
}
alarm[0] = 25;
