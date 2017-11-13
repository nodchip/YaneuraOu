#include "kifu_reader.h"

#include <numeric>
#include <sstream>
#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include "misc.h"
#include "shogi.h"

using USI::Option;
using USI::OptionsMap;

namespace {
	constexpr int kBufferSize = 1 << 26; // 64MB
}

Learner::KifuReader::KifuReader(const std::string& folder_name, int num_loops)
	: folder_name_(folder_name), num_loops_(num_loops) {
}

Learner::KifuReader::~KifuReader() {
	Close();
}

bool Learner::KifuReader::Read(int num_records, std::vector<Record>& records) {
	records.resize(num_records);
	for (auto& record : records) {
		if (!Read(record)) {
			return false;
		}
	}
	return true;
}

bool Learner::KifuReader::Read(Record& record) {
	// ファイルリストを取得し、ファイルを開いた状態にする
	if (!EnsureOpen()) {
		return false;
	}

	// ループ終了条件は以下の通りとする
	if (file_index_ == static_cast<int>(file_paths_.size()) && loop_ == num_loops_) {
		return false;
	}

	// 1局面読み込む
	if (std::fread(&record, sizeof(record), 1, file_) == 1) {
		return true;
	}

	++file_index_;
	if (file_index_ == static_cast<int>(file_paths_.size())) {
		++loop_;
		if (loop_ == num_loops_) {
			return false;
		}

		file_index_ = 0;
	}

	if (std::fclose(file_)) {
		sync_cout << "info string Failed to close a kifu file." << sync_endl;
		return false;
	}

	file_ = std::fopen(file_paths_[file_index_].c_str(), "rb");
	if (file_ == nullptr) {
		// ファイルを開くのに失敗したら
		// 読み込みを終了する
		sync_cout << "into string Failed to open a kifu file: "
			<< file_paths_[file_index_] << sync_endl;
		return false;
	}

	if (std::setvbuf(file_, nullptr, _IOFBF, kBufferSize)) {
		sync_cout << "into string Failed to set a file buffer: "
			<< file_paths_[file_index_] << sync_endl;
	}

	if (std::fread(&record, sizeof(record), 1, file_) != 1) {
		return false;
	}

	return true;
}

bool Learner::KifuReader::Close() {
	if (fclose(file_)) {
		return false;
	}
	file_ = nullptr;
	return true;
}

bool Learner::KifuReader::EnsureOpen() {
	if (!file_paths_.empty()) {
		return true;
	}

	// http://qiita.com/tkymx/items/f9190c16be84d4a48f8a
	HANDLE find = nullptr;
	WIN32_FIND_DATAA find_data = { 0 };

	std::string search_name = folder_name_ + "/*.*";
	find = FindFirstFileA(search_name.c_str(), &find_data);

	if (find == INVALID_HANDLE_VALUE) {
		sync_cout << "Failed to find kifu files." << sync_endl;
		sync_cout << "search_name=" << search_name << sync_endl;
		return false;
	}

	do {
		if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			continue;
		}

		std::string file_path = folder_name_ + "/" + find_data.cFileName;
		file_paths_.push_back(file_path);
	} while (FindNextFileA(find, &find_data));

	FindClose(find);
	find = nullptr;

	if (file_paths_.empty()) {
		return false;
	}

	file_ = std::fopen(file_paths_[0].c_str(), "rb");

	if (!file_) {
		sync_cout << "into string Failed to open a kifu file: " << file_paths_[0]
			<< sync_endl;
		return false;
	}

	if (std::setvbuf(file_, nullptr, _IOFBF, kBufferSize)) {
		sync_cout << "into string Failed to set a file buffer: "
			<< file_paths_[0] << sync_endl;
	}

	return true;
}
