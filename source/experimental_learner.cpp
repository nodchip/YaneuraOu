#ifdef USE_EXPERIMENTAL_LEARNER

#include "experimental_learner.h"

#include <direct.h>
#include <omp.h>
#include <array>
#include <ctime>
#include <fstream>
#include <numeric>

#include "eval/progress.h"
#include "kifu_reader.h"
#include "position.h"
#include "progress_report.h"
#include "search.h"
#include "thread.h"

namespace Learner {
std::pair<Value, std::vector<Move> > qsearch(Position& pos);
}

namespace Eval {
typedef std::array<int16_t, 2> ValueKpp;
typedef std::array<int32_t, 2> ValueKkp;
typedef std::array<int32_t, 2> ValueKk;
// FV_SCALEで割る前の値が入る
extern ALIGNED(32) ValueKk kk[SQ_NB][SQ_NB];
extern ALIGNED(32) ValueKpp kpp[SQ_NB][fe_end][fe_end];
extern ALIGNED(32) ValueKkp kkp[SQ_NB][SQ_NB][fe_end];
extern const int FV_SCALE = 32;

void save_eval();
void eval_learn_init();
extern BonaPiece inv_piece[fe_end];
extern BonaPiece mir_piece[fe_end];
}

using Eval::BonaPiece;
using Eval::fe_end;
using Eval::inv_piece;
using Eval::mir_piece;
using USI::Option;
using USI::OptionsMap;

namespace {
using WeightType = double;

enum WeightKind {
    WEIGHT_KIND_COLOR,
    WEIGHT_KIND_TURN,
    WEIGHT_KIND_ZERO = 0,
    WEIGHT_KIND_NB = 2,
};
ENABLE_FULL_OPERATORS_ON(WeightKind);

struct Weight {
    // 勾配の和
    WeightType sum_gradient_raw;
    WeightType sum_gradient_lower_dimension;
    // 重み
    WeightType w;
    // Adam用変数
    WeightType m;
    WeightType v;

    void Weight::AddRawGradient(double gradient) { sum_gradient_raw += gradient; }

    void Weight::AddLowerDimensionGradient(double gradient) {
        sum_gradient_lower_dimension += gradient;
    }

    template <typename T>
    void Weight::UpdateWeight(double adam_beta1_t, double adam_beta2, double adam_beta2_t,
                              double learning_rate, double fobos_l1_parameter,
                              double fobos_l2_parameter, T& eval_weight) {
        // Adam
        m = kAdamBeta1 * m + (1.0 - kAdamBeta1) * sum_gradient_lower_dimension;
        v = adam_beta2 * v +
            (1.0 - adam_beta2) * sum_gradient_lower_dimension * sum_gradient_lower_dimension;
        // 高速化のためpow(ADAM_BETA1, t)の値を保持しておく
        WeightType mm = m / (1.0 - adam_beta1_t);
        WeightType vv = v / (1.0 - adam_beta2_t);
        WeightType delta = learning_rate * mm / (std::sqrt(vv) + kEps);
        w -= delta;
        if (w > fobos_l1_parameter) {
            w -= fobos_l1_parameter;
        } else if (w < -fobos_l1_parameter) {
            w += fobos_l1_parameter;
        } else {
            w = 0.0;
        }
        w *= fobos_l2_parameter;

        // 重みテーブルに書き戻す
        eval_weight = static_cast<T>(std::round(w));

        sum_gradient_raw = 0.0;
        sum_gradient_lower_dimension = 0.0;
    }
};

constexpr int kFvScale = 32;
constexpr WeightType kEps = 1e-8;
constexpr WeightType kAdamBeta1 = 0.9;
constexpr int kMaxGamePlay = 256;
constexpr int64_t kMaxPositionsForErrorMeasurement = 1000'0000LL;
constexpr int64_t kMaxPositionsForBenchmark = 1'0000'0000LL;
constexpr int64_t kShowProgressAtMostSec = 600;  // 10分

constexpr char* kOptionValueLearnerNumPositions = "LearnerNumPositions";
constexpr char* kOptionValueMinLearningRate = "MinLearningRate";
constexpr char* kOptionValueMaxLearningRate = "MaxLearningRate";
constexpr char* kOptionValueNumLearningRateCycles = "NumLearningRateCycles";
constexpr char* kOptionValueKifuForTestDir = "KifuForTestDir";
constexpr char* kOptionValueLearnerNumPositionsForTest = "LearnerNumPositionsForTest";
constexpr char* kOptionValueMiniBatchSize = "MiniBatchSize";
constexpr char* kOptionValueFobosL1Parameter = "FobosL1Parameter";
constexpr char* kOptionValueFobosL2Parameter = "FobosL2Parameter";
constexpr char* kOptionValueElmoLambda = "ElmoLambda";
constexpr char* kOptionValueValueToWinningRateCoefficient = "ValueToWinningRateCoefficient";
constexpr char* kOptionValueAdamBeta2 = "AdamBeta2";
constexpr char* kOptionValueUseProgressAsElmoLambda = "UseProgressAsElmoLambda";
constexpr char* kOptionValueBreedEvalBaseFolderPath = "BreedEvalBaseFolderPath";
constexpr char* kOptionValueBreedEvalAnotherFolderPath = "BreedEvalAnotherFolderPath";
constexpr char* kOptionValueBreedEvalOutputFolderPath = "BreedEvalOutputFolderPath";

class Kpp {
   public:
    Kpp() {}

    Kpp(Square king, BonaPiece piece0, BonaPiece piece1, WeightKind weight_kind)
        : king_(king), piece0_(piece0), piece1_(piece1), weight_kind_(weight_kind) {}

    // Inclusive
    static int MinIndex() { return 0; }

    // Exclusive
    static int MaxIndex() {
        constexpr int k = SQ_NB;
        constexpr int p = fe_end;
        constexpr int w = WEIGHT_KIND_NB;
        return k * p * p * w;
    }

    static bool IsValid(int dimension) { return MinIndex() <= dimension && dimension < MaxIndex(); }

    int ToIndex() const {
        constexpr int k = SQ_NB;
        constexpr int p = fe_end;
        constexpr int w = WEIGHT_KIND_NB;
        int index = ((king_ * p + piece0_) * p + piece1_) * w + weight_kind_;
        ASSERT_LV3(IsValid(index));
        return index;
    }

