#!/usr/bin/python
# coding:utf-8

import argparse
import csv
import datetime
import enum
import os
import re
import shutil
import subprocess
import sys


class State(enum.Enum):
  generate_kifu = 1
  generate_kifu_for_test = 2
  shuffle_kifu = 3
  learn = 4
  self_play = 5


def GetSubfolders(folder_path):
  return [x for x in os.listdir(folder_path) if os.path.isdir(os.path.join(folder_path, x))]


def GetDateTimeString():
  now = datetime.datetime.now()
  return now.strftime("%Y-%m-%d-%H-%M-%S")


def AdoptSubfolder(subfolder):
  num_positions = int(subfolder)
  return num_positions % 1000000000 == 0 or num_positions % 10000 != 0


def GenerateKifu(args, eval_folder_path, kifu_folder_path, num_positions, kifu_tag):
  print(locals(), flush=True)
  input = '''usi
setoption name EvalDir value {eval_folder_path}
setoption name KifuDir value {kifu_folder_path}
setoption name Threads value {num_threads_to_generate_kifu}
setoption name Hash value 16384
setoption name GeneratorNumPositions value {num_positions}
setoption name GeneratorMinSearchDepth value {search_depth}
setoption name GeneratorMaxSearchDepth value {search_depth}
setoption name GeneratorKifuTag value {kifu_tag}
setoption name GeneratorStartposFileName value startpos.sfen
setoption name GeneratorMinBookMove value 0
setoption name GeneratorMaxBookMove value 32
setoption name GeneratorValueThreshold value {value_threshold}
setoption name MultiPV value 5
setoption name GeneratorMaxValueDifferenceInMultiPv value {max_value_difference_in_multi_pv}
isready
usinewgame
generate_kifu
'''.format(
  eval_folder_path=eval_folder_path,
  kifu_folder_path=kifu_folder_path,
  num_threads_to_generate_kifu=args.num_threads_to_generate_kifu,
  num_positions=num_positions,
  search_depth=args.search_depth,
  kifu_tag=kifu_tag,
  value_threshold=args.value_threshold,
  max_value_difference_in_multi_pv=args.max_value_difference_in_multi_pv).encode('utf-8')
  print(input.decode('utf-8'), flush=True)
  subprocess.run([args.generate_kifu_exe_file_path], input=input, check=True)


def ShuffleKifu(args, kifu_folder_path, shuffled_kifu_folder_path):
  print(locals(), flush=True)
  input = '''usi
setoption name KifuDir value {kifu_folder_path}
setoption name ShuffledKifuDir value {shuffled_kifu_folder_path}
isready
usinewgame
shuffle_kifu
'''.format(
  kifu_folder_path=kifu_folder_path,
  shuffled_kifu_folder_path=shuffled_kifu_folder_path).encode('utf-8')
  print(input.decode('utf-8'), flush=True)
  subprocess.run([args.generate_kifu_exe_file_path], input=input, check=True)


def Learn(args, eval_folder_path, kifu_folder_path, kif_for_test_folder_path,
          new_eval_folder_path_base):
  print(locals(), flush=True)
  input = '''usi
setoption name Threads value {num_threads_to_learn}
setoption name MaxMovesToDraw value 300
setoption name EvalDir value {eval_folder_path}
setoption name KifuDir value {kifu_folder_path}
setoption name LearnerNumPositions value {num_positions_to_learn}
setoption name MinLearningRate value {min_learning_rate}
setoption name MaxLearningRate value {max_learning_rate}
setoption name NumLearningRateCycles value {num_learning_rate_cycles}
setoption name KifuForTestDir value {kif_for_test_folder_path}
setoption name LearnerNumPositionsForTest value 1000000
setoption name MiniBatchSize value {mini_batch_size}
setoption name FobosL1Parameter value {fobos_l1_parameter}
setoption name FobosL2Parameter value {fobos_l2_parameter}
setoption name ElmoLambda value {elmo_lambda}
setoption name ValueToWinningRateCoefficient value {value_to_winning_rate_coefficient}
setoption name AdamBeta2 value {adam_beta2}
setoption name UseProgressAsElmoLambda value {use_progress_as_elmo_lambda}
isready
usinewgame
learn output_folder_path_base {learner_output_folder_path_base}
'''.format(
  num_threads_to_learn=args.num_threads_to_learn,
  eval_folder_path=eval_folder_path,
  kifu_folder_path=kifu_folder_path,
  num_positions_to_learn=args.num_positions_to_learn,
  kif_for_test_folder_path=kif_for_test_folder_path,
  learner_output_folder_path_base=new_eval_folder_path_base,
  min_learning_rate=args.min_learning_rate,
  max_learning_rate=args.max_learning_rate,
  num_learning_rate_cycles=args.num_learning_rate_cycles,
  mini_batch_size=args.mini_batch_size,
  fobos_l1_parameter=args.fobos_l1_parameter,
  fobos_l2_parameter=args.fobos_l2_parameter,
  elmo_lambda=args.elmo_lambda,
  value_to_winning_rate_coefficient=args.value_to_winning_rate_coefficient,
  adam_beta2=args.adam_beta2,
  use_progress_as_elmo_lambda=args.use_progress_as_elmo_lambda).encode('utf-8')
  print(input.decode('utf-8'), flush=True)
  subprocess.run([args.learner_exe_file_path], input=input, check=True)


