# tanuki-optimizer
tanuki-optimizerはPythonで実装された、探索パラメーターの自動調整スクリプトです。コマンドライン経由で実行可能です。
HyperoptとGaussian Processを用いています。

tanuki-optimizerは2段階で探索パラメーターを調整します。
1段階目はoptimize_parameters.pyでHyperoptを用いて探索パラメーターセットと自己対戦の勝率のペアのリストを生成します。
このときリストはpickleファイルとして保存されます。
2段階目は生成したリストからGaussian Processを用いて最も良い探索パラメーターセットを選択します。
詳しくは[蒼天幻想ナイツ・オブ・タヌキアピール文書](http://www2.computer-shogi.org/wcsc27/appeal/tanuki-/appeal.pdf)をご覧ください

以下のドキュメントは古い情報を元に書かれています。誤りがあればIssuesにてお知らせ下さい。

# 準備

1. Python 2.7.* 64ビット版をインストールする
	* http://www.python.org
2. 以下のライブラリをダウンロードする
	* [numpy-*.*.*+mkl-cp27-cp27m-win_amd64.whl](http://www.lfd.uci.edu/~gohlke/pythonlibs/)
	* [scipy-*.*.*-cp27-none-win_amd64.whl](http://www.lfd.uci.edu/~gohlke/pythonlibs/)
3. 2. でダウンロードしたパッケージとその他の必須パッケージをインストールする
	* python -m pip install --upgrade pip
	* pip install numpy-*.*.*+mkl-cp27-cp27m-win_amd64.whl scipy-*.*.*-cp27-none-win_amd64.whl hyperopt pymongo networkx pandas sklearn matplotlib
4. やねうら王の2017-early-search.cppを編集しビルドする
	* USE_AUTO_TUNE_PARAMETERSを有効化する
	* PARAM_FILEを"2017-early-param-temp.h"に変更してビルドし、YaneuraOu-2017-early-temp.exeにリネームする
	* PARAM_FILEを"2017-early-param-default.h"に変更してビルドし、YaneuraOu-2017-early-default.exeにリネームする
5. parameters.csvを編集し、調整したいパラメーターを指定する
6. optimize_parameters.py内のspace・build_argument_names・YaneuraouBuilder.build()内のヘッダーファイルのテンプレートを適切に編集する
	* parameters_generator.pyを用いてソースコードの断片を生成してコピペすると楽です
7. 以下のファイルをexeフォルダにコピーする
	* TanukiColiseum.exe
	* YaneuraOu-2017-early-temp.exe
	* YaneuraOu-2017-early-default.exe
	* analyze_optimizer_log.py
	* hyperopt_state.py
	* optimize_parameters.py

# 使用方法
1. optimize_parameters.py を実行し***.pickleを生成する
2. analyze_optimizer_log.py を実行しparameters_generated.hppを生成する

# コマンドラインオプション
## optimize_parameters.py
|オプション|説明|
|------------|-------------|
|--store-interval|pickleファイルを保存する間隔を指定します。0を指定するとpickleファイルを保存しません。|
|--resume|指定されたpickleファイルからパラメーターの探索を再開します|
|--dump-log|指定されたpickleファイルの内容を出力します|
|--max-evals|サンプリングするパラメーターセット数を指定します|
|--num_threads|1回のサンプリングでの対局数を指定します|
|--thinking_time_ms|サンプリング時の思考時間を指定します|
|--num_numa_nodes|NUMAノード数を指定します|
|--hash|置換表のサイズをMB単位で指定します|

## コマンドライン例
    optimize_parameters.py --max-evals 1000 --num_threads 24 --thinking_time_ms 10000 --num_numa_nodes 2 --hash 256

## optimize_parameters.py
	analyze_optimizer_log.py [パラメーターcsvファイルパス] [pickleファイルパス]

生成されたparameters_generated.hppの内容を適宜2017-early-param.h等にコピペしてお使い下さい。
