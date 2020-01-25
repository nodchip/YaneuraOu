#ifndef _TANUKI_LAZY_CLUNTER_H_
#define _TANUKI_LAZY_CLUNTER_H_

#include "config.h"

#ifdef EVAL_LEARN

#include "usi.h"

class Thread;

namespace Tanuki {
	namespace LazyCluster {
		constexpr const char* kEnableLazyCluster = "EnableLazyCluster";
		constexpr const char* kLazyClusterSendIntervalMs = "LazyClusterSendIntervalMs";
		constexpr const char* kLazyClusterDontSendFirstMs = "LazyClusterDontSendFirstMs";
		constexpr const char* kLazyClusterSendTo = "LazyClusterSendTo";
		constexpr const char* kLazyClusterRecievePort = "LazyClusterRecievePort";

		// Lazy Cluster上で送受信される通信パケット
		struct Packet {
			uint64_t key;
			uint16_t move;
			int16_t value;
			int16_t eval;
			uint8_t is_pv : 1;
			uint8_t bound : 2;
			uint8_t depth;
		};
		static_assert(sizeof(Packet) == 16);

		void InitializeLazyCluster(USI::OptionsMap& o);

		void Start();
		void Stop();
		void Send(Thread& thread);
	}
}

#endif

#endif
