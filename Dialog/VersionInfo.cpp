#include "stdafx.h"
#include "../res/ResExtractData.h"
#include "VersionInfo.h"

void CVersionInfo::DoModal(HWND hWnd)
{
	HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
	DialogBoxParam(hInst, _T("MYVERSION"), hWnd, (DLGPROC)WndStaticProc, (LPARAM)this);
}

// Version info dialog
LRESULT CVersionInfo::WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
		case WM_INITDIALOG:
			Init();
			SetFocus(GetDlgItem(hWnd, IDC_OKBUTTON));
			return FALSE;

		case WM_COMMAND:
		{
			switch (LOWORD(wp))
			{
				case IDC_OKBUTTON:
					EndDialog(hWnd, IDOK);
					return TRUE;

				case IDCANCEL:
					EndDialog(hWnd, IDCANCEL);
					return TRUE;
			}
			return FALSE;
		}
	}

	return FALSE;
}