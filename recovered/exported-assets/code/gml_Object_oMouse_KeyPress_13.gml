if (!global.playing)
{
    for (i = 0; i < ((room_width / 16) + 1); i++)
    {
        for (j = 0; j < ((room_height / 16) + 1); j++)
        {
            if (instance_place((i * 16) - 8, (j * 16) - 8, all) != -4 && instance_place((i * 16) - 8, (j * 16) - 8, all) != id)
            {
                level[i][j] = instance_place((i * 16) - 8, (j * 16) - 8, all).object_index;
            }
            else
            {
                level[i][j] = -4;
            }
        }
    }
}
else
{
    with (all)
    {
        if (id != oMouse.id)
        {
            instance_destroy();
        }
    }
    instance_create_depth(room_width + 8, 0, 0, oFlower);
    global.buttons = 0;
    for (i = 0; i < ((room_width / 16) + 1); i++)
    {
        for (j = 0; j < ((room_height / 16) + 1); j++)
        {
            if (level[i][j] != -4)
            {
                instance_create_depth((i * 16) - 16, (j * 16) - 16, 0, level[i][j]);
            }
        }
    }
}
global.playing = !global.playing;
