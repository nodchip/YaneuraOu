# tanuki-proxy
tanuki-optimizerはC#で実装された、思考エンジンクラスタマスタープログラムです。
思考エンジンとして将棋所等で使用できます。

クラスタアルゴリズムに多数決合議制を採用しています。
得票数1位の指し手が複数ある場合は、それらの中で楽観合議を用いて指し手を選択します。

時間制御のために1インスタンスだけ時間制御用インスタンスを使います。
このインスタンスが指し手を返したタイミングで合議が行われ、差し手を選択します。

# 準備
1. proxy-setting.sample.jsonをproxy-setting.jsonにコピーする
2. proxy-setting.jsonを編集してスレーブノードを登録する

## 思考エンジンの実装
tanuki-proxyを使うためには思考エンジン側にいくつかの変更を加えなければなりません。
* goコマンド受診時に"info string position ..."の形で現在思考中の局面を出力するようにする
* info pvは基本的に全て出力するようにする
    * 定跡選択時もinfo pvを出力する
* positionコマンド受診時に現在の思考が終わるのを待つようにする

# proxy-setting.json書式
proxy-setting.jsonはJSON形式の設定ファイルです。

## ProxySettingオブジェクト
設定ファイルのrootオブジェクトです

|名前|説明|
|------------|-------------|
|engines|EngineOptionオブジェクトの配列|
|logDirectory|動作ログを保存するフォルダパス|

## EngineOptionオブジェクト
スレーブ1インスタンスを表すオブジェクトです

|名前|説明|
|------------|-------------|
|engineName|インスタンス名、内部的な管理のために使います|
|fileName|思考エンジンの実行ファイルパス|
|arguments|思考エンジンのコマンドライン引数|
|workingDirectory|進行エンジンの作業フォルダパス|
|optionOverrides|Optionの配列|
|timeKeeper|時間制御用インスタンスとして使う場合はtrue|

## Optionオブジェクト
GUIからUSIオプションが渡されてきたときに、一部のオプションを上書きするための設定オブジェクトです

|名前|説明|
|------------|-------------|
|name|USIオプションの名前|
|value|USIオプションの値|

# 完全な例
proxy-setting.sample.jsonをご覧ください。

# TIPS
USI on sshプロトコルを用いて、リモートマシンで思考エンジンを立ち上げ、スレーブとして使用することができます。
リモートノードのホームディレクトリに思考エンジンを立ち上げて標準入力を受け渡せるようなスクリプト等が必要です。
詳しくはproxy-setting.sample.jsonをご覧ください。
