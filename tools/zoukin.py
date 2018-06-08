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


SHUFFLED_BIN_FILE_NAME = 'shuffled.bin'


class State(enum.Enum):
    generate_kifu = 1
    shuffle_kifu = 2
    learn = 3
    self_play = 4


def GetSubfolders(folder_path):
    return [x for x in os.listdir(folder_path) if os.path.isdir(os.path.join(folder_path, x))]


def GetDateTimeString():
    now = datetime.datetime.now()
    return now.strftime("%Y-%m-%d-%H-%M-%S")


def AdoptSubfolder(subfolder):
    num_positions = int(subfolder)
    return num_positions % 1000000000 == 0 or num_positions % 10000 != 0


def ToOptionValueString(b):
    if b:
        return 'True'
    else:
        return 'False'


def generate_kifu(args):
    print(locals(), flush=True)
    os.makedirs(args.kifu_dir, exist_ok=True)
    input = '''usi
EvalDir {eval_dir}
KifuDir {kifu_dir}
Threads {threads}
Hash {hash}
GeneratorNumPositions {generator_num_positions}
GeneratorSearchDepth {generator_search_depth}
GeneratorKifuTag {generator_kifu_tag}
GeneratorStartposFileName {generator_startpos_file_name}
GeneratorValueThreshold {generator_value_threshold}
GeneratorOptimumNodesSearched {generator_optimum_nodes_searched}
isready
usinewgame
generate_kifu
quit
'''.format(eval_dir=args.eval_dir,
    kifu_dir=args.kifu_dir,
    threads=args.num_threads_to_generate_kifu,
    hash=args.hash_to_generate_kifu,
    generator_num_positions=args.generator_num_positions,
    generator_search_depth=args.generator_search_depth,
    generator_kifu_tag=args.generator_kifu_tag,
    generator_startpos_file_name=args.generator_startpos_file_name,
    generator_value_threshold=args.generator_value_threshold,
    generator_optimum_nodes_searched=args.generator_optimum_nodes_searched)
    print(input, flush=True)
    subprocess.run([args.generate_kifu_exe_file_path], input=input.encode('utf-8'), check=True)


def shuffle_kifu(args, split=False):
    print(locals(), flush=True)
    shutil.rmtree(args.shuffled_kifu_dir, ignore_errors=True)
    os.makedirs(args.shuffled_kifu_dir, exist_ok=True)
    input = '''usi
SkipLoadingEval true
KifuDir {kifu_dir}
ShuffledKifuDir {shuffled_kifu_dir}
isready
usinewgame
shuffle_kifu
quit
'''.format(kifu_dir=args.kifu_dir,
           shuffled_kifu_dir=args.shuffled_kifu_dir)
    print(input, flush=True)
    subprocess.run([args.shuffle_kifu_exe_file_path], input=input.encode('utf-8'), check=True)

    if not split:
        return

    with open(os.path.join(args.shuffled_kifu_dir, SHUFFLED_BIN_FILE_NAME), 'wb') as output:
        remained = args.validation_set_file_size
        for file_name in os.listdir(shuffled_kifu_folder_path):
            with open(os.path.join(shuffled_kifu_folder_path, file_name), 'rb') as input:
                data = input.read(remained)
            remained -= len(data)
            output.write(data)


