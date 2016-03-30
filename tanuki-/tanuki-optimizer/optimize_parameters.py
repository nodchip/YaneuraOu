#!/usr/bin/python
# coding:utf-8
#
# Preparation
# http://qiita.com/mojaie/items/995661f7467ffdb40331
#
# 1. Install the following softwares
# - Python 2.7.*
# -- http://www.python.org
# - numpy-*.*.*-win32-superpack-python2.7.exe
# -- http://sourceforge.net/projects/numpy
# - scipy-*.*.*-win32-superpack-python2.7.exe
# -- http://sourceforge.net/projects/scipy
#
# 2. Execute the following commands.
# - python -m pip install --upgrade pip
# - pip install hyperopt bson pymongo networkx
#
# 3. If using MSVC instead of MSYS (Windows)
# - install MSVC14 (Visual Studio 2015)
# - uncomment the line: buider = MSVCBuilder()
# - open developer console
# - move in src directory
# - run this script
from hyperopt import fmin, tpe, hp, rand, Trials
from math import log
import os
import sys
import datetime
import re
import shutil
import subprocess
import time
import argparse
import cPickle as pickle

space = [
  hp.quniform('QSEARCH_FUTILITY_MARGIN', 0, 256, 1),
  hp.quniform('SEARCH_FUTILITY_MARGIN_DEPTH_THRESHOLD', 2, 28, 1),
  hp.quniform('SEARCH_FUTILITY_MARGIN_INTERCEPT', 0, 136986, 1),
  hp.quniform('SEARCH_FUTILITY_MARGIN_LOG_D_COEFFICIENT', 0, 138096, 1),
  hp.quniform('SEARCH_FUTILITY_MARGIN_MOVE_COUNT_COEFFICIENT', 0, 16384, 1),
  hp.quniform('SEARCH_FUTILITY_MOVE_COUNTS_INTERCEPT', 0, 6146, 1),
  hp.quniform('SEARCH_FUTILITY_MOVE_COUNTS_POWER', 1024, 3686, 1),
  hp.quniform('SEARCH_FUTILITY_MOVE_COUNTS_SCALE', 0, 614, 1),
  hp.quniform('SEARCH_FUTILITY_PRUNING_NON_PV_REDUCTION_INTERCEPT', 0, 682, 1),
  hp.quniform('SEARCH_FUTILITY_PRUNING_NON_PV_REDUCTION_SLOPE', 0, 910, 1),
  hp.quniform('SEARCH_FUTILITY_PRUNING_PREDICTED_DEPTH_THRESHOLD', 2, 16, 1),
  hp.quniform('SEARCH_FUTILITY_PRUNING_PV_REDUCTION_INTERCEPT', 0, 256, 1),
  hp.quniform('SEARCH_FUTILITY_PRUNING_PV_REDUCTION_SLOPE', 0, 682, 1),
  hp.quniform('SEARCH_FUTILITY_PRUNING_SCORE_GAIN_SLOPE', 0, 4096, 1),
  hp.quniform('SEARCH_INTERNAL_ITERATIVE_DEEPENING_NON_PV_DEPTH_SCALE', 0, 1024, 1),
  hp.quniform('SEARCH_INTERNAL_ITERATIVE_DEEPENING_NON_PV_NODE_DEPTH_THRESHOLD', 2, 32, 1),
  hp.quniform('SEARCH_INTERNAL_ITERATIVE_DEEPENING_PV_NODE_DEPTH_DELTA', 2, 8, 1),
  hp.quniform('SEARCH_INTERNAL_ITERATIVE_DEEPENING_PV_NODE_DEPTH_THRESHOLD', 2, 20, 1),
  hp.quniform('SEARCH_INTERNAL_ITERATIVE_DEEPENING_SCORE_MARGIN', 0, 512, 1),
  hp.quniform('SEARCH_LATE_MOVE_REDUCTION_DEPTH_THRESHOLD', 2, 12, 1),
  hp.quniform('SEARCH_NULL_FAIL_LOW_SCORE_DEPTH_THRESHOLD', 2, 20, 1),
  hp.quniform('SEARCH_NULL_MOVE_DEPTH_THRESHOLD', 2, 8, 1),
  hp.quniform('SEARCH_NULL_MOVE_MARGIN', 0, 180, 1),
  hp.quniform('SEARCH_NULL_MOVE_NULL_SCORE_DEPTH_THRESHOLD', 2, 24, 1),
  hp.quniform('SEARCH_NULL_MOVE_REDUCTION_INTERCEPT', 2, 12, 1),
  hp.quniform('SEARCH_NULL_MOVE_REDUCTION_SLOPE', 0, 512, 1),
  hp.quniform('SEARCH_PROBCUT_DEPTH_THRESHOLD', 3, 20, 1),
  hp.quniform('SEARCH_PROBCUT_RBETA_DEPTH_DELTA', 2, 16, 1),
  hp.quniform('SEARCH_PROBCUT_RBETA_SCORE_DELTA', 0, 400, 1),
  hp.quniform('SEARCH_RAZORING_DEPTH', 2, 16, 1),
  hp.quniform('SEARCH_RAZORING_MARGIN_INTERCEPT', 0, 1048576, 1),
  hp.quniform('SEARCH_RAZORING_MARGIN_SLOPE', 0, 32768, 1),
  hp.quniform('SEARCH_SINGULAR_EXTENSION_DEPTH_THRESHOLD', 2, 32, 1),
  hp.quniform('SEARCH_SINGULAR_EXTENSION_NULL_WINDOW_SEARCH_DEPTH_SCALE', 0, 1024, 1),
  hp.quniform('SEARCH_SINGULAR_EXTENSION_TTE_DEPTH_THRESHOLD', 2, 12, 1),
  hp.quniform('SEARCH_STATIC_NULL_MOVE_PRUNING_DEPTH_THRESHOLD', 2, 16, 1),
]

