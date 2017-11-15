# 概要
tanuki-シリーズは『やねうら王』より派生したUSIプロトコル将棋エンジンです。
開発コンセプトは「楽に楽しく」です。
配布ライセンスは『やねうら王』に準拠します。


# 現状
第5回電王トーナメント版『平成将棋合戦ぽんぽこ』の棋譜生成ルーチン・機械学習ルーチン・定跡データベース生成ルーチン・評価関数ファイル・定跡データベースを公開しました。


# ファイルの説明
* source/experimental_book.* 定跡データベース生成ルーチン
* source/experimental_learner.* 機械学習ルーチン
* source/experimental_progress.* 進行度ルーチン
* source/kifu_converter.* 棋譜データ変換ルーチン
* source/kifu_generator.* 棋譜生成ルーチン
* source/kifu_reader.* 棋譜読み込みルーチン
* source/kifu_shuffler.* 棋譜シャッフルルーチン
* source/kifu_writer.* 棋譜書き込みルーチン
* source/progress_report.* 処理残り時間表示ルーチン
* tanuki-optimizer 探索パラメーター自動調整プログラム
* tanuki-phoenix 使わないで下さい
* tanuki-proxy クラスタマスタープログラム
* TanukiColiseum 自己対戦プログラム


# 利用環境
メモリに最低でも 2GB 程度空きがあること。
64bit OS であること。
Haswell マイクロアーキテクチャ以降の Intel 製 CPU


# 使い方
Releaseページより評価関数ファイルと定跡データベースをダウンロードし、
『やねうら王2017Early KPPTビルド V4.79』以降と共にご利用下さい。
https://github.com/nodchip/hakubishin-/releases
具体的な利用法はやねうら王に付属の
YaneuraOu-2017-early-readme.txtをご覧ください。


# 開発者向け注意点
Visual Studio 2015 Comunity Edition Update 3 を用いて開発しています。
文字コードは UTF-8 です。
clang-formatを用いてフォーマットしています。設定は以下の通りです。
* BasedOnStyle: Google
* IndentWidth: 4
* ColumnLimit: 100


# tanuki-proxy
tanuki-proxyは第26回世界コンピュータ将棋選手権でたぬきのもりが使用したクラスタマスタープログラムです。
tanuki-proxyの実行には.Net Framework 4.6.1が必要です。
tanuki-proxyには早押しクイズ方式と合議制の2種類の実装があります。
* tanuki-proxy-wcsc26-fastest-finger-first.exe … 早押しクイズ方式
* tanuki-proxy-wcsc26-collegial-system.exe … 合議制
詳しくは「たぬきのもり 技術文書」をご覧ください。 https://twitter.com/nodchip/status/727878218917629953?lang=ja
使用前にproxy-setting.sample.jsonをproxy-setting.jsonにコピーし、内容を編集して下さい。
キーの意味は以下のとおりです。
* engines …　スレーブとなる思考エンジンの配列
* engineName … エンジン名、早押しクイズ方式の時にどの思考エンジンが最も早く結果を返したか表示するために使います
* optionOverrides … UIから渡ってきたオプションを上書きするよう指定します
* name … 上書きするオプション名を指定します
* value …　上書きするオプションの値を指定します
* arguments … 思考エンジンに渡す引数を指定します
* workingDirectory …　作業フォルダを指定します
* fileName … 思考エンジンのファイル名を指定します
* logDirectory …　ログを出力するフォルダを指定します
スレーブとなる思考エンジンは、goコマンド受信時に、直前に渡されるpositionに"info string "を付けた文字列を出力しなければなりません。
例) info string position startpos moves 7g7f 3c3d 2g2f
実際にクラスタを作る場合は、各スレーブノードにsshdをインストールし、tanuki-proxy-***.exeからssh経由で各ノード内の思考エンジンを呼び出す形をおすすめします。
tanuki-proxyを使用する際はBook_Sleep_Timeを1500程度に設定してからご使用ください。


# Using tanuki- with WinBoard 4.8.0
Please follow the steps below.

1. Install WinBoard 4.8.0. The installation path is "C:\WinBoard-4.8.0".
2. Download "apery_twig_sdt3.7z" from http://hiraokatakuya.github.io/apery/, and extract to "C:\WinBoard-4.8.0\apery_twig_sdt3".
3. Download "tanuki-2016-09-10.7z" from https://github.com/nodchip/tanuki-/releases, and extract to "C:\WinBoard-4.8.0\apery_twig_sdt3\bin".
4. Start "C:\WinBoard-4.8.0\WinBoard\winboard.exe".
5. Check "Advanced options", and set -uxiAdapter {UCI2WB -%variant "%fcp" "%fd"). Please refer #8 (comment) about this step.
6. Add "tanuki- WCSC26" with the following settings:
      Engine (.exe or .jar): C:\WinBoard-4.8.0\apery_twig_sdt3\bin\tanuki-wcsc26-sse42-gcc-lazy-smp.exe
      command-line parameters: empty
      Special WinBoard options: empty
      directory: empty
      UCCI/USI [uses specified /uxiAdapter]: on
7. Engine > Engine #1 Settings... > Set "Minimum_Thinking_Time" to "0".

The steps will be changed in future versions.

# Q & A
Q. Do you plan to create an all-in-one package with WinBoard and tanuki-?
A. There are no plans for it.

Q. Do you plan to support WB/CECP-protocol in the next tanuki- edition? http://hgm.nubati.net/CECP.html v2 - http://home.hccnet.nl/h.g.muller/engine-intf.html v1
A. There are no plans for it.

Q. Do you plan to add any logos to tanuki-?
A. There are no plans for it.

Q. Why tanuki- uses up all its time with 1 min + 0 sec/move?
A. Start "C:\WinBoard-4.8.0\WinBoard\winboard.exe", check "Advanced options", and set `-uxiAdapter {UCI2WB -%variant "%fcp" "%fd")`.

Q. Why tanuki- uses up all its time in 27-36 moves with 1 min + 0 sec/move?
A. Engine > Engine #1 Settings... > Set "Minimum_Thinking_Time" to "0".
