#include "kifu_shuffler.h"

#include <direct.h>
#include <cstdio>
#include <random>

#include "experimental_learner.h"
#include "kifu_reader.h"
#include "kifu_writer.h"
#include "misc.h"

namespace {
static const constexpr char* kShuffledKifuDir = "ShuffledKifuDir";
// �V���b�t����̃t�@�C����
// Windows�ł͈�x��512�܂ł̃t�@�C�������J���Ȃ�����
// 256�ɐ������Ă���
static const constexpr int kNumShuffledKifuFiles = 256;
static const constexpr int kMaxPackedSfenValues = 1024 * 1024;
static const constexpr char* kOptoinNameUseDiscount = "UseDiscount";
static const constexpr char* kOptoinNameUseWinningRateForDiscount = "UseWinningRateForDiscount";
static const constexpr char* kOptoinNameDiscountRatio = "DiscountRatio";
static const constexpr char* kOptoinNameOverwriteGameResults = "OverwriteGameResults";

double ToScaledScore(double raw_score, bool use_winning_rate_for_discount,
                     double value_to_winning_rate_coefficient) {
    if (use_winning_rate_for_discount) {
        return Learner::ToWinningRate(raw_score, value_to_winning_rate_coefficient);
        ;
    } else {
        return raw_score;
    }
}

double ToRawScore(double scaled_score, bool use_winning_rate_for_discount,
                  double value_to_winning_rate_coefficient) {
    if (use_winning_rate_for_discount) {
        return Learner::ToValue(scaled_score, value_to_winning_rate_coefficient);
    } else {
        return scaled_score;
    }
}
}  // namespace

void Learner::InitializeKifuShuffler(USI::OptionsMap& o) {
    o[kShuffledKifuDir] << USI::Option("kifu_shuffled");
    o[kOptoinNameUseDiscount] << USI::Option(false);
    o[kOptoinNameUseWinningRateForDiscount] << USI::Option(false);
    o[kOptoinNameOverwriteGameResults] << USI::Option(true);
    o[kOptoinNameDiscountRatio] << USI::Option("0.9");
}

