#ifndef _KIFU_READER_H_
#define _KIFU_READER_H_

#include <fstream>
#include <mutex>
#include <string>
#include <vector>

#include "experimental_learner.h"

namespace Learner {

	class KifuReader {
	public:
		KifuReader(const std::string& folder_name, int num_loops);
		virtual ~KifuReader();
		bool Read(Record& record);
		bool Read(int num_records, std::vector<Record>& records);
		bool Close();

	private:
		bool EnsureOpen();

		const std::string folder_name_;
		const int num_loops_;
		std::vector<std::string> file_paths_;
		FILE* file_;
		int loop_ = 0;
		int file_index_ = 0;
	};

}

#endif
