#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("# Usage: %s noise.png\n", argv[0]);
        return 0;
    }

    int x, y, n;
    unsigned char *noise = stbi_load(argv[1], &x, &y, &n, 4);
    printf("# Loaded: %s [%ix%ix%i]\n", argv[1], x, y, n);

    // DO PROCESSING HERE

    stbi_image_free(noise);
    return 0;
}