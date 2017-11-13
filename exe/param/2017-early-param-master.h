#ifndef _2017_EARLY_PARAMETERS_
#define _2017_EARLY_PARAMETERS_
// Created at: 2017-03-31 11:17:05
// Log: optimize_parameters.hyperopt_state.20170324_105241.b10000.pickle
// Parameters CSV: param\parameters.opening.csv
// n_iterations_to_use = 1000
// minima_method = Data

// |--@---+-----------| raw=122.999999999, min=100, max=240 default=150
PARAM_DEFINE PARAM_FUTILITY_MARGIN_ALPHA_OPENING = 123;

// |---------@--+-----| raw=179.999999997, min=100, max=240 default=200
PARAM_DEFINE PARAM_FUTILITY_MARGIN_BETA_OPENING = 180;

// |--------@---+-----| raw=106.999999998, min=50, max=160 default=128
PARAM_DEFINE PARAM_FUTILITY_MARGIN_QUIET_OPENING = 107;

// |----@-------------| raw=6.999999995, min=5, max=13 default=7
PARAM_DEFINE PARAM_FUTILITY_RETURN_DEPTH_OPENING = 7;

// |----@-------------| raw=6.999999995, min=5, max=13 default=7
PARAM_DEFINE PARAM_FUTILITY_AT_PARENT_NODE_DEPTH_OPENING = 7;

// |----------@--+----| raw=218.999999998, min=100, max=300 default=256
PARAM_DEFINE PARAM_FUTILITY_AT_PARENT_NODE_MARGIN1_OPENING = 219;

// |-----------+-@----| raw=244.99999999, min=0, max=300 default=200
PARAM_DEFINE PARAM_FUTILITY_AT_PARENT_NODE_MARGIN2_OPENING = 245;

// |--------+@--------| raw=36.9999999939, min=20, max=50 default=35
PARAM_DEFINE PARAM_FUTILITY_AT_PARENT_NODE_GAMMA1_OPENING = 37;

// |---@--+-----------| raw=27.9999999989, min=20, max=60 default=35
PARAM_DEFINE PARAM_FUTILITY_AT_PARENT_NODE_GAMMA2_OPENING = 28;

// |---@-+------------| raw=677.999999999, min=500, max=1500 default=823
PARAM_DEFINE PARAM_NULL_MOVE_DYNAMIC_ALPHA_OPENING = 678;

// |-----@------------| raw=65.9999999952, min=50, max=100 default=67
PARAM_DEFINE PARAM_NULL_MOVE_DYNAMIC_BETA_OPENING = 66;

// |--------+--@------| raw=42.9999999906, min=10, max=60 default=35
PARAM_DEFINE PARAM_NULL_MOVE_MARGIN_OPENING = 43;

// |------------+--@--| raw=13.9999999917, min=4, max=15 default=12
PARAM_DEFINE PARAM_NULL_MOVE_RETURN_DEPTH_OPENING = 14;

// |----+--@----------| raw=5.9999999925, min=3, max=10 default=5
PARAM_DEFINE PARAM_PROBCUT_DEPTH_OPENING = 6;

// |-----@--+---------| raw=166.999999998, min=100, max=300 default=200
PARAM_DEFINE PARAM_PROBCUT_MARGIN_OPENING = 167;

// |-------@----------| raw=7.999999995, min=4, max=13 default=8
PARAM_DEFINE PARAM_SINGULAR_EXTENSION_DEPTH_OPENING = 8;

// |--------+--@------| raw=303.999999991, min=128, max=400 default=256
PARAM_DEFINE PARAM_SINGULAR_MARGIN_OPENING = 304;

// |-----+-@----------| raw=18.9999999913, min=8, max=32 default=16
PARAM_DEFINE PARAM_SINGULAR_SEARCH_DEPTH_ALPHA_OPENING = 19;

// |-----+-@----------| raw=18.9999999913, min=8, max=32 default=16
PARAM_DEFINE PARAM_PRUNING_BY_MOVE_COUNT_DEPTH_OPENING = 19;

// |+@----------------| raw=3.99999999, min=2, max=32 default=3
PARAM_DEFINE PARAM_PRUNING_BY_HISTORY_DEPTH_OPENING = 4;

// |-----@+-----------| raw=7792.0, min=4000, max=15000 default=8000
PARAM_DEFINE PARAM_REDUCTION_BY_HISTORY_OPENING = 7792;

