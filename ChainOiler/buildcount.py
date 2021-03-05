#!/usr/bin/python3
# -*- coding: utf-8 -*-

from os import path, rename, remove
import re
from datetime import datetime

__author__ = 'Dirk Marciniak'
__copyright__ = 'Copyright 2020'
__license__ = 'GPL'
__version__ = '0.1'


COUNTFILE = "count.cnt"
TEMPFILE = "tempfile.tmp"
SRCFOLDER = "src/"
PREFS_FILE = "AppPreferences.cpp"

REGEX_EXPR = r"^\s+const\s+char\s+.Preferences::serialStr\s+=\s+\".*\";.*"
REPLACEMENT = "  const char *Preferences::serialStr = \"{}\";\n"


def create_countfile():
    try:
        ofile = open(COUNTFILE, mode='w', newline='\n')
        ofile.write("0")
        ofile.close()
    except IOError as e:
        print("can't open countfile. abort : {}".format(e))
        return (-1)


def main():
    print("build script PRE...")
    source_file = SRCFOLDER + PREFS_FILE
    if not path.isfile(COUNTFILE):
        create_countfile()
    try:
        ifile = open(COUNTFILE, mode='r+')
        value = int(ifile.readline())
        ifile.seek(0)
        value = value + 1
        ifile.write(str(value))
        ifile.close()
    except IOError as e:
        print("can't open countfile. abort : {}".format(e))
        return (-1)
    try:
        ifile = open(source_file, mode='r', newline='\n')
        ofile = open(TEMPFILE, mode='w', newline='\n')
        for line in ifile:
            if re.match(REGEX_EXPR, line):
                print("hit line: <{}>".format(line))
                now = datetime.now()
                date_time_str = now.strftime(
                    "%Y%m%d-%H%M%S-") + "build-{:04d}".format(value)
                ofile.write(REPLACEMENT.format(date_time_str))
            else:
                ofile.write(line)
        ifile.close()
        ofile.close()
    except IOError as e:
        print("can't open countfile. abort : {}".format(e))
        return (-1)
    if path.isfile(source_file + ".old"):
        remove(source_file + ".old")
    rename(source_file, source_file + ".old")
    rename(TEMPFILE, source_file)


# if __name__ == '__main__':
main()