    void ToLowerDimensions(Kpp kpp[4]) const {
        kpp[0] = Kpp(king_, piece0_, piece1_, weight_kind_);
        kpp[1] = Kpp(king_, piece1_, piece0_, weight_kind_);
        kpp[2] = Kpp(Mir(king_), mir_piece[piece0_], mir_piece[piece1_], weight_kind_);
        kpp[3] = Kpp(Mir(king_), mir_piece[piece1_], mir_piece[piece0_], weight_kind_);
    }

    static Kpp ForIndex(int index) {
        int original_index = index;
        ASSERT_LV3(IsValid(index));
        constexpr int k = SQ_NB;
        constexpr int p = fe_end;
        constexpr int w = WEIGHT_KIND_NB;
        index -= MinIndex();
        int weight_kind = index % w;
        index /= w;
        int piece1 = index % p;
        index /= p;
        int piece0 = index % p;
        index /= p;
        int king = index;
        ASSERT_LV3(king < k);
        Kpp kpp(static_cast<Square>(king), static_cast<BonaPiece>(piece0),
                static_cast<BonaPiece>(piece1), static_cast<WeightKind>(weight_kind));
        ASSERT_LV3(kpp.ToIndex() == original_index);
        return kpp;
    }

    Square king() const { return king_; }
    BonaPiece piece0() const { return piece0_; }
    BonaPiece piece1() const { return piece1_; }
    WeightKind weight_kind() const { return weight_kind_; }

   private:
    Square king_;
    BonaPiece piece0_;
    BonaPiece piece1_;
    WeightKind weight_kind_;
};

class Kkp {
   public:
    Kkp() {}

    Kkp(Square king0, Square king1, BonaPiece piece, WeightKind weight_kind,
        bool invert_sign = false)
        : king0_(king0),
          king1_(king1),
          piece_(piece),
          weight_kind_(weight_kind),
          invert_sign_(invert_sign) {}

    // Inclusive
    static int MinIndex() { return Kpp::MaxIndex(); }

    // Exclusive
    static int MaxIndex() {
        constexpr int k = SQ_NB;
        constexpr int p = fe_end;
        constexpr int w = WEIGHT_KIND_NB;
        return MinIndex() + k * k * p * w;
    }

    static bool IsValid(int dimension) { return MinIndex() <= dimension && dimension < MaxIndex(); }

    int ToIndex() const {
        constexpr int k = SQ_NB;
        constexpr int p = fe_end;
        constexpr int w = WEIGHT_KIND_NB;
        int index = MinIndex() + ((king0_ * k + king1_) * p + piece_) * w + weight_kind_;
        ASSERT_LV3(IsValid(index));
        return index;
    }

    void ToLowerDimensions(Kkp kkp[4]) const {
        kkp[0] = Kkp(king0_, king1_, piece_, weight_kind_);
        kkp[1] = Kkp(Mir(king0_), Mir(king1_), mir_piece[piece_], weight_kind_);
        kkp[2] = Kkp(Inv(king1_), Inv(king0_), inv_piece[piece_], weight_kind_,
                     weight_kind_ == WEIGHT_KIND_COLOR);
        kkp[3] = Kkp(Inv(Mir(king1_)), Inv(Mir(king0_)), inv_piece[mir_piece[piece_]], weight_kind_,
                     weight_kind_ == WEIGHT_KIND_COLOR);
    }

    static Kkp ForIndex(int index) {
        int original_index = index;
        ASSERT_LV3(IsValid(index));
        constexpr int k = SQ_NB;
        constexpr int p = fe_end;
        constexpr int w = WEIGHT_KIND_NB;
        index -= MinIndex();
        int weight_kind = index % w;
        index /= w;
        int piece = index % p;
        index /= p;
        int king1 = index % k;
        index /= k;
        int king0 = index;
        ASSERT_LV3(king0 < k);
        Kkp kkp(static_cast<Square>(king0), static_cast<Square>(king1),
                static_cast<BonaPiece>(piece), static_cast<WeightKind>(weight_kind));
        ASSERT_LV3(kkp.ToIndex() == original_index);
        return kkp;
    }

    Square king0() const { return king0_; }
    Square king1() const { return king1_; }
    BonaPiece piece() const { return piece_; }
    WeightKind weight_kind() const { return weight_kind_; }
    bool invert_sign() const { return invert_sign_; }

   private:
    Square king0_;
    Square king1_;
    BonaPiece piece_;
    WeightKind weight_kind_;
    bool invert_sign_;
};

class Kk {
   public:
    Kk() {}

    Kk(Square king0, Square king1, WeightKind weight_kind, bool invert_sign = false)
        : king0_(king0), king1_(king1), weight_kind_(weight_kind), invert_sign_(invert_sign) {}

    // Inclusive
    static int MinIndex() { return Kkp::MaxIndex(); }

    // Exclusive
    static int MaxIndex() {
        constexpr int k = SQ_NB;
        constexpr int p = fe_end;
        constexpr int w = WEIGHT_KIND_NB;
        return MinIndex() + k * k * w;
    }

    static bool IsValid(int dimension) { return MinIndex() <= dimension && dimension < MaxIndex(); }

    int ToIndex() const {
        constexpr int k = SQ_NB;
        constexpr int p = fe_end;
        constexpr int w = WEIGHT_KIND_NB;
        int index = MinIndex() + (king0_ * k + king1_) * w + weight_kind_;
        ASSERT_LV3(IsValid(index));
        return index;
    }

    void ToLowerDimensions(Kk kk[4]) const {
        kk[0] = Kk(king0_, king1_, weight_kind_);
        kk[1] = Kk(Mir(king0_), Mir(king1_), weight_kind_);
        kk[2] = Kk(Inv(king1_), Inv(king0_), weight_kind_, weight_kind_ == WEIGHT_KIND_COLOR);
        kk[3] =
            Kk(Inv(Mir(king1_)), Inv(Mir(king0_)), weight_kind_, weight_kind_ == WEIGHT_KIND_COLOR);
    }

    static Kk ForIndex(int index) {
        int original_index = index;
        ASSERT_LV3(IsValid(index));
        constexpr int k = SQ_NB;
        constexpr int p = fe_end;
        constexpr int w = WEIGHT_KIND_NB;
        index -= MinIndex();
        int weight_kind = index % w;
        index /= w;
        int king1 = index % k;
        index /= k;
        int king0 = index;
        ASSERT_LV3(king0 < k);
        Kk kk(static_cast<Square>(king0), static_cast<Square>(king1),
              static_cast<WeightKind>(weight_kind));
        ASSERT_LV3(kk.ToIndex() == original_index);
        return kk;
    }

