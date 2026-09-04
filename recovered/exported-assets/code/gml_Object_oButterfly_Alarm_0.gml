if (distance_to_point(xstart, ystart) < 6)
{
    dir = random(360);
}
else
{
    dir = point_direction(x, y, xstart, ystart);
    hspd -= 0.003;
    vspd -= 0.003;
}
alarm[0] = random_range(10, 60);
