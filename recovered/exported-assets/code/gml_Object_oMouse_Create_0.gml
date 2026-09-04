num = 0;
xx = 0;
yy = 0;
for (i = 0; i < 10; i++)
{
    for (j = 0; j < 10; j++)
    {
        object[i][j] = 0;
    }
}
object[0][1] = 13;
object[1][1] = 23;
object[2][1] = 10;
object[3][1] = 11;
object[4][1] = 0;
object[5][1] = 2;
object[6][1] = 7;
object[7][1] = 28;
object[8][1] = 1;
object[9][1] = 25;
object[0][0] = 23;
object[1][0] = 7;
object[2][0] = 1;
object[3][0] = 24;
object[4][0] = 4;
object[5][0] = 5;
object[6][0] = 3;
object[7][0] = 19;
object[8][0] = 10;
object[9][0] = 6;
global.playing = 0;
for (i = 0; i < (room_width / 16); i++)
{
    for (j = 0; j < (room_height / 16); j++)
    {
        level[i][j] = 0;
    }
}