    Square king0() const { return king0_; }
    Square king1() const { return king1_; }
    WeightKind weight_kind() const { return weight_kind_; }
    bool invert_sign() const { return invert_sign_; }

   private:
    Square king0_;
    Square king1_;
    WeightKind weight_kind_;
    bool invert_sign_;
};

struct Weights {
    typedef std::array<int16_t, 2> ValueKpp;
    typedef std::array<int32_t, 2> ValueKkp;
    typedef std::array<int32_t, 2> ValueKk;

    static constexpr const char* kKkSynthesizedFileName = "KK_synthesized.bin";
    static constexpr const char* kKkpSynthesizedFileName = "KKP_synthesized.bin";
    static constexpr const char* kKppSynthesizedFileName = "KPP_synthesized.bin";

    ValueKk kk[SQ_NB][SQ_NB];
    ValueKpp kpp[SQ_NB][BonaPiece::fe_end][BonaPiece::fe_end];
    ValueKkp kkp[SQ_NB][SQ_NB][BonaPiece::fe_end];

    bool load(const std::string& folder_path) {
        sync_cout << __FUNCTION__ << " " << folder_path << sync_endl;

        // KK
        std::ifstream ifsKK(path_combine(folder_path, kKkSynthesizedFileName), std::ios::binary);
        if (!ifsKK) {
            sync_cout << "Failed to open " << kKkSynthesizedFileName << sync_endl;
            return false;
        }
        ifsKK.read(reinterpret_cast<char*>(kk), sizeof(kk));

        // KKP
        std::ifstream ifsKKP(path_combine(folder_path, kKkpSynthesizedFileName), std::ios::binary);
        if (!ifsKKP) {
            sync_cout << "Failed to open " << kKkpSynthesizedFileName << sync_endl;
            return false;
        }
        ifsKKP.read(reinterpret_cast<char*>(kkp), sizeof(kkp));

        // KPP
        std::ifstream ifsKPP(path_combine(folder_path, kKppSynthesizedFileName), std::ios::binary);
        if (!ifsKPP) {
            sync_cout << "Failed to open " << kKppSynthesizedFileName << sync_endl;
            return false;
        }
        ifsKPP.read(reinterpret_cast<char*>(kpp), sizeof(kpp));

        return true;
    }

    bool save(const std::string& folder_path) {
        sync_cout << __FUNCTION__ << " " << folder_path << sync_endl;

        mkdir(folder_path.c_str());

        // KK
        std::ofstream ofsKK(path_combine(folder_path, kKkSynthesizedFileName), std::ios::binary);
        if (!ofsKK) {
            sync_cout << "Failed to open " << kKkSynthesizedFileName << sync_endl;
            return false;
        }
        ofsKK.write(reinterpret_cast<char*>(kk), sizeof(kk));

        // KKP
        std::ofstream ofsKKP(path_combine(folder_path, kKkpSynthesizedFileName), std::ios::binary);
        if (!ofsKKP) {
            sync_cout << "Failed to open " << kKkpSynthesizedFileName << sync_endl;
            return false;
        }
        ofsKKP.write(reinterpret_cast<char*>(kkp), sizeof(kkp));

        // KPP
        std::ofstream ofsKPP(path_combine(folder_path, kKppSynthesizedFileName), std::ios::binary);
        if (!ofsKPP) {
            sync_cout << "Failed to open " << kKppSynthesizedFileName << sync_endl;
            return false;
        }
        ofsKPP.write(reinterpret_cast<char*>(kpp), sizeof(kpp));

        return true;
    }
};

double sigmoid(double x) { return 1.0 / (1.0 + std::exp(-x)); }

double winning_rate(Value value, double value_to_winning_rate_coefficient) {
    return sigmoid(static_cast<int>(value) / value_to_winning_rate_coefficient);
}

double dsigmoid(double x) { return sigmoid(x) * (1.0 - sigmoid(x)); }

std::string GetDateTimeString() {
    time_t time = std::time(nullptr);
    struct tm* tm = std::localtime(&time);
    char buffer[1024];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d-%H-%M-%S", tm);
    return buffer;
}

void save_eval(const std::string& output_folder_path_base, int64_t position_index) {
    sync_cout << "Creating a folder: output_folder_path_base=" << output_folder_path_base
              << sync_endl;
    _mkdir(output_folder_path_base.c_str());

    char buffer[1024];
    std::sprintf(buffer, "%s/%I64d", output_folder_path_base.c_str(), position_index);
    std::printf("Writing eval files: %s\n", buffer);
    std::fflush(stdout);
    Options["EvalDir"] = buffer;
    sync_cout << "Creating a folder: buffer=" << buffer << sync_endl;
    _mkdir(buffer);
    Eval::save_eval();
}

// 浅い探索付きの*Strapを行う
// record 学習局面
// elmo_lambda elmo lambda係数 use_progress_as_elmo_lambdaがtrueの場合、この値は無視される
// use_progress_as_elmo_lambda elmo_lambdaの代わりに進行度を用いる
// progress 進行度推定ルーチン
// f 各局面の学習に使用するコールバック
void Strap(const Learner::Record& record, double elmo_lambda, bool use_progress_as_elmo_lambda,
           const std::shared_ptr<Progress>& progress,
           std::function<void(Value record_value, Color win_color, Value value, Color root_color,
                              double elmo_lambda, Position& pos)>
               f) {
    int thread_index = omp_get_thread_num();
    Thread& thread = *Threads[thread_index];
    Position& pos = thread.rootPos;
    pos.set_from_packed_sfen(record.packed);
    pos.set_this_thread(&thread);
    if (!pos.pos_is_ok()) {
        sync_cout << "Position is not ok! Exiting..." << sync_endl;
        sync_cout << pos << sync_endl;
        std::exit(1);
    }

    Color root_color = pos.side_to_move();

    // 0手読み+静止探索を行う
    auto value_and_pv = Learner::qsearch(pos);

    // Eval::evaluate()を使うと差分計算のおかげで少し速くなるはず
    // 全計算はPosition::set()の中で行われているので差分計算ができる
    Value value = Eval::evaluate(pos);
    StateInfo stateInfo[MAX_PLY];
    std::vector<Move>& pv = value_and_pv.second;
    for (int shallow_search_pv_play_index = 0;
         shallow_search_pv_play_index < static_cast<int>(pv.size());
         ++shallow_search_pv_play_index) {
        pos.do_move(pv[shallow_search_pv_play_index], stateInfo[shallow_search_pv_play_index]);
        value = Eval::evaluate(pos);
    }

    // Eval::evaluate()は常に手番から見た評価値を返すので
    // 探索開始局面と手番が違う場合は符号を反転する
    if (root_color != pos.side_to_move()) {
        value = -value;
    }

    if (use_progress_as_elmo_lambda) {
        elmo_lambda = 1.0 - progress->Estimate(pos);
    }

    f(static_cast<Value>(record.value), static_cast<Color>(record.win_color), value, root_color,
      elmo_lambda, pos);

    // 局面を浅い探索のroot局面にもどす
    for (auto rit = pv.rbegin(); rit != pv.rend(); ++rit) {
        pos.undo_move(*rit);
    }
}
}

