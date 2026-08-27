#include "stdafx.h"
#include "MoveList.h"
#include "Controller.h"
#include "EventTimer.h"
#include "MiniMap.h"
#include "PrintPlayer.h"
#include "Protect.h"

CMoveList gMoveList;

CMoveList::CMoveList()
{
	this->MoveListSwitch = false;

	this->m_Pos.x = MOVELIST_WINDOW_POSX;
	this->m_Pos.y = MOVELIST_WINDOW_POSY;

	this->m_Size.cx = ((MOVELIST_SECTION_WIDTH * 3.5f) + 10);
	this->m_Size.cy = MOVELIST_BASE_HEIGHT;

	this->m_SectionWidth = MOVELIST_SECTION_WIDTH;

	this->m_nShowingLines = -1;
	this->m_iCurrentRenderEndLine = -1;

	this->m_ScrollBarPos[0] = (float)this->m_Pos.x + this->m_Size.cx - SCROLL_WIDTH - 3.0f + 1.0f;
	this->m_ScrollBarPos[1] = 0.0f;

	this->m_ScrollBarSize[0] = SCROLL_BAR_SIZE;
	this->m_ScrollBarSize[1] = 0.0f;

	this->m_MoveList.clear();
}

CMoveList::~CMoveList()
{

}

bool CMoveList::GetMoveListState()
{
	return this->MoveListSwitch;
}

void CMoveList::Toggle()
{
	if (gProtect.m_MainInfo.EnableMoveList == 0)
	{
		return;
	}

	if (CheckInputInterfaces())
	{
		return;
	}

	if (CheckRightInterfaces())
	{
		this->MoveListSwitch = false;

		return;
	}

	if (ErrorMessage)
	{
		this->MoveListSwitch = false;

		return;
	}

	if (gEventTimer.GetEventTimerState())
	{
		gEventTimer.Toggle();
	}

	if (gMiniMap.GetMiniMapState())
	{
		gMiniMap.Toggle();
	}

	this->MoveListSwitch ^= 1;

	if (this->MoveListSwitch)
	{
		this->m_SectionWidth = MOVELIST_SECTION_WIDTH + (25.0f / g_fScreenRate_x);

		this->Scrolling(0);

		this->UpdateWndSize();

		this->UpdateScrollSize();

		this->UpdateScrollPos();
	}

	PlayBuffer(25, 0, 0);
}

void CMoveList::Render()
{
	if (!this->MoveListSwitch)
	{
		return;
	}

	this->RenderFrame();

	this->RenderScrollbar();

	this->RenderMapsList();
}

void CMoveList::UpdateMouse()
{
	if (gProtect.m_MainInfo.EnableMoveList == 0)
	{
		return;
	}

	if (!this->MoveListSwitch)
	{
		return;
	}

	if (CheckRightInterfaces())
	{
		this->MoveListSwitch = false;

		return;
	}

	if (ErrorMessage)
	{
		this->MoveListSwitch = false;

		return;
	}

	if (IsWorkZone(this->m_Pos.x, this->m_Pos.y, (int)this->m_Size.cx, (int)this->m_Size.cy))
	{
		MouseOnWindow = true;

		if (this->CheckScrolling())
		{
			return;
		}

		if (this->CheckClickOnMap())
		{
			return;
		}

		if (this->CheckClickOnClose())
		{
			return;
		}

		if (MouseLButton && MouseLButtonPush)
		{
			MouseLButtonPush = false;

			MouseUpdateTime = 0;

			MouseUpdateTimeMax = 6;
		}
	}
}

