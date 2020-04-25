//#pragma optimize( "", off )

#include "tanuki_lazy_cluster.h"
#include "config.h"

#ifdef EVAL_LEARN

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/smart_ptr.hpp>

#include "misc.h"
#include "thread.h"
#include "tt.h"
#include "usi.h"

// ARM���Boost::shared_ptr�������N�G���[ - PEEE802.11 http://peee.hatenablog.com/entry/2014/03/06/020623
namespace boost {
	void throw_exception(std::exception const& e) { }
}

namespace {
	// ��x�ɑ���p�P�b�g�̐��̍ő�l�B
	// UDP�ł͈�x�ɑ��M����f�[�^�͍ő�ł�500�`1300�{�ƒ��x�ɂ���̂���ʓI�炵���B
	// C# - C# UdpClient �������M���@�bteratail https://teratail.com/questions/32868
	// 16�o�C�g * 80 = 1280�o�C�g
	constexpr const int kMaxNumPacketsToSend = 80;

	static boost::asio::io_context IO_CONTEXT;
	static std::shared_ptr<boost::asio::ip::udp::socket> UDP_SOCKET;
	static std::vector<boost::asio::ip::udp::endpoint> ENDPOINTS;
	static std::thread IO_CONTEXT_THREAD;
	static std::atomic<bool> SERVER_RUNNING;
	struct Initializer {
		Initializer() {
			SERVER_RUNNING = false;
		}
	} INITIALIZER;
	static boost::array<Tanuki::LazyCluster::Packet, kMaxNumPacketsToSend> RECEIVE_BUFFER;

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

	void OnReceivePacket(const boost::system::error_code& error, std::size_t bytes_transferred) {
		if (!error) {
			int num_packets = bytes_transferred / sizeof(Tanuki::LazyCluster::Packet);
			//sync_cout << "info string Lazy Cluster server: Received " << num_packets << " packets." << sync_endl;
			for (int packet_index = 0; packet_index < num_packets; ++packet_index) {
				Deserialize(RECEIVE_BUFFER[packet_index]);
			}
		}
		else {
			sync_cout << "info string Failed to receive packets. " << error << sync_endl;
			return;
		}

		UDP_SOCKET->async_receive(boost::asio::buffer(RECEIVE_BUFFER), OnReceivePacket);
	}

	void OnSendPacket(std::vector<Tanuki::LazyCluster::Packet> /*packet*/,
		const boost::system::error_code& error, std::size_t bytes_transferred) {
		if (error) {
			sync_cout << "info string Failed to send packets. " << error << sync_endl;
		}
		//sync_cout << "info string Lazy Cluster client: Sent " << bytes_transferred / sizeof(Tanuki::LazyCluster::Packet) << " packets." << sync_endl;
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

	if (SERVER_RUNNING) {
		return;
	}

	if (!static_cast<bool>(Options[kEnableLazyCluster])) {
		return;
	}

	// ���M�̏������s���B
	// LazyClusterSendTo�I�v�V��������͂��A�A�h���X�ƃ|�[�g��ENDPOINTS�Ɋi�[����B
	ENDPOINTS.clear();

	int port = static_cast<int>(Options[Tanuki::LazyCluster::kLazyClusterRecievePort]);
	UDP_SOCKET = std::make_shared<udp::socket>(IO_CONTEXT, udp::endpoint(udp::v4(), port));
	udp::resolver resolver(IO_CONTEXT);

	// ���M��̃A�h���X�ɕ�������
	std::string addresses = static_cast<std::string>(Options[kLazyClusterSendTo]);
	std::istringstream addresses_iss(addresses);
	std::string address_and_port;
	while (std::getline(addresses_iss, address_and_port, ',')) {
		// �A�h���X�ƃ|�[�g�ɕ�������
		std::istringstream address_and_port_iss(address_and_port);
		std::string address;
		std::getline(address_and_port_iss, address, ':');
		ASSERT_LV3(!address.empty());
		std::string port;
		std::getline(address_and_port_iss, port);
		ASSERT_LV3(!port.empty());

		sync_cout << "info string addresss=" << address << sync_endl;
		sync_cout << "info string port=" << port << sync_endl;

		udp::endpoint receiver_endpoint =
			*resolver.resolve(udp::v4(), address, port).begin();
		ENDPOINTS.push_back(receiver_endpoint);
	}

	UDP_SOCKET->async_receive(boost::asio::buffer(RECEIVE_BUFFER), OnReceivePacket);
	SERVER_RUNNING = true;
	IO_CONTEXT_THREAD = std::thread([]() {
		IO_CONTEXT.run();
		});
}

void Tanuki::LazyCluster::Stop() {
	using boost::asio::ip::udp;

	if (!static_cast<bool>(Options[kEnableLazyCluster])) {
		return;
	}

	if (!SERVER_RUNNING) {
		return;
	}

	SERVER_RUNNING = false;
	IO_CONTEXT.stop();
	IO_CONTEXT_THREAD.join();
}

void Tanuki::LazyCluster::Send(Thread& thread) {
	if (!static_cast<bool>(Options[kEnableLazyCluster])) {
		return;
	}

	// �ʐM�p�P�b�g�ɃV���A���C�Y����B
	int multiPV = static_cast<int>(Options["MultiPV"]);
	Position& position = thread.rootPos;
	auto packets = boost::make_shared<std::vector<Packet>>();

	// �ePV�ɂ��ď�������
	multiPV = std::min<int>(multiPV, thread.rootMoves.size());
	for (int pv_index = 0; pv_index < multiPV; ++pv_index) {
		StateInfo state_info[MAX_PLY] = {};

		Packet packet = {};
		if (Serialize(position.key(), packet)) {
			packets->push_back(packet);
		}

		const auto& pv = thread.rootMoves[pv_index].pv;
		for (int moveIndex = 0; moveIndex < pv.size(); ++moveIndex) {
			auto move = pv[moveIndex];
			if (!is_ok(move)) {
				break;
			}

			position.do_move(move, state_info[moveIndex]);
			if (Serialize(position.key(), packet)) {
				packets->push_back(packet);
			}
		}
		for (auto rit = pv.rbegin(); rit != pv.rend(); ++rit) {
			position.undo_move(*rit);
		}
	}

	// �ʐM�p�P�b�g�̃T�C�Y��UDP��1��ő���₷���T�C�Y�Ɍ��炷�B
	if (packets->size() > kMaxNumPacketsToSend) {
		packets->resize(kMaxNumPacketsToSend);
	}

	if (packets->empty()) {
		return;
	}

	// ���M����B
	for (const auto& receiver_endpoint : ENDPOINTS) {
		// ���M����������܂�packets����������ɕێ��������Ȃ���΂Ȃ�Ȃ��B
		// �����_�֐��ɃL���v�`�������A���M�����܂ŕێ���������B
		UDP_SOCKET->async_send_to(boost::asio::buffer(*packets), receiver_endpoint,
			[packets](const boost::system::error_code& /*error*/, std::size_t bytes_transferred) {});

		//boost::system::error_code error_code;
		//UDP_SOCKET->send_to(boost::asio::buffer(packets), receiver_endpoint, 0, error_code);
		//if (error_code.failed()) {
		//	sync_cout << "info string Falied to send packets. error_code.message()=" << error_code.message() << sync_endl;
		//	continue;
		//}
	}

	sync_cout << "info string Lazy Cluster client: Sent " << packets.size() << " packets." << sync_endl;
}

#endif
