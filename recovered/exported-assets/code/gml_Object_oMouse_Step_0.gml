x = mouse_x;
y = mouse_y;
xx = floor(x / 16) * 16;
yy = floor(y / 16) * 16;
if (mouse_check_button_pressed(mb_left))
{
    ins = instance_create_depth(xx, yy, 0, object[num][0]);
}
if (mouse_check_button_pressed(mb_right))
{
    ins = instance_place(x, y, all);
    if (ins != -4 && ins.object_index != oDogPart)
    {
        with (ins)
        {
            instance_destroy();
        }
    }
}