build_argument_names = [
  'QSEARCH_FUTILITY_MARGIN',
  'SEARCH_FUTILITY_MARGIN_DEPTH_THRESHOLD',
  'SEARCH_FUTILITY_MARGIN_INTERCEPT',
  'SEARCH_FUTILITY_MARGIN_LOG_D_COEFFICIENT',
  'SEARCH_FUTILITY_MARGIN_MOVE_COUNT_COEFFICIENT',
  'SEARCH_FUTILITY_MOVE_COUNTS_INTERCEPT',
  'SEARCH_FUTILITY_MOVE_COUNTS_POWER',
  'SEARCH_FUTILITY_MOVE_COUNTS_SCALE',
  'SEARCH_FUTILITY_PRUNING_NON_PV_REDUCTION_INTERCEPT',
  'SEARCH_FUTILITY_PRUNING_NON_PV_REDUCTION_SLOPE',
  'SEARCH_FUTILITY_PRUNING_PREDICTED_DEPTH_THRESHOLD',
  'SEARCH_FUTILITY_PRUNING_PV_REDUCTION_INTERCEPT',
  'SEARCH_FUTILITY_PRUNING_PV_REDUCTION_SLOPE',
  'SEARCH_FUTILITY_PRUNING_SCORE_GAIN_SLOPE',
  'SEARCH_INTERNAL_ITERATIVE_DEEPENING_NON_PV_DEPTH_SCALE',
  'SEARCH_INTERNAL_ITERATIVE_DEEPENING_NON_PV_NODE_DEPTH_THRESHOLD',
  'SEARCH_INTERNAL_ITERATIVE_DEEPENING_PV_NODE_DEPTH_DELTA',
  'SEARCH_INTERNAL_ITERATIVE_DEEPENING_PV_NODE_DEPTH_THRESHOLD',
  'SEARCH_INTERNAL_ITERATIVE_DEEPENING_SCORE_MARGIN',
  'SEARCH_LATE_MOVE_REDUCTION_DEPTH_THRESHOLD',
  'SEARCH_NULL_FAIL_LOW_SCORE_DEPTH_THRESHOLD',
  'SEARCH_NULL_MOVE_DEPTH_THRESHOLD',
  'SEARCH_NULL_MOVE_MARGIN',
  'SEARCH_NULL_MOVE_NULL_SCORE_DEPTH_THRESHOLD',
  'SEARCH_NULL_MOVE_REDUCTION_INTERCEPT',
  'SEARCH_NULL_MOVE_REDUCTION_SLOPE',
  'SEARCH_PROBCUT_DEPTH_THRESHOLD',
  'SEARCH_PROBCUT_RBETA_DEPTH_DELTA',
  'SEARCH_PROBCUT_RBETA_SCORE_DELTA',
  'SEARCH_RAZORING_DEPTH',
  'SEARCH_RAZORING_MARGIN_INTERCEPT',
  'SEARCH_RAZORING_MARGIN_SLOPE',
  'SEARCH_SINGULAR_EXTENSION_DEPTH_THRESHOLD',
  'SEARCH_SINGULAR_EXTENSION_NULL_WINDOW_SEARCH_DEPTH_SCALE',
  'SEARCH_SINGULAR_EXTENSION_TTE_DEPTH_THRESHOLD',
  'SEARCH_STATIC_NULL_MOVE_PRUNING_DEPTH_THRESHOLD',
  ]

