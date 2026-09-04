if (instance_exists(oMouse))
{
    if (global.playing)
    {
        instance_create_depth(x + 8, y + 16, 0, oGoal);
        var win = instance_create_depth(x - 8, y + 16, 0, oWin);
        win.image_xscale = 2;
        instance_destroy();
    }
}
else
{
    instance_create_depth(x + 8, y + 16, 0, oGoal);
    var win = instance_create_depth(x - 8, y + 16, 0, oWin);
    win.image_xscale = 2;
    instance_destroy();
}
