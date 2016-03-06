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
from hyperopt import fmin, tpe, hp, rand
from math import log
import datetime
import re
import shutil
import subprocess
import time

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
  hp.quniform('SEARCH_PROBCUT_DEPTH_THRESHOLD', 2, 20, 1),
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

COUNTER = 0;
MAX_EVALS = 300;
START_TIME_SEC = time.time()

def function(args):
  print('-' * 78)

  global COUNTER
  print(args)

  if COUNTER:
    current_time_sec = time.time()
    delta = current_time_sec - START_TIME_SEC
    sec_per_one = delta / COUNTER
    remaining = datetime.timedelta(seconds=sec_per_one*(MAX_EVALS-COUNTER))
    print(COUNTER, '/', MAX_EVALS, str(remaining))
  COUNTER += 1

  popenargs = [
    'make',
    '-j4',
    'clean',
    'native',
    'TARGET_PREFIX=tanuki-modified',
    'QSEARCH_FUTILITY_MARGIN=' + str(int(args[0])),
    'SEARCH_FUTILITY_MARGIN_DEPTH_THRESHOLD=' + str(int(args[1])),
    'SEARCH_FUTILITY_MARGIN_INTERCEPT=' + str(int(args[2])),
    'SEARCH_FUTILITY_MARGIN_LOG_D_COEFFICIENT=' + str(int(args[3])),
    'SEARCH_FUTILITY_MARGIN_MOVE_COUNT_COEFFICIENT=' + str(int(args[4])),
    'SEARCH_FUTILITY_MOVE_COUNTS_INTERCEPT=' + str(int(args[5])),
    'SEARCH_FUTILITY_MOVE_COUNTS_POWER=' + str(int(args[6])),
    'SEARCH_FUTILITY_MOVE_COUNTS_SCALE=' + str(int(args[7])),
    'SEARCH_FUTILITY_PRUNING_NON_PV_REDUCTION_INTERCEPT=' + str(int(args[8])),
    'SEARCH_FUTILITY_PRUNING_NON_PV_REDUCTION_SLOPE=' + str(int(args[9])),
    'SEARCH_FUTILITY_PRUNING_PREDICTED_DEPTH_THRESHOLD=' + str(int(args[10])),
    'SEARCH_FUTILITY_PRUNING_PV_REDUCTION_INTERCEPT=' + str(int(args[11])),
    'SEARCH_FUTILITY_PRUNING_PV_REDUCTION_SLOPE=' + str(int(args[12])),
    'SEARCH_FUTILITY_PRUNING_SCORE_GAIN_SLOPE=' + str(int(args[13])),
    'SEARCH_INTERNAL_ITERATIVE_DEEPENING_NON_PV_DEPTH_SCALE=' + str(int(args[14])),
    'SEARCH_INTERNAL_ITERATIVE_DEEPENING_NON_PV_NODE_DEPTH_THRESHOLD=' + str(int(args[15])),
    'SEARCH_INTERNAL_ITERATIVE_DEEPENING_PV_NODE_DEPTH_DELTA=' + str(int(args[16])),
    'SEARCH_INTERNAL_ITERATIVE_DEEPENING_PV_NODE_DEPTH_THRESHOLD=' + str(int(args[17])),
    'SEARCH_INTERNAL_ITERATIVE_DEEPENING_SCORE_MARGIN=' + str(int(args[18])),
    'SEARCH_LATE_MOVE_REDUCTION_DEPTH_THRESHOLD=' + str(int(args[19])),
    'SEARCH_NULL_FAIL_LOW_SCORE_DEPTH_THRESHOLD=' + str(int(args[20])),
    'SEARCH_NULL_MOVE_DEPTH_THRESHOLD=' + str(int(args[21])),
    'SEARCH_NULL_MOVE_MARGIN=' + str(int(args[22])),
    'SEARCH_NULL_MOVE_NULL_SCORE_DEPTH_THRESHOLD=' + str(int(args[23])),
    'SEARCH_NULL_MOVE_REDUCTION_INTERCEPT=' + str(int(args[24])),
    'SEARCH_NULL_MOVE_REDUCTION_SLOPE=' + str(int(args[25])),
    'SEARCH_PROBCUT_DEPTH_THRESHOLD=' + str(int(args[26])),
    'SEARCH_PROBCUT_RBETA_DEPTH_DELTA=' + str(int(args[27])),
    'SEARCH_PROBCUT_RBETA_SCORE_DELTA=' + str(int(args[28])),
    'SEARCH_RAZORING_DEPTH=' + str(int(args[29])),
    'SEARCH_RAZORING_MARGIN_INTERCEPT=' + str(int(args[30])),
    'SEARCH_RAZORING_MARGIN_SLOPE=' + str(int(args[31])),
    'SEARCH_SINGULAR_EXTENSION_DEPTH_THRESHOLD=' + str(int(args[32])),
    'SEARCH_SINGULAR_EXTENSION_NULL_WINDOW_SEARCH_DEPTH_SCALE=' + str(int(args[33])),
    'SEARCH_SINGULAR_EXTENSION_TTE_DEPTH_THRESHOLD=' + str(int(args[34])),
    'SEARCH_STATIC_NULL_MOVE_PRUNING_DEPTH_THRESHOLD=' + str(int(args[35])),
  ]
  print(popenargs)
  while True:
    try:
      subprocess.check_output(popenargs)
      break
    except subprocess.CalledProcessError:
      continue

  popenargs = [
    './YaneuraOu.exe',
    'go',
    'btime',
    '50',
  ]
  print(popenargs)
  output = subprocess.check_output(popenargs)
  print(output)
  matched = re.compile('GameResult (\\d+) - (\\d+) - (\\d+)').search(output)
  lose = float(matched.group(1))
  draw = float(matched.group(2))
  win = float(matched.group(3))
  ratio = win / (lose + draw + win)
  print ratio

  subprocess.call(['pkill', 'tanuki-baseline'])
  subprocess.call(['pkill', 'tanuki-modified'])

  return -ratio

# shutil.copyfile('../tanuki-/x64/Release/tanuki-.exe', 'tanuki-.exe')
best = fmin(function, space, algo=tpe.suggest, max_evals=MAX_EVALS)
print("best estimate parameters", best)
for key in sorted(best.keys()):
  print("{0}={1}".format(key, str(int(best[key]))))