void Learner::InitializeLearner(USI::OptionsMap& o) {
    o[kOptionValueLearnerNumPositions] << Option("10000000000");
    o[kOptionValueMinLearningRate] << Option("0.5");
    o[kOptionValueMaxLearningRate] << Option("0.5");
    o[kOptionValueNumLearningRateCycles] << Option("10.0");
    o[kOptionValueKifuForTestDir] << Option("kifu_for_test");
    o[kOptionValueLearnerNumPositionsForTest] << Option("1000000");
    o[kOptionValueMiniBatchSize] << Option("1000000");
    o[kOptionValueFobosL1Parameter] << Option("0.0");
    double fobos_l2_parameter = std::pow(0.9, 1.0 / 1000.0);
    char buffer[1024];
    sprintf(buffer, "%.20f", fobos_l2_parameter);
    o[kOptionValueFobosL2Parameter] << Option(buffer);
    o[kOptionValueElmoLambda] << Option("1.0");
    o[kOptionValueValueToWinningRateCoefficient] << Option("600.0");
    o[kOptionValueAdamBeta2] << Option("0.999");
    o[kOptionValueUseProgressAsElmoLambda] << Option(false);
    o[kOptionValueBreedEvalBaseFolderPath] << Option("eval");
    o[kOptionValueBreedEvalAnotherFolderPath] << Option("eval");
    o[kOptionValueBreedEvalOutputFolderPath] << Option("eval");
}

