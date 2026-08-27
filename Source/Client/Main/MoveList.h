#pragma once

#include "ProtocolDefines.h"

//**********************************************//
//************ GameServer -> Client ************//
//**********************************************//

struct PMSG_MOVE_LIST_RECV
{
	PSWMSG_HEAD header; // C2:F3:E5
	BYTE PKLimitFree;
	BYTE count;
};

struct MOVE_LIST_INFO
{
	BYTE MapNumber;
	char MapName[32];
	bool CanMove;
	short MinLevel;
	short MaxLevel;
	short MinReset;
	short MaxReset;
	short AccountLevel;
	DWORD Money;
};

//**********************************************//
//**********************************************//
//**********************************************//

class CMoveList
{
	enum
	{
		MOVELIST_WINDOW_POSX = 1,
		MOVELIST_WINDOW_POSY = 1,

		MOVELIST_SECTION_WIDTH = 50,
		MOVELIST_BASE_HEIGHT = 60,

		// Scroll settings
		SCROLL_WIDTH = 4, // Width of the scroll bar
		SCROLL_BAR_SIZE = 2,

		NUMBER_OF_SHOWING_MAPS = 20,
	};

public:

	CMoveList();

	virtual ~CMoveList();

	bool GetMoveListState();

	void Toggle();

	void Render();

	void UpdateMouse();

	void GCMoveListRecv(PMSG_MOVE_LIST_RECV* lpMsg);

private:

	void RenderFrame();

	void RenderScrollbar();

	void RenderMapsList();

	bool CheckScrolling();

	bool CheckClickOnMap();

	bool CheckClickOnClose();

	bool CheckMove(const MOVE_LIST_INFO& Move);

	bool CheckSpecialRequirements(const MOVE_LIST_INFO& Move);

	void SetNumberOfShowingLines(int nShowingLines);

	void Scrolling(int nRenderEndLine);

	void UpdateWndSize();

	void UpdateScrollSize();
	void UpdateScrollPos();

private:

	bool MoveListSwitch;

	POINT m_Pos;

	SIZEF m_Size;

	float m_SectionWidth;

	int m_nShowingLines;
	int m_iCurrentRenderEndLine;

	float m_ScrollBarPos[2];
	float m_ScrollBarSize[2];

	std::vector<MOVE_LIST_INFO> m_MoveList;

	BYTE PKLimitFree;
};

extern CMoveList gMoveList;