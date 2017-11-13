#ifndef _2017_EARLY_PARAMETERS_
#define _2017_EARLY_PARAMETERS_

// パラメーターの説明に "fixed"と書いてあるパラメーターはランダムパラメーター化するときでも変化しない。
// 「前提depth」は、これ以上ならその枝刈りを適用する(かも)の意味。
// 「適用depth」は、これ以下ならその枝刈りを適用する(かも)の意味。

// 現在の値から、min～maxの範囲で、+step,-step,+0を試す。
// interval = 2だと、-2*step,-step,+0,+step,2*stepの5つを試す。

//
// futility pruning
//

// 深さに比例したfutility pruning
// depth手先で評価値が変動する幅が = depth * PARAM_FUTILITY_MARGIN_DEPTH
// 元の値 = 150
// [PARAM] min:100,max:240,step:1,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_FUTILITY_MARGIN_ALPHA_OPENING = 143;
PARAM_DEFINE PARAM_FUTILITY_MARGIN_ALPHA_ENDING = 143;

// 
// 元の値 = 200
// [PARAM] min:100,max:240,step:2,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_FUTILITY_MARGIN_BETA_OPENING = 200;
PARAM_DEFINE PARAM_FUTILITY_MARGIN_BETA_ENDING = 200;


// 静止探索でのfutility pruning
// 元の値 = 128
// [PARAM] min:50,max:160,step:2,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_FUTILITY_MARGIN_QUIET_OPENING = 145;
PARAM_DEFINE PARAM_FUTILITY_MARGIN_QUIET_ENDING = 145;

// futility pruningの適用depth。
// 元の値 = 7
// 7より8のほうが良さげなので固定。
// [PARAM] min:5,max:13,step:1,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_FUTILITY_RETURN_DEPTH_OPENING = 10;
PARAM_DEFINE PARAM_FUTILITY_RETURN_DEPTH_ENDING = 10;

// 親nodeでのfutilityの適用depth。
// この枝刈り、depthの制限自体が要らないような気がする。
// 元の値 = 7
// [PARAM] min:5,max:13,step:1,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_FUTILITY_AT_PARENT_NODE_DEPTH_OPENING = 13;
PARAM_DEFINE PARAM_FUTILITY_AT_PARENT_NODE_DEPTH_ENDING = 13;

// 親nodeでのfutility margin
// 元の値 = 256
// [PARAM] min:100,max:300,step:2,interval:2,time_rate:1,fixed
PARAM_DEFINE PARAM_FUTILITY_AT_PARENT_NODE_MARGIN1_OPENING = 246;
PARAM_DEFINE PARAM_FUTILITY_AT_PARENT_NODE_MARGIN1_ENDING = 246;

// staticEvalから減算するmargin
// 元の値 = 200
// [PARAM] min:0,max:300,step:25,interval:2,time_rate:1,fixed
PARAM_DEFINE PARAM_FUTILITY_AT_PARENT_NODE_MARGIN2_OPENING = 198;
PARAM_DEFINE PARAM_FUTILITY_AT_PARENT_NODE_MARGIN2_ENDING = 198;

// depthが2乗されるので影響大きい
// 元の値 = 35
// [PARAM] min:20,max:50,step:1,interval:2,time_rate:1,fixed
PARAM_DEFINE PARAM_FUTILITY_AT_PARENT_NODE_GAMMA1_OPENING = 41;
PARAM_DEFINE PARAM_FUTILITY_AT_PARENT_NODE_GAMMA1_ENDING = 41;

// depthが2乗されるので影響大きい
// 元の値 = 35
// [PARAM] min:20,max:60,step:3,interval:2,time_rate:1,fixed
PARAM_DEFINE PARAM_FUTILITY_AT_PARENT_NODE_GAMMA2_OPENING = 51;
PARAM_DEFINE PARAM_FUTILITY_AT_PARENT_NODE_GAMMA2_ENDING = 51;

//
// null move dynamic pruning
//

// null move dynamic pruningのときの
//  Reduction = (α + β * depth ) / 256 + ...みたいなαとβ

// 元の値 = 823
// [PARAM] min:500,max:1500,step:2,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_NULL_MOVE_DYNAMIC_ALPHA_OPENING = 823;
PARAM_DEFINE PARAM_NULL_MOVE_DYNAMIC_ALPHA_ENDING = 823;

