#!/usr/bin/env python3
import logging

from argparse import ArgumentParser, ArgumentDefaultsHelpFormatter
from json import dumps as json_dumps, load as json_load
from random import seed as random_seed, random as random_random
from math import tau, cos as mcos, sin as msin
from os.path import join as path_join
from os import makedirs as os_makedirs

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

def fuzzy(extent=1.0):
    return (random_random() - 0.5) * extent * 2

#######################################################################################################################

@register
def nebula(output,
           iterations=100,      # (1, 500, 1)
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
    for radius, colour in [(50, (1.0, 0.1, 0.1)),
                           (100, (1.0, 0.5, 0.1)),
                           (150, (0.3, 1, 0.3)),
                           (200, (0.25, 1, 0.75)),
                           (250, (0.2, 0.2, 1)),
                           (300, (0.75, 0.25, 1))
                          ]:
        LOG.info('# Generating: %s radius:%i colour:%s', 'circle', radius, colour)
        output.write('SIMULATE {} {} {} {} {}\n'.format(iterations, step_sample_rate, damping, noisy, fuzz))
        output.write('COLOUR {} {} {}\n'.format(*tuple(c * intensity for c in colour)))
        for angle in frange(0, tau, 0.5 / radius):
            for _ in range(spawn):
                x, y = 500 + radius * mcos(angle), 500 + radius * msin(angle)
                output.write('PARTICLE {} {} {} {}\n'.format(x, y, fuzzy(initial_vx), fuzzy(initial_vy)))
        output.write('END\n')
    output.write('TONEMAP {}\n'.format(exposure))

#######################################################################################################################

def parse_args():
    parser = ArgumentParser(formatter_class=ArgumentDefaultsHelpFormatter)
    parser.add_argument('config', help='Configuration JSON')
    parser.add_argument('path', help='Output path')
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
        random_seed(args.seed)
        fn_name = config.pop('_fn')

        os_makedirs(args.path, exist_ok=True)
        filename = '{:03}.{}.txt'.format(n, fn_name)
        filename = path_join(args.path, filename)
        with open(filename, 'w') as f:
            REGISTERED_FUNCTIONS[fn_name](f, **config)
        filenames.append(filename)
        LOG.debug('# %s', filename)

    return {
        'outputs': filenames
    }

if __name__ == '__main__':
    RESULT = main()
    print(json_dumps(RESULT, sort_keys=True, indent=2))
