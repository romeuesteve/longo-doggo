audio_play_sound(snd_placeholder, 0, true);
x = (camera_get_view_width(view_camera[0]) / 4) - 20;
y = (camera_get_view_height(view_camera[0]) / 4) - 20;
wave1 = 0;
wave2 = 0;
with (oDog)
{
    sprite_index = sprDogDown;
    dir = 0;
    xx = 168;
    yy = 168;
    x = 168;
    y = 168;
    ins[0].xx = xx;
    ins[0].yy = yy - 16;
    ins[1].xx = ins[0].xx - 16;
    ins[1].yy = ins[0].yy;
    ins[2].xx = ins[1].xx - 16;
    ins[2].yy = ins[1].yy;
    ins[3].xx = ins[2].xx;
    ins[3].yy = ins[2].yy + 16;
    ins[4].xx = ins[3].xx + 16;
    ins[4].yy = ins[3].yy;
    alarm[0] = 10;
}