void Learner::Learn(std::istringstream& iss) {
#ifndef USE_FALSE_PROBE_IN_TT
    static_assert(false, "Please define USE_FALSE_PROBE_IN_TT.");
#endif

    sync_cout << __FUNCTION__ << sync_endl;

    Eval::eval_learn_init();

    ASSERT_LV3(Kk::MaxIndex() ==
               static_cast<int>(SQ_NB) * static_cast<int>(Eval::fe_end) *
                       static_cast<int>(Eval::fe_end) * WEIGHT_KIND_NB +
                   static_cast<int>(SQ_NB) * static_cast<int>(SQ_NB) *
                       static_cast<int>(Eval::fe_end) * WEIGHT_KIND_NB +
                   static_cast<int>(SQ_NB) * static_cast<int>(SQ_NB) * WEIGHT_KIND_NB);

    Eval::eval_learn_init();

    omp_set_num_threads((int)Options["Threads"]);

    std::string output_folder_path_base = "learner_output/" + GetDateTimeString();
    std::string token;
    while (iss >> token) {
        if (token == "output_folder_path_base") {
            iss >> output_folder_path_base;
        }
    }

    std::vector<int64_t> write_eval_per_positions;
    for (int64_t i = 1; i <= 20; ++i) {
        write_eval_per_positions.push_back(i * 5'0000'0000LL);
    }
    write_eval_per_positions.push_back(std::numeric_limits<int64_t>::max());
    std::sort(write_eval_per_positions.begin(), write_eval_per_positions.end());
    int write_eval_per_positions_index = 0;

    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    auto kifu_reader = std::make_unique<Learner::KifuReader>((std::string)Options["KifuDir"],
                                                             std::numeric_limits<int>::max());

    Eval::load_eval();

    int vector_length = Kk::MaxIndex();

    std::vector<Weight> weights(vector_length);
    memset(&weights[0], 0, sizeof(weights[0]) * weights.size());

// 評価関数テーブルから重みベクトルへ重みをコピーする
// 並列化を効かせたいのでdimension_indexで回す
#pragma omp parallel for schedule(guided)
    for (int dimension_index = 0; dimension_index < vector_length; ++dimension_index) {
        if (Kpp::IsValid(dimension_index)) {
            Kpp kpp = Kpp::ForIndex(dimension_index);
            weights[dimension_index].w = static_cast<WeightType>(
                Eval::kpp[kpp.king()][kpp.piece0()][kpp.piece1()][kpp.weight_kind()]);

        } else if (Kkp::IsValid(dimension_index)) {
            Kkp kkp = Kkp::ForIndex(dimension_index);
            weights[dimension_index].w = static_cast<WeightType>(
                Eval::kkp[kkp.king0()][kkp.king1()][kkp.piece()][kkp.weight_kind()]);

        } else if (Kk::IsValid(dimension_index)) {
            Kk kk = Kk::ForIndex(dimension_index);
            weights[dimension_index].w =
                static_cast<WeightType>(Eval::kk[kk.king0()][kk.king1()][kk.weight_kind()]);

        } else {
            ASSERT_LV3(false);
        }
    }

    Search::LimitsType limits;
    // 引き分けの手数付近で引き分けの値が返るのを防ぐため1 << 16にする
    limits.max_game_ply = 1 << 16;
    limits.depth = MAX_PLY;
    limits.silent = true;
    limits.enteringKingRule = EKR_27_POINT;
    Search::Limits = limits;

    // 作成・破棄のコストが高いためループの外に宣言する
    std::vector<Record> records;

    // 全学習データに対してループを回す
    time_t start;
    std::time(&start);
    int num_mini_batches = 0;
    double min_learning_rate = Options[kOptionValueMinLearningRate].cast<double>();
    double max_learning_rate = Options[kOptionValueMaxLearningRate].cast<double>();
    double num_learning_rate_cycles = Options[kOptionValueNumLearningRateCycles].cast<double>();
    int64_t max_positions_for_learning = Options[kOptionValueLearnerNumPositions].cast<int64_t>();
    std::string kifu_for_test_dir = Options[kOptionValueKifuForTestDir];
    int64_t num_positions_for_test =
        Options[kOptionValueLearnerNumPositionsForTest].cast<int64_t>();
    int64_t mini_batch_size = Options[kOptionValueMiniBatchSize].cast<int64_t>();
    double fobos_l1_parameter = Options[kOptionValueFobosL1Parameter].cast<double>();
    double fobos_l2_parameter = Options[kOptionValueFobosL2Parameter].cast<double>();
    double elmo_lambda = Options[kOptionValueElmoLambda].cast<double>();
    double value_to_winning_rate_coefficient =
        Options[kOptionValueValueToWinningRateCoefficient].cast<double>();
    double adam_beta2 = Options[kOptionValueAdamBeta2].cast<double>();
    bool use_progress_as_elmo_lambda = Options[kOptionValueUseProgressAsElmoLambda];

    sync_cout << "min_learning_rate=" << min_learning_rate << sync_endl;
    sync_cout << "max_learning_rate=" << max_learning_rate << sync_endl;
    sync_cout << "num_learning_rate_cycles=" << num_learning_rate_cycles << sync_endl;
    sync_cout << "max_positions_for_learning=" << max_positions_for_learning << sync_endl;
    sync_cout << "kifu_for_test_dir=" << kifu_for_test_dir << sync_endl;
    sync_cout << "num_positions_for_test=" << num_positions_for_test << sync_endl;
    sync_cout << "mini_batch_size=" << mini_batch_size << sync_endl;
    sync_cout << "fobos_l1_parameter=" << fobos_l1_parameter << sync_endl;
    sync_cout << "fobos_l2_parameter=" << fobos_l2_parameter << sync_endl;
    sync_cout << "elmo_lambda=" << elmo_lambda << sync_endl;
    sync_cout << "value_to_winning_rate_coefficient=" << value_to_winning_rate_coefficient
              << sync_endl;
    sync_cout << "adam_beta2=" << adam_beta2 << sync_endl;
    sync_cout << "use_progress_as_elmo_lambda=" << use_progress_as_elmo_lambda << sync_endl;

    auto kifu_reader_for_test = std::make_unique<Learner::KifuReader>(kifu_for_test_dir, false);
    std::vector<Record> records_for_test;
    if (!kifu_reader_for_test->Read(num_positions_for_test, records_for_test)) {
        sync_cout << "Failed to read kifu for test." << sync_endl;
        std::exit(1);
    }

    std::shared_ptr<Progress> progress;
    if (use_progress_as_elmo_lambda) {
        progress = std::make_shared<Progress>();
        if (!progress->Load()) {
            std::exit(1);
        }
    }

    // 未学習の評価関数ファイルを出力しておく
    save_eval(output_folder_path_base, 0);

    // 損失関数ログの作成
    char train_loss_file_name[_MAX_PATH];
    std::sprintf(train_loss_file_name, "%s/loss.csv", output_folder_path_base.c_str());
    FILE* file_loss = std::fopen(train_loss_file_name, "w");
    std::fprintf(file_loss,
                 ",train_rmse_value,"
                 "train_rmse_winning_percentage,"
                 "train_mean_cross_entropy,"
                 "train_mean_cross_entropy_eval,"
                 "train_mean_cross_entropy_win,"
                 "test_rmse_value,"
                 "test_rmse_winning_percentage,"
                 "test_mean_cross_entropy,"
                 "test_mean_cross_entropy_eval,"
                 "test_mean_cross_entropy_win,"
                 "norm,"
                 "train_rmse_win_or_lose,"
                 "test_rmse_win_or_lose,"
                 "train_mean_entropy_eval,"
                 "train_mean_kld_eval,"
                 "test_mean_entropy_eval,"
                 "test_mean_kld_eval\n");
    std::fflush(file_loss);

    ProgressReport progress_report(max_positions_for_learning, kShowProgressAtMostSec);

    for (int64_t num_processed_positions = 0;
         num_processed_positions < max_positions_for_learning;) {
        progress_report.Show(num_processed_positions);

        int num_records = static_cast<int>(
            std::min(max_positions_for_learning - num_processed_positions, mini_batch_size));
        if (!kifu_reader->Read(num_records, records)) {
            sync_cout << "Failed to read kifu for train." << sync_endl;
            std::exit(1);
        }
        ++num_mini_batches;

        double sum_train_squared_error_of_value = 0.0;
        double sum_norm = 0.0;
        double sum_train_squared_error_of_winning_percentage = 0.0;
        double sum_train_cross_entropy = 0.0;
        double sum_train_cross_entropy_eval = 0.0;
        double sum_train_cross_entropy_win = 0.0;
        double sum_train_entropy_eval = 0.0;
        double sum_train_kld_eval = 0.0;
        double sum_test_squared_error_of_value = 0.0;
        double sum_test_squared_error_of_winning_percentage = 0.0;
        double sum_test_cross_entropy = 0.0;
        double sum_test_cross_entropy_eval = 0.0;
        double sum_test_cross_entropy_win = 0.0;
        double sum_test_entropy_eval = 0.0;
        double sum_test_kld_eval = 0.0;
        double sum_train_squared_error_of_win_or_lose = 0.0;
        double sum_test_squared_error_of_win_or_lose = 0.0;

#pragma omp parallel
        {
            int thread_index = omp_get_thread_num();
            WinProcGroup::bindThisThread(thread_index);
// ミニバッチ
// num_records個の学習データの勾配の和を求めて重みを更新する
#pragma omp for schedule(dynamic, 1000) reduction(+ : sum_train_squared_error_of_value) reduction( \
    + : sum_norm) reduction(+ : sum_train_squared_error_of_winning_percentage) reduction(          \
        + : sum_train_cross_entropy) reduction(+ : sum_train_cross_entropy_eval) reduction(        \
            + : sum_train_cross_entropy_win)                                                       \
                reduction(+ : sum_test_squared_error_of_win_or_lose) reduction(                    \
                    + : sum_train_entropy_eval) reduction(+ : sum_train_kld_eval)
            for (int record_index = 0; record_index < num_records; ++record_index) {
                auto f = [value_to_winning_rate_coefficient, &weights,
                          &sum_train_squared_error_of_value, &sum_norm,
                          &sum_train_squared_error_of_winning_percentage, &sum_train_cross_entropy,
                          &sum_train_cross_entropy_eval, &sum_train_cross_entropy_win,
                          &sum_train_squared_error_of_win_or_lose, &progress,
                          &sum_train_entropy_eval, &sum_train_kld_eval](
                    Value record_value, Color win_color, Value value, Color root_color,
                    double elmo_lambda, Position& pos) {
                    // 評価値から推定した勝率の分布の交差エントロピー
                    double p = winning_rate(record_value, value_to_winning_rate_coefficient);
                    double q = winning_rate(value, value_to_winning_rate_coefficient);
                    double t = (root_color == win_color) ? 1.0 : 0.0;
                    // elmo_lambdaは浅い探索の評価値で深い探索の評価値を近似する項に入れる
                    WeightType delta = elmo_lambda * (q - p) + (1.0 - elmo_lambda) * (q - t);

                    double diff_value = record_value - value;
                    sum_train_squared_error_of_value += diff_value * diff_value;
                    double diff_winning_percentage = q - p;
                    sum_train_squared_error_of_winning_percentage +=
                        diff_winning_percentage * diff_winning_percentage;
                    double cross_entropy_eval =
                        -p * std::log(q + kEps) - (1.0 - p) * std::log(1.0 - q + kEps);
                    double cross_entropy_win =
                        -t * std::log(q + kEps) - (1.0 - t) * std::log(1.0 - q + kEps);
                    sum_train_cross_entropy_eval += cross_entropy_eval;
                    sum_train_cross_entropy_win += cross_entropy_win;
                    sum_train_cross_entropy +=
                        elmo_lambda * cross_entropy_eval + (1.0 - elmo_lambda) * cross_entropy_win;
                    double entropy_eval =
                        -p * std::log(p + kEps) - (1.0 - p) * std::log(1.0 - p + kEps);
                    sum_train_entropy_eval += entropy_eval;
                    sum_train_kld_eval += cross_entropy_eval - entropy_eval;
                    sum_norm += abs(value);
                    sum_train_squared_error_of_win_or_lose += (q - t) * (q - t);

                    // 先手から見た評価値の差分。sum.p[?][0]に足したり引いたりする。
                    WeightType delta_color = (root_color == BLACK ? delta : -delta);
                    // 手番から見た評価値の差分。sum.p[?][1]に足したり引いたりする。
                    WeightType delta_turn = (root_color == pos.side_to_move() ? delta : -delta);

                    // 値を更新する
                    Square sq_bk = pos.king_square(BLACK);
                    Square sq_wk = pos.king_square(WHITE);
                    const auto& list0 = pos.eval_list()->piece_list_fb();
                    const auto& list1 = pos.eval_list()->piece_list_fw();

                    // 勾配の値を加算する

                    // KK
                    weights[Kk(sq_bk, sq_wk, WEIGHT_KIND_COLOR).ToIndex()].AddRawGradient(
                        delta_color);
                    weights[Kk(sq_bk, sq_wk, WEIGHT_KIND_TURN).ToIndex()].AddRawGradient(
                        delta_turn);

                    for (int i = 0; i < PIECE_NO_KING; ++i) {
                        Eval::BonaPiece k0 = list0[i];
                        Eval::BonaPiece k1 = list1[i];
                        for (int j = 0; j < i; ++j) {
                            Eval::BonaPiece l0 = list0[j];
                            Eval::BonaPiece l1 = list1[j];

                            // 常にp0 < p1となるようにアクセスする

                            // KPP
                            Eval::BonaPiece p0b = std::min(k0, l0);
                            Eval::BonaPiece p1b = std::max(k0, l0);
                            weights[Kpp(sq_bk, p0b, p1b, WEIGHT_KIND_COLOR).ToIndex()]
                                .AddRawGradient(delta_color);
                            weights[Kpp(sq_bk, p0b, p1b, WEIGHT_KIND_TURN).ToIndex()]
                                .AddRawGradient(delta_turn);

                            // KPP
                            Eval::BonaPiece p0w = std::min(k1, l1);
                            Eval::BonaPiece p1w = std::max(k1, l1);
                            weights[Kpp(Inv(sq_wk), p0w, p1w, WEIGHT_KIND_COLOR).ToIndex()]
                                .AddRawGradient(-delta_color);
                            weights[Kpp(Inv(sq_wk), p0w, p1w, WEIGHT_KIND_TURN).ToIndex()]
                                .AddRawGradient(delta_turn);
                        }

                        // KKP
                        weights[Kkp(sq_bk, sq_wk, k0, WEIGHT_KIND_COLOR).ToIndex()].AddRawGradient(
                            delta_color);
                        weights[Kkp(sq_bk, sq_wk, k0, WEIGHT_KIND_TURN).ToIndex()].AddRawGradient(
                            delta_turn);
                    }
                };
                Strap(records[record_index], elmo_lambda, use_progress_as_elmo_lambda, progress, f);
            }

// 損失関数を計算する
#pragma omp for schedule(dynamic, 1000) reduction(+ : sum_test_squared_error_of_value) reduction( \
    + : sum_test_squared_error_of_winning_percentage)                                             \
        reduction(+ : sum_test_cross_entropy) reduction(                                          \
            + : sum_test_cross_entropy_eval) reduction(+ : sum_test_cross_entropy_win) reduction( \
                + : sum_test_squared_error_of_win_or_lose)                                        \
                    reduction(+ : sum_test_entropy_eval) reduction(+ : sum_test_kld_eval)
            for (int record_index = 0; record_index < num_records; ++record_index) {
                auto f = [elmo_lambda, value_to_winning_rate_coefficient, &weights,
                          &sum_test_squared_error_of_value,
                          &sum_test_squared_error_of_winning_percentage, &sum_test_cross_entropy,
                          &sum_test_cross_entropy_eval, &sum_test_cross_entropy_win,
                          &sum_test_squared_error_of_win_or_lose, &sum_test_entropy_eval,
                          &sum_test_kld_eval](Value record_value, Color win_color, Value value,
                                              Color root_color, double elmo_lambda, Position& pos) {
                    // 評価値から推定した勝率の分布の交差エントロピー
                    double p = winning_rate(record_value, value_to_winning_rate_coefficient);
                    double q = winning_rate(value, value_to_winning_rate_coefficient);
                    double t = (root_color == win_color) ? 1.0 : 0.0;

                    double diff_value = record_value - value;
                    sum_test_squared_error_of_value += diff_value * diff_value;
                    double diff_winning_percentage = q - p;
                    sum_test_squared_error_of_winning_percentage +=
                        diff_winning_percentage * diff_winning_percentage;
                    double cross_entropy_eval =
                        -p * std::log(q + kEps) - (1.0 - p) * std::log(1.0 - q + kEps);
                    double cross_entropy_win =
                        -t * std::log(q + kEps) - (1.0 - t) * std::log(1.0 - q + kEps);
                    sum_test_cross_entropy_eval += cross_entropy_eval;
                    sum_test_cross_entropy_win += cross_entropy_win;
                    sum_test_cross_entropy +=
                        elmo_lambda * cross_entropy_eval + (1.0 - elmo_lambda) * cross_entropy_win;
                    double entropy_eval =
                        -p * std::log(p + kEps) - (1.0 - p) * std::log(1.0 - p + kEps);
                    sum_test_entropy_eval += entropy_eval;
                    sum_test_kld_eval += cross_entropy_eval - entropy_eval;
                    sum_test_squared_error_of_win_or_lose += (q - t) * (q - t);
                };
                Strap(records_for_test[record_index], elmo_lambda, use_progress_as_elmo_lambda,
                      progress, f);
            }

// 低次元へ分配する
// 並列化を効かせたいのでdimension_indexで回す
#pragma omp for schedule(dynamic, 20000)
            for (int dimension_index = 0; dimension_index < vector_length; ++dimension_index) {
                if (Kpp::IsValid(dimension_index)) {
                    auto& from = weights[dimension_index];
                    Kpp kpp[4];
                    Kpp::ForIndex(dimension_index).ToLowerDimensions(kpp);
                    for (const auto& to : kpp) {
                        weights[to.ToIndex()].AddLowerDimensionGradient(from.sum_gradient_raw);
                    }
                } else if (Kkp::IsValid(dimension_index)) {
                    auto& from = weights[dimension_index];
                    Kkp kkp[4];
                    Kkp::ForIndex(dimension_index).ToLowerDimensions(kkp);
                    for (const auto& to : kkp) {
                        weights[to.ToIndex()].AddLowerDimensionGradient(
                            !to.invert_sign() ? from.sum_gradient_raw : -from.sum_gradient_raw);
                    }
                } else if (Kk::IsValid(dimension_index)) {
                    auto& from = weights[dimension_index];
                    Kk kk[4];
                    Kk::ForIndex(dimension_index).ToLowerDimensions(kk);
                    for (const auto& to : kk) {
                        weights[to.ToIndex()].AddLowerDimensionGradient(
                            !to.invert_sign() ? from.sum_gradient_raw : -from.sum_gradient_raw);
                    }
                } else {
                    ASSERT_LV3(false);
                }
            }

            // 重みを更新する
            double adam_beta1_t = std::pow(kAdamBeta1, num_mini_batches);
            double adam_beta2_t = std::pow(adam_beta2, num_mini_batches);

            // Cyclical Learning Rates for Training Neural Networks
            // https://arxiv.org/abs/1506.01186
            double learning_rate_scale = static_cast<double>(num_processed_positions) *
                                         num_learning_rate_cycles /
                                         static_cast<double>(max_positions_for_learning);
            double learning_rate_scale_raw = learning_rate_scale;
            learning_rate_scale = fmod(learning_rate_scale, 1.0);
            learning_rate_scale *= 2.0;
            if (learning_rate_scale > 1.0) {
                learning_rate_scale = 2.0 - learning_rate_scale;
            }
            double learning_rate =
                (max_learning_rate - min_learning_rate) * learning_rate_scale + min_learning_rate;

// 並列化を効かせたいのでdimension_indexで回す
#pragma omp for schedule(dynamic, 20000)
            for (int dimension_index = 0; dimension_index < vector_length; ++dimension_index) {
                if (Kpp::IsValid(dimension_index)) {
                    Kpp kpp = Kpp::ForIndex(dimension_index);
                    weights[dimension_index].UpdateWeight(
                        adam_beta1_t, adam_beta2, adam_beta2_t, learning_rate, fobos_l1_parameter,
                        fobos_l2_parameter,
                        Eval::kpp[kpp.king()][kpp.piece0()][kpp.piece1()][kpp.weight_kind()]);
                } else if (Kkp::IsValid(dimension_index)) {
                    Kkp kkp = Kkp::ForIndex(dimension_index);
                    weights[dimension_index].UpdateWeight(
                        adam_beta1_t, adam_beta2, adam_beta2_t, learning_rate, fobos_l1_parameter,
                        fobos_l2_parameter,
                        Eval::kkp[kkp.king0()][kkp.king1()][kkp.piece()][kkp.weight_kind()]);
                } else if (Kk::IsValid(dimension_index)) {
                    Kk kk = Kk::ForIndex(dimension_index);
                    weights[dimension_index].UpdateWeight(
                        adam_beta1_t, adam_beta2, adam_beta2_t, learning_rate, fobos_l1_parameter,
                        fobos_l2_parameter, Eval::kk[kk.king0()][kk.king1()][kk.weight_kind()]);
                } else {
                    ASSERT_LV3(false);
                }
            }
        }

        // 損失関数の値を出力する
        fprintf(file_loss, "%I64d,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n",
                num_processed_positions, std::sqrt(sum_train_squared_error_of_value / num_records),
                std::sqrt(sum_train_squared_error_of_winning_percentage / num_records),
                sum_train_cross_entropy / num_records, sum_train_cross_entropy_eval / num_records,
                sum_train_cross_entropy_win / num_records,
                std::sqrt(sum_test_squared_error_of_value / num_records),
                std::sqrt(sum_test_squared_error_of_winning_percentage / num_records),
                sum_test_cross_entropy / num_records, sum_test_cross_entropy_eval / num_records,
                sum_test_cross_entropy_win / num_records, sum_norm / num_records,
                std::sqrt(sum_train_squared_error_of_win_or_lose / num_records),
                std::sqrt(sum_test_squared_error_of_win_or_lose / num_records),
                sum_train_entropy_eval / num_records, sum_train_kld_eval / num_records,
                sum_test_entropy_eval / num_records, sum_test_kld_eval / num_records);
        std::fflush(file_loss);

        num_processed_positions += num_records;
    }

    // 評価関数ファイルの書き出し
    save_eval(output_folder_path_base, max_positions_for_learning);

    sync_cout << "Finished..." << sync_endl;
}

