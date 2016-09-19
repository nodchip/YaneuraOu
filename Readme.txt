# 概要
tanuki- は Apery(大樹の枝) より派生した USI プロトコルの将棋エンジンです。
開発コンセプトは「たのしく！おもしろく！」です。
配布ライセンスは Apery に準拠します。


# 開発状況
第26回世界コンピュータ将棋選手権バージョン「たぬきのもり」です。


# ファイルの説明
* Readme.txt, このファイルです。
* Copying.txt, GNU General Public License version 3 条文です。
* src/, tanuki- のソースコードのフォルダです。
* utils/, tanuki- 開発で使用する本体以外のソフトのソースコードのフォルダです。


# 利用環境
メモリに最低でも 2GB 程度空きがあること。
64bit OS であること。
Haswell マイクロアーキテクチャ以降の Intel 製 CPU


# 使い方
利用にあたり『大樹の枝』の評価関数ファイルが必要となります。
予め『大樹の枝』のバイナリ版を適切なフォルダに解凍し、
『tanuki-』のバイナリを上書きして下さい。
詳しくは Apery の Readme.txt を参照してください。

使い方についての質問は https://twitter.com/nodchip で受け付けております。お気軽にお声がけください。


# 開発者向け注意点
Visual Studio 2015 Comunity Edition Update 2 を用いて開発しています。
インデントは半角スペース 2 個です。
文字コードは UTF-8 です。
単体テストフレームワークとして Google Test を使用しています。
ビルドの前にexternalフォルダの7zファイルを解凍しておいて下さい。


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


# tanuki-optimizer
探索パラメーターの自動調整スクリプトです。現時点で公開の予定はありません。

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
