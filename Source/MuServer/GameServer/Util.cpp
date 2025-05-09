#include "stdafx.h"
#include "Util.h"
#include "GameMain.h"
#include "GameServer.h"
#include "HackCheck.h"
#include "ItemManager.h"
#include "Message.h"
#include "Resource.h"
#include "ServerInfo.h"
#include "SocketManager.h"
#include "Viewport.h"

std::mt19937 seed;

std::uniform_int_distribution<int> dist;

short RoadPathTable[MAX_ROAD_PATH_TABLE] = { -1, -1, 0, -1, 1, -1, 1, 0, 1, 1, 0, 1, -1, 1, -1, 0 };

int SafeGetItem(int index)
{
	return CHECK_ITEM(index);
}

float GetRoundValue(float value)
{
	float integral;

	if (modf(value, &integral) > 0.5f)
	{
		return ceil(value);
	}

	return floor(value);
}

BYTE GetExceOptionCount(BYTE ExceOption)
{
	BYTE count = 0;

	for (int n = 0; n < MAX_EXC_OPTION; n++)
	{
		if ((ExceOption & (1 << n)) != 0)
		{
			count++;
		}
	}

	return count;
}

int LevelSmallConvert(int level)
{
	if (level >= 11)
	{
		return 7;
	}
	else if (level == 10)
	{
		return 6;
	}
	else if (level == 9)
	{
		return 5;
	}
	else if (level == 8)
	{
		return 4;
	}
	else if (level == 7)
	{
		return 3;
	}
	else if (level >= 5 && level <= 6)
	{
		return 2;
	}
	else if (level >= 3 && level <= 4)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

bool CheckSpecialText(char* Text)
{
	for (unsigned char* lpszCheck = (unsigned char*)Text; *lpszCheck; ++lpszCheck)
	{
		if (_mbclen(lpszCheck) == 1) // One byte
		{
			//if ( *lpszCheck < 0x21 || *lpszCheck > 0x7E)
			if (*lpszCheck < 48 || (58 <= *lpszCheck && *lpszCheck < 65) || (91 <= *lpszCheck && *lpszCheck < 97) || *lpszCheck > 122)
			{
				return false;
			}
		}
		else // Two bytes
		{
			unsigned char* lpszTrail = lpszCheck + 1;

			if (0x81 <= *lpszCheck && *lpszCheck <= 0xC8) // Korean
			{
				if ((0x41 <= *lpszTrail && *lpszTrail <= 0x5A)
				    || (0x61 <= *lpszTrail && *lpszTrail <= 0x7A)
				    || (0x81 <= *lpszTrail && *lpszTrail <= 0xFE))
				{ // Excluding transparent characters
					// Areas of special characters that are not allowed
					if (0xA1 <= *lpszCheck && *lpszCheck <= 0xAF && 0xA1 <= *lpszTrail)
					{
						return false;
					}
					else if (*lpszCheck == 0xC6 && 0x53 <= *lpszTrail && *lpszTrail <= 0xA0)
					{
						return false;
					}
					else if (0xC7 <= *lpszCheck && *lpszCheck <= 0xC8 && *lpszTrail <= 0xA0)
					{
						return false;
					}
				}
				else
				{
					return false;
				}
			}
			else
			{
				return false;
			}

			++lpszCheck;
		}
	}

	return true;
}

bool CheckSpaceCharacter(char* Text)
{
	for (unsigned char* lpszCheck = (unsigned char*)Text; *lpszCheck; ++lpszCheck)
	{
		if (*lpszCheck == ' ')
		{
			return false;
		}
	}

	return true;
}

BYTE GetPathPacketDirPos(int px, int py)
{
	if (px <= -1 && py <= -1)
	{
		return 0;
	}
	else if (px <= -1 && py == 0)
	{
		return 7;
	}
	else if (px <= -1 && py >= 1)
	{
		return 6;
	}
	else if (px == 0 && py <= -1)
	{
		return 1;
	}
	else if (px == 0 && py >= 1)
	{
		return 5;
	}
	else if (px >= 1 && py <= -1)
	{
		return 2;
	}
	else if (px >= 1 && py == 0)
	{
		return 3;
	}
	else if (px >= 1 && py >= 1)
	{
		return 4;
	}

	return 0;
}

void PacketArgumentDecrypt(char* out_buff, char* in_buff, int size)
{
	BYTE XorTable[3] = { 0xFC, 0xCF, 0xAB };

	for (int n = 0; n < size; n++)
	{
		out_buff[n] = in_buff[n] ^ XorTable[n % 3];
	}
}

void ErrorMessageBox(char* message, ...)
{
	char buff[256];

	memset(buff, 0, sizeof(buff));

	va_list arg;

	va_start(arg, message);

	vsprintf_s(buff, message, arg);

	va_end(arg);

	MessageBox(0, buff, "Error", MB_OK | MB_ICONERROR);

	ExitProcess(0);
}

void LogAdd(eLogColor color, char* text, ...)
{
	tm today;

	time_t ltime;

	time(&ltime);

	if (localtime_s(&today, &ltime) != 0)
	{
		return;
	}

	char time[32];

	if (asctime_s(time, sizeof(time), &today) != 0)
	{
		return;
	}

	char temp[1024];

	va_list arg;

	va_start(arg, text);

	vsprintf_s(temp, text, arg);

	va_end(arg);

	char log[1024];

	wsprintf(log, "%.8s %s", &time[11], temp);

	gServerDisplayer.LogAddText(color, log, strlen(log));
}

void ConsoleProtocolLog(int type, int aIndex, BYTE* lpMsg, int size)
{
	BYTE head, subhead;

	BYTE header = lpMsg[0];

	if (header == 0xC1 || header == 0xC3)
	{
		head = lpMsg[2];
	}
	else if (header == 0xC2 || header == 0xC4)
	{
		head = lpMsg[3];
	}

	subhead = ((header == 0xC1) ? lpMsg[3] : lpMsg[4]);

	gConsole.Output(type, "[%s] Index: %d, Header: 0x%02X, Head: 0x%02X, SubHead: 0x%02X, Size: %d", (type == CON_PROTO_TCP_RECV) ? "RECV" : "SEND", aIndex, header, head, subhead, size);
}

bool DataSend(int aIndex, BYTE* lpMsg, DWORD size)
{
	ConsoleProtocolLog(CON_PROTO_TCP_SEND, aIndex, lpMsg, size);

	return gSocketManager.DataSend(aIndex, lpMsg, size);
}

void DataSendAll(BYTE* lpMsg, int size)
{
	for (int n = OBJECT_START_USER; n < MAX_OBJECT; n++)
	{
		if (gObjIsConnected(n) != 0)
		{
			DataSend(n, lpMsg, size);
		}
	}
}

bool DataSendSocket(SOCKET socket, BYTE* lpMsg, DWORD size)
{
	if (socket == INVALID_SOCKET)
	{
		return 0;
	}

#if(ENCRYPT_STATE==1)

	EncryptData(lpMsg, size);

#endif

	int count = 0, result = 0;

	while (size > 0)
	{
		if ((result = send(socket, (char*)&lpMsg[count], size, 0)) == SOCKET_ERROR)
		{
			if (WSAGetLastError() != WSAEWOULDBLOCK)
			{
				return 0;
			}
		}
		else
		{
			count += result;

			size -= result;
		}
	}

	return 1;
}

void MsgSendV2(LPOBJ lpObj, BYTE* lpMsg, int size)
{
	for (int n = 0; n < MAX_VIEWPORT; n++)
	{
		if (lpObj->VpPlayer2[n].state != VIEWPORT_NONE && lpObj->VpPlayer2[n].type == OBJECT_USER)
		{
			DataSend(lpObj->VpPlayer2[n].index, lpMsg, size);
		}
	}
}

void CloseClient(int aIndex)
{
	gSocketManager.Disconnect(aIndex);
}

void PostMessageGold(char* name, char* serverName, int message, char* text)
{
	char buff[256] = { 0 };

	PMSG_CHAT_WHISPER_SEND pMsg;

	memcpy(pMsg.name, name, sizeof(pMsg.name));

	for (int n = OBJECT_START_USER; n < MAX_OBJECT; n++)
	{
		if (gObjIsConnectedGP(n) != 0)
		{
			wsprintf(buff, gMessage.GetTextMessage(message, gObj[n].Lang), serverName, text);

			int size = strlen(buff);

			size = ((size > MAX_CHAT_MESSAGE_SIZE) ? MAX_CHAT_MESSAGE_SIZE : size);

			pMsg.header.set(0x02, (sizeof(pMsg) - (sizeof(pMsg.message) - (size + 1))));

			memcpy(pMsg.message, buff, size);

			pMsg.message[size] = 0;

			DataSend(n, (BYTE*)&pMsg, pMsg.header.size);
		}
	}
}

void PostMessageBlue(char* name, char* serverName, int message, char* text)
{
	char buff[256] = { '~' };

	PMSG_CHAT_SEND pMsg;

	memcpy(pMsg.name, name, sizeof(pMsg.name));

	for (int n = OBJECT_START_USER; n < MAX_OBJECT; n++)
	{
		if (gObjIsConnectedGP(n) != 0)
		{
			wsprintf(&buff[1], gMessage.GetTextMessage(message, gObj[n].Lang), serverName, text);

			int size = strlen(buff);

			size = ((size > MAX_CHAT_MESSAGE_SIZE) ? MAX_CHAT_MESSAGE_SIZE : size);

			pMsg.header.set(0x00, (sizeof(pMsg) - (sizeof(pMsg.message) - (size + 1))));

			memcpy(pMsg.message, buff, size);

			pMsg.message[size] = 0;

			DataSend(n, (BYTE*)&pMsg, pMsg.header.size);
		}
	}
}

void PostMessageGreen(char* name, char* serverName, int message, char* text)
{
	char buff[256] = { '@' };

	PMSG_CHAT_SEND pMsg;

	memcpy(pMsg.name, name, sizeof(pMsg.name));

	for (int n = OBJECT_START_USER; n < MAX_OBJECT; n++)
	{
		if (gObjIsConnectedGP(n) != 0)
		{
			wsprintf(&buff[1], gMessage.GetTextMessage(message, gObj[n].Lang), serverName, text);

			int size = strlen(buff);

			size = ((size > MAX_CHAT_MESSAGE_SIZE) ? MAX_CHAT_MESSAGE_SIZE : size);

			pMsg.header.set(0x00, (sizeof(pMsg) - (sizeof(pMsg.message) - (size + 1))));

			memcpy(pMsg.message, buff, size);

			pMsg.message[size] = 0;

			DataSend(n, (BYTE*)&pMsg, pMsg.header.size);
		}
	}
}

void PostMessageLightGreen(char* name, char* serverName, int message, char* text)
{
	char buff[256] = { '@', '@' };

	PMSG_CHAT_SEND pMsg;

	memcpy(pMsg.name, name, sizeof(pMsg.name));

	for (int n = OBJECT_START_USER; n < MAX_OBJECT; n++)
	{
		if (gObjIsConnectedGP(n) != 0)
		{
			wsprintf(&buff[2], gMessage.GetTextMessage(message, gObj[n].Lang), serverName, text);

			int size = strlen(buff);

			size = ((size > MAX_CHAT_MESSAGE_SIZE) ? MAX_CHAT_MESSAGE_SIZE : size);

			pMsg.header.set(0x00, (sizeof(pMsg) - (sizeof(pMsg.message) - (size + 1))));

			memcpy(pMsg.message, buff, size);

			pMsg.message[size] = 0;

			DataSend(n, (BYTE*)&pMsg, pMsg.header.size);
		}
	}
}

void SetLargeRand()
{
	std::random_device m_rd;

	seed = std::mt19937(m_rd());

	dist = std::uniform_int_distribution<int>(0, 2147483647);
}

long GetLargeRand()
{
	uuid_vector_t;

	return dist(seed);
}

void CreateSubMenuItem(int hBaseMenu, int hSubmenuIndex, const char* hMenuLabel)
{
	if (hBaseMenu != ID_STARTINVASION && hBaseMenu != ID_STARTBONUS)
	{
		return;
	}

	if (hSubmenuIndex < 0 || hSubmenuIndex >= 30)
	{
		return;
	}

	HMENU hMenu = GetMenu(hWnd); // Obtener el handle al men� principal

	if (!hMenu)
	{
		return;
	}

	HMENU hEventsMenu = GetSubMenu(hMenu, 2); // Tercer popup es "Events"

	if (!hEventsMenu)
	{
		return;
	}

	HMENU hMenuEvent = GetSubMenu(hEventsMenu, (hBaseMenu == ID_STARTINVASION) ? 2 : 3); // 2 = "Start Invasion" / 3 = "Start Bonus"

	if (!hMenuEvent)
	{
		return;
	}

	AppendMenu(hMenuEvent, MF_STRING, hSubmenuIndex + hBaseMenu, hMenuLabel);

	// Forzar el redibujado del men�
	DrawMenuBar(hWnd);
}