def Learn(args):
    print(locals(), flush=True)
    input = '''nnue\source\YaneuraOu-by-gcc.exe ^
EvalDir %EvalDirNNUE% , ^
SkipLoadingEval %SkipLoadingEval% , ^
Threads %Threads% , ^
EvalSaveDir %EvalSaveDir% , ^
learn targetdir %ShuffledKifuDir% ^
loop %loop% ^
batchsize %batchsize% ^
lambda %lambda% ^
eta %eta% ^
newbob_decay %newbob_decay% ^
eval_save_interval %eval_save_interval% ^
loss_output_interval %loss_output_interval% ^
mirror_percentage %mirror_percentage% ^
validation_set_file_name %ShuffledKifuDirForTest%\xaa ^
nn_batch_size %nn_batch_size% ^
eval_limit %eval_limit% || EXIT /B 1
'''.format(num_threads_to_learn=args.num_threads_to_learn,
    eval_folder_path=eval_folder_path,
    kifu_folder_path=kifu_folder_path,
    num_positions_to_learn=args.num_positions_to_learn,
    kif_for_test_folder_path=kif_for_test_folder_path,
    new_eval_folder_path=new_eval_folder_path,
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
    args = ['TanukiColiseum.exe',
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
        '--sfen_file_name', args.startpos_file_name_for_self_play,]
    print(args, flush=True)
    if subprocess.run(args).returncode:
        sys.exit('Failed to calculate the winning rate...')


def GetSubfolders(folder_path):
    return [x for x in os.listdir(folder_path) if os.path.isdir(os.path.join(folder_path, x))]


def main():
    parser = argparse.ArgumentParser(description='iteration')
    parser.add_argument('--learner_output_folder_path_base',
        action='store',
        required=True,
        help='Folder path baseof the output folders. ex) eval')
    parser.add_argument('--kifu_output_folder_path_base',
        action='store',
        required=True,
        help='Folder path baseof the output kifu. ex) kifu')
    parser.add_argument('--kifu_for_test_output_folder_path_base',
        action='store',
        required=True,
        help='Folder path baseof the output kifu for test. ex) kifu for test')
    parser.add_argument('--initial_eval_folder_path',
        action='store',
        required=True,
        help='Folder path of the inintial eval files. ex) eval')
    parser.add_argument('--initial_kifu_folder_path',
        action='store',
        required=True,
        help='Folder path of the inintial kifu files. ex) kifu/2017-05-25')
    parser.add_argument('--initial_kifu_for_test_folder_path',
        action='store',
        required=True,
        help='Folder path of the inintial kifu files for test. ex) kifu_for_test/2017-05-25')
    parser.add_argument('--initial_shuffled_kifu_folder_path',
        action='store',
        required=True,
        help='Folder path of the inintial shuffled kifu files. ex) kifu/2017-05-25-shuffled')
    parser.add_argument('--initial_new_eval_folder_path',
        action='store',
        required=True,
        help='Folder path base of the inintial new eval files. ex) eval')
    parser.add_argument('--initial_state',
        action='store',
        required=True,
        help='Initial state. [' + ', '.join([state.name for state in State]) + ']')
    parser.add_argument('--final_state',
        action='store',
        required=True,
        help='Final state. [' + ', '.join([state.name for state in State]) + ']')
    parser.add_argument('--generate_kifu_exe_file_path',
        action='store',
        required=True,
        help='Exe file name of the kifu generator. ex) YaneuraOu.2016-08-05.generate_kifu.exe')
    parser.add_argument('--learner_exe_file_path',
        action='store',
        required=True,
        help='Exe file name of the learner. ex) YaneuraOu.2016-08-05.learn.exe')
    parser.add_argument('--num_threads_to_generate_kifu',
        action='store',
        type=int,
        required=True,
        help='Number of the threads used for learning. ex) 8')
    parser.add_argument('--num_threads_to_learn',
        action='store',
        type=int,
        required=True,
        help='Number of the threads used for learning. ex) 8')
    parser.add_argument('--num_threads_to_selfplay',
        action='store',
        type=int,
        required=True,
        help='Number of the threads used for selfplay. ex) 8')
    parser.add_argument('--num_games_to_selfplay',
        action='store',
        type=int,
        required=True,
        help='Number of the games to play for selfplay. ex) 100')
    parser.add_argument('--num_positions_to_generator_train',
        action='store',
        type=int,
        required=True,
        help='Number of the games to play for generator. ex) 100')
    parser.add_argument('--num_positions_to_generator_test',
        action='store',
        type=int,
        required=True,
        help='Number of the games to play for generator. ex) 100')
    parser.add_argument('--num_positions_to_learn',
        action='store',
        type=int,
        required=True,
        help='Number of the positions for learning. ex) 10000')
    parser.add_argument('--num_iterations',
        action='store',
        type=int,
        required=True,
        help='Number of the iterations. ex) 100')
    parser.add_argument('--search_depth',
        action='store',
        type=int,
        required=True,
        help='Search depth. ex) 8')
    parser.add_argument('--min_learning_rate',
        action='store',
        type=float,
        required=True,
        help='Min learning rate. ex) 2.0')
    parser.add_argument('--max_learning_rate',
        action='store',
        type=float,
        required=True,
        help='Max learning rate. ex) 2.0')
    parser.add_argument('--num_learning_rate_cycles',
        action='store',
        type=float,
        required=True,
        help='Number of learning rate cycles. ex) 10')
    parser.add_argument('--mini_batch_size',
        action='store',
        type=int,
        required=True,
        help='Learning rate. ex) 1000000')
    parser.add_argument('--fobos_l1_parameter',
        action='store',
        type=float,
        required=True,
        help='Learning rate. ex) 0.0')
    parser.add_argument('--fobos_l2_parameter',
        action='store',
        type=float,
        required=True,
        help='Learning rate. ex) 0.99989464503')
    parser.add_argument('--num_numa_nodes',
        action='store',
        type=int,
        required=True,
        help='Number of the NUMA nodes. ex) 2')
    parser.add_argument('--num_divisions_to_generator_train',
        action='store',
        type=int,
        required=True,
        help='Number of the divisions to generate train data. ex) 10')
    parser.add_argument('--initial_division_to_generator_train',
        action='store',
        type=int,
        required=True,
        help='Initial division to generate train data. ex) 2')
    parser.add_argument('--reference_eval_folder_paths',
        action='store',
        required=True,
        help='Comma-separated folder paths for reference eval files. ex) eval/tanuki_wcsc27,eval/elmo_wcsc27')
    parser.add_argument('--selfplay_exe_file_path',
        action='store',
        required=True,
        help='Exe file name for the self plays. ex) tanuki-wcsc27-2017-05-07-1-avx2.exe')
    parser.add_argument('--thinking_time_ms',
        action='store',
        type=int,
        required=True,
        help='Thinking time for self play in milliseconds. ex) 1000')
    parser.add_argument('--self_play_hash_size',
        action='store',
        type=int,
        required=True,
        help='Hash size for self play. ex) 256')
    parser.add_argument('--elmo_lambda',
        action='store',
        type=float,
        required=True,
        help='Elmo Lambda. ex) 1.0')
    parser.add_argument('--value_threshold',
        action='store',
        type=int,
        required=True,
        help='Value threshold to include positions to the learning data. ex) 30000')
    parser.add_argument('--value_to_winning_rate_coefficient',
        action='store',
        type=float,
        required=True,
        help='Coefficient to convert a value to the winning rate. ex) 600.0')
    parser.add_argument('--adam_beta2',
        action='store',
        type=float,
        required=True,
        help='Adam beta2 coefficient. ex) 0.999')
    parser.add_argument('--use_progress_as_elmo_lambda',
        action='store',
        type=str,
        required=True,
        help='"true" to use progress as elmo lambda. Otherwise, "false" ex) true')
    parser.add_argument('--startpos_file_name_for_self_play',
        action='store',
        default='records_2017-05-19.sfen',
        type=str,
        help='')
    parser.add_argument('--use_discount',
        action='store',
        type=str,
        required=True,
        help='"True" to use discount for shuffle. Otherwise, "False" ex) True')
    parser.add_argument('--use_winning_rate_for_discount',
        action='store',
        type=str,
        required=True,
        help='"True" to winning rate for discount. Otherwise, "False" ex) True')

    args = parser.parse_args()

    learner_output_folder_path_base = args.learner_output_folder_path_base
    kifu_output_folder_path_base = args.kifu_output_folder_path_base
    kifu_for_test_output_folder_path_base = args.kifu_for_test_output_folder_path_base
    initial_eval_folder_path = args.initial_eval_folder_path
    initial_kifu_folder_path = args.initial_kifu_folder_path
    initial_kifu_for_test_folder_path = args.initial_kifu_for_test_folder_path
    initial_shuffled_kifu_folder_path = args.initial_shuffled_kifu_folder_path
    initial_new_eval_folder_path = args.initial_new_eval_folder_path
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
    new_eval_folder_path = initial_new_eval_folder_path
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
            new_eval_folder_path = os.path.join(learner_output_folder_path_base,
                                                GetDateTimeString())
            Learn(args, old_eval_folder_path, shuffled_kifu_folder_path, kifu_for_test_folder_path,
                new_eval_folder_path)
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