void CMoveList::RenderFrame()
{
	EnableAlphaTest(true);

	glColor4f(0.0f, 0.0f, 0.0f, 0.8f);

	RenderColor((float)this->m_Pos.x, (float)this->m_Pos.y, this->m_Size.cx, this->m_Size.cy);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	DisableAlphaBlend();

	EnableAlphaTest(true);

	int PosX = this->m_Pos.x + 5;
	int PosY = this->m_Pos.y + 5;

	DWORD backupBgTextColor = SetBackgroundTextColor;
	DWORD backupTextColor = SetTextColor;

	SelectObject(m_hFontDC, g_hFontBold);
	SetBackgroundTextColor = Color4b(0, 0, 0, 0);
	SetTextColor = Color4b(255, 204, 26, 255);

	// Teleport Window
	RenderText(PosX, PosY, "Teleport Window", REAL_WIDTH((int)(this->m_Size.cx - 10.0f)), RT3_SORT_CENTER, NULL);

	PosY = this->m_Pos.y + 20;

	SelectObject(m_hFontDC, g_hFont);
	SetTextColor = Color4b(127, 178, 255, 255);

	// Map
	RenderText(PosX, PosY, "Map", REAL_WIDTH((int)this->m_SectionWidth), RT3_SORT_CENTER, NULL);

	PosX += ((int)this->m_SectionWidth);

	// Min. Level
	RenderText(PosX, PosY, "Min. Level", REAL_WIDTH((int)this->m_SectionWidth), RT3_SORT_CENTER, NULL);

	PosX += ((int)this->m_SectionWidth);

	// Cost
	RenderText(PosX, PosY, "Cost", REAL_WIDTH((int)this->m_SectionWidth), RT3_SORT_CENTER, NULL);

	PosX += ((int)this->m_SectionWidth);

	// VIP
	RenderText(PosX, PosY, "VIP", REAL_WIDTH((int)(this->m_SectionWidth * 0.5f)), RT3_SORT_CENTER, NULL);

	PosX = this->m_Pos.x + 5;
	PosY = this->m_Pos.y + (int)this->m_Size.cy - 15;

	SetBackgroundTextColor = Color4b(255, 0, 0, 255);
	SetTextColor = Color4b(255, 255, 255, 255);

	// Close
	RenderText(PosX, PosY, "Close", REAL_WIDTH((int)(this->m_Size.cx - 10.0f)), RT3_SORT_CENTER, NULL);

	SetBackgroundTextColor = backupBgTextColor;
	SetTextColor = backupTextColor;
}

void CMoveList::RenderScrollbar()
{
	EnableAlphaTest(true);

	glColor4f(0.5f, 0.7f, 1.0f, 0.8f);

	RenderColor(this->m_ScrollBarPos[0], this->m_ScrollBarPos[1], this->m_ScrollBarSize[0], this->m_ScrollBarSize[1]);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	DisableAlphaBlend();
}

void CMoveList::RenderMapsList()
{
	DWORD backupBgTextColor = SetBackgroundTextColor;
	DWORD backupTextColor = SetTextColor;

	EnableAlphaTest(true);

	int PosX = (int)this->m_Pos.x + 5;

	int PosY = (int)this->m_Pos.y + 35;

	if (this->m_MoveList.empty())
	{
		SelectObject(m_hFontDC, g_hFontBig);

		SetBackgroundTextColor = Color4b(255, 255, 255, 0);

		SetTextColor = Color4b(255, 255, 255, 255);

		RenderText(PosX, PosY - 8, "NO MOVE INFO", REAL_WIDTH((int)(this->m_Size.cx - 10.0f)), RT3_SORT_CENTER, NULL);
	}
	else
	{
		SelectObject(m_hFontDC, g_hFont);

		char text[32];

		// Draw message
		int iRenderStartLine = 0;

		if (this->m_iCurrentRenderEndLine >= this->m_nShowingLines)
		{
			iRenderStartLine = this->m_iCurrentRenderEndLine - this->m_nShowingLines + 1;
		}

		for (int i = iRenderStartLine; i <= this->m_iCurrentRenderEndLine; i++)
		{
			if (i >= (int)this->m_MoveList.size())
			{
				break;
			}

			MOVE_LIST_INFO& it = this->m_MoveList[i];

			it.CanMove = this->CheckMove(it);

			if (it.CanMove)
			{
				if (IsWorkZone(PosX, PosY, (int)this->m_Size.cx - 10, 10))
				{
					EnableAlphaTest(true);

					glColor4f(0.8f, 0.8f, 0.1f, 0.6f);

					RenderColor((float)PosX, (float)PosY, this->m_Size.cx - 10.0f, 10.0f);

					glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

					EnableAlphaTest(true);
				}

				SetTextColor = Color4b(255, 255, 255, 255);
			}
			else
			{
				SetTextColor = Color4b(164, 39, 17, 255);
			}

			SetBackgroundTextColor = Color4b(255, 255, 255, 0);

			RenderText(PosX, PosY, it.MapName, REAL_WIDTH((int)this->m_SectionWidth), RT3_SORT_CENTER, NULL);

			PosX += ((int)this->m_SectionWidth);

			if (it.MinLevel == -1) // MinLevel -1
			{
				wsprintf(text, "~");
			}
			else // Valid
			{
				wsprintf(text, "%d", it.MinLevel);
			}

			RenderText(PosX, PosY, text, REAL_WIDTH((int)this->m_SectionWidth), RT3_SORT_CENTER, NULL);

			PosX += ((int)this->m_SectionWidth);

			ConvertGold(it.Money, text);
			RenderText(PosX, PosY, text, REAL_WIDTH((int)this->m_SectionWidth), RT3_SORT_CENTER, NULL);

			PosX += ((int)this->m_SectionWidth);

			if (it.AccountLevel != -1 && it.AccountLevel > 0)
			{
				SetTextColor = Color4b(255, 0, 0, 255);
				RenderText(PosX, PosY, "[VIP]", REAL_WIDTH((int)(this->m_SectionWidth * 0.5f)), RT3_SORT_CENTER, NULL);
			}

			PosY += 12;

			PosX = this->m_Pos.x + 5;
		}
	}

	SelectObject(m_hFontDC, g_hFont);
	SetBackgroundTextColor = backupBgTextColor;
	SetTextColor = backupTextColor;
}

