#!/usr/bin/python3
# -*- coding: utf-8 -*-

from argparse import ArgumentParser
from os import path, remove
import gzip
import shutil

"""
Versionsnummer hochzählen für UPDATE
"""
__author__ = 'Dirk Marciniak'
__copyright__ = 'Copyright 2021'
__license__ = 'GPL'
__version__ = '0.3'

"""
mache index.html zu index.hex
"""
INFILE_FRAGMENT = "index"
OUTPATH = "../include/"
LENSTR = "const uint32_t INDEX_PAGE_SIZE = {};"
CONTENTSTR_START = "uint8_t INDEX_PAGE_CONTENT[] PROGMEM = { "
CONTENTSTR_END = " };"


def encode_to_hexdump(file):
    infile = file + ".html"
    outfile = OUTPATH + file + ".hex"
    countfile = OUTPATH + file + ".inc"
    columnCounter = 0
    compressed_file = 'temp_' + file + '.gz'
    print("encode ascii source file <{0}> to hexdump file <{1}>".format(
        infile, outfile))
    #
    # erst komprimieren
    #
    try:
        with open(infile, 'rb') as f_in:
            with gzip.open(compressed_file, 'wb') as f_out:
                shutil.copyfileobj(f_in, f_out)
    except IOError as e:
        print("can't compress infile. abort : {}".format(e))
        return (-1)
    #
    # jetzt hexdump erzeugen
    #
    try:
        print("open compressed infile {}...".format(compressed_file))
        i_file = open(compressed_file, 'rb')
    except IOError as e:
        print("can't open infile. abort : {}".format(e))
        return (-1)
    try:
        print("open hexdump outfile {}...".format(outfile))
        o_file = open(outfile, mode='w', newline='\n')
        o_file.write(CONTENTSTR_START + "\n")
    except IOError as e:
        i_file.close()
        print("can't open hexdump outfile. abort : {}".format(e))
        remove(compressed_file)
        return (-1)
    rbyte = i_file.read(1)
    while rbyte:
        #
        # wandeln nach code
        #
        num = int.from_bytes(rbyte, 'little', signed=False)
        ascii_str = "{0:d}".format(num)
        # print("char: {0}".format(ascii_str))
        #
        # versuchen das nächste Byte zu holen
        #
        rbyte = i_file.read(1)
        if rbyte:
            ascii_str += ","
        o_file.write(ascii_str)
        # wie viel schreibe ich rein
        # spalten zählen
        columnCounter += 1
        if columnCounter >= 29:
            columnCounter = 0
            o_file.write("\n")
    o_file.write(CONTENTSTR_END)
    o_file.close()
    i_file.close()
    file_size = path.getsize(compressed_file)
    #
    # date mit der Länge schreiben
    #
    try:
        print("open length outfile {}...".format(countfile))
        o_file = open(countfile, mode='w', newline='\n')
        o_file.write(LENSTR.format(file_size))
    except IOError as e:
        i_file.close()
        print("can't open length outfile. abort : {}".format(e))
        remove(compressed_file)
        return (-1)
    remove(compressed_file)
    print("compressed file size: {} bytes".format(file_size))


def main():
    infile = INFILE_FRAGMENT
    #
    # parse argumente
    #
    parser = ArgumentParser(fromfile_prefix_chars='@',
                            description='encode html to hex for c++ program',
                            epilog="from Dirk Marciniak\n")
    parser.add_argument("--infile", type=str, help="input file without .html")
    #
    args = parser.parse_args()
    #
    # eingabedatei
    #
    if args.infile:
        infile = args.infile
    if path.isfile(infile + ".html"):
        print("infile is a file and his name is {}".format(infile + ".html"))
    else:
        print('infile {} is not exist. abort.'.format(infile + ".html"))
        exit(-1)
    encode_to_hexdump(infile)


# Press the green button in the gutter to run the script.
if __name__ == '__main__':
    main()
