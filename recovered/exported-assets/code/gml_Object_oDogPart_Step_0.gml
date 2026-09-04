if (instance_exists(oDog) && follow != -4)
{
    if ((oDog.xmove != 0 || oDog.ymove != 0) && !oDog.block && (xprev != x || yprev != y) && !oDog.key_cooldown)
    {
        xprev = x;
        yprev = y;
        if (draw_legs)
        {
            legs_angle = 30;
            alarm[0] = 15;
        }
    }
    else
    {
        x = follow.xprev;
        y = follow.yprev;
    }
    if (!instance_exists(oTitle))
    {
        xx = lerp(xx, x, 0.2);
        yy = lerp(yy, y, 0.2);
    }
}
else
{
    instance_destroy();
}
