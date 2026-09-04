function MoveDogX()
{
    key_cooldown = 0;
    alarm[1] = 2;
    block = 0;
    xprev = x;
    yprev = y;
    x += (16 * xmove);
    yy = y;
    if (xmove > 0)
    {
        sprite_index = sprDogRight;
        dir = 90;
    }
    else
    {
        sprite_index = sprDogLeft;
        dir = 270;
    }
    TheBullshitDogCode();
}
