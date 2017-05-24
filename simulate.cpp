#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#include <array>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <vector>

typedef std::vector<std::array<float, 3>> PixelVector;
typedef std::array<float, 4> ParticleArray;
typedef std::vector<ParticleArray> ParticleVector;

std::random_device rd;
std::mt19937 rgen(rd());
std::uniform_real_distribution<> rdis(0, 1);

float fuzzy(float extent) { return (rdis(rgen) - 0.5f) * extent * 2.0f; }

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("# Usage: %s commands.txt noise.png output.png\n", argv[0]);
        return 0;
    }

    int width, height, channels;
    unsigned char *noiseTexture = stbi_load(argv[2], &width, &height, &channels, 4);
    printf("# Loaded: %s [%ix%ix%i]\n", argv[2], width, height, channels);
    auto noiseSample = [noiseTexture, width, height, channels](float x, float y) {
        if ((int)x < 0 || (int)x >= width || (int)y < 0 || (int)y >= height)
        {
            return std::pair<float, float>{0.0f, 0.0f};
        }
        size_t index = ((int)x + (int)y * width) * channels;
        float noiseFirst = (float)(noiseTexture[index] - 127) / 127.0f;
        float noiseSecond = (float)(noiseTexture[index + 1] - 127) / 127.0f;
        return std::pair<float, float>{noiseFirst, noiseSecond};
    };

    PixelVector hdr;
    hdr.resize(width * height);

    // Comment
    const std::string commentString("#");
    const std::string stepSampleRateString("STEP_SAMPLE_RATE");
    const std::string particleString("PARTICLE");
    const std::string colourString("COLOUR");
    const std::string simulateString("SIMULATE");
    const std::string tonemapString("TONEMAP");

    int stepSampleRate;
    float red, green, blue;

    ParticleVector particles;

    std::ifstream commands(argv[1]);
    if (!commands.is_open())
    {
        std::cout << "# Failed to open: " << argv[1] << std::endl;
        return -1;
    }

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
        else if (token.compare(stepSampleRateString) == 0)
        {
            ss >> stepSampleRate;
        }
        else if (token.compare(particleString) == 0)
        {
            float x, y, vx, vy;
            ss >> x >> y >> vx >> vy;
            particles.emplace_back(ParticleArray{x, y, vx, vy});
        }
        else if (token.compare(colourString) == 0)
        {
            ss >> red >> green >> blue;
        }
        else if (token.compare(simulateString) == 0)
        {
            int iterations;
            float damping, noisy, fuzz;
            ss >> iterations >> damping >> noisy >> fuzz;
            std::cout << "# Simulating" << std::endl;

            for (ParticleArray &p : particles)
            {
                for (int i = 0; i < iterations; i++)
                {
                    float x = p[0];
                    float y = p[1];
                    auto noise = noiseSample(x, y);
                    float vx = p[2] * damping + (noise.first * 4.0 * noisy) + fuzzy(0.1) * fuzz;
                    float vy = p[3] * damping + (noise.second * 4.0 * noisy) + fuzzy(0.1) * fuzz;
                    float step = 1.0f / stepSampleRate;
                    for (int j = 0; j < stepSampleRate; j++)
                    {
                        x += vx * step;
                        y += vy * step;
                        if ((int)x < 0 || (int)x >= width || (int)y < 0 || (int)y >= height)
                        {
                            break;
                        }
                        size_t index = (int)x + (int)y * width;
                        hdr[index][0] += red;
                        hdr[index][1] += green;
                        hdr[index][2] += blue;
                    }
                    p[0] = x;
                    p[1] = y;
                    p[2] = vx;
                    p[3] = vy;
                }
            }
            std::cout << "# Simulating Complete" << std::endl;
            particles.clear();
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
            readComms = false;
        }
    }

    stbi_image_free(noiseTexture);
    return 0;
}