bool CMoveList::CheckScrolling()
{
	if (gController.MouseWheel > 0)
	{
		this->Scrolling(this->m_iCurrentRenderEndLine - 1);
	}
	else if (gController.MouseWheel < 0)
	{
		this->Scrolling(this->m_iCurrentRenderEndLine + 1);
	}

	if (gController.MouseWheel != 0)
	{
		gController.MouseWheel = 0;

		return true;
	}

	return false;
}

bool CMoveList::CheckClickOnMap()
{
	if (this->m_MoveList.empty())
	{
		return false;
	}

	int PosX = this->m_Pos.x + 5;

	int PosY = this->m_Pos.y + 35;

	int iRenderStartLine = 0;

	if (this->m_iCurrentRenderEndLine >= this->m_nShowingLines)
	{
		iRenderStartLine = this->m_iCurrentRenderEndLine - this->m_nShowingLines + 1;
	}

	for (int i = iRenderStartLine; i <= this->m_iCurrentRenderEndLine; i++)
	{
		if (i >= (int)this->m_MoveList.size())
		{
			break;
		}

		const MOVE_LIST_INFO& it = this->m_MoveList[i];

		if (IsWorkZone(PosX, PosY, (int)(this->m_Size.cx - 10.0f), 10))
		{
			if (MouseLButton && MouseLButtonPush)
			{
				MouseLButtonPush = false;

				MouseUpdateTime = 0;

				MouseUpdateTimeMax = 6;

				if (it.CanMove)
				{
					char Text[100];

					wsprintf(Text, "/move %s", it.MapName);

					SendChat(Text);

					this->Toggle();
				}

				return true;
			}
		}

		PosY += 12;
	}

	return false;
}

bool CMoveList::CheckClickOnClose()
{
	if (IsWorkZone((int)(this->m_Pos.x + 5.0f), (int)(this->m_Pos.y + this->m_Size.cy - 15.0f), (int)(this->m_Size.cx - 10.0f), 10))
	{
		if (MouseLButton && MouseLButtonPush)
		{
			MouseLButtonPush = false;

			MouseUpdateTime = 0;

			MouseUpdateTimeMax = 6;

			this->Toggle();
		}

		return true;
	}

	return false;
}

bool CMoveList::CheckMove(const MOVE_LIST_INFO& Move)
{
	bool bResult = true;

	STRUCT_DECRYPT;

	if (Move.MinLevel != -1 && *(WORD*)(CharacterAttribute + 0x0E) < Move.MinLevel)
	{
		bResult = false;

		goto EXIT;
	}

	if (Move.MaxLevel != -1 && *(WORD*)(CharacterAttribute + 0x0E) > Move.MaxLevel)
	{
		bResult = false;

		goto EXIT;
	}

	if (Move.MinReset != -1 && gPrintPlayer.ViewReset < (DWORD)Move.MinReset)
	{
		bResult = false;

		goto EXIT;
	}

	if (Move.MaxReset != -1 && gPrintPlayer.ViewReset > (DWORD)Move.MaxReset)
	{
		bResult = false;

		goto EXIT;
	}

	if (*(DWORD*)(CharacterMachine + 0x548) < Move.Money)
	{
		bResult = false;

		goto EXIT;
	}

	if (this->PKLimitFree == 0 && *(BYTE*)(Hero + 0x2EA) >= PKLVL_OUTLAW)
	{
		bResult = false;

		goto EXIT;
	}

	bResult = this->CheckSpecialRequirements(Move);

EXIT:

	STRUCT_ENCRYPT;

	return bResult;
}

bool CMoveList::CheckSpecialRequirements(const MOVE_LIST_INFO& Move)
{
	switch (Move.MapNumber)
	{
		case MAP_ATLANS:
		{
			if (*(short*)(Hero + 0x2B8) == GET_ITEM_MODEL(13, 2) // Uniria
				|| *(short*)(Hero + 0x2B8) == GET_ITEM_MODEL(13, 3)) // Dinorant
			{
				return false;
			}

			break;
		}

		case MAP_ICARUS:
		{
			if (*(short*)(Hero + 0x2A0) == -1
				&& *(short*)(Hero + 0x2B8) != GET_ITEM_MODEL(13, 3)) // No wings or Dinorant
			{
				return false;
			}

			if (*(short*)(Hero + 0x2B8) == GET_ITEM_MODEL(13, 2)) // Uniria
			{
				return false;
			}

			break;
		}
	}

	return true;
}