def SelfPlay(args, old_eval_folder_path, new_eval_folder_path):
  print(locals(), flush=True)
  args = [
    'TanukiColiseum.exe',
    '--engine1', args.selfplay_exe_file_path,
    '--engine2', args.selfplay_exe_file_path,
    '--eval1', new_eval_folder_path,
    '--eval2', old_eval_folder_path,
    '--num_concurrent_games', str(args.num_threads_to_selfplay),
    '--num_games', str(args.num_games_to_selfplay),
    '--hash', str(args.self_play_hash_size),
    '--time', str(args.thinking_time_ms),
    '--num_numa_nodes', str(args.num_numa_nodes),
    '--num_book_moves1', '0',
    '--num_book_moves2', '0',
    '--book_file_name1', 'no_book',
    '--book_file_name2', 'no_book',
    '--num_book_moves', '24',
    '--no_gui',
    '--sfen_file_name', args.startpos_file_name_for_self_play,
	]
  print(args, flush=True)
  if subprocess.run(args).returncode:
    sys.exit('Failed to calculate the winning rate...');


def GetSubfolders(folder_path):
  return [x for x in os.listdir(folder_path) if os.path.isdir(os.path.join(folder_path, x))]


def main():
  parser = argparse.ArgumentParser(description='iteration')
  parser.add_argument(
    '--learner_output_folder_path_base',
    action='store',
    required=True,
    help='Folder path baseof the output folders. ex) eval')
  parser.add_argument(
    '--kifu_output_folder_path_base',
    action='store',
    required=True,
    help='Folder path baseof the output kifu. ex) kifu')
  parser.add_argument(
    '--kifu_for_test_output_folder_path_base',
    action='store',
    required=True,
    help='Folder path baseof the output kifu for test. ex) kifu for test')
  parser.add_argument(
    '--initial_eval_folder_path',
    action='store',
    required=True,
    help='Folder path of the inintial eval files. ex) eval')
  parser.add_argument(
    '--initial_kifu_folder_path',
    action='store',
    required=True,
    help='Folder path of the inintial kifu files. ex) kifu/2017-05-25')
  parser.add_argument(
    '--initial_kifu_for_test_folder_path',
    action='store',
    required=True,
    help='Folder path of the inintial kifu files for test. ex) kifu_for_test/2017-05-25')
  parser.add_argument(
    '--initial_shuffled_kifu_folder_path',
    action='store',
    required=True,
    help='Folder path of the inintial shuffled kifu files. ex) kifu/2017-05-25-shuffled')
  parser.add_argument(
    '--initial_new_eval_folder_path_base',
    action='store',
    required=True,
    help='Folder path base of the inintial new eval files. ex) eval')
  parser.add_argument(
    '--initial_state',
    action='store',
    required=True,
    help='Initial state. [' + ', '.join([state.name for state in State]) + ']')
  parser.add_argument(
    '--final_state',
    action='store',
    required=True,
    help='Final state. [' + ', '.join([state.name for state in State]) + ']')
  parser.add_argument(
    '--generate_kifu_exe_file_path',
    action='store',
    required=True,
    help='Exe file name of the kifu generator. ex) YaneuraOu.2016-08-05.learn.exe')
  parser.add_argument(
    '--learner_exe_file_path',
    action='store',
    required=True,
    help='Exe file name of the learner. ex) YaneuraOu.2016-08-05.generate_kifu.exe')
  parser.add_argument(
    '--num_threads_to_generate_kifu',
    action='store',
    type=int,
    required=True,
    help='Number of the threads used for learning. ex) 8')
  parser.add_argument(
    '--num_threads_to_learn',
    action='store',
    type=int,
    required=True,
    help='Number of the threads used for learning. ex) 8')
  parser.add_argument(
    '--num_threads_to_selfplay',
    action='store',
    type=int,
    required=True,
    help='Number of the threads used for selfplay. ex) 8')
  parser.add_argument(
    '--num_games_to_selfplay',
    action='store',
    type=int,
    required=True,
    help='Number of the games to play for selfplay. ex) 100')
  parser.add_argument(
    '--num_positions_to_generator_train',
    action='store',
    type=int,
    required=True,
    help='Number of the games to play for generator. ex) 100')
  parser.add_argument(
    '--num_positions_to_generator_test',
    action='store',
    type=int,
    required=True,
    help='Number of the games to play for generator. ex) 100')
  parser.add_argument(
    '--num_positions_to_learn',
    action='store',
    type=int,
    required=True,
    help='Number of the positions for learning. ex) 10000')
  parser.add_argument(
    '--num_iterations',
    action='store',
    type=int,
    required=True,
    help='Number of the iterations. ex) 100')
  parser.add_argument(
    '--search_depth',
    action='store',
    type=int,
    required=True,
    help='Search depth. ex) 8')
  parser.add_argument(
    '--min_learning_rate',
    action='store',
    type=float,
    required=True,
    help='Min learning rate. ex) 2.0')
  parser.add_argument(
    '--max_learning_rate',
    action='store',
    type=float,
    required=True,
    help='Max learning rate. ex) 2.0')
  parser.add_argument(
    '--num_learning_rate_cycles',
    action='store',
    type=float,
    required=True,
    help='Number of learning rate cycles. ex) 10')
  parser.add_argument(
    '--mini_batch_size',
    action='store',
    type=int,
    required=True,
    help='Learning rate. ex) 1000000')
  parser.add_argument(
    '--fobos_l1_parameter',
    action='store',
    type=float,
    required=True,
    help='Learning rate. ex) 0.0')
  parser.add_argument(
    '--fobos_l2_parameter',
    action='store',
    type=float,
    required=True,
    help='Learning rate. ex) 0.99989464503')
  parser.add_argument(
    '--num_numa_nodes',
    action='store',
    type=int,
    required=True,
    help='Number of the NUMA nodes. ex) 2')
  parser.add_argument(
    '--num_divisions_to_generator_train',
    action='store',
    type=int,
    required=True,
    help='Number of the divisions to generate train data. ex) 10')
  parser.add_argument(
    '--initial_division_to_generator_train',
    action='store',
    type=int,
    required=True,
    help='Initial division to generate train data. ex) 2')
  parser.add_argument(
    '--reference_eval_folder_paths',
    action='store',
    required=True,
    help='Comma-separated folder paths for reference eval files. ex) eval/tanuki_wcsc27,eval/elmo_wcsc27')
  parser.add_argument(
    '--selfplay_exe_file_path',
    action='store',
    required=True,
    help='Exe file name for the self plays. ex) tanuki-wcsc27-2017-05-07-1-avx2.exe')
  parser.add_argument(
    '--thinking_time_ms',
    action='store',
    type=int,
    required=True,
    help='Thinking time for self play in milliseconds. ex) 1000')
  parser.add_argument(
    '--self_play_hash_size',
    action='store',
    type=int,
    required=True,
    help='Hash size for self play. ex) 256')
  parser.add_argument(
    '--elmo_lambda',
    action='store',
    type=float,
    required=True,
    help='Elmo Lambda. ex) 1.0')
  parser.add_argument(
    '--value_threshold',
    action='store',
    type=int,
    required=True,
    help='Value threshold to include positions to the learning data. ex) 30000')
  parser.add_argument(
    '--value_to_winning_rate_coefficient',
    action='store',
    type=float,
    required=True,
    help='Coefficient to convert a value to the winning rate. ex) 600.0')
  parser.add_argument(
    '--max_value_difference_in_multi_pv',
    action='store',
    type=int,
    required=True,
    help='Max value difference in Multi PV. ex) 100')
  parser.add_argument(
    '--adam_beta2',
    action='store',
    type=float,
    required=True,
    help='Adam beta2 coefficient. ex) 0.999')
  parser.add_argument(
    '--use_progress_as_elmo_lambda',
    action='store',
    type=str,
    required=True,
    help='"true" to use progress as elmo lambda. Otherwise, "false" ex) true')
  parser.add_argument(
    '--startpos_file_name_for_self_play',
    action='store',
    default='records_2017-05-19.sfen',
    type=str,
    help='')
  
  
  args = parser.parse_args()

  learner_output_folder_path_base = args.learner_output_folder_path_base
  kifu_output_folder_path_base = args.kifu_output_folder_path_base
  kifu_for_test_output_folder_path_base = args.kifu_for_test_output_folder_path_base
  initial_eval_folder_path = args.initial_eval_folder_path
  initial_kifu_folder_path = args.initial_kifu_folder_path
  initial_kifu_for_test_folder_path = args.initial_kifu_for_test_folder_path
  initial_shuffled_kifu_folder_path = args.initial_shuffled_kifu_folder_path
  initial_new_eval_folder_path_base = args.initial_new_eval_folder_path_base
  initial_state = State[args.initial_state]
  if not initial_state:
    sys.exit('Unknown initial state: %s' % args.initial_state)
  final_state = State[args.final_state]
  if not final_state:
    sys.exit('Unknown final state: %s' % args.initial_state)
  if args.reference_eval_folder_paths:
    reference_eval_folder_paths = args.reference_eval_folder_paths.split(',')
  else:
    reference_eval_folder_paths = []
  generate_kifu_exe_file_path = args.generate_kifu_exe_file_path
  learner_exe_file_path = args.learner_exe_file_path
  num_threads_to_generate_kifu = args.num_threads_to_generate_kifu
  num_threads_to_learn = args.num_threads_to_learn
  num_threads_to_selfplay = args.num_threads_to_selfplay
  num_games_to_selfplay = args.num_games_to_selfplay
  num_positions_to_generator_train = args.num_positions_to_generator_train
  num_positions_to_generator_test = args.num_positions_to_generator_test
  num_positions_to_learn = args.num_positions_to_learn
  num_iterations = args.num_iterations
  search_depth = args.search_depth
  mini_batch_size = args.mini_batch_size
  fobos_l1_parameter = args.fobos_l1_parameter
  fobos_l2_parameter = args.fobos_l2_parameter
  num_numa_nodes = args.num_numa_nodes
  num_divisions_to_generator_train = args.num_divisions_to_generator_train
  initial_division_to_generator_train = args.initial_division_to_generator_train

  kifu_folder_path = initial_kifu_folder_path
  kifu_for_test_folder_path = initial_kifu_for_test_folder_path
  shuffled_kifu_folder_path = initial_shuffled_kifu_folder_path
  old_eval_folder_path = initial_eval_folder_path
  new_eval_folder_path = os.path.join(initial_new_eval_folder_path_base, str(num_positions_to_learn))
  state = initial_state

  iteration = 0
  while iteration < num_iterations:
    print('-' * 80)
    print('- %s' % state)
    print('-' * 80, flush=True)

    stop_on_this_state = (state == final_state)

    if state == State.generate_kifu:
      kifu_folder_path = os.path.join(kifu_output_folder_path_base, GetDateTimeString())
      for division in range(initial_division_to_generator_train, num_divisions_to_generator_train):
          GenerateKifu(args, old_eval_folder_path, kifu_folder_path,
                       int(num_positions_to_generator_train / num_divisions_to_generator_train),
                       'train.{0}'.format(division))
      state = State.generate_kifu_for_test

    elif state == State.generate_kifu_for_test:
      kifu_for_test_folder_path = os.path.join(kifu_for_test_output_folder_path_base,
                                               GetDateTimeString())
      GenerateKifu(args, old_eval_folder_path, kifu_for_test_folder_path,
                   num_positions_to_generator_test, 'test')
      state = State.shuffle_kifu

    elif state == State.shuffle_kifu:
      shuffled_kifu_folder_path = kifu_folder_path + '-shuffled'
      ShuffleKifu(args, kifu_folder_path, shuffled_kifu_folder_path)
      state = State.learn

    elif state == State.learn:
      new_eval_folder_path_base = os.path.join(learner_output_folder_path_base,
                                               GetDateTimeString())
      Learn(args, old_eval_folder_path, shuffled_kifu_folder_path, kifu_for_test_folder_path,
            new_eval_folder_path_base)
      new_eval_folder_path = os.path.join(new_eval_folder_path_base, str(num_positions_to_learn))
      state = State.self_play

    elif state == State.self_play:
      for reference_eval_folder_path in [old_eval_folder_path] + reference_eval_folder_paths:
        SelfPlay(args, reference_eval_folder_path, new_eval_folder_path)
      state = State.generate_kifu
      iteration += 1
      old_eval_folder_path = new_eval_folder_path

    else:
      sys.exit('Invalid state: state=%s' % state)

    if stop_on_this_state:
      break


if __name__ == '__main__':
  main()
