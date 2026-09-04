i = 0;
num = 6;
if (room != rm_credits)
{
    xscale = 4;
    yscale = 2;
    oDog.play = 0;
}
else
{
    xscale = 10;
    yscale = 2;
}
if (room == rm_tutorial)
{
    dbox[0][0] = 103;
    dbox[0][1] = 80;
    dbox[0][2] = "This is Longo Doggo";
    dbox[1][0] = 136;
    dbox[1][1] = 56;
    dbox[1][2] = "He wants to enter his house, but he's too long so there's no room for him";
    dbox[2][0] = 198;
    dbox[2][1] = 119;
    dbox[2][2] = "Apples make Longo Doggo longer";
    dbox[3][0] = oSkull.x - 8;
    dbox[3][1] = 119;
    dbox[3][2] = "Instead, pears make him shorter";
    dbox[4][0] = 144;
    dbox[4][1] = 56;
    dbox[4][2] = "The house number shows how many length units you must lose";
    dbox[5][0] = 103;
    dbox[5][1] = 80;
    dbox[5][2] = "If you get stuck press 'R' to retry";
    dbox[6][0] = 103;
    dbox[6][1] = 80;
    dbox[6][2] = "Control Longo Doggo with the arrow keys";
    num = 6;
}
else if (room == rm_level6)
{
    dbox[0][0] = 247;
    dbox[0][1] = 51;
    dbox[0][2] = "This is a door";
    dbox[1][0] = 174;
    dbox[1][1] = 120;
    dbox[1][2] = "It only opens once all the buttons are pressed simultaneously";
    dbox[2][0] = 102;
    dbox[2][1] = 65;
    dbox[2][2] = "Buttons can be pressed either by Longo Doggo or boxes, which you can push on the sides";
    num = 2;
}
else if (room == rm_level4)
{
    dbox[0][0] = 119;
    dbox[0][1] = 37;
    dbox[0][2] = "Holes will prevent you from advancing unless you fill them with something";
    num = 0;
}
else if (room == rm_credits)
{
    dbox[0][0] = 151;
    dbox[0][1] = 37;
    dbox[0][2] = "Thank you for playing! \n \n Game made by Romeu Esteve (@Romeuski) for the 'Tu juego a juicio Jam 2021' \n Using Game Maker Studio 2, freesound.org and Ableton Live 10";
    dbox[1][0] = 151;
    dbox[1][1] = 37;
    dbox[1][2] = "If you enjoyed the experience please leave a comment in the itch.io page, I love feedback!";
    num = 1;
}
