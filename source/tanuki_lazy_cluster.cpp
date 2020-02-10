//#pragma optimize( "", off )

#include "tanuki_lazy_cluster.h"
#include "config.h"

#ifdef EVAL_LEARN

#include <boost/array.hpp>
#include <boost/asio.hpp>

#include "misc.h"
#include "thread.h"
#include "tt.h"
#include "usi.h"

// ARM上でBoost::shared_ptrがリンクエラー - PEEE802.11 http://peee.hatenablog.com/entry/2014/03/06/020623
namespace boost {
	void throw_exception(std::exception const& e) { }
}

namespace {
	// 一度に送るパケットの数の最大値。
	// UDPでは一度に送信するデータは最大でも500〜1300倍と程度にするのが一般的らしい。
	// C# - C# UdpClient 分割送信方法｜teratail https://teratail.com/questions/32868
	// 16バイト * 80 = 1280バイト
	constexpr const int kMaxNumPacketsToSend = 80;

	static std::shared_ptr<boost::asio::io_context> IO_CONTEXT;
	static std::shared_ptr<boost::asio::ip::udp::socket> UDP_SOCKET;
	static std::vector<boost::asio::ip::udp::endpoint> ENDPOINTS;
	static std::thread SERVER_THREAD;
	static std::atomic<bool> SERVER_RUNNING;

	bool Serialize(Key key, Tanuki::LazyCluster::Packet& packet) {
		bool found = false;
		const auto* entry = TT.probe(key, found);
		if (!found) {
			return false;
		}

		packet.key = static_cast<uint16_t>(key);
		packet.move = static_cast<uint16_t>(entry->move());
		packet.value = static_cast<int16_t>(entry->value());
		packet.eval = static_cast<int16_t>(entry->eval());
		packet.is_pv = static_cast<uint16_t>(entry->is_pv());
		packet.bound = static_cast<uint16_t>(entry->bound());
		packet.depth = static_cast<uint16_t>(entry->depth());
		return true;
	}

	void Deserialize(const Tanuki::LazyCluster::Packet& packet) {
		bool found = false;
		auto* entry = TT.probe(packet.key, found);

		Key k = static_cast<Key>(packet.key);
		Value v = static_cast<Value>(packet.value);
		bool pv = static_cast<bool>(packet.is_pv);
		Bound b = static_cast<Bound>(packet.bound);
		Depth d = static_cast<Depth>(packet.depth);
		Move m = static_cast<Move>(packet.move);
		Value ev = static_cast<Value>(packet.eval);
		entry->save(k, v, pv, b, d, m, ev);
	}

	void ServerProcedure() {
		using boost::asio::ip::udp;

		while (SERVER_RUNNING) {
			boost::array<Tanuki::LazyCluster::Packet, kMaxNumPacketsToSend> recv_buffer = {};
			boost::system::error_code error_code;
			size_t bytes = UDP_SOCKET->receive(boost::asio::buffer(recv_buffer), 0, error_code);
			if (error_code.failed()) {
				sync_cout << "info string Falied to receive packets. error_code.message()=" << error_code.message() << sync_endl;
				continue;
			}

			int num_packets = bytes / sizeof(Tanuki::LazyCluster::Packet);
			//sync_cout << "info string Lazy Cluster server: Received " << num_packets << " packets." << sync_endl;
			for (int packet_index = 0; packet_index < num_packets; ++packet_index) {
				Deserialize(recv_buffer[packet_index]);
			}
		}
	}
}

void Tanuki::LazyCluster::InitializeLazyCluster(USI::OptionsMap& o) {
	o[kEnableLazyCluster] << USI::Option(false);
	o[kLazyClusterSendIntervalMs] << USI::Option(0, 0, 1000);
	o[kLazyClusterDontSendFirstMs] << USI::Option(0, 0, 1000);
	o[kLazyClusterSendTo] << USI::Option("");
	o[kLazyClusterRecievePort] << USI::Option(30001, 0, 65535);
}

