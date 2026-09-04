if (oGoalUp.remain <= 0)
{
    if (!instance_exists(oMouse))
    {
        oTransition.room_num++;
        oTransition.next_lvl = 1;
        oTransition.open_transition = 1;
        instance_destroy();
    }
    else
    {
        keyboard_key_press(vk_enter);
    }
}
