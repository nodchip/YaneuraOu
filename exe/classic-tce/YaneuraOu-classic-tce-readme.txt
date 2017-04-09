﻿
・やねうら王classic-tceとは？

やねうら王classic-tceはやねうら王classicにPonderの処理、思考時間制御(秒読み、フィッシャールール対応)などを追加した思考エンジンです。(R3250程度)

ちなみに、tceとはtime control enabledの略です。

・やねうら王classic-tceの遊び方

    将棋所かShogiGUIから思考エンジンとして登録して使ってください。
    実行ファイルはWindows 64bit版、CPUはAVX2用にコンパイルされています。
    ※　昔のCPU環境で動く実行ファイルに関しては要望が多ければコンパイルしたものを公開します。

    以下のようにファイルを配置します。

    YaneuraOu-classic-tce.exe      : やねうら王classic-tce本体
    YaneuraOu-classic-tce_ja.txt   : これを用いると思考エンジンの設定項目の表示が日本語化される。
    book/standard_book.db   : 基本定跡

    eval/kkp32ap.bin        : 3駒関係の評価関数で用いるファイル(KKP)
    eval/kpp16ap.bin        : 3駒関係の評価関数で用いるファイル(KPP)
        ※　evalフォルダに入れる評価関数バイナリ(上記の2ファイル)は、
            CSAのサイトからダウンロードできます。
            
            CSAのコンピュータ将棋選手権使用可能ライブラリ
            http://www.computer-shogi.org/library/
            「やねうら王 コンピューター将棋フレームワーク」の「評価関数ライブラリ」と
            書いてあるところからダウンロードできます。

    ・入玉宣言勝ちに対応しています。
    ・Ponder(相手番で思考する)に対応しています。
    ・秒読み、フィッシャールールに対応しています。
    ・最小思考時間設定に対応しています。
    ・スレッド数は思考エンジン設定で選べます。
    ・定跡の指し手がランダムに選択されます。
    ・置換表サイズは、思考エンジン設定のところで設定した値に従います。

	その他は、docs/
			https://github.com/yaneurao/YaneuraOu/tree/master/docs
	配下にある、
		解説.txt
		USI拡張コマンド.txt
	なども併せてご覧ください。

