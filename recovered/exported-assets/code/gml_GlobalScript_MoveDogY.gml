function MoveDogY()
{
    key_cooldown = 0;
    alarm[1] = 2;
    block = 0;
    xprev = x;
    yprev = y;
    y += (16 * ymove);
    xx = x;
    if (ymove > 0)
    {
        sprite_index = sprDogDown;
        dir = 0;
    }
    else
    {
        sprite_index = sprDogUp;
        dir = 180;
    }
    TheBullshitDogCode();
}
