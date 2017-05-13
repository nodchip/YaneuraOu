#!/usr/bin/python
# coding:utf-8
#
# Preparation
#
# 1. Install Python 2.7.* 64-bit ver
# - Python 2.7.* 64bit
# -- http://www.python.org
#
# 2. Download the following libraries.
# - numpy-*.*.*+mkl-cp27-cp27m-win_amd64.whl
# -- http://www.lfd.uci.edu/~gohlke/pythonlibs/
# - scipy-*.*.*-cp27-none-win_amd64.whl
# -- http://www.lfd.uci.edu/~gohlke/pythonlibs/
#
# 3. Execute the following command.
# - python -m pip install --upgrade pip
# - pip install numpy-*.*.*+mkl-cp27-cp27m-win_amd64.whl scipy-*.*.*-cp27-none-win_amd64.whl hyperopt pymongo networkx pandas sklearn matplotlib
#
# 4. If using MSVC instead of MSYS (Windows)
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
from hyperopt_state import HyperoptState

space = [
  hp.quniform('PARAM_FUTILITY_MARGIN_ALPHA', 100, 240, 1),
  hp.quniform('PARAM_FUTILITY_MARGIN_BETA', 100, 240, 1),
  hp.quniform('PARAM_FUTILITY_MARGIN_QUIET', 50, 160, 1),
  hp.quniform('PARAM_FUTILITY_RETURN_DEPTH', 5, 13, 1),
  hp.quniform('PARAM_FUTILITY_AT_PARENT_NODE_DEPTH', 5, 13, 1),
  hp.quniform('PARAM_FUTILITY_AT_PARENT_NODE_MARGIN1', 100, 300, 1),
  hp.quniform('PARAM_FUTILITY_AT_PARENT_NODE_MARGIN2', 0, 300, 1),
  hp.quniform('PARAM_FUTILITY_AT_PARENT_NODE_GAMMA1', 20, 50, 1),
  hp.quniform('PARAM_FUTILITY_AT_PARENT_NODE_GAMMA2', 20, 60, 1),
  hp.quniform('PARAM_NULL_MOVE_DYNAMIC_ALPHA', 500, 1500, 1),
  hp.quniform('PARAM_NULL_MOVE_DYNAMIC_BETA', 50, 100, 1),
  hp.quniform('PARAM_NULL_MOVE_MARGIN', 10, 60, 1),
  hp.quniform('PARAM_NULL_MOVE_RETURN_DEPTH', 4, 15, 1),
  hp.quniform('PARAM_PROBCUT_DEPTH', 3, 10, 1),
  hp.quniform('PARAM_PROBCUT_MARGIN', 100, 300, 1),
  hp.quniform('PARAM_SINGULAR_EXTENSION_DEPTH', 4, 13, 1),
  hp.quniform('PARAM_SINGULAR_MARGIN', 128, 400, 1),
  hp.quniform('PARAM_SINGULAR_SEARCH_DEPTH_ALPHA', 8, 32, 1),
  hp.quniform('PARAM_PRUNING_BY_MOVE_COUNT_DEPTH', 8, 32, 1),
  hp.quniform('PARAM_PRUNING_BY_HISTORY_DEPTH', 2, 32, 1),
  hp.quniform('PARAM_REDUCTION_BY_HISTORY', 2000, 8000, 1),
  hp.quniform('PARAM_IID_MARGIN_ALPHA', 128, 384, 1),
  hp.quniform('PARAM_RAZORING_MARGIN2', 400, 700, 1),
  hp.quniform('PARAM_RAZORING_MARGIN3', 400, 700, 1),
  hp.quniform('PARAM_RAZORING_MARGIN4', 400, 700, 1),
  hp.quniform('PARAM_REDUCTION_ALPHA', 64, 256, 1),
  hp.quniform('PARAM_FUTILITY_MOVE_COUNT_ALPHA0', 150, 400, 1),
  hp.quniform('PARAM_FUTILITY_MOVE_COUNT_ALPHA1', 300, 600, 1),
  hp.quniform('PARAM_FUTILITY_MOVE_COUNT_BETA0', 500, 2000, 1),
  hp.quniform('PARAM_FUTILITY_MOVE_COUNT_BETA1', 500, 2000, 1),
  hp.quniform('PARAM_QUIET_SEARCH_COUNT', 32, 128, 1),
]

