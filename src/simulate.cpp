#include "stb_image.h"
#include "stb_image_write.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <atomic>
#include <thread>

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

    std::vector<std::pair<float, float>> noiseVector(width * height);
    for (int n = 0; n < (width * height); n++)
    {
        noiseVector[n] = {(float)(noiseTexture[n * channels] - 127) / 127.0f,
                          (float)(noiseTexture[n * channels + 1] - 127) / 127.0f};
    }
    stbi_image_free(noiseTexture);

    std::vector<std::array<float, 3>> hdr(width * height);

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

    std::string token;
    while (commands >> token)
    {
        if (token.compare(particleString) == 0)
        {
            float x, y, vx, vy;
            commands >> x >> y >> vx >> vy;
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
            commands >> red >> green >> blue;
        }
        else if (token.compare(simulateString) == 0)
        {
            commands >> iterations >> stepSampleRate >> damping >> noisy >> fuzz;
            prng = PRNG();
            std::cout << "# Simulating" << std::endl;
        }
        else if (token.compare(tonemapString) == 0)
        {
            float exposure;
            commands >> exposure;
            std::cout << "# Tonemapping" << std::endl;

            auto tonemap = [exposure](float n) {
                return (unsigned char)((1.0f - std::pow(2.0f, -n * 0.005f * exposure)) * 255);
            };

            std::unique_ptr<unsigned char[]> output(new unsigned char[width * height * channels]);

            std::vector<std::thread> threads;
            std::atomic<int> x_next{0};
            for (unsigned n = 0; n < 4; n++)
            {
                threads.emplace_back([&]() {
                    for (int x; (x = x_next++) < width; x++)
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
                });
            }
            for (auto &t : threads)
                t.join();

            stbi_write_png(argv[3], width, height, channels, output.get(), width * channels);
            std::cout << "# Saved: " << argv[3] << std::endl;
        }
    }

    commands.close();
    return 0;
}