void Tanuki::LazyCluster::Start() {
	using boost::asio::ip::udp;

	if (!static_cast<bool>(Options[kEnableLazyCluster])) {
		return;
	}

	// 送信の準備を行う。
	// LazyClusterSendToオプションを解析し、アドレスとポートをENDPOINTSに格納する。
	ENDPOINTS.clear();

	IO_CONTEXT = std::make_shared< boost::asio::io_context>();

	int port = static_cast<int>(Options[Tanuki::LazyCluster::kLazyClusterRecievePort]);
	UDP_SOCKET = std::make_shared<udp::socket>(*IO_CONTEXT, udp::endpoint(udp::v4(), port));
	udp::resolver resolver(*IO_CONTEXT);

	// 送信先のアドレスに分割する
	std::string addresses = static_cast<std::string>(Options[kLazyClusterSendTo]);
	std::istringstream addresses_iss(addresses);
	std::string address_and_port;
	while (std::getline(addresses_iss, address_and_port, ',')) {
		// アドレスとポートに分割する
		std::istringstream address_and_port_iss(address_and_port);
		std::string address;
		std::getline(address_and_port_iss, address, ':');
		ASSERT_LV3(!address.empty());
		std::string port;
		std::getline(address_and_port_iss, port);
		ASSERT_LV3(!port.empty());

		sync_cout << "info string addresss=" << address << " port=" << port << sync_endl;

		udp::endpoint receiver_endpoint =
			*resolver.resolve(udp::v4(), address, port).begin();
		ENDPOINTS.push_back(receiver_endpoint);
	}

	SERVER_RUNNING = true;
	SERVER_THREAD = std::thread(ServerProcedure);
}

void Tanuki::LazyCluster::Stop() {
	using boost::asio::ip::udp;

	if (!static_cast<bool>(Options[kEnableLazyCluster])) {
		return;
	}

	SERVER_RUNNING = false;

	// サーバーに空の通信パケットを送り、ループを抜けさせる。
	int port = Options[kLazyClusterRecievePort];
	boost::asio::io_context io_context;
	udp::resolver resolver(io_context);
	udp::endpoint receiver_endpoint =
		*resolver.resolve(udp::v4(), "localhost", std::to_string(port)).begin();
	udp::socket socket(io_context);
	socket.open(udp::v4());
	boost::array<Packet, 0> send_buffer = {};
	socket.send_to(boost::asio::buffer(send_buffer), receiver_endpoint);

	SERVER_THREAD.join();
}

void Tanuki::LazyCluster::Send(Thread& thread) {
	if (!static_cast<bool>(Options[kEnableLazyCluster])) {
		return;
	}

	// 通信パケットにシリアライズする。
	int multiPV = static_cast<int>(Options["MultiPV"]);
	Position& position = thread.rootPos;
	std::vector<Packet> packets;

	// 各PVについて処理する
	for (int pv_index = 0; pv_index < multiPV; ++pv_index) {
		StateInfo state_info[MAX_PLY] = {};

		Packet packet = {};
		if (Serialize(position.key(), packet)) {
			packets.push_back(packet);
		}

		const auto& pv = thread.rootMoves[pv_index].pv;
		for (auto move : pv) {
			if (!is_ok(move)) {
				break;
			}

			position.do_move(move, state_info[position.game_ply()]);
			if (Serialize(position.key(), packet)) {
				packets.push_back(packet);
			}
		}
		for (auto rit = pv.rbegin(); rit != pv.rend(); ++rit) {
			position.undo_move(*rit);
		}
	}

	// 通信パケットのサイズをUDPで1回で送りやすいサイズに減らす。
	if (packets.size() > kMaxNumPacketsToSend) {
		packets.resize(kMaxNumPacketsToSend);
	}

	// 送信する。
	for (const auto& receiver_endpoint : ENDPOINTS) {
		boost::system::error_code error_code;
		UDP_SOCKET->send_to(boost::asio::buffer(packets), receiver_endpoint, 0, error_code);
		if (error_code.failed()) {
			sync_cout << "info string Falied to send packets. error_code.message()=" << error_code.message() << sync_endl;
			continue;
		}
	}

	//sync_cout << "info string Lazy Cluster client: Sent " << packets.size() << " packets." << sync_endl;
}

#endif
