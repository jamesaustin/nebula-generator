#!/usr/bin/env python3
import logging

from argparse import ArgumentParser, ArgumentDefaultsHelpFormatter
from json import dumps as json_dumps, load as json_load
from random import seed as random_seed, random as random_random
from math import tau, cos as mcos, sin as msin, pow as mpow
from os.path import join as path_join
from os import makedirs as os_makedirs
from threading import Timer

# Requires module "cairocffi"
# PYPI: https://pypi.python.org/pypi/cairocffi
# DOCS: http://cairocffi.readthedocs.io/en/latest/
# Install with: pip3 install cairocffi
import cairocffi as cairo

# INFO:
# * Arc is clockwise from horizonal

LOG = logging.getLogger(__name__)

REGISTERED_FUNCTIONS = {}
def register(func):
    REGISTERED_FUNCTIONS[func.__name__] = func
    return func

def frange(start, end, step):
    if start < end:
        while start < end:
            yield start
            start += step
    elif start > end:
        while start > end:
            yield start
            start += step

class Progress(object):
    def __init__(self, path, payload):
        self.timers = []
        self.path = path
        self.payload = payload
        os_makedirs(path, exist_ok=True)
        LOG.info('# Progress: Creating Queue')
        self.add_timer()
    def cancel(self):
        LOG.info('# Progress: Deleting Queue[%i]', len(self.timers))
        for t in self.timers:
            t.cancel()
        self.timers.clear()
    def add_timer(self, time=1):
        t = Timer(time, self.save_progress)
        t.start()
        self.timers.append(t)
    def save_progress(self):
        filename = path_join(self.path, '{:05}.png'.format(len(self.timers)))
        LOG.info('# Progress: Saving %s', filename)
        image = self.payload
        image.flush()
        image.write_to_png(filename)
        self.add_timer()

#######################################################################################################################

class OctaveNoise(object):
    def __init__(self, width, height, octaves=8, filename='octave.png'):
        try:
            image = cairo.ImageSurface.create_from_png(filename)
            LOG.info('Loading: %s', filename)
        except FileNotFoundError:
            LOG.info('Creating: %s', filename)
            image = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
            draw = cairo.Context(image)
            draw.rectangle(0, 0, width, height)
            draw.set_source_rgb(0, 0, 0)
            draw.fill()
            alpha = 1.0 / octaves
            for octave in range(0, octaves):
                step = mpow(2, octave)
                LOG.info('%i of %i, step %i', octave, octaves, step)
                for x in frange(0, width, step):
                    for y in frange(0, height, step):
                        draw.rectangle(x, y, step, step)
                        draw.set_source_rgba(random_random(), random_random(), random_random(), alpha)
                        draw.fill()
            image.flush()
            image.write_to_png(filename)
        self.noise_bytearray = bytearray(image.get_data())
        self.width = width

    def value(self, x, y, channel):
        index = (int(x) + int(y) * self.width) * 4 + channel
        if index > len(self.noise_bytearray):
            return 0
        else:
            return (self.noise_bytearray[index] - 127) / 127.0

    def __str__(self):
        response = []
        for c in range(4):
            directions = [0, 0, 0, 0]
            for x in range(1000):
                for y in range(1000):
                    v = self.value(x, y, c)
                    if v > 0:
                        directions[0] += 1
                    if v > 0:
                        directions[1] += 1
                    if v < 0:
                        directions[2] += 1
                    if v < 0:
                        directions[3] += 1
            response.append('{}: {}'.format(c, directions))
        return '\n'.join(response)

#######################################################################################################################

