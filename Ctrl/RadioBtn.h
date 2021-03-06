#pragma once

#include "CtrlBase.h"

class CRadioBtn : public CCtrlBase
{
private:
	std::vector<HWND> m_hRadios;
	std::vector<UINT> m_IDs;

public:
	HWND Create(HWND hWnd, LPCTSTR lpCaption, UINT uID, int x = 0, int y = 0, int cx = 0, int cy = 0);
	void SetCheck(BOOL flag)            { Button_SetCheck(GetCtrlHandle(), flag); }
	void SetCheck(DWORD num, BOOL flag) { Button_SetCheck(m_hRadios[num], flag); }
	BOOL GetCheck()                     { return Button_GetCheck(GetCtrlHandle()); }
	BOOL GetCheck(DWORD num)            { return Button_GetCheck(m_hRadios[num]); }
	UINT GetID(DWORD num)               { return (num >= m_IDs.size()) ? 0 : m_IDs[num]; }
	void Close()                        { m_hRadios.clear(); m_IDs.clear(); }
};
