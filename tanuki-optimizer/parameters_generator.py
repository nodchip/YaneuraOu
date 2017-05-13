#!/usr/bin/python
# -*- coding: utf-8 -*-

import csv
import math

def main():
  with open('parameters.csv', 'r') as f:
    reader = csv.reader(f)
    for index, row in enumerate(reader):
      name, default_value, min_value, max_value = row
      default_value = int(default_value)
      min_value = int(min_value)
      max_value = int(max_value)
      #if not name.endswith('_OPENING'):
      #  continue
      #if not name.endswith('_ENDING'):
      #  continue
      #print("{0}={1}".format(name, default_value))
      #print('-D{0}=$({0})'.format(name))
      #print("  hp.quniform('{0}', {1}, {2}, 1),".format(name, min_value, max_value))
      #print("    '{0}=' + str(int(args[{1}])),".format(name, index))
      #print('#ifndef {0}'.format(name))
      #print('#define {0} {1}'.format(name, str(int(parameters[index]))))
      #print('#endif')
      #print("{0}={1}".format(name, str(int(parameters[index]))))
      print("  '{0}',".format(name))
      #sigma = float(default_value) / 10.0 / 3.0
      #print("  hp.qnormal('{0}', {1}, {2}, 1),".format(name, default_value, sigma))
      #print("  '{0}',".format(name))
      #print('  {name},'.format(name=name))
      #print("    f.write('PARAM_DEFINE {name} = {{{name}}};\\n'.format({name}=int(round(scipy.stats.norm.ppf({name}, loc={default_value}, scale={sigma})))))".format(name=name, default_value=default_value, sigma=sigma))
      #print("  STATE['{name}'].append({name})".format(name=name))
      #print("  '{name}': [0.0, 1.0],".format(name=name))
      #print("  print('{name:50s}= {{{name}}}'.format({name}={name}))".format(name=name))
      #print('PARAM_DEFINE {name} = {default_value};'.format(name=name, default_value=default_value))
      #min_value = max(min_value, int(math.floor(float(default_value) * 0.75)))
      #max_value = min(max_value, int(math.ceil(float(default_value) * 1.25)))
      #print("  hp.quniform('{0}', {1}, {2}, 1),".format(name, min_value, max_value))


if __name__ == '__main__':
    main()
