# TanukiColiseum
TanukiColiseumはC#で実装された自己対戦ツールです。

# 準備
適切なフォルダにTanukiColiseum.exeを配置して下さい。
続いて適切なフォルダに互角の局面集を配置して下さい。
互角の局面集は [自己対局用に互角の局面集を公開しました | やねうら王 公式サイト](http://yaneuraou.yaneu.com/2016/08/24/%E8%87%AA%E5%B7%B1%E5%AF%BE%E5%B1%80%E7%94%A8%E3%81%AB%E4%BA%92%E8%A7%92%E3%81%AE%E5%B1%80%E9%9D%A2%E9%9B%86%E3%82%92%E5%85%AC%E9%96%8B%E3%81%97%E3%81%BE%E3%81%97%E3%81%9F/ "自己対局用に互角の局面集を公開しました | やねうら王 公式サイト") から入手できます。
拙作の互角局面集も利用可能です。 [Release 互角局面集2017-05-19 · nodchip/hakubishin-](https://github.com/nodchip/hakubishin-/releases/tag/even-positions-2017-05-19 "Release 互角局面集2017-05-19 · nodchip/hakubishin-")

# 利用方法
起動するとGUIが表示されます。

# コマンドラインオプション
|オプション|説明|
|------------|-------------|
|--no_gui|GUIを表示せずコンソール上で動きます。下記のオプションが利用可能です。|
|--engine1|作業フォルダから思考エンジン1への相対パス|
|--engine2|作業フォルダから思考エンジン2への相対パス|
|--eval1|作業フォルダから思考エンジン1に読み込ませる評価関数フォルダへのパス|
|--eval2|作業フォルダから思考エンジン1に読み込ませる評価関数フォルダへのパス|
|--num_concurrent_games|同自対局数(物理コア数い合わせることをおすすめします)|
|--num_games|合計対局数|
|--hash|置換表サイズ(単位はMB)|
|--time|思考時間(単位はミリ秒)|
|--num_numa_nodes|NUMAノード数|
|--num_book_moves1|思考エンジン1で何手目まで定跡データベースを使用するか|
|--num_book_moves2|思考エンジン2で何手目まで定跡データベースを使用するか|
|--book_file_name1|思考エンジン1に使用する定跡データベースファイル名|
|--book_file_name2|思考エンジン2に使用する定跡データベースファイル名|
|--num_book_moves|互角の局面集の何手目から対局を開始するか|

# コマンドライン例
    TanukiColiseum.exe --no_gui --engine1 YaneuraOu-2017-early.exe --engine2 YaneuraOu-2017-early.exe --eval1 eval --eval2 eval --num_concurrent_games 4 --num_games 1000 --hash 256 --time 1000 --num_numa_nodes 1 --num_book_moves1 0 --num_book_moves2 0 --book_file_name1 book.bin --book_file_name2 yaneura_book1.db --num_book_moves 24

# 出力例
    Initializing engines...
    Starting the engine process 0
    Starting the engine process 1
    Starting the engine process 2
    
    ...
    
    Initialized engines...
    Started games...
    T1,b1000,1 - 0 - 0(100.00% R0.00) win black: white = 0.00% : 100.00%
    T1,b1000,4 - 0 - 4(50.00% R0.00) win black: white = 50.00% : 50.00%
    T1,b1000,10 - 0 - 8(55.56% R38.76) win black: white = 50.00% : 50.00%
    
    ...
    
    engine1=YaneuraOu-2017-early.exe eval1=eval
    engine2=YaneuraOu-2017-early.exe eval2=eval
    T1,b1000,530 - 37 - 433(55.04% R35.12) win black: white = 52.86% : 47.14%
