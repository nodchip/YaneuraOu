#include "kifu_shuffler.h"

#include <cstdio>
#include <direct.h>

#include "experimental_learner.h"
#include "kifu_reader.h"
#include "kifu_writer.h"
#include "misc.h"

namespace {
	static const constexpr char* kShuffledKifuDir = "ShuffledKifuDir";
	static const constexpr int kNumShuffledKifuFiles = 256;
}

void Learner::InitializeKifuShuffler(USI::OptionsMap& o) {
	o[kShuffledKifuDir] << USI::Option("kifu_shuffled");
}

void Learner::ShuffleKifu() {
	// 棋譜を入力し、複数のファイルにランダムに追加していく
	sync_cout << "info string Reading and dividing kifu files..." << sync_endl;

	std::string kifu_dir = Options["KifuDir"];
	auto reader = std::make_unique<KifuReader>(kifu_dir, 1);

	std::string shuffled_kifu_dir = Options[kShuffledKifuDir];
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
	Record record;
	while (reader->Read(record)) {
		if (!writers[dist(mt)]->Write(record)) {
			sync_cout << "info string Failed to write a record to a kifu file. " << sync_endl;
		}
		++num_records;
		if (num_records % 10000000 == 0) {
			sync_cout << "info string " << num_records << sync_endl;
		}
	}
	for (auto& writer : writers) {
		writer->Close();
	}

	// 各ファイルをシャッフルする
	for (const auto& file_path : file_paths) {
		sync_cout << "info string " << file_path << sync_endl;

		FILE* file = std::fopen(file_path.c_str(), "rb");
		if (file == nullptr) {
			sync_cout << "info string Failed to open a kifu file. " << file_path << sync_endl;
			return;
		}
		_fseeki64(file, 0, SEEK_END);
		int64_t size = _ftelli64(file);
		_fseeki64(file, 0, SEEK_SET);
		std::vector<Record> records(size / sizeof(Record));
		std::fread(&records[0], sizeof(Record), size / sizeof(Record), file);
		std::fclose(file);
		file = nullptr;

		std::shuffle(records.begin(), records.end(), mt);

		file = std::fopen(file_path.c_str(), "wb");
		if (file == nullptr) {
			sync_cout << "info string Failed to open a kifu file. " << file_path << sync_endl;
		}
		if (std::fwrite(&records[0], sizeof(Record), records.size(), file) != records.size()) {
			sync_cout << "info string Failed to write records to a kifu file. " << file_path <<
				sync_endl;
			return;
		}
		std::fclose(file);
		file = nullptr;
	}
}