// |--------+-@-------| raw=279.999999993, min=128, max=384 default=256
PARAM_DEFINE PARAM_IID_MARGIN_ALPHA_OPENING = 280;

// |@---+-------------| raw=413.999999999, min=400, max=700 default=483
PARAM_DEFINE PARAM_RAZORING_MARGIN1_OPENING = 414;

// |--@------+--------| raw=448.999999999, min=400, max=700 default=570
PARAM_DEFINE PARAM_RAZORING_MARGIN2_OPENING = 449;

// |-----@-----+------| raw=488.999999999, min=400, max=700 default=603
PARAM_DEFINE PARAM_RAZORING_MARGIN3_OPENING = 489;

// |--------+@--------| raw=566.999999995, min=400, max=700 default=554
PARAM_DEFINE PARAM_RAZORING_MARGIN4_OPENING = 567;

// |-----+-@----------| raw=149.999999991, min=64, max=256 default=128
PARAM_DEFINE PARAM_REDUCTION_ALPHA_OPENING = 150;

// |------@-----------| raw=242.999999995, min=150, max=400 default=240
PARAM_DEFINE PARAM_FUTILITY_MOVE_COUNT_ALPHA0_OPENING = 243;

// |------@--+--------| raw=239.999999998, min=150, max=400 default=290
PARAM_DEFINE PARAM_FUTILITY_MOVE_COUNT_ALPHA1_OPENING = 240;

// |--@+--------------| raw=735.999999996, min=500, max=2000 default=773
PARAM_DEFINE PARAM_FUTILITY_MOVE_COUNT_BETA0_OPENING = 736;

// |-----@+-----------| raw=995.999999996, min=500, max=2000 default=1045
PARAM_DEFINE PARAM_FUTILITY_MOVE_COUNT_BETA1_OPENING = 996;

// |-----+@-----------| raw=70.9999999928, min=32, max=128 default=64
PARAM_DEFINE PARAM_QUIET_SEARCH_COUNT_OPENING = 71;

// Created at: 2017-04-16 22:17:42
// Log: optimize_parameters.hyperopt_state.20170401_170936_b10000_2ndpart.pickle
// Parameters CSV: param\parameters.ending.csv
// n_iterations_to_use = 1000
// minima_method = Data

// |----@-+-----------| raw=132.999999997, min=100, max=240 default=150
PARAM_DEFINE PARAM_FUTILITY_MARGIN_ALPHA_ENDING = 133;

// |--------@---+-----| raw=170.999999998, min=100, max=240 default=200
PARAM_DEFINE PARAM_FUTILITY_MARGIN_BETA_ENDING = 171;

// |-------@----+-----| raw=96.0, min=50, max=160 default=128
PARAM_DEFINE PARAM_FUTILITY_MARGIN_QUIET_ENDING = 96;

// |----+-@-----------| raw=7.9999999925, min=5, max=13 default=7
PARAM_DEFINE PARAM_FUTILITY_RETURN_DEPTH_ENDING = 8;

// |----@-------------| raw=6.999999995, min=5, max=13 default=7
PARAM_DEFINE PARAM_FUTILITY_AT_PARENT_NODE_DEPTH_ENDING = 7;

// |----------@--+----| raw=220.999999997, min=100, max=300 default=256
PARAM_DEFINE PARAM_FUTILITY_AT_PARENT_NODE_MARGIN1_ENDING = 221;

// |--------@--+------| raw=155.999999999, min=0, max=300 default=200
PARAM_DEFINE PARAM_FUTILITY_AT_PARENT_NODE_MARGIN2_ENDING = 156;

// |-------@+---------| raw=33.9999999956, min=20, max=50 default=35
PARAM_DEFINE PARAM_FUTILITY_AT_PARENT_NODE_GAMMA1_ENDING = 34;

// |------+-@---------| raw=39.9999999922, min=20, max=60 default=35
PARAM_DEFINE PARAM_FUTILITY_AT_PARENT_NODE_GAMMA2_ENDING = 40;

// |-----+-@----------| raw=919.999999993, min=500, max=1500 default=823
PARAM_DEFINE PARAM_NULL_MOVE_DYNAMIC_ALPHA_ENDING = 920;

// |----@+------------| raw=62.9999999962, min=50, max=100 default=67
PARAM_DEFINE PARAM_NULL_MOVE_DYNAMIC_BETA_ENDING = 63;