void Learner::CalculateTestDataEntropy(std::istringstream& iss) {
    sync_cout << __FUNCTION__ << sync_endl;

    std::string kifu_for_test_dir = Options[kOptionValueKifuForTestDir];
    int64_t num_positions_for_test =
        Options[kOptionValueLearnerNumPositionsForTest].cast<int64_t>();

    sync_cout << "kifu_for_test_dir=" << kifu_for_test_dir << sync_endl;
    sync_cout << "num_positions_for_test=" << num_positions_for_test << sync_endl;

    auto kifu_reader_for_test = std::make_unique<Learner::KifuReader>(kifu_for_test_dir, false);
    std::vector<Record> records_for_test;
    if (!kifu_reader_for_test->Read(num_positions_for_test, records_for_test)) {
        sync_cout << "Failed to read kifu for test." << sync_endl;
        std::exit(1);
    }

    std::ofstream ofs("eval.csv");

    double eval_entropy = 0.0;
    double eval_winlose_cross_entropy = 0.0;
    Position& pos = Threads[0]->rootPos;
    for (const auto& record : records_for_test) {
        pos.set_from_packed_sfen(record.packed);
        double p = winning_rate(static_cast<Value>(record.value), 600.0);
        double t = (pos.side_to_move() == record.win_color) ? 1.0 : 0.0;
        eval_entropy += -p * std::log(p + kEps) - (1.0 - p) * std::log(1.0 - p + kEps);
        eval_winlose_cross_entropy +=
            -t * std::log(p + kEps) - (1.0 - t) * std::log(1.0 - p + kEps);
        ofs << record.value << std::endl;
    }

    sync_cout << "eval_entropy=" << eval_entropy / records_for_test.size() << sync_endl;
    sync_cout << "eval_winlose_cross_entropy="
              << eval_winlose_cross_entropy / records_for_test.size() << sync_endl;
}

