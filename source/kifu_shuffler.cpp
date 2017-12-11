#include "kifu_shuffler.h"

#include <direct.h>
#include <cstdio>
#include <random>

#include "experimental_learner.h"
#include "kifu_reader.h"
#include "kifu_writer.h"
#include "misc.h"

namespace {
static const constexpr double kDiscountRatio = 0.9;
static const constexpr char* kShuffledKifuDir = "ShuffledKifuDir";
// �V���b�t����̃t�@�C����
// Windows�ł͈�x��512�܂ł̃t�@�C�������J���Ȃ�����
// 256�ɐ������Ă���
static const constexpr int kNumShuffledKifuFiles = 256;
static const constexpr char* kOptoinNameUseDiscount = "UseDiscount";
static const constexpr char* kOptoinNameUseWinningRateForDiscount = "UseWinningRateForDiscount";
}

void Learner::InitializeKifuShuffler(USI::OptionsMap& o) {
    o[kShuffledKifuDir] << USI::Option("kifu_shuffled");
    o[kOptoinNameUseDiscount] << USI::Option(false);
    o[kOptoinNameUseWinningRateForDiscount] << USI::Option(false);
}

void Learner::ShuffleKifu() {
    // ��������͂��A�����̃t�@�C���Ƀ����_���ɒǉ����Ă���
    sync_cout << "info string Reading and dividing kifu files..." << sync_endl;

    std::string kifu_dir = Options["KifuDir"];
    std::string shuffled_kifu_dir = Options[kShuffledKifuDir];
    bool use_discount = (bool)Options[kOptoinNameUseDiscount];
    bool use_winning_rate_for_discount = (bool)Options[kOptoinNameUseWinningRateForDiscount];
    double value_to_winning_rate_coefficient =
        Options[kOptionValueValueToWinningRateCoefficient].cast<double>();

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

    std::mt19937_64 mt(std::time(nullptr));
    std::uniform_int_distribution<> dist(0, kNumShuffledKifuFiles - 1);
    int64_t num_records = 0;
    for (;;) {
        std::vector<PackedSfenValue> records;
        PackedSfenValue record;
        while (reader->Read(record)) {
            records.push_back(record);
            if (record.last_position) {
                break;
            }
        }

        if (records.empty()) {
            break;
        }

        if (use_discount) {
            // DeepStack: Expert-Level Artificial Intelligence in No-Limit Poker
            // https://arxiv.org/abs/1701.01724
            // discount��p�����d�ݕt���]���l���v�Z���A���̕]���l���㏑������
            for (int i = 0; i < records.size(); ++i) {
                double sum_weighted_value = 0.0;
                double sum_weights = 0.0;
                double weight = 1.0;
                // ���̕����𔽓]���邽�߂̕ϐ�
                int sign = 1;
                for (int j = i; j < records.size(); ++j) {
                    double value;
                    if (use_winning_rate_for_discount) {
                        // discount�̌v�Z�ɕ]���l���琄�肵��������p����ꍇ
                        value = ToWinningRate(static_cast<Value>(sign * records[j].score),
                                              value_to_winning_rate_coefficient);
                    } else {
                        // discount�̌v�Z�ɕ]���l�����̂܂ܗp����ꍇ
                        value = sign * records[j].score;
                    }
                    sum_weighted_value += value * weight;
                    sum_weights += weight;
                    weight *= kDiscountRatio;
                    sign = -sign;
                }

                Value weighted_value;
                if (use_winning_rate_for_discount) {
                    double winning_rate = sum_weighted_value / sum_weights;
                    weighted_value = ToValue(winning_rate, value_to_winning_rate_coefficient);
                } else {
                    weighted_value =
                        static_cast<Value>(static_cast<int16_t>(sum_weighted_value / sum_weights));
                }
                records[i].score = static_cast<int16_t>(weighted_value);
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