@register
def nebula(draw,
           noise,
           max_age=100,         # (1, 500, 1)
           exposure=1.0,        # (0, 5, 0.01)
           damping=0.8,         # (0, 1.2, 0.01)
           noisy=1.0,           # (0, 10, 0.01)
           fuzz=1.0,            # (0, 10, 0.01)
           intensity=1.0,       # (0, 5, 0.01)
           initial_vx=10,       # (0, 100, 0.01)
           initial_vy=10,       # (0, 100, 0.01)
           spawn=10,            # (1, 100, 1)
           step_sample_rate=10  # Sample rate for each particle render.
          ):

    width, height, channels = 1000, 1000, 4
    hdr_floats = [0.0] * (width * height * channels)

    # image.get_data() -> channels are '0b 1g 2r 3a' written to via bytes([x])
    # ord(image.get_data()[index]) = int
    # bytearray gives indexed access to the ints.
    output_buffer = draw.get_target().get_data()

    def tonemap(n):
        return (1 - mpow(2, -n * 0.005 * exposure)) * 255

    def fuzzy(fmax, base=0):
        return base + (random_random() - 0.5) * fmax * 2

    class Particle(object):
        def __init__(self, vx, vy, x, y):
            self.vx, self.vy = vx, vy
            self.x, self.y = x, y
            self.age = 0

        def __repr__(self):
            return 'p({},{})v({},{})@{}'.format(self.x, self.y, self.vx, self.vy, self.age)

        def tick(self):
            self.vx = (self.vx * damping) + (noise.value(self.x, self.y, 2) * 4.0 * noisy) + fuzzy(0.1) * fuzz
            self.vy = (self.vy * damping) + (noise.value(self.x, self.y, 1) * 4.0 * noisy) + fuzzy(0.1) * fuzz
            self.age += 1

        def wander(self, step_sample_rate, colour):
            red, green, blue = colour
            step = 1 / step_sample_rate
            x, y = self.x, self.y
            vx, vy = self.vx * step, self.vy * step
            for _ in range(step_sample_rate):
                x += vx
                y += vy
                if x < 0 or x > width or y < 0 or y > height:
                    break
                index = (int(x) + int(y) * width) * channels
                hdr_floats[index+2] += red
                hdr_floats[index+1] += green
                hdr_floats[index+0] += blue
            self.x, self.y = x, y

    def simulate(particles, colour):
        num_particles = len(particles)
        LOG.info('Num particles: %i', num_particles)
        while num_particles > 0:
            num_particles = 0
            for p in particles:
                if p.age < max_age:
                    num_particles += 1
                    p.tick()
                    p.wander(step_sample_rate, colour)

    for radius, colour in [
            (50, (1.0, 0.1, 0.1)),
            #(100, (1, 0.5, 0.1)),
            #(150, (0.3, 1, 0.3)),
            #(200, (0.25, 1, 0.75)),
            #(250, (0.2, 0.2, 1)),
            #(300, (0.75, 0.25, 1)),
        ]:
        LOG.info('Radius: %i', radius)
        particles = []
        for angle in frange(0, tau, 0.5 / radius):
            for _ in range(spawn):
                particles.append(Particle(fuzzy(initial_vx), fuzzy(initial_vy), 500 + radius * mcos(angle), 500 + radius * msin(angle)))

        colour = tuple(c * intensity for c in colour)
        simulate(particles, colour)

    for x in range(width):
        for y in range(height):
            index = (x + y * width) * channels
            output_buffer[index:index+3] = bytes([
                int(tonemap(hdr_floats[index+0])),
                int(tonemap(hdr_floats[index+1])),
                int(tonemap(hdr_floats[index+2]))
            ])

#######################################################################################################################

def parse_args():
    parser = ArgumentParser(formatter_class=ArgumentDefaultsHelpFormatter)
    parser.add_argument('config', help='Configuration JSON')
    parser.add_argument('--path', help='Output path')
    parser.add_argument('--noise', help='Noise PNG, will be generated if it doesn\'t exist', default='octave.png')
    parser.add_argument('--progress', help='Capture progress to folder')
    parser.add_argument('--seed', default='Boundless', help='Generation seed')
    parser.add_argument('-v', '--verbose', action='store_true')
    parser.add_argument('-d', '--debug', action='store_true')
    args = parser.parse_args()

    if args.debug:
        level = logging.DEBUG
    elif args.verbose:
        level = logging.INFO
    else:
        level = logging.WARNING
    logging.basicConfig(format='%(message)s', level=level)
    LOG.debug('# %s', str(args))

    return args

def main():
    args = parse_args()

    with open(args.config) as f:
        configs = json_load(f)

    filenames = []
    for n, config in enumerate(configs):
        width, height = 1000, 1000
        image = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
        draw = cairo.Context(image)

        # Black background by default
        draw.rectangle(0, 0, width, height)
        draw.set_source_rgb(0, 0, 0)
        draw.fill()

        # Translate the context into [-1,1]x[-1,1]
        draw.translate(width / 2.0, height / 2.0)
        draw.scale(min(width, height) / 2.0, min(width, height) / 2.0)

        config['noise'] = OctaveNoise(width, height, filename=args.noise)

        if args.progress:
            progress = Progress(args.progress, image)

        random_seed(args.seed)
        fn_name = config.pop('_fn')
        REGISTERED_FUNCTIONS[fn_name](draw, **config)

        if args.progress:
            progress.cancel()

        filename = '{:03}.{}.png'.format(n, fn_name)
        if args.path:
            os_makedirs(args.path, exist_ok=True)
            filename = path_join(args.path, filename)

        image.flush()
        image.write_to_png(filename)
        filenames.append(filename)
        LOG.debug('# %s', filename)

    return {
        'dimensions': [width, height],
        'outputs': filenames
    }

if __name__ == '__main__':
    RESULT = main()
    print(json_dumps(RESULT, sort_keys=True, indent=2))
