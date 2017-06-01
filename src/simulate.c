#include "stb_image.h"
#include "stb_image_write.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

uint64_t PRNG_mSeed = 12118;
void PRNG_Reset() { PRNG_mSeed = 12118; }
float PRNG_Extents(float extents)
{
    PRNG_mSeed ^= PRNG_mSeed >> 12;
    PRNG_mSeed ^= PRNG_mSeed << 25;
    PRNG_mSeed ^= PRNG_mSeed >> 27;
    return -extents + (float)(PRNG_mSeed * UINT64_C(2685821657736338717) * 5.4210105e-20) * (2 * extents);
}

unsigned char tonemap(float n, float exposure)
{
    return (unsigned char)((1.0f - powf(2.0f, -n * 0.005f * exposure)) * 255);
}

typedef struct
{
    float u, v;
} Vector2;

typedef struct
{
    float x, y, z;
} Vector3;

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("# Usage: %s commands.txt noise.png output.png\n", argv[0]);
        return 0;
    }

    /* Load noise texture and process it into a normalised float vec2. */
    int width, height, channels;
    unsigned char *noiseTexture = stbi_load(argv[2], &width, &height, &channels, 4);
    printf("# Loaded: %s [%ix%ix%i]\n", argv[2], width, height, channels);

    Vector2 *noiseVector = (Vector2 *)malloc(sizeof(Vector2) * width * height);
    for (int n = 0; n < (width * height); n++)
    {
        noiseVector[n].u = (float)(noiseTexture[n * channels] - 127) / 127.0f;
        noiseVector[n].v = (float)(noiseTexture[n * channels + 1] - 127) / 127.0f;
    }
    stbi_image_free(noiseTexture);

    /* Parse the command stream. */
    const char particleLabel[] = "PARTICLE";
    const char colourLabel[] = "COLOUR";
    const char simulateLabel[] = "SIMULATE";
    const char tonemapLabel[] = "TONEMAP";

    FILE *commands = fopen(argv[1], "r");
    if (commands == NULL)
    {
        printf("# Failed to open: %s\n", argv[1]);
        return -1;
    }
    else
    {
        printf("# Commands: %s\n", argv[1]);
    }

    int iterations, stepSampleRate;
    float damping, noisy, fuzz;
    float red, green, blue;
    Vector3 *hdr = (Vector3 *)malloc(sizeof(Vector3) * width * height);

    char line[1024];

    while (fgets(line, sizeof(line), commands))
    {
        if (0 == strncmp(line, particleLabel, sizeof(particleLabel) - 1))
        {
            char *next = line + sizeof(particleLabel);
            float x = strtof(next, &next);
            float y = strtof(next, &next);
            float vx = strtof(next, &next);
            float vy = strtof(next, &next);
            // sscanf(line + sizeof(particleLabel), "%f %f %f %f", &x, &y, &vx, &vy);

            size_t index = (int)x + (int)y * width;
            bool alive = true;
            for (int i = 0; alive && i < iterations; i++)
            {
                vx = vx * damping + (noiseVector[index].u * 4.0 * noisy) + PRNG_Extents(0.1) * fuzz;
                vy = vy * damping + (noiseVector[index].v * 4.0 * noisy) + PRNG_Extents(0.1) * fuzz;
                float step = 1.0f / stepSampleRate;
                for (int j = 0; j < stepSampleRate; j++)
                {
                    x += vx * step;
                    y += vy * step;
                    if ((int)x < 0 || (int)x >= width || (int)y < 0 || (int)y >= height)
                    {
                        alive = false;
                        break;
                    }
                    index = (int)x + (int)y * width;
                    hdr[index].x += red;
                    hdr[index].y += green;
                    hdr[index].z += blue;
                }
            }
        }
        else if (0 == strncmp(line, colourLabel, sizeof(colourLabel) - 1))
        {
            sscanf(line + sizeof(colourLabel), "%f %f %f", &red, &green, &blue);
        }
        else if (0 == strncmp(line, simulateLabel, sizeof(simulateLabel) - 1))
        {
            sscanf(line + sizeof(simulateLabel), "%i %i %f %f %f", &iterations, &stepSampleRate, &damping, &noisy,
                   &fuzz);
            PRNG_Reset();
            printf("# Simulate: %i %i %f %f %f\n", iterations, stepSampleRate, damping, noisy, fuzz);
        }
        else if (0 == strncmp(line, tonemapLabel, sizeof(tonemapLabel) - 1))
        {
            float exposure;
            sscanf(line + sizeof(tonemapLabel), "%f", &exposure);
            printf("# Tonemap: %f\n", exposure);

            unsigned char *output = (unsigned char *)malloc(sizeof(unsigned char) * width * height * channels);
            for (int x = 0; x < width; x++)
            {
                for (int y = 0; y < height; y++)
                {
                    size_t index = (x + y * width);
                    output[index * channels] = tonemap(hdr[index].x, exposure);
                    output[index * channels + 1] = tonemap(hdr[index].y, exposure);
                    output[index * channels + 2] = tonemap(hdr[index].z, exposure);
                    output[index * channels + 3] = 255;
                }
            }
            stbi_write_png(argv[3], width, height, channels, output, width * channels);
            printf("# Saved: %s\n", argv[3]);
            free(output);
        }
    }

    fclose(commands);
    free(hdr);
    free(noiseVector);

    return 0;
}
