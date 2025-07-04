#include <iostream>
#include <unordered_map>
#include "../net/network.h"

enum class GameMsg : uint32_t
{
	Server_GetStatus,
	Server_GetPing,

	Client_Accepted,
	Client_AssignID,
	Client_RegisterWithServer,
	Client_UnregisterWithServer,

	Game_AddPlayer,
	Game_RemovePlayer,
	Game_UpdatePlayer
};

struct sPlayerDescription
{
	int nUniqueID;
	char message[256];
};

class client : olc::net::client_interface<GameMsg>
{
public:
	client()
	{
	}
private:
	std::unordered_map<uint32_t, sPlayerDescription> mapObjects;
	uint32_t nPlayerID = 0;
	sPlayerDescription descPlayer;
	bool bWaitingForConnection = true;
	std::mutex console_mutex; // Mutex to protect console access
public:
	bool OnUserCreate()
	{
		if (Connect("127.0.0.1", 60000))
		{
			return true;
		}
		return false;
	}

	bool OnUserUpdate()
	{
		while (true)
		{
			// Check for incoming network messages
			if (IsConnected())
			{
				while (!Incoming().empty())
				{
					auto msg = Incoming().pop_front().msg;

					switch (msg.header.id)
					{
					case(GameMsg::Client_Accepted):
					{
						std::lock_guard<std::mutex> lock(console_mutex); // Lock before printing
						std::cout << "Server accepted client - you're in!\n";
						olc::net::message<GameMsg> msg;
						msg.header.id = GameMsg::Client_RegisterWithServer;
						msg << descPlayer;
						Send(msg);
						break;
					}
					case(GameMsg::Client_AssignID):
					{
						// Server is assigning us OUR id
						msg >> nPlayerID;
						std::lock_guard<std::mutex> lock(console_mutex); // Lock before printing
						std::cout << "Assigned Client ID = " << nPlayerID << "\n";
						break;
					}
					case(GameMsg::Game_AddPlayer):
					{
						sPlayerDescription desc;
						msg >> desc;
						mapObjects.insert_or_assign(desc.nUniqueID, desc);

						if (desc.nUniqueID == nPlayerID)
						{
							// Now we exist in game world
							bWaitingForConnection = false;
						}
						break;
					}
					case(GameMsg::Game_UpdatePlayer):
					{
						sPlayerDescription desc;
						msg >> desc;
						std::lock_guard<std::mutex> lock(console_mutex); // Lock before printing
						std::cout << "[" << desc.nUniqueID << "]: " << desc.message << "\n";
						break;
					}
					}
				}
			}

			if (bWaitingForConnection)
			{
				continue;
			}

			//return true;
		}
	}
	void Control()
	{
		while (true)
		{
			olc::net::message<GameMsg> msg;
			msg.header.id = GameMsg::Game_UpdatePlayer;
			std::string userMsg = "";
			std::getline(std::cin, userMsg);
			sPlayerDescription desc;
			char charArray[256];
			strcpy_s(desc.message, userMsg.c_str());
			desc.nUniqueID = this->nPlayerID;
			msg << desc;
			Send(msg);
		}
	}
};

int main()
{
	client demo;
	if (demo.OnUserCreate())
	{
		//demo.OnUserUpdate();
		//demo.Control();
		std::thread sending(&client::Control, &demo);
		std::thread updates(&client::OnUserUpdate, &demo);
		sending.join();
		updates.join();
	}
	return 0;
}