COUNTER = 0;
MAX_EVALS = 100;
START_TIME_SEC = time.time()

# arguments
if __name__=='__main__':
  parser = argparse.ArgumentParser('optimize_parameters.py')
  parser.add_argument('--store-interval', type=int, default=1,
      help=u'store internal state of hyper-parameter search after every <store_interval> iterations. set 0 to disable storing.')
  parser.add_argument('--resume', type=str, default=None,
      help=u'resume hyper-parameter search from a file.')
  parser.add_argument('--dump-log', type=str, default=None,
      help=u'open a hyper-parameter search file and dump its log.')
  parser.add_argument('--builder', type=str, default='MSYS',
      help=u'select building environment. MSYS or MSVC.')
  commandline_args = parser.parse_args()

# pause/resume
class HyperoptState(object):
  def __init__(self):
    self.trials = Trials()
    self.iteration_logs = []

  def get_trials(self): return self.trials

  def get_n_accumulated_iterations(self): return len(self.iteration_logs)

  def record_iteration(self, **kwargs):
    # record any pickle-able object as a dictionary at each iteration.
    # also, len(self.iteration_logs) is used to keep the accumulated number of iterations.
    self.iteration_logs.append(kwargs)

  def calc_max_evals(self, n_additional_evals): return self.get_n_accumulated_iterations() + n_additional_evals

  @staticmethod
  def load(file_path):
    with open(file_path, 'rb') as fi:
      state = pickle.load(fi)
      print('resume from state: {} ({} iteration done)'.format(file_path, state.get_n_accumulated_iterations()))
      return state

  def save(self, file_path):
    try:
      with open(file_path, 'wb') as fo:
        pickle.dump(self, fo, protocol=-1)
        print('saved state to: {}'.format(file_path))
    except:
      print('failed to save state. continue.')

  def dump_log(self):
    for index, entry in enumerate(self.iteration_logs):
      # emulate function()'s output.
      print('-' * 78)
      print(datetime.datetime.today().strftime("%Y-%m-%d %H:%M:%S"))
      print(entry['args'])
      if index:
        remaining = datetime.timedelta(seconds=0)
        print(index, '/', self.get_n_accumulated_iterations(), str(remaining))
      popenargs = ['./YaneuraOu.exe',]
      print(popenargs)
      print(entry['output'])
      lose = entry['lose']
      draw = entry['draw']
      win = entry['win']
      ratio = 0.0
      if lose + draw + win > 0.1:
       ratio = win / (lose + draw + win)
      print ratio

if __name__=='__main__':
  state = HyperoptState()
  state_store_path = 'optimize_parameters.hyperopt_state.{}.pickle'.format(datetime.datetime.now().strftime('%Y%m%d_%H%M%S'))
  if commandline_args.dump_log is not None:
    state = HyperoptState.load(commandline_args.dump_log)
    state.dump_log()
    sys.exit(0)

  if commandline_args.resume is not None:
    state = HyperoptState.load(commandline_args.resume)

class MSYSBuilder(object):
  def __init__(self):
    pass

  def clean(self):
    popenargs = [
      'make',
      'clean',
    ]
    print(popenargs)
    while True:
      try:
        with open('make-clean.txt', 'w') as file:
          output = subprocess.check_output(popenargs)
          file.write(output)
        break
      except subprocess.CalledProcessError:
        continue

  def build(self, args):
    popenargs = [
      'make',
      '-j4',
      'native',
      'TARGET_PREFIX=tanuki-modified',
      ]
    for key, val in zip(build_argument_names, args):
      popenargs.append('{}={}'.format(key, int(val)))

    print('Executing: make native (MSYS) ...')
    while True:
      try:
        with open('make-native.txt', 'w') as file:
          output = subprocess.check_output(popenargs)
          file.write(output)
        break
      except subprocess.CalledProcessError:
        continue

  def kill(self, process_name):
    subprocess.call(['pkill', process_name])

