# 将棋ソフトelmo（評価関数生成用コード）

elmoは2017年の世界コンピュータ将棋選手権で優勝した将棋ソフトです。
本コードはelmoの評価関数を生成する*開発者向け*プログラムとなります。
対局用のプログラムはやねうら王を推奨していますが、旧Apery形式(2017年3月迄のApery)の評価関数を利用するプログラムであれば
どのプログラムでも利用出来ます。2017年5月時点で以下プログラムで利用出来ることを確認しています。
(利用できることを保証するものではありません)
- [Apery 浮かむ瀬以前](https://github.com/HiraokaTakuya/apery/tree/8220c20fdcfd2c273b3a69c09e7daf80d9df2ddd)
- [やねうら王](https://github.com/yaneurao/YaneuraOu)
- [SILENT_MAJORITY](https://github.com/Jangja/silent_majority) 
- [読み太](https://github.com/TukamotoRyuzo/Yomita)
- [蒼天幻想ナイツ・オブ・タヌキ(tanuki-)](https://github.com/nodchip/hakubishin-)


## 開発者の皆様へ

評価関数生成時はメモリを非常に必要とします。1スレッドで7GB, その後1スレッド増える度に2GB必要とします。
つまり、***4コアで4スレッド利用する場合、15GBを必要とします。***ご注意ください。

elmoの技術的特徴については、簡易なものですが以下をご参照ください。
- [第27回世界コンピュータ将棋選手権 アピール文書](http://www2.computer-shogi.org/wcsc27/appeal/elmo/elmo_wcsc27_appeal_r2_0.txt)

バグ等を発見されましたら、[twitter](https://twitter.com/mktakizawa)へご報告いただけますと幸いです。

### コンパイルの方法

以下のようにコマンドを実行してください。

```
cd elmo/src
make -j 8 sse
mv elmo ../bin/
```
上記8はコンパイル時のスレッド数(幾つでも良いです)、sseはSSE4.2でのコンパイルを示します。
AVX2対応のCPUをご利用の場合は、sseをbmi2に置き換えてコンパイルしてください。

### 学習の方法

#### Step 0. 事前準備

1. 評価関数生成に使うコンピュータの準備
  - CPU: SSE 4.2 対応のCPU（Core iシリーズ以降のアーキテクチャ）
  - メモリ: 16GB以上の空きメモリ（32GB以上のメモリを推奨）
  - OS: Ubuntu Linux 14.04以上(Ubuntu Mate 16.04で主に開発しています)。
  - コンパイラ等: g++ 4.8以上, make
2. 初期評価関数の取得
  - elmo: https://t.co/NJQ95elVma にアクセスし、elmo.shogi.zipをダウンロードします。
  - 上記ファイルを解凍し、evalフォルダ配下の3ファイルをelmo/bin/20161007/に保存します。
  - 以下の通りファイル名を変更します。
    - KK_synthesized.bin → kks.kk.bin
    - KKP_synthesized.bin → kkps.kkp.bin
    - KPP_synthesized.bin → kpps.kpp.bin
3. 初期局面集ファイルの取得
  - AperyGenerateTeacher: https://hiraokatakuya.github.io/aperygenerateteacher/index.html にアクセスし、AperyGenerateTeacherを取得する。
  - 上記ファイルを解凍し、展開されたroots.hcp(初期局面ファイル)をelmo/bin/に保存します。

#### Step 1. 教師局面の生成

以下のコマンドを実行し、教師局面を生成します。

``` 
./elmo make_teacher roots.hcp <出力ファイル名> <実行スレッド数> <生成する局面数>
```

出力ファイル名は任意のものをご指定ください。
実行スレッド数は論理CPUコア数と同程度を目安に設定してください。
生成する局面数も任意ですが、100万程度(1000000)で実行してみてください。

*最終的に50億局面程度必要となりますが、下記Step 2.でファイルサイズ分のメモリが必要となります。私(瀧澤)は1千万局面毎に生成し、メモリが許容出来るサイズ程度に結合(catコマンド)してStep 2.を適用しています。シャッフル後にそれらファイルを結合しています。*

#### Step 2. 教師局面のシャッフル

Step 1.で生成した教師局面ファイル名を説明の為に以下、output.teacherとして記載します。
```
cd ../utils/shuffle_hcpe/
make
cd ../../bin/
../utils/shuffle_hcpe/shuffle_hcpe output.teacher <シャッフル後のファイル名>
```

#### Step 3. 評価関数の生成

Step 2.で生成した教師局面ファイル名を説明の為に以下、output.shuffleとして記載します。
以下のコマンドを実行し、評価関数の生成を行います。

```
./elmo use_teacher output.shuffle <実行スレッド数>
```

注意：実行スレッド数は*搭載メモリ量*および*CPUコア数*を考慮し、決定してください。
必要なメモリ量の目安はページ上部の「開発者の皆様へ」の欄をご確認ください。
***メモリが不足した場合、OSの動作が不安定となりますのでご注意ください。***

## 謝辞

本Readmeは[技巧](https://github.com/gikou-official/Gikou)を参考に同様のアウトラインで作成させて頂きました。
分り易く秀逸な文章と思います。この場を借りて感謝申し上げます。

本学習コードの大部分はAperyのコードをそのまま利用させて頂いています。
Apery無しにelmoの評価関数を作製することは非常に困難でした。心から感謝しております。

最後に、2017年の選手権での対局は評価関数と定跡部分を除き、やねうら王をそのまま利用していました。
elmoの優勝にやねうら王の貢献は欠かせないものでした。大会当日までご尽力頂き、頭が下がる思いです。

## 最後に

最近のコンピュータ将棋界は、開発者同士でワイワイやっています。
- [大合神クジラちゃんの選手権映像](https://www.youtube.com/channel/UCcwZkz7v1SY5-IGFRCrF-1Q)

ご興味を持たれましたら是非大会への参加をご検討ください。開発者は全員歓迎すると思います。
大会出場には思考部分への独自性が必要となりますが、elmoもライブラリとして利用可能とする予定ですので、
私がApery/やねうら王を利用したように、ご利用頂ければ大変嬉しく思います。

「elmo」開発者 瀧澤 誠