// 元の値 = 67
// [PARAM] min:50,max:100,step:2,interval:2,time_rate:1,fixed
PARAM_DEFINE PARAM_NULL_MOVE_DYNAMIC_BETA_OPENING = 53;
PARAM_DEFINE PARAM_NULL_MOVE_DYNAMIC_BETA_ENDING = 53;

// 元の値 = 35
// [PARAM] min:10,max:60,step:1,interval:2,time_rate:1,fixed
PARAM_DEFINE PARAM_NULL_MOVE_MARGIN_OPENING = 32;
PARAM_DEFINE PARAM_NULL_MOVE_MARGIN_ENDING = 32;

// null moveでbeta値を上回ったときに、これ以下ならreturnするdepth。適用depth。
// 元の値 = 12
// [PARAM] min:4,max:15,step:1,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_NULL_MOVE_RETURN_DEPTH_OPENING = 13;
PARAM_DEFINE PARAM_NULL_MOVE_RETURN_DEPTH_ENDING = 13;


//
// probcut
//

// probcutの前提depth
// 元の値 = 5
// [PARAM] min:3,max:10,step:1,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_PROBCUT_DEPTH_OPENING = 4;
PARAM_DEFINE PARAM_PROBCUT_DEPTH_ENDING = 4;

// probcutのmargin
// 元の値 = 200
// [PARAM] min:100,max:300,step:2,interval:2,time_rate:1,fixed
PARAM_DEFINE PARAM_PROBCUT_MARGIN_OPENING = 216;
PARAM_DEFINE PARAM_PROBCUT_MARGIN_ENDING = 216;


//
// singular extension
//

// singular extensionの前提depth。
// これ変更すると他のパラメーターががらっと変わるので固定しておく。
// 10秒設定だと6か8あたりに局所解があるようだ。
// 元の値 = 8
// [PARAM] min:4,max:13,step:1,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_SINGULAR_EXTENSION_DEPTH_OPENING = 8;
PARAM_DEFINE PARAM_SINGULAR_EXTENSION_DEPTH_ENDING = 8;

// singular extensionのmarginを計算するときの係数
// rBeta = std::max(ttValue - PARAM_SINGULAR_MARGIN * depth / (8 * ONE_PLY), -VALUE_MATE);
// 元の値 = 256
// [PARAM] min:128,max:400,step:4,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_SINGULAR_MARGIN_OPENING = 200;
PARAM_DEFINE PARAM_SINGULAR_MARGIN_ENDING = 200;

// singular extensionで浅い探索をするときの深さに関する係数
// このパラメーター、長い時間でないと調整できないし下手に調整すべきではない。
// 元の値 = 16
// [PARAM] min:8,max:32,step:1,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_SINGULAR_SEARCH_DEPTH_ALPHA_OPENING = 16;
PARAM_DEFINE PARAM_SINGULAR_SEARCH_DEPTH_ALPHA_ENDING = 16;


//
// pruning by move count,history,etc..
//

// move countによる枝刈りをする深さ。適用depth。
// 元の値 = 16
// [PARAM] min:8,max:32,step:1,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_PRUNING_BY_MOVE_COUNT_DEPTH_OPENING = 17;
PARAM_DEFINE PARAM_PRUNING_BY_MOVE_COUNT_DEPTH_ENDING = 17;

// historyによる枝刈りをする深さ。適用depth。
// これ、将棋ではそこそこ上げたほうが長い時間では良さげ。
// 元の値 = 3
// [PARAM] min:2,max:32,step:1,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_PRUNING_BY_HISTORY_DEPTH_OPENING = 9;
PARAM_DEFINE PARAM_PRUNING_BY_HISTORY_DEPTH_ENDING = 9;


// historyの値によってreductionするときの係数
// これ、元のが (hist - 8000) / 20000みたいな意味ありげな値なので下手に変更しないほうが良さげ。
// 元の値 = 8000
// [PARAM] min:4000,max:15000,step:40,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_REDUCTION_BY_HISTORY_OPENING = 8000;
PARAM_DEFINE PARAM_REDUCTION_BY_HISTORY_ENDING = 8000;


//
// Internal iterative deeping
// 

// historyの値によってreductionするときの係数
// 元の値 = 256
// [PARAM] min:128,max:384,step:1,interval:2,time_rate:1,fixed
PARAM_DEFINE PARAM_IID_MARGIN_ALPHA_OPENING = 261;
PARAM_DEFINE PARAM_IID_MARGIN_ALPHA_ENDING = 261;


//
// razoring pruning
// 