void CMoveList::SetNumberOfShowingLines(int nShowingLines)
{
	if (this->m_nShowingLines == nShowingLines)
	{
		return;
	}

	this->m_nShowingLines = nShowingLines;

	this->Scrolling(0);

	this->UpdateWndSize();

	this->UpdateScrollSize();

	this->UpdateScrollPos();
}

void CMoveList::Scrolling(int nRenderEndLine)
{
	int newEndLine;

	if ((int)this->m_MoveList.size() <= this->m_nShowingLines)
	{
		newEndLine = (int)this->m_MoveList.size() - 1;
	}
	else if (nRenderEndLine < this->m_nShowingLines)
	{
		newEndLine = this->m_nShowingLines - 1;
	}
	else if (nRenderEndLine >= (int)this->m_MoveList.size())
	{
		newEndLine = (int)this->m_MoveList.size() - 1;
	}
	else
	{
		newEndLine = nRenderEndLine;
	}

	if (this->m_iCurrentRenderEndLine == newEndLine)
	{
		return;
	}

	this->m_iCurrentRenderEndLine = newEndLine;

	this->UpdateScrollPos();
}

void CMoveList::UpdateWndSize()
{
	this->m_Size.cx = ((this->m_SectionWidth * 3.5f) + 10.0f);

	this->m_Size.cy = MOVELIST_BASE_HEIGHT + this->m_nShowingLines * 12.0f;
}

void CMoveList::UpdateScrollSize()
{
	this->m_ScrollBarSize[0] = SCROLL_BAR_SIZE;

	this->m_ScrollBarSize[1] = this->m_nShowingLines * 12.0f;

	const int listSize = (int)this->m_MoveList.size();

	if (listSize > this->m_nShowingLines)
	{
		const int numberOfLines = listSize - this->m_nShowingLines + 1;

		this->m_ScrollBarSize[1] /= numberOfLines;
	}
}

void CMoveList::UpdateScrollPos()
{
	if (this->m_nShowingLines >= (int)this->m_MoveList.size())
	{
		return;
	}

	// Calculate scroll bar position
	float fPosRate = 1.0f;

	if (this->m_nShowingLines < (int)this->m_MoveList.size())
	{
		int numberOfLines = (int)this->m_MoveList.size() - (this->m_nShowingLines - 1);

		int currentRenderEndLine = this->m_iCurrentRenderEndLine - (this->m_nShowingLines - 1);

		fPosRate = (float)(numberOfLines - currentRenderEndLine);
	}

	this->m_ScrollBarPos[0] = (float)this->m_Pos.x + this->m_Size.cx - SCROLL_WIDTH + SCROLL_BAR_SIZE;

	this->m_ScrollBarPos[1] = ((float)this->m_Pos.y + this->m_nShowingLines * 12.0f + 35.0f) - (this->m_ScrollBarSize[1] * fPosRate);
}

void CMoveList::GCMoveListRecv(PMSG_MOVE_LIST_RECV* lpMsg)
{
	if (gProtect.m_MainInfo.EnableMoveList == 0)
	{
		return;
	}

	this->PKLimitFree = lpMsg->PKLimitFree;

	this->m_MoveList.clear();

	for (int i = 0; i < lpMsg->count; i++)
	{
		MOVE_LIST_INFO* lpInfo = (MOVE_LIST_INFO*)(((BYTE*)lpMsg) + sizeof(PMSG_MOVE_LIST_RECV) + (sizeof(MOVE_LIST_INFO) * i));

		MOVE_LIST_INFO info;

		info.MapNumber = lpInfo->MapNumber;

		memcpy(info.MapName, lpInfo->MapName, sizeof(info.MapName));

		info.MinLevel = lpInfo->MinLevel;

		info.MaxLevel = lpInfo->MaxLevel;

		info.MinReset = lpInfo->MinReset;

		info.MaxReset = lpInfo->MaxReset;

		info.AccountLevel = lpInfo->AccountLevel;

		info.Money = lpInfo->Money;

		info.CanMove = lpInfo->CanMove;

		this->m_MoveList.push_back(info);
	}

	this->m_SectionWidth = MOVELIST_SECTION_WIDTH + (25.0f / g_fScreenRate_x);

	this->SetNumberOfShowingLines(NUMBER_OF_SHOWING_MAPS);
}