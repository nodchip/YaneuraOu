#!/usr/bin/python
# -*- coding: utf-8 -*-

import csv

def main():
  parameters = [118.0, 24.0, 83017.0, 92371.0, 15174.0, 2212.0, 3529.0, 94.0, 658.0, 35.0, 14.0, 47.0, 16.0, 29.0, 239.0, 17.0, 3.0, 14.0, 21.0, 9.0, 4.0, 5.0, 142.0, 8.0, 7.0, 52.0, 4.0, 12.0, 192.0, 7.0, 371920.0, 7932.0, 7.0, 219.0, 3.0, 4.0]
  with open('parameters.csv', 'r') as f:
    reader = csv.reader(f)
    for index, row in enumerate(reader):
      name, default_value, min_value, max_value = row
      #print("{0}={1}".format(name, default_value))
      #print('-D{0}=$({0})'.format(name))
      #print("  hp.quniform('{0}', {1}, {2}, 1),".format(name, min_value, max_value))
      #print("    '{0}=' + str(int(args[{1}])),".format(name, index))
      print('#ifndef {0}'.format(name))
      print('#define {0} {1}'.format(name, str(int(parameters[index]))))
      print('#endif')
      #print("{0}={1}".format(name, str(int(parameters[index]))))

if __name__ == '__main__':
    main()
