﻿// Definition of input features and network structure used in NNUE evaluation function
// NNUE評価関数で用いる入力特徴量とネットワーク構造の定義
#ifndef NNUE_HALFKA_HALFKV_256X2_32_32_H_INCLUDED
#define NNUE_HALFKA_HALFKV_256X2_32_32_H_INCLUDED

#include "../features/feature_set.h"
#include "../features/half_ka.h"
#include "../features/half_kv.h"

#include "../layers/input_slice.h"
#include "../layers/affine_transform.h"
#include "../layers/clipped_relu.h"

namespace Eval::NNUE {

// Input features used in evaluation function
// 評価関数で用いる入力特徴量
using RawFeatures = Features::FeatureSet<
    Features::HalfKA<Features::Side::kFriend>,
    Features::HalfKV<Features::Side::kFriend>>;

// Number of input feature dimensions after conversion
// 変換後の入力特徴量の次元数
constexpr IndexType kTransformedFeatureDimensions = 256;

namespace Layers {

// Define network structure
// ネットワーク構造の定義
using InputLayer = InputSlice<kTransformedFeatureDimensions * 2>;
using HiddenLayer1 = ClippedReLU<AffineTransform<InputLayer, 32>>;
using HiddenLayer2 = ClippedReLU<AffineTransform<HiddenLayer1, 32>>;
using OutputLayer = AffineTransform<HiddenLayer2, 1>;

}  // namespace Layers

using Network = Layers::OutputLayer;

}  // namespace Eval::NNUE

#endif // #ifndef NNUE_HALFKA_HALFKV_256X2_32_32_H_INCLUDED