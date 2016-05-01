#!/usr/bin/env python

#
# setup and usage:
#
# git clone https://github.com/MatthieuDartiailh/pyclibrary.git
# cd pyclibrary
# python setup.py install
# python http_parser_merge.py -h
#

import argparse
import fileinput
import os
import re
import shutil
from distutils.spawn import find_executable
from pyclibrary import CParser
from subprocess import call

cli_parser = argparse.ArgumentParser()

cli_parser.add_argument('-i',
                        '--source-dir',
                        help='http-parser source directory',
                        default='..')
cli_parser.add_argument('-f',
                        '--merge-filename',
                        help='merged http-parser header filename',
                        default='http_parser_merged.h')
cli_parser.add_argument('-o',
                        '--dest-dir',
                        help='destination (temporary) working directory',
                        default='_merge_dir')
cli_parser.add_argument('-v',
                        '--verbose',
                        help='use verbose output',
                        action='store_true')

args = cli_parser.parse_args()

stock_c_filename = 'http_parser.c'
stock_h_filename = 'http_parser.h'
temp_c_filename = '_http_parser.c'
temp_c_path = args.dest_dir + '/' + temp_c_filename

formatter = find_executable('clang-format')
formatter_style = '-style={ColumnLimit: 200, AllowShortFunctionsOnASingleLine: false}'

print 'start parsing...'

if os.path.isfile(formatter) is not True and os.access(formatter, os.X_OK) is not True:
    print 'clang-format not available'
    sys.exit(1)

if args.verbose:
    print 'initializing parser\n'

parser = CParser([args.source_dir + '/' + stock_c_filename])

if args.verbose:
    print 'parsing function definitions'

functions = parser.defs['functions']

if args.verbose:
    print 'creating a temporary directory ', args.dest_dir

if not os.path.exists(args.dest_dir):
    os.makedirs(args.dest_dir)

if args.verbose:
    print 'copying http-parser implementation file to a temporary directory'

shutil.copyfile(args.source_dir + '/' + stock_c_filename, temp_c_path)

if args.verbose:
    print 'running clang-format against the file'

if call([formatter, '-i', formatter_style, temp_c_path]) is not 0:
    print 'clang-format failed'
    sys.exit(1)

if args.verbose:
    print 'editing file'

output_file = open(args.dest_dir + '/' + args.merge_filename, 'w')
stock_header = open(args.source_dir + '/' + stock_h_filename, 'r')
stock_implementation = open(temp_c_path, 'r')

output_file.write(stock_header.read())
output_file.write('\n\n')

for line in stock_implementation:
    def iterate_functions():
        for key, value in functions.iteritems():
            if '#include "http_parser.h"' == line.strip():
                return

            if (line.find(key) is not -1 and line.find('if (') is -1 and
                line.find('for (') is -1 and line.find(';') is -1 and
                line.find('?') is -1):

                return 'function'

        return 'copy'

    res = iterate_functions()

    if res == 'function':
        output_file.write(line.replace(line, 'inline ' + line))
    elif res == 'copy':
        output_file.write(line)

if args.verbose:
    print 'cleaning up'

output_file.close()
stock_header.close()
stock_implementation.close()
os.remove(temp_c_path)

print '\nhttp-parser merged to', args.dest_dir + '/' + args.merge_filename
