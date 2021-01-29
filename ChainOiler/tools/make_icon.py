#!/usr/bin/python3
# -*- coding: utf-8 -*-

from os import path, remove
import shutil

"""
icon machen
"""
__author__ = 'Dirk Marciniak'
__copyright__ = 'Copyright 2020'
__license__ = 'GPL'
__version__ = '0.1'

"""
mache index.html zu index.hex
"""
INFILE = "ledicon.ico"
OUTFILE = "../include/favicon.hex"
LENSTR = "const uint32_t FAVICON_SIZE = {};"
CONTENTSTR_START = "uint8_t FAVICON_CONTENT[] PROGMEM = { "
CONTENTSTR_END = " };"


def encode_to_hexdump(infile, outfile):
    columnCounter = 0
    print("encode icon file <{0}> to hexdump file <{1}>".format(
        infile, outfile))
    file_size = path.getsize(infile)

    #
    # jetzt hexdump erzeugen
    #
    try:
        print("open infile {}...".format(infile))
        i_file = open(infile, 'rb')
    except IOError as e:
        print("can't open infile. abort : {}".format(e))
        return (-1)
    try:
        print("open hpp outfile {}...".format(outfile))
        o_file = open(outfile, mode='w', newline='\n')
    except IOError as e:
        i_file.close()
        print("can't open hpp outfile. abort : {}".format(e))
        return (-1)
    o_file.write("#pragma once\n")
    o_file.write("\n\n// laenge des content\n")
    o_file.write(LENSTR.format(file_size))
    o_file.write("\n\n// content\n")
    o_file.write(CONTENTSTR_START + "\n")
    rbyte = i_file.read(1)
    while rbyte:
        #
        # wandeln nach code
        #
        num = int.from_bytes(rbyte, 'little', signed=False)
        ascii_str = "{0:d}".format(num)
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
    o_file.write("\n\n")
    i_file.close()
    o_file.close()
    print("icon file size: {} bytes".format(file_size))


def main():
    encode_to_hexdump(INFILE, OUTFILE)


# Press the green button in the gutter to run the script.
if __name__ == '__main__':
    main()
