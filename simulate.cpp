#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

typedef std::vector<std::pair<float, float>> NoiseVector;
typedef std::vector<std::array<float, 3>> PixelVector;
typedef std::array<float, 4> ParticleArray;
typedef std::vector<ParticleArray> ParticleVector;

class PRNG
{
  private:
    uint64_t mSeed;

  public:
    PRNG(uint64_t seed = 12118) : mSeed(seed == 0 ? 1 : seed){}; // seed CANNOT be 0!!
    float Extents(float extents)
    {
        mSeed ^= mSeed >> 12;
        mSeed ^= mSeed << 25;
        mSeed ^= mSeed >> 27;
        return -extents + float(mSeed * UINT64_C(2685821657736338717) * 5.4210105e-20) * (2 * extents);
    };
};

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        std::cout << "# Usage: " << argv[0] << " commands.txt noise.png output.png" << std::endl;
        return 0;
    }

    int width, height, channels;
    unsigned char *noiseTexture = stbi_load(argv[2], &width, &height, &channels, 4);
    printf("# Loaded: %s [%ix%ix%i]\n", argv[2], width, height, channels);

    NoiseVector noiseVector(width * height);
    for (int n = 0; n < (width * height); n++)
    {
        noiseVector[n] = {(float)(noiseTexture[n * channels] - 127) / 127.0f,
                          (float)(noiseTexture[n * channels + 1] - 127) / 127.0f};
    }
    stbi_image_free(noiseTexture);

    PixelVector hdr(width * height);

    const std::string commentString("#");
    const std::string particleString("PARTICLE");
    const std::string colourString("COLOUR");
    const std::string simulateString("SIMULATE");
    const std::string tonemapString("TONEMAP");

    std::ifstream commands(argv[1]);
    if (!commands.is_open())
    {
        std::cout << "# Failed to open: " << argv[1] << std::endl;
        return -1;
    }
    else
    {
        std::cout << "# Commands: " << argv[1] << std::endl;
    }

    int iterations, stepSampleRate;
    float damping, noisy, fuzz;
    float red, green, blue;
    PRNG prng;

    bool readComms = true;
    while (readComms)
    {
        std::string line;
        std::getline(commands, line);
        std::stringstream ss(line);
        std::string token;
        ss >> token;

        if (token.compare(commentString) == 0)
        {
            std::cout << line << std::endl;
            continue;
        }
        else if (token.compare(particleString) == 0)
        {
            float x, y, vx, vy;
            ss >> x >> y >> vx >> vy;
            size_t index = (int)x + (int)y * width;
            bool alive = true;

            for (int i = 0; alive && i < iterations; i++)
            {
                vx = vx * damping + (noiseVector[index].first * 4.0 * noisy) + prng.Extents(0.1) * fuzz;
                vy = vy * damping + (noiseVector[index].second * 4.0 * noisy) + prng.Extents(0.1) * fuzz;
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
                    hdr[index][0] += red;
                    hdr[index][1] += green;
                    hdr[index][2] += blue;
                }
            }
        }
        else if (token.compare(colourString) == 0)
        {
            ss >> red >> green >> blue;
        }
        else if (token.compare(simulateString) == 0)
        {
            ss >> iterations >> stepSampleRate >> damping >> noisy >> fuzz;
            prng = PRNG();
            std::cout << "# Simulating" << std::endl;
        }
        else if (token.compare(tonemapString) == 0)
        {
            float exposure;
            ss >> exposure;
            std::cout << "# Tonemapping" << std::endl;

            auto tonemap = [exposure](float n) {
                return (unsigned char)((1.0f - std::pow(2.0f, -n * 0.005f * exposure)) * 255);
            };

            std::unique_ptr<unsigned char[]> output(new unsigned char[width * height * channels]);

            for (int x = 0; x < width; x++)
            {
                for (int y = 0; y < height; y++)
                {
                    size_t index = (x + y * width);
                    output[index * channels] = tonemap(hdr[index][0]);
                    output[index * channels + 1] = tonemap(hdr[index][1]);
                    output[index * channels + 2] = tonemap(hdr[index][2]);
                    output[index * channels + 3] = 255;
                }
            }

            stbi_write_png(argv[3], width, height, channels, output.get(), width * channels);
            std::cout << "# Saved: " << argv[3] << std::endl;

            readComms = false;
        }
    }

    commands.close();
    return 0;
}