build_argument_names = [
  'PARAM_FUTILITY_MARGIN_ALPHA',
  'PARAM_FUTILITY_MARGIN_BETA',
  'PARAM_FUTILITY_MARGIN_QUIET',
  'PARAM_FUTILITY_RETURN_DEPTH',
  'PARAM_FUTILITY_AT_PARENT_NODE_DEPTH',
  'PARAM_FUTILITY_AT_PARENT_NODE_MARGIN1',
  'PARAM_FUTILITY_AT_PARENT_NODE_MARGIN2',
  'PARAM_FUTILITY_AT_PARENT_NODE_GAMMA1',
  'PARAM_FUTILITY_AT_PARENT_NODE_GAMMA2',
  'PARAM_NULL_MOVE_DYNAMIC_ALPHA',
  'PARAM_NULL_MOVE_DYNAMIC_BETA',
  'PARAM_NULL_MOVE_MARGIN',
  'PARAM_NULL_MOVE_RETURN_DEPTH',
  'PARAM_PROBCUT_DEPTH',
  'PARAM_PROBCUT_MARGIN',
  'PARAM_SINGULAR_EXTENSION_DEPTH',
  'PARAM_SINGULAR_MARGIN',
  'PARAM_SINGULAR_SEARCH_DEPTH_ALPHA',
  'PARAM_PRUNING_BY_MOVE_COUNT_DEPTH',
  'PARAM_PRUNING_BY_HISTORY_DEPTH',
  'PARAM_REDUCTION_BY_HISTORY',
  'PARAM_IID_MARGIN_ALPHA',
  'PARAM_RAZORING_MARGIN2',
  'PARAM_RAZORING_MARGIN3',
  'PARAM_RAZORING_MARGIN4',
  'PARAM_REDUCTION_ALPHA',
  'PARAM_FUTILITY_MOVE_COUNT_ALPHA0',
  'PARAM_FUTILITY_MOVE_COUNT_ALPHA1',
  'PARAM_FUTILITY_MOVE_COUNT_BETA0',
  'PARAM_FUTILITY_MOVE_COUNT_BETA1',
  'PARAM_QUIET_SEARCH_COUNT',
]

START_COUNTER = 0
CURRENT_COUNTER = 0
MAX_EVALS = 10000
START_TIME_SEC = time.time()
EVAL_DIR = r'eval'
ENGINE1 = r'YaneuraOu-2017-early-temp.exe'
ENGINE2 = r'YaneuraOu-2017-early-default.exe'
NUM_THREADS = 24
THINKING_TIME_MS = 10000
NUM_NUMA_NODES = 1
HASH = 256
HISTOGRAM_WIDTH = 80


class YaneuraouBuilder(object):
  FILENAME = 'param/2017-early-param-temp.h'
  def __init__(self):
    pass

  def clean(self):
    try:
      os.remove(self.FILENAME)
    except WindowsError:
      pass

  def build(self, args):
    with open(self.FILENAME, 'w') as f:
      f.write('#ifndef _2017_EARLY_PARAMETERS_\n')
      f.write('#define _2017_EARLY_PARAMETERS_\n')
      for key, val in zip(build_argument_names, args):
        f.write('PARAM_DEFINE {0} = {1};\n'.format(key, str(int(val))))
      f.write('''
PARAM_DEFINE PARAM_RAZORING_MARGIN1 = 483;
PARAM_DEFINE PARAM_QSEARCH_MATE1 = 1;
PARAM_DEFINE PARAM_SEARCH_MATE1 = 1;
PARAM_DEFINE PARAM_WEAK_MATE_PLY = 1;
''')
      f.write('#endif\n')

  def kill(self, process_name):
    subprocess.call(['taskkill', '/T', '/F', '/IM', process_name])


