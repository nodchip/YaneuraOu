#ifndef _KIFU_WRITER_H_
#define _KIFU_WRITER_H_

#include "experimental_learner.h"
#include "position.h"
#include "shogi.h"

namespace Learner {

	class KifuWriter {
	public:
		KifuWriter(const std::string& output_file_path);
		virtual ~KifuWriter();
		bool Write(const Record& record);
		bool Close();

	private:
		const std::string output_file_path_;
		FILE* file_ = nullptr;

		bool EnsureOpen();
	};

}

#endif