// |--------+-@-------| raw=41.9999999911, min=10, max=60 default=35
PARAM_DEFINE PARAM_NULL_MOVE_MARGIN_ENDING = 42;

// |------------+--@--| raw=13.9999999917, min=4, max=15 default=12
PARAM_DEFINE PARAM_NULL_MOVE_RETURN_DEPTH_ENDING = 14;

// |----@-------------| raw=4.999999995, min=3, max=10 default=5
PARAM_DEFINE PARAM_PROBCUT_DEPTH_ENDING = 5;

// |--------@---------| raw=203.999999995, min=100, max=300 default=200
PARAM_DEFINE PARAM_PROBCUT_MARGIN_ENDING = 204;

// |---@---+----------| raw=6.0, min=4, max=13 default=8
PARAM_DEFINE PARAM_SINGULAR_EXTENSION_DEPTH_ENDING = 6;

// |-----@--+---------| raw=209.999999999, min=128, max=400 default=256
PARAM_DEFINE PARAM_SINGULAR_MARGIN_ENDING = 210;

// |-----+--@---------| raw=19.99999999, min=8, max=32 default=16
PARAM_DEFINE PARAM_SINGULAR_SEARCH_DEPTH_ALPHA_ENDING = 20;

// |-----@------------| raw=15.999999995, min=8, max=32 default=16
PARAM_DEFINE PARAM_PRUNING_BY_MOVE_COUNT_DEPTH_ENDING = 16;

// |@-----------------| raw=2.999999995, min=2, max=32 default=3
PARAM_DEFINE PARAM_PRUNING_BY_HISTORY_DEPTH_ENDING = 3;

// |---@--+-----------| raw=6155.0, min=4000, max=15000 default=8000
PARAM_DEFINE PARAM_REDUCTION_BY_HISTORY_ENDING = 6155;

// |--------+---@-----| raw=314.99999999, min=128, max=384 default=256
PARAM_DEFINE PARAM_IID_MARGIN_ALPHA_ENDING = 315;

// |----+---@---------| raw=549.999999993, min=400, max=700 default=483
PARAM_DEFINE PARAM_RAZORING_MARGIN1_ENDING = 550;

// |-----@---+--------| raw=498.999999997, min=400, max=700 default=570
PARAM_DEFINE PARAM_RAZORING_MARGIN2_ENDING = 499;

// |----------@+------| raw=586.999999995, min=400, max=700 default=603
PARAM_DEFINE PARAM_RAZORING_MARGIN3_ENDING = 587;

// |--------+-@-------| raw=576.999999994, min=400, max=700 default=554
PARAM_DEFINE PARAM_RAZORING_MARGIN4_ENDING = 577;

// |-----@------------| raw=128.999999995, min=64, max=256 default=128
PARAM_DEFINE PARAM_REDUCTION_ALPHA_ENDING = 129;

// |-----@+-----------| raw=231.999999996, min=150, max=400 default=240
PARAM_DEFINE PARAM_FUTILITY_MOVE_COUNT_ALPHA0_ENDING = 232;

// |---------+---@----| raw=345.999999991, min=150, max=400 default=290
PARAM_DEFINE PARAM_FUTILITY_MOVE_COUNT_ALPHA1_ENDING = 346;

// |--@+--------------| raw=762.999999995, min=500, max=2000 default=773
PARAM_DEFINE PARAM_FUTILITY_MOVE_COUNT_BETA0_ENDING = 763;

// |------@-----------| raw=1041.0, min=500, max=2000 default=1045
PARAM_DEFINE PARAM_FUTILITY_MOVE_COUNT_BETA1_ENDING = 1041;

// |---@-+------------| raw=50.9999999991, min=32, max=128 default=64
PARAM_DEFINE PARAM_QUIET_SEARCH_COUNT_ENDING = 51;

PARAM_DEFINE PARAM_QSEARCH_MATE1_OPENING = 1;
PARAM_DEFINE PARAM_QSEARCH_MATE1_ENDING = 1;
PARAM_DEFINE PARAM_SEARCH_MATE1_OPENING = 1;
PARAM_DEFINE PARAM_SEARCH_MATE1_ENDING = 1;
PARAM_DEFINE PARAM_WEAK_MATE_PLY_OPENING = 2;
PARAM_DEFINE PARAM_WEAK_MATE_PLY_ENDING = 1;
#endif
