#!/usr/bin/python
# -*- coding: utf-8 -*-

import csv

def main():
    with open('parameters.csv', 'r') as f:
        reader = csv.reader(f)
        for index, row in enumerate(reader):
            name, default_value, min_value, max_value = row
            #print("{0}={1}".format(name, default_value))
            #print('-D{0}=$({0})'.format(name))
            #print("hp.quniform('{0}', {1}, {2}),".format(name, min_value, max_value))
            print("		'{0}=' + args[{1}],".format(name, index))

if __name__ == '__main__':
    main()