namespace {
template <typename WeightType>
void Blend(WeightType base, WeightType another, WeightType& output, int& num_zero_base,
           int& num_zero_another, int& num_zero_both) {
    if (base == 0) {
        ++num_zero_base;
    }
    if (another == 0) {
        ++num_zero_another;
    }
    if (base == 0 && another == 0) {
        ++num_zero_both;
    }

    if (base != 0) {
        output = base;
    } else {
        output = another;
    }

    // double ratio = std::pow(2.0, -std::abs(base));
    // output = static_cast<WeightType>(std::round((1.0 - ratio) * base + ratio * another));

    // output = (base + another) / 2;
}
}

void Learner::BreedEval(std::istringstream& iss) {
    sync_cout << __FUNCTION__ << sync_endl;

    std::string base_folder_path = Options[kOptionValueBreedEvalBaseFolderPath];
    std::string another_folder_path = Options[kOptionValueBreedEvalAnotherFolderPath];
    std::string output_folder_path = Options[kOptionValueBreedEvalOutputFolderPath];

    std::unique_ptr<Weights> base_weights = std::make_unique<Weights>();
    if (!base_weights->load(base_folder_path)) {
        std::exit(-1);
    }

    std::unique_ptr<Weights> another_weights = std::make_unique<Weights>();
    if (!another_weights->load(another_folder_path)) {
        std::exit(-1);
    }

    std::unique_ptr<Weights> output_weights = std::make_unique<Weights>();

    int num_zero_base = 0;
    int num_zero_another = 0;
    int num_zero_both = 0;

    int vector_length = Kk::MaxIndex();
    for (int dimension_index = 0; dimension_index < vector_length; ++dimension_index) {
        if (dimension_index % 1000000 == 0) {
            sync_cout << dimension_index << " / " << vector_length << sync_endl;
        }

        if (Kpp::IsValid(dimension_index)) {
            Kpp kpp = Kpp::ForIndex(dimension_index);
            for (int i = 0; i < 2; ++i) {
                auto base_weight = base_weights->kpp[kpp.king()][kpp.piece0()][kpp.piece1()][i];
                auto another_weight =
                    another_weights->kpp[kpp.king()][kpp.piece0()][kpp.piece1()][i];
                auto& output_weight =
                    output_weights->kpp[kpp.king()][kpp.piece0()][kpp.piece1()][i];
                if (kpp.piece0() == BonaPiece::BONA_PIECE_ZERO ||
                    kpp.piece1() == BonaPiece::BONA_PIECE_ZERO) {
                    output_weight = 0;
                    continue;
                }
                Blend(base_weight, another_weight, output_weight, num_zero_base, num_zero_another,
                      num_zero_both);
            }
        } else if (Kkp::IsValid(dimension_index)) {
            Kkp kkp = Kkp::ForIndex(dimension_index);
            for (int i = 0; i < 2; ++i) {
                auto base_weight = base_weights->kkp[kkp.king0()][kkp.king1()][kkp.piece()][i];
                auto another_weight =
                    another_weights->kkp[kkp.king0()][kkp.king1()][kkp.piece()][i];
                auto& output_weight = output_weights->kkp[kkp.king0()][kkp.king1()][kkp.piece()][i];
                if (kkp.piece() == BonaPiece::BONA_PIECE_ZERO) {
                    output_weight = 0;
                    continue;
                }
                Blend(base_weight, another_weight, output_weight, num_zero_base, num_zero_another,
                      num_zero_both);
            }
        } else if (Kk::IsValid(dimension_index)) {
            Kk kk = Kk::ForIndex(dimension_index);
            for (int i = 0; i < 2; ++i) {
                auto base_weight = base_weights->kk[kk.king0()][kk.king1()][i];
                auto another_weight = another_weights->kk[kk.king0()][kk.king1()][i];
                auto& output_weight = output_weights->kk[kk.king0()][kk.king1()][i];
                Blend(base_weight, another_weight, output_weight, num_zero_base, num_zero_another,
                      num_zero_both);
            }
        } else {
            sync_cout << "Invalid dimension index." << sync_endl;
            std::exit(-1);
        }
    }

    output_weights->save(output_folder_path);

    sync_cout << "Finished..." << sync_endl;
    sync_cout << "num_zero_base   =" << num_zero_base << sync_endl;
    sync_cout << "num_zero_another=" << num_zero_another << sync_endl;
    sync_cout << "num_zero_both   =" << num_zero_both << sync_endl;
}

#endif  // USE_EXPERIMENTAL_LEARNER
