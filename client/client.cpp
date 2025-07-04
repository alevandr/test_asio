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
public:
	bool OnUserCreate()
	{
		if (Connect("127.0.0.1", 60000))
		{
			return true;
		}
		return false;
	}

	bool OnUserUpdate(float fElapsedTime = 0.0)
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
				//case(GameMsg::Game_RemovePlayer):
				//{
				//	uint32_t nRemovalID = 0;
				//	msg >> nRemovalID;
				//	mapObjects.erase(nRemovalID);
				//	break;
				//}
				case(GameMsg::Game_UpdatePlayer):
				{
					sPlayerDescription desc;
					msg >> desc;
					//std::cout << "temp: " << desc.msg<< "\n";
					//sPlayerDescription desc;
					//msg >> desc;
					//mapObjects.insert_or_assign(desc.nUniqueID, desc);
					break;
				}
				}
			}
		}

		if (bWaitingForConnection)
		{
			return true;
		}

		// Control of Player Object
		/*std::string userMsg = "";
		while (userMsg != "exit")
		{
			std::getline(std::cin, userMsg);
			olc::net::message<GameMsg> msg;
			msg.header.id = GameMsg::Game_UpdatePlayer;
			msg << userMsg;
			Send(msg);
		}*/

		olc::net::message<GameMsg> msg;
		msg.header.id = GameMsg::Game_UpdatePlayer;
		std::string userMsg = "";
		//while (userMsg != "exit")
		//{
			std::getline(std::cin, userMsg);
			//mapObjects[nPlayerID].msg = userMsg;
			msg << mapObjects[nPlayerID];
			Send(msg);
		//}
		return true;
	}
};

int main()
{
	client demo;
	if (demo.OnUserCreate())
	{
		while (1)
		{
			demo.OnUserUpdate();
		}
	}
	return 0;
}