class MSVCBuilder(object):
  def __init__(self):
    self.devenv_path = 'devenv.exe'
    self.header_path = r'builder_generated.hpp'

  def clean(self):
    log_path = 'make-clean.txt'
    popenargs = [
      self.devenv_path,
      r'..\tanuki-\tanuki-.sln',
      '/Clean',
      'Release|x64',
      '/Project',
      r'..\tanuki-\tanuki-\tanuki-.vcxproj',
      '/Out',
      log_path,
      ]
    print(popenargs)
    while True:
      try:
        if os.path.isfile(log_path):
          os.unlink(log_path)
        subprocess.call(popenargs)
        break
      except subprocess.CalledProcessError:
        print('retry')
        continue

  def update_header(self, args):
    while True:
      try:
        with open(self.header_path, 'w') as file:
          file.write('#ifndef __BUILDER_GENERATED_HPP__\n')
          file.write('#define __BUILDER_GENERATED_HPP__\n')
          file.write('// this file was automatically generated.\n')
          file.write('// generated at {}\n'.format(datetime.datetime.now().strftime('%Y%m%d-%H%M%S')))
          for key, val in zip(build_argument_names, args):
            file.write('#define {} {}\n'.format(key, int(val)))
          file.write('#endif // __BUILDER_GENERATED_HPP__\n')
        break
      except IOError:
        print('retry')
        continue
  
  def clean_header(self):
    while True:
      try:
        with open(self.header_path, 'w') as file:
          file.write('// this file was automatically generated.\n')
          file.write('// generated at {}\n'.format(datetime.datetime.now().strftime('%Y%m%d-%H%M%S')))
        break
      except IOError:
        print('retry')
        continue

  def move_exe(self):
    shutil.copyfile(
        r'..\tanuki-\x64\Release\tanuki-.exe',
        r'tanuki-modified.exe',
        )

  def build(self, args):
    self.update_header(args)

    log_path = 'make-native.txt'
    popenargs = [
      self.devenv_path,
      r'..\tanuki-\tanuki-.sln',
      '/Build',
      'Release|x64',
      '/Project',
      r'..\tanuki-\tanuki-\tanuki-.vcxproj',
      '/Out',
      log_path,
      ]
    print('Executing: make native (MSVC) ...')
    while True:
      try:
        if os.path.isfile(log_path):
          os.unlink(log_path)
        output = subprocess.call(popenargs)
        break
      except subprocess.CalledProcessError:
        print('retry')
        continue

    self.move_exe()
    self.clean_header()

  def kill(self, process_name):
    subprocess.call(['taskkill', '/T', '/F', '/IM', process_name + '.exe'])

if __name__=='__main__':
  # build environment.
  if commandline_args.builder == 'MSVC':
    builder = MSVCBuilder()
  else:
    builder = MSYSBuilder()

def function(args):
  print('-' * 78)

  print(datetime.datetime.today().strftime("%Y-%m-%d %H:%M:%S"))

  global COUNTER
  print(args)

  if COUNTER:
    current_time_sec = time.time()
    delta = current_time_sec - START_TIME_SEC
    sec_per_one = delta / COUNTER
    remaining = datetime.timedelta(seconds=sec_per_one*(MAX_EVALS-COUNTER))
    print(COUNTER, '/', MAX_EVALS, str(remaining))
  COUNTER += 1

  builder.clean()
  builder.build(args)

  popenargs = ['./YaneuraOu.exe',]
  print(popenargs)
  output = None
  try:
    with open('yaneuraou-config.txt', 'r') as file:
      output = subprocess.check_output(popenargs, stdin=file)
  except subprocess.CalledProcessError:
    pass
  print(output)
  matched = re.compile('GameResult (\\d+) - (\\d+) - (\\d+)').search(output)
  lose = float(matched.group(1))
  draw = float(matched.group(2))
  win = float(matched.group(3))
  ratio = 0.0
  if lose + draw + win > 0.1:
   ratio = win / (lose + draw + win)
  print ratio

  builder.kill('tanuki-baseline')
  builder.kill('tanuki-modified')

  global state
  state.record_iteration(
      args=args,
      output=output,
      lose=lose,
      draw=draw,
      win=win,
      )
  if commandline_args.store_interval > 0 and state.get_n_accumulated_iterations() % commandline_args.store_interval == 0:
    state.save(state_store_path)

  return -ratio

if __name__=='__main__':
  # shutil.copyfile('../tanuki-/x64/Release/tanuki-.exe', 'tanuki-.exe')
  best = fmin(function, space, algo=tpe.suggest, max_evals=state.calc_max_evals(MAX_EVALS), trials=state.get_trials())
  print("best estimate parameters", best)
  for key in sorted(best.keys()):
    print("{0}={1}".format(key, str(int(best[key]))))

