
#include "Connection.hpp"

#include "hex_dump.hpp"

#include <chrono>
#include <stdexcept>
#include <iostream>
#include <cassert>
#include <unordered_map>

#include <random>

int main(int argc, char **argv) {
#ifdef _WIN32
	//when compiled on windows, unhandled exceptions don't have their message printed, which can make debugging simple issues difficult.
	try {
#endif

	//------------ argument parsing ------------

	if (argc != 2) {
		std::cerr << "Usage:\n\t./server <port>" << std::endl;
		return 1;
	}

	//------------ initialization ------------

	Server server(argv[1]);


	//------------ main loop ------------
	constexpr float ServerTick = 1.0f; //TODO: set a server tick that makes sense for your game

	//server state:
	bool game_started = false;

	// https://diego.assencio.com/?index=6890b8c50169ef45b74db135063c227c
	std::random_device device;
	std::mt19937 mt(device());
	std::uniform_int_distribution<int> distribution(0,2);

	uint32_t question_index = 0;
	uint32_t round_num = 0;
	char result = 'n';

	float round_timer = 0;
	const float MaxTime = 60.0f;	// Maximum round time in seconds

	bool is_dirty = false;

	int p1_choice = 0, p2_choice = 0;

	// game logic: per-round tracking
	int answers_recvd = 0;

	//per-client state:
	struct PlayerInfo {
		PlayerInfo() {
			static uint32_t next_player_id = 1;
			name = "Player" + std::to_string(next_player_id);
			next_player_id += 1;
		}

		std::string name;
		uint32_t choice = 0;
		uint32_t score = 0;
	};
	std::unordered_map< Connection *, PlayerInfo > players;

	// generating random distribution int for image index
	auto get_random_index = [&mt, &distribution] (int current_index) {
		int random_num;
		do
		{
			random_num = distribution(mt);
		} while (random_num == current_index);
		return random_num;
	};
	question_index = get_random_index(question_index);

	// update + network sync loop
	while (true) {

		// game state update (game rules)
		if (game_started)
		{
			if (answers_recvd > 1)
			{
				assert(players.size() == 2);
				uint32_t choice = 0;;
				// if both players chose correctly, show win. else, show lose
				for (auto &[c, player] : players) {
					(void)player;

					if (choice == 0)
						choice = player.choice;
					else if (choice == player.choice) {
						result = 'w';
					}
					else {
						result = 'l';
					}
				}

				// start a new round
				// pick a different question
				round_num++;
				answers_recvd = 0;
				question_index = get_random_index(question_index);
				is_dirty = true;
			}
			
		}
		

		static auto next_tick = std::chrono::steady_clock::now() + std::chrono::duration< double >(ServerTick);
		//process incoming data from clients until a tick has elapsed:
		while (true) {
			auto now = std::chrono::steady_clock::now();
			double remain = std::chrono::duration< double >(next_tick - now).count();
			if (remain < 0.0) {
				next_tick += std::chrono::duration< double >(ServerTick);
				break;
			}
			server.poll([&](Connection *c, Connection::Event evt){
				if (evt == Connection::OnOpen) {
					//client connected:

					//create some player info for them:
					players.emplace(c, PlayerInfo());

					// start game only when 2 players are connected
					if (players.size() > 1)
					{
						game_started = true;
						round_timer = MaxTime;
						is_dirty = true;
					}

					std::cout << "Player connected. Total: " << players.size() << std::endl;

				} else if (evt == Connection::OnClose) {
					//client disconnected:

					//remove them from the players list:
					auto f = players.find(c);
					assert(f != players.end());
					players.erase(f);

					if (players.size() < 1)
					{
						game_started = false;
					}
				} else { assert(evt == Connection::OnRecv);
					//got data from client:
					//std::cout << "got bytes:\n" << hex_dump(c->recv_buffer); std::cout.flush();

					//look up in players list:
					auto f = players.find(c);
					assert(f != players.end());
					PlayerInfo &player = f->second;

					//handle messages from client:
					while (c->recv_buffer.size() >= 1) {
						//expecting 2-byte messages 'c' (player choice)
						char type = c->recv_buffer[0];
						if (type != 'c') {
							std::cout << " message of non-'c' type received from client!" << std::endl;
							//shut down client connection:
							c->close();
							return;
						}
						// 2nd byte tells whether new data is being sent by client
						bool new_data = (c->recv_buffer[1] == 'y');
						if (new_data)
						{
							uint32_t player_choice = c->recv_buffer[2];
							player.choice = player_choice;
							answers_recvd++;
						}

						c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + c->recv_buffer.size());
					}
				}
			}, remain);
		}

		//send updated game state to all clients
		if (!game_started)
		{
			continue;
		}
		

		/*
		* Server game state structure: (40 bytes)
		* - round num
		* - time left (in seconds : int)
		* - chosen image index
		* - p1 score, p2 score
		*/
		for (auto &[c, player] : players) {
			(void)player; //work around "unused variable" warning on whatever g++ github actions uses

			//send an update starting with 'm'
			c->send('m');
			c->send(is_dirty ? 'y':'n');
			if (is_dirty)
			{
				c->send((uint8_t)round_num);
				c->send((uint8_t)question_index);
				c->send(result);
			}
		}
		is_dirty = false;

	}


	return 0;

#ifdef _WIN32
	} catch (std::exception const &e) {
		std::cerr << "Unhandled exception:\n" << e.what() << std::endl;
		return 1;
	} catch (...) {
		std::cerr << "Unhandled exception (unknown type)." << std::endl;
		throw;
	}
#endif
}