// 元の値 = 483
// [PARAM] min:400,max:700,step:5,interval:2,time_rate:1,fixed
PARAM_DEFINE PARAM_RAZORING_MARGIN1_OPENING = 483;
PARAM_DEFINE PARAM_RAZORING_MARGIN1_ENDING = 483;

// 元の値 = 570
// [PARAM] min:400,max:700,step:5,interval:2,time_rate:1,fixed
PARAM_DEFINE PARAM_RAZORING_MARGIN2_OPENING = 555;
PARAM_DEFINE PARAM_RAZORING_MARGIN2_ENDING = 555;

// 元の値 = 603
// [PARAM] min:400,max:700,step:5,interval:2,time_rate:1,fixed
PARAM_DEFINE PARAM_RAZORING_MARGIN3_OPENING = 593;
PARAM_DEFINE PARAM_RAZORING_MARGIN3_ENDING = 593;

// 元の値 = 554
// [PARAM] min:400,max:700,step:5,interval:2,time_rate:1,fixed
PARAM_DEFINE PARAM_RAZORING_MARGIN4_OPENING = 539;
PARAM_DEFINE PARAM_RAZORING_MARGIN4_ENDING = 539;


//
// LMR reduction table
//

// 元の値 = 128
// [PARAM] min:64,max:256,step:2,interval:2,time_rate:1,fixed
PARAM_DEFINE PARAM_REDUCTION_ALPHA_OPENING = 124;
PARAM_DEFINE PARAM_REDUCTION_ALPHA_ENDING = 124;


//
// futility move count table
//

// どうも、元の値ぐらいが最適値のようだ…。

// 元の値 = 240
// [PARAM] min:150,max:400,step:1,interval:2,time_rate:1,fixed
PARAM_DEFINE PARAM_FUTILITY_MOVE_COUNT_ALPHA0_OPENING = 240;
PARAM_DEFINE PARAM_FUTILITY_MOVE_COUNT_ALPHA0_ENDING = 240;

// 元の値 = 290
// [PARAM] min:150,max:400,step:1,interval:2,time_rate:1,fixed
PARAM_DEFINE PARAM_FUTILITY_MOVE_COUNT_ALPHA1_OPENING = 288;
PARAM_DEFINE PARAM_FUTILITY_MOVE_COUNT_ALPHA1_ENDING = 288;

// 元の値 = 773
// [PARAM] min:500,max:2000,step:2,interval:2,time_rate:1,fixed
PARAM_DEFINE PARAM_FUTILITY_MOVE_COUNT_BETA0_OPENING = 773;
PARAM_DEFINE PARAM_FUTILITY_MOVE_COUNT_BETA0_ENDING = 773;

// 元の値 = 1045
// [PARAM] min:500,max:2000,step:2,interval:2,time_rate:1,fixed
PARAM_DEFINE PARAM_FUTILITY_MOVE_COUNT_BETA1_OPENING = 1041;
PARAM_DEFINE PARAM_FUTILITY_MOVE_COUNT_BETA1_ENDING = 1041;


//
// etc..
// 

// この個数までquietの指し手を登録してhistoryなどを増減させる。
// 元の値 = 64
// 将棋では駒打ちがあるから少し増やしたほうがいいかも。
// →　そうでもなかった。固定しておく。
// [PARAM] min:32,max:128,step:1,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_QUIET_SEARCH_COUNT_OPENING = 64;
PARAM_DEFINE PARAM_QUIET_SEARCH_COUNT_ENDING = 64;


// 静止探索での1手詰め
// 元の値 = 1
// →　1スレ2秒で対技巧だと有りのほうが強かったので固定しておく。
// [PARAM] min:0,max:1,step:1,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_QSEARCH_MATE1_OPENING = 1;
PARAM_DEFINE PARAM_QSEARCH_MATE1_ENDING = 1;

// 通常探索での1手詰め
// →　よくわからないが1スレ2秒で対技巧だと無しのほうが強かった。
//     1スレ3秒にすると有りのほうが強かった。やはり有りのほうが良いのでは..
// 元の値 = 1
// [PARAM] min:0,max:1,step:1,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_SEARCH_MATE1_OPENING = 1;
PARAM_DEFINE PARAM_SEARCH_MATE1_ENDING = 1;

// 1手詰めではなくN手詰めを用いる
// 元の値 = 1
// [PARAM] min:1,max:5,step:2,interval:1,time_rate:1,fixed
PARAM_DEFINE PARAM_WEAK_MATE_PLY_OPENING = 1;
PARAM_DEFINE PARAM_WEAK_MATE_PLY_ENDING = 1;


#endif
