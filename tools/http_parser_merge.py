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
import os
import shutil
import sys
import tempfile
from distutils.spawn import find_executable
from pyclibrary import CParser
from subprocess import call

# parse command-line arguments
cli_parser = argparse.ArgumentParser()

cli_parser.add_argument('-i',
                        '--source-dir',
                        help='http-parser source directory')
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

# set basic variables and executables
stock_c_filename = 'http_parser.c'
stock_h_filename = 'http_parser.h'
temp_c_filename = '_http_parser.c'
temp_c_path = args.dest_dir + '/' + temp_c_filename

source_dir_cleanup = False
http_parser_url = 'https://github.com/nodejs/http-parser.git'

git = find_executable('git')
formatter = find_executable('clang-format')
formatter_style = '-style={ColumnLimit: 200, AllowShortFunctionsOnASingleLine: false}'

if os.path.isfile(formatter) is not True and os.access(formatter, os.X_OK) is not True:
    print 'clang-format not available'
    sys.exit(1)

if os.path.isfile(git) is not True and os.access(git, os.X_OK) is not True:
    print 'git executable not available'
    sys.exit(1)

# if http-parser source directory not defined, fetch it from git
# if it fails, fall back to '$PWD/..'
if args.source_dir is None:
    if call(['git',
             'clone',
             http_parser_url,
             tempfile.gettempdir() + '/http-parser']) is 0:

        args.source_dir = tempfile.gettempdir() + '/http-parser'
        source_dir_cleanup = True
    else:
        if args.verbose:
            print 'failed cloning http-parser'

        args.source_dir = '..'

print '\nstart parsing...'

# initialize pyclibrary parser and fetch http-parser function names
if args.verbose:
    print 'initializing parser\n'

parser = CParser([args.source_dir + '/' + stock_c_filename])

if args.verbose:
    print 'parsing function definitions'

functions = parser.defs['functions']

# create temporary working directory and copy stock sources to it
if args.verbose:
    print 'creating a temporary directory ', args.dest_dir

if not os.path.exists(args.dest_dir):
    os.makedirs(args.dest_dir)

if args.verbose:
    print 'copying http-parser implementation file to a temporary directory'

shutil.copyfile(args.source_dir + '/' + stock_c_filename, temp_c_path)

# basic clang-format linting for easier function parsing
if args.verbose:
    print 'running clang-format against the file'

if call([formatter, '-i', formatter_style, temp_c_path]) is not 0:
    print 'clang-format failed'
    sys.exit(1)

if args.verbose:
    print 'editing file'

# open files for reading/writing
output_file = open(args.dest_dir + '/' + args.merge_filename, 'w')
stock_header = open(args.source_dir + '/' + stock_h_filename, 'r')
stock_implementation = open(temp_c_path, 'r')

# copy header to a new merge file
output_file.write(stock_header.read())
output_file.write('\n\n')

# iterate stock implementation file (http_parser.c)
# set functions to inline and append to a merge file
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

# close file handles and delete temporary files
if args.verbose:
    print 'cleaning up'

if source_dir_cleanup is True and args.source_dir != args.dest_dir:
    shutil.rmtree(args.source_dir)

output_file.close()
stock_header.close()
stock_implementation.close()
os.remove(temp_c_path)

print '\nhttp-parser merged to', args.dest_dir + '/' + args.merge_filename