void Learner::ShuffleKifu() {
    // ��������͂��A�����̃t�@�C���Ƀ����_���ɒǉ����Ă���
    sync_cout << "info string Reading and dividing kifu files..." << sync_endl;

    std::string kifu_dir = Options["KifuDir"];
    std::string shuffled_kifu_dir = Options[kShuffledKifuDir];
    bool use_discount = (bool)Options[kOptoinNameUseDiscount];
    bool use_winning_rate_for_discount = (bool)Options[kOptoinNameUseWinningRateForDiscount];
    bool overwrite_game_results = (bool)Options[kOptoinNameOverwriteGameResults];
    double value_to_winning_rate_coefficient =
        Options[kOptionValueValueToWinningRateCoefficient].cast<double>();
    double discount_ratio = Options[kOptoinNameDiscountRatio].cast<double>();

    auto reader = std::make_unique<KifuReader>(kifu_dir, 1);
    _mkdir(shuffled_kifu_dir.c_str());

    std::vector<std::string> file_paths;
    for (int file_index = 0; file_index < kNumShuffledKifuFiles; ++file_index) {
        char file_path[_MAX_PATH];
        sprintf(file_path, "%s/shuffled.%03d.bin", shuffled_kifu_dir.c_str(), file_index);
        file_paths.push_back(file_path);
    }

    std::vector<std::shared_ptr<KifuWriter> > writers;
    for (const auto& file_path : file_paths) {
        writers.push_back(std::make_shared<KifuWriter>(file_path));
    }

    double weights[4096];
    weights[0] = 1.0;
    for (int i = 1; i < sizeof(weights) / sizeof(weights[0]); ++i) {
        weights[i] = weights[i - 1] * discount_ratio;
    }

    std::mt19937_64 mt(std::time(nullptr));
    std::uniform_int_distribution<> dist(0, kNumShuffledKifuFiles - 1);
    int64_t num_records = 0;
    for (;;) {
        std::vector<PackedSfenValue> records;
        PackedSfenValue record;
        while (reader->Read(record)) {
            records.push_back(record);
            if (record.last_position || static_cast<int>(records.size()) > kMaxPackedSfenValues) {
                break;
            }
        }

        if (records.empty()) {
            break;
        }

        if (overwrite_game_results) {
            int game_result = GameResultDraw;
            if (records.back().score > VALUE_ZERO) {
                game_result = GameResultWin;
            } else {
                game_result = GameResultLose;
            }

            for (auto rit = records.rbegin(); rit != records.rend(); ++rit) {
                rit->game_result = game_result;
                game_result = -game_result;
            }
        }

        if (use_discount) {
            // DeepStack: Expert-Level Artificial Intelligence in No-Limit Poker
            // https://arxiv.org/abs/1701.01724
            // discount��p�����d�ݕt���]���l���v�Z���A���̕]���l���㏑������
            // ��납��v�Z����Ɛ��`���ԂŌv�Z�ł���

            // �e�ǖʂ̕]���l�ɏd�݂�����������
            // use_winning_rate_for_discount �̏ꍇ�ɂ͏����ɕϊ������l������
            double weighted_scores[4096];
            int num_records = records.size();
            // ��Ԃ��猩���]���l���J�n��Ԃ��猩���]���l�ɕϊ����邽�߂̕���
            double sign = 1.0;
            for (int i = 0; i < num_records; ++i) {
                weighted_scores[i] =
                    ToScaledScore(sign * records[i].score, use_winning_rate_for_discount,
                                  value_to_winning_rate_coefficient) *
                    weights[i];
                sign = -sign;
            }

            double sum_weighted_scores = 0.0;
            double sum_weights = 0.0;
            // ��x���]����ƍŌ�̋ǖʂ̕����ƈ�v����
            sign = -sign;
            // ��납�瑫���Ă����Ɛ��`���ԂŌv�Z�ł���
            for (int i = static_cast<int>(records.size() - 1); i >= 0; --i) {
                sum_weighted_scores += weighted_scores[i];
                sum_weights += weights[i];
                double scaled_score = sum_weighted_scores / sum_weights;
                double raw_score = ToRawScore(scaled_score, use_winning_rate_for_discount,
                                              value_to_winning_rate_coefficient);
                // std::cout << records[i].score << " -> " << sign * raw_score << std::endl;
                records[i].score = static_cast<Value>(static_cast<int>(sign * raw_score));
                sign = -sign;
            }
        }

        for (const auto& record : records) {
            if (!writers[dist(mt)]->Write(record)) {
                sync_cout << "info string Failed to write a record to a kifu file. " << sync_endl;
            }
            ++num_records;
            if (num_records % 10000000 == 0) {
                sync_cout << "info string " << num_records << sync_endl;
            }
        }
    }
    for (auto& writer : writers) {
        writer->Close();
    }

    // �e�t�@�C�����V���b�t������
    for (const auto& file_path : file_paths) {
        sync_cout << "info string " << file_path << sync_endl;

        // �t�@�C���S�̂�ǂݍ���
        FILE* file = std::fopen(file_path.c_str(), "rb");
        if (file == nullptr) {
            sync_cout << "info string Failed to open a kifu file. " << file_path << sync_endl;
            return;
        }
        _fseeki64(file, 0, SEEK_END);
        int64_t size = _ftelli64(file);
        _fseeki64(file, 0, SEEK_SET);
        std::vector<PackedSfenValue> records(size / sizeof(PackedSfenValue));
        std::fread(&records[0], sizeof(PackedSfenValue), size / sizeof(PackedSfenValue), file);
        std::fclose(file);
        file = nullptr;

        // �����S�̂��V���b�t������
        std::shuffle(records.begin(), records.end(), mt);

        // �����S�̂��㏑�����ď����߂�
        file = std::fopen(file_path.c_str(), "wb");
        if (file == nullptr) {
            sync_cout << "info string Failed to open a kifu file. " << file_path << sync_endl;
        }
        if (std::fwrite(&records[0], sizeof(PackedSfenValue), records.size(), file) !=
            records.size()) {
            sync_cout << "info string Failed to write records to a kifu file. " << file_path
                      << sync_endl;
            return;
        }
        std::fclose(file);
        file = nullptr;
    }
}
