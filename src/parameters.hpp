#ifndef APERY_PARAMETERS_HPP
#define APERY_PARAMETERS_HPP

constexpr int FLOAT_SCALE = 1 << 10;

// Created at: 2016-04-22 12:24:09
// Log: 2016-04-22.pickle.txt
// Parameters CSV: parameters.csv
// n_iterations_to_use = -1
// minima_method = Data

#ifndef MY_NAME
#define MY_NAME "tanuki-2016-04-22-122409"
#endif

#ifndef QSEARCH_FUTILITY_MARGIN
// |--------@---------| raw=131.0, min=0, max=256 default=128
#define QSEARCH_FUTILITY_MARGIN 131
#endif

#ifndef SEARCH_FUTILITY_MARGIN_DEPTH_THRESHOLD
// |-------+@---------| raw=15.0, min=2, max=28 default=14
#define SEARCH_FUTILITY_MARGIN_DEPTH_THRESHOLD 15
#endif

#ifndef SEARCH_FUTILITY_MARGIN_INTERCEPT
// |-@------+---------| raw=10795.0, min=0, max=136986 default=68493
#define SEARCH_FUTILITY_MARGIN_INTERCEPT 10795
#endif

#ifndef SEARCH_FUTILITY_MARGIN_LOG_D_COEFFICIENT
// |--------+------@--| raw=123307.0, min=0, max=138096 default=69048
#define SEARCH_FUTILITY_MARGIN_LOG_D_COEFFICIENT 123307
#endif

#ifndef SEARCH_FUTILITY_MARGIN_MOVE_COUNT_COEFFICIENT
// |---@----+---------| raw=3543.0, min=0, max=16384 default=8192
#define SEARCH_FUTILITY_MARGIN_MOVE_COUNT_COEFFICIENT 3543
#endif

#ifndef SEARCH_FUTILITY_MOVE_COUNTS_INTERCEPT
// |----@---+---------| raw=1508.0, min=0, max=6146 default=3073
#define SEARCH_FUTILITY_MOVE_COUNTS_INTERCEPT 1508
#endif

#ifndef SEARCH_FUTILITY_MOVE_COUNTS_POWER
// |-----+@-----------| raw=2101.0, min=1024, max=3686 default=1843
#define SEARCH_FUTILITY_MOVE_COUNTS_POWER 2101
#endif

#ifndef SEARCH_FUTILITY_MOVE_COUNTS_SCALE
// |--------+--@------| raw=429.0, min=0, max=614 default=307
#define SEARCH_FUTILITY_MOVE_COUNTS_SCALE 429
#endif

#ifndef SEARCH_FUTILITY_PRUNING_NON_PV_REDUCTION_INTERCEPT
// |--@-----+---------| raw=89.0, min=0, max=682 default=337
#define SEARCH_FUTILITY_PRUNING_NON_PV_REDUCTION_INTERCEPT 89
#endif

#ifndef SEARCH_FUTILITY_PRUNING_NON_PV_REDUCTION_SLOPE
// |------@-+---------| raw=370.0, min=0, max=910 default=455
#define SEARCH_FUTILITY_PRUNING_NON_PV_REDUCTION_SLOPE 370
#endif

#ifndef SEARCH_FUTILITY_PRUNING_PREDICTED_DEPTH_THRESHOLD
// |-------+----@-----| raw=12.0, min=2, max=16 default=8
#define SEARCH_FUTILITY_PRUNING_PREDICTED_DEPTH_THRESHOLD 12
#endif

#ifndef SEARCH_FUTILITY_PRUNING_PV_REDUCTION_INTERCEPT
// |------@-+---------| raw=94.0, min=0, max=256 default=128
#define SEARCH_FUTILITY_PRUNING_PV_REDUCTION_INTERCEPT 94
#endif

#ifndef SEARCH_FUTILITY_PRUNING_PV_REDUCTION_SLOPE
// |-------@+---------| raw=291.0, min=0, max=682 default=341
#define SEARCH_FUTILITY_PRUNING_PV_REDUCTION_SLOPE 291
#endif

#ifndef SEARCH_FUTILITY_PRUNING_SCORE_GAIN_SLOPE
// |--------+---@-----| raw=2894.0, min=0, max=4096 default=2048
#define SEARCH_FUTILITY_PRUNING_SCORE_GAIN_SLOPE 2894
#endif

#ifndef SEARCH_INTERNAL_ITERATIVE_DEEPENING_NON_PV_DEPTH_SCALE
// |---@----+---------| raw=189.0, min=0, max=1024 default=512
#define SEARCH_INTERNAL_ITERATIVE_DEEPENING_NON_PV_DEPTH_SCALE 189
#endif

#ifndef SEARCH_INTERNAL_ITERATIVE_DEEPENING_NON_PV_NODE_DEPTH_THRESHOLD
// |-------+-@--------| raw=19.0, min=2, max=32 default=16
#define SEARCH_INTERNAL_ITERATIVE_DEEPENING_NON_PV_NODE_DEPTH_THRESHOLD 19
#endif

#ifndef SEARCH_INTERNAL_ITERATIVE_DEEPENING_PV_NODE_DEPTH_DELTA
// |-----+-----------@| raw=8.0, min=2, max=8 default=4
#define SEARCH_INTERNAL_ITERATIVE_DEEPENING_PV_NODE_DEPTH_DELTA 8
#endif

