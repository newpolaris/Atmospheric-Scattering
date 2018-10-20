#! /usr/bin/python3
## ==========================================================
## FreeImage 3
##
## Design and implementation by
## - thfabian (fabian_thuering@hotmail.com)
##
##
## This file is part of FreeImage 3
##
## COVERED CODE IS PROVIDED UNDER THIS LICENSE ON AN "AS IS" BASIS, WITHOUT WARRANTY
## OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, WITHOUT LIMITATION, WARRANTIES
## THAT THE COVERED CODE IS FREE OF DEFECTS, MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE
## OR NON-INFRINGING. THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE COVERED
## CODE IS WITH YOU. SHOULD ANY COVERED CODE PROVE DEFECTIVE IN ANY RESPECT, YOU (NOT
## THE INITIAL DEVELOPER OR ANY OTHER CONTRIBUTOR) ASSUME THE COST OF ANY NECESSARY
## SERVICING, REPAIR OR CORRECTION. THIS DISCLAIMER OF WARRANTY CONSTITUTES AN ESSENTIAL
## PART OF THIS LICENSE. NO USE OF ANY COVERED CODE IS AUTHORIZED HEREUNDER EXCEPT UNDER
## THIS DISCLAIMER.
##
## Use at your own risk!
## ==========================================================

import os
import shutil
import argparse

g_extensions = ['.h', '.hpp', '.c', '.cpp', '.cxx', '.hxx']
g_backlist = ['Makefile']

def copy_files(src, dest):
    cmake_sources = []
    for path, subdirs, files in os.walk(src):
        for name in files:
            filename, file_extension = os.path.splitext(name)
            if file_extension in g_extensions and not filename in g_backlist:
                from_file = os.path.join(path, name)
                relpath = os.path.relpath(from_file, src)
                to_file = os.path.join(dest, relpath)             
                cmake_sources += [relpath]
                os.makedirs(os.path.dirname(to_file), exist_ok=True)
                shutil.copyfile(from_file, to_file)
                    
    print("set(FreeImage_SOURCES")
    for src in cmake_sources:    
        print("  src/%s" % src)
    print(")")

def main():
    parser = argparse.ArgumentParser(description='Copy FreeImage source files')
    parser.add_argument('src', help='source directory (to copy files from)')
    parser.add_argument('dest', help='destination directory (to copy files to)')    
    args = parser.parse_args()
    copy_files(args.src, args.dest)

if __name__ == '__main__':
    main()