def function(args):
  print('-' * 78)

  print(datetime.datetime.today().strftime("%Y-%m-%d %H:%M:%S"))

  global START_COUNTER
  global CURRENT_COUNTER
  global MAX_EVALS
  global START_TIME_SEC
  global EVAL_DIR
  global ENGINE1
  global ENGINE2
  global NUM_THREADS
  global THINKING_TIME_MS
  global NUM_NUMA_NODES
  global HASH
  print(args)
  sys.stdout.flush()

  if START_COUNTER < CURRENT_COUNTER:
    current_time_sec = time.time()
    delta = current_time_sec - START_TIME_SEC
    sec_per_one = delta / (CURRENT_COUNTER - START_COUNTER)
    remaining = datetime.timedelta(seconds=sec_per_one * (MAX_EVALS - CURRENT_COUNTER))
    print(CURRENT_COUNTER, '/', MAX_EVALS, str(remaining))
    sys.stdout.flush()
  CURRENT_COUNTER += 1

  builder.clean()
  builder.build(args)

  engine_invoker_args = [
    'TanukiColiseum.exe',
    '--engine1', ENGINE1,
    '--engine2', ENGINE2,
    '--eval1', EVAL_DIR,
    '--eval2', EVAL_DIR,
    '--num_concurrent_games', str(NUM_THREADS),
    '--num_games', str(NUM_THREADS),
    '--hash', str(HASH),
    '--time', str(THINKING_TIME_MS),
    '--num_numa_nodes', str(NUM_NUMA_NODES)]

  print(engine_invoker_args)
  sys.stdout.flush()

  process = subprocess.Popen(engine_invoker_args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
  stdoutdata, stderrdata = process.communicate()
  if stdoutdata:
    print('-' * 78)
    print('- stdout')
    print('-' * 78)
    print('')
    print(stdoutdata)
    print('')
  if stderrdata:
    print('-' * 78)
    print('- sterr')
    print('-' * 78)
    print('')
    print(stderrdata)
    print('')
  sys.stdout.flush()

  if process.returncode:
    sys.exit('Failed to execute engine_invoker...')

  win = 0
  draw = 0
  lose = 0
  for match in re.compile(',(\\d+) - (\\d+) - (\\d+)\\(').finditer(stdoutdata):
    win = float(match.group(1))
    draw = float(match.group(2))
    lose = float(match.group(3))

  ratio = 0.0
  if lose + draw + win > 0.1:
   ratio = (win * 1.0 + draw * 0.5 + lose * 0.0) / (lose + draw + win)
  print('ratio={0}'.format(ratio))
  sys.stdout.flush()

  builder.kill(ENGINE1)
  builder.kill(ENGINE2)

  global state
  state.record_iteration(args=args,
      output=stdoutdata,
      lose=lose,
      draw=draw,
      win=win,)
  if commandline_args.store_interval > 0 and state.get_n_accumulated_iterations() % commandline_args.store_interval == 0:
    state.save(state_store_path)

  # Show the histogram.
  hist = [0] * (NUM_THREADS + 1)
  max_win = 0
  for record in state.iteration_logs:
    win = int(record['win'])
    hist[win] += 1
    max_win = max(max_win, hist[win])

  for win, count in enumerate(hist):
    print('{0:4d} '.format(win) + '*' * (count * HISTOGRAM_WIDTH / max_win))
  sys.stdout.flush()

  return -ratio

# arguments
if __name__ == '__main__':
  parser = argparse.ArgumentParser('optimize_parameters.py')
  parser.add_argument('--store-interval', type=int, default=1,
      help=u'store internal state of hyper-parameter search after every <store_interval> iterations. set 0 to disable storing.')
  parser.add_argument('--resume', type=str, default=None,
      help=u'resume hyper-parameter search from a file.')
  parser.add_argument('--dump-log', type=str, default=None,
      help=u'open a hyper-parameter search file and dump its log.')
  parser.add_argument('--max-evals', type=int, default=MAX_EVALS,
      help=u'max evaluation for hyperopt. (default: use MAX_EVALS={})'.format(MAX_EVALS))
  parser.add_argument('--num_threads', type=int, default=NUM_THREADS,
      help=u'number of threads. (default: use NUM_THREADS={})'.format(NUM_THREADS))
  parser.add_argument('--thinking_time_ms', type=int, default=THINKING_TIME_MS,
      help=u'thinking time. (default: use THINKING_TIME_MS={})'.format(THINKING_TIME_MS))
  parser.add_argument('--num_numa_nodes', type=int, default=NUM_NUMA_NODES,
      help=u'Number of the NUMA nodes. (default: use NUM_NUMA_NODES={})'.format(NUM_NUMA_NODES))
  parser.add_argument('--hash', type=int, default=HASH,
      help=u'Transposition table hash size. (default: use HASH={})'.format(HASH))
  commandline_args = parser.parse_args()
  MAX_EVALS = commandline_args.max_evals
  NUM_THREADS = commandline_args.num_threads
  THINKING_TIME_MS = commandline_args.thinking_time_ms
  NUM_NUMA_NODES = commandline_args.num_numa_nodes
  HASH = commandline_args.hash

  state = HyperoptState()
  state_store_path = 'optimize_parameters.hyperopt_state.{}.pickle'.format(datetime.datetime.now().strftime('%Y%m%d_%H%M%S'))
  if commandline_args.dump_log is not None:
    state = HyperoptState.load(commandline_args.dump_log)
    state.dump_log()
    sys.exit(0)

  if commandline_args.resume is not None:
    state = HyperoptState.load(commandline_args.resume)
    START_COUNTER = state.get_n_accumulated_iterations()
    CURRENT_COUNTER = START_COUNTER

  # build environment.
  builder = YaneuraouBuilder()

  best = fmin(function, space, algo=tpe.suggest, max_evals=state.calc_max_evals(MAX_EVALS), trials=state.get_trials())
  print("best estimate parameters", best)
  for key in sorted(best.keys()):
    print("{0}={1}".format(key, str(int(best[key]))))