#ifndef SEARCH_INTERNAL_ITERATIVE_DEEPENING_PV_NODE_DEPTH_THRESHOLD
// |----@--+----------| raw=7.0, min=2, max=20 default=10
#define SEARCH_INTERNAL_ITERATIVE_DEEPENING_PV_NODE_DEPTH_THRESHOLD 7
#endif

#ifndef SEARCH_INTERNAL_ITERATIVE_DEEPENING_SCORE_MARGIN
// |--------@---------| raw=267.0, min=0, max=512 default=256
#define SEARCH_INTERNAL_ITERATIVE_DEEPENING_SCORE_MARGIN 267
#endif

#ifndef SEARCH_LATE_MOVE_REDUCTION_DEPTH_THRESHOLD
// |-----@+-----------| raw=5.0, min=2, max=12 default=6
#define SEARCH_LATE_MOVE_REDUCTION_DEPTH_THRESHOLD 5
#endif

#ifndef SEARCH_NULL_FAIL_LOW_SCORE_DEPTH_THRESHOLD
// |------@+----------| raw=9.0, min=2, max=20 default=10
#define SEARCH_NULL_FAIL_LOW_SCORE_DEPTH_THRESHOLD 9
#endif

#ifndef SEARCH_NULL_MOVE_DEPTH_THRESHOLD
// |-----+-----------@| raw=8.0, min=2, max=8 default=4
#define SEARCH_NULL_MOVE_DEPTH_THRESHOLD 8
#endif

#ifndef SEARCH_NULL_MOVE_MARGIN
// |-----@--+---------| raw=63.0, min=0, max=180 default=90
#define SEARCH_NULL_MOVE_MARGIN 63
#endif

#ifndef SEARCH_NULL_MOVE_NULL_SCORE_DEPTH_THRESHOLD
// |-------+--------@-| raw=23.0, min=2, max=24 default=12
#define SEARCH_NULL_MOVE_NULL_SCORE_DEPTH_THRESHOLD 23
#endif

#ifndef SEARCH_NULL_MOVE_REDUCTION_INTERCEPT
// |------+-@---------| raw=7.0, min=2, max=12 default=6
#define SEARCH_NULL_MOVE_REDUCTION_INTERCEPT 7
#endif

#ifndef SEARCH_NULL_MOVE_REDUCTION_SLOPE
// |--------+------@--| raw=478.0, min=0, max=512 default=256
#define SEARCH_NULL_MOVE_REDUCTION_SLOPE 478
#endif

#ifndef SEARCH_PROBCUT_DEPTH_THRESHOLD
// |-------@----------| raw=10.0, min=3, max=20 default=10
#define SEARCH_PROBCUT_DEPTH_THRESHOLD 10
#endif

#ifndef SEARCH_PROBCUT_RBETA_DEPTH_DELTA
// |-------+---------@| raw=16.0, min=2, max=16 default=8
#define SEARCH_PROBCUT_RBETA_DEPTH_DELTA 16
#endif

#ifndef SEARCH_PROBCUT_RBETA_SCORE_DELTA
// |--------+-------@-| raw=398.0, min=0, max=400 default=200
#define SEARCH_PROBCUT_RBETA_SCORE_DELTA 398
#endif

#ifndef SEARCH_RAZORING_DEPTH
// |-------+@---------| raw=9.0, min=2, max=16 default=8
#define SEARCH_RAZORING_DEPTH 9
#endif

#ifndef SEARCH_RAZORING_MARGIN_INTERCEPT
// |--------+---@-----| raw=773975.0, min=0, max=1048576 default=524288
#define SEARCH_RAZORING_MARGIN_INTERCEPT 773975
#endif

#ifndef SEARCH_RAZORING_MARGIN_SLOPE
// |----@---+---------| raw=8636.0, min=0, max=32768 default=16384
#define SEARCH_RAZORING_MARGIN_SLOPE 8636
#endif

#ifndef SEARCH_SINGULAR_EXTENSION_DEPTH_THRESHOLD
// |-@-----+----------| raw=4.0, min=2, max=32 default=16
#define SEARCH_SINGULAR_EXTENSION_DEPTH_THRESHOLD 4
#endif

#ifndef SEARCH_SINGULAR_EXTENSION_NULL_WINDOW_SEARCH_DEPTH_SCALE
// |-------@+---------| raw=476.0, min=0, max=1024 default=512
#define SEARCH_SINGULAR_EXTENSION_NULL_WINDOW_SEARCH_DEPTH_SCALE 476
#endif

#ifndef SEARCH_SINGULAR_EXTENSION_TTE_DEPTH_THRESHOLD
// |-@----+-----------| raw=3.0, min=2, max=12 default=6
#define SEARCH_SINGULAR_EXTENSION_TTE_DEPTH_THRESHOLD 3
#endif

#ifndef SEARCH_STATIC_NULL_MOVE_PRUNING_DEPTH_THRESHOLD
// |-------+----@-----| raw=12.0, min=2, max=16 default=8
#define SEARCH_STATIC_NULL_MOVE_PRUNING_DEPTH_THRESHOLD 12
#endif

#endif
