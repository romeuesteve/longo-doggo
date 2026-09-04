y = lerp(y, ystart - 16, 0.1);
image_alpha -= 0.02;
if (image_alpha <= 0.1)
{
    instance_destroy();
}
