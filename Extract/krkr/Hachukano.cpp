#include "stdafx.h"
#include "Hachukano.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Check if it can be decoded
//
// Parameters:
//   - pclArc - Archive

BOOL CHachukano::OnCheckDecrypt(CArcFile* pclArc)
{
	return pclArc->CheckExe(_T("hachukano.exe"));
}

//////////////////////////////////////////////////////////////////////////////////////////
// Initialize Decryption Process
//
// Parameters:
//   - pclArc - Archive

DWORD CHachukano::OnInitDecrypt(CArcFile* pclArc)
{
	SFileInfo* pstFileInfo = pclArc->GetOpenFileInfo();
	LPCTSTR pszFileExt = PathFindExtension(pstFileInfo->name);

	if ((lstrcmp(pszFileExt, _T(".dll")) == 0) || (pstFileInfo->name == _T("startup.tjs")))
	{
		// Files we don't decode
		SetDecryptRequirement(FALSE);
		return 0;
	}

	// Size to decrypt
	if ((lstrcmp(pszFileExt, _T(".ks")) != 0) && (lstrcmp(pszFileExt, _T(".tjs")) != 0) && (lstrcmp(pszFileExt, _T(".asd")) != 0))
	{
		SetDecryptSize(512);
	}

	// Decryption key
	m_dwChangeDecryptKey = 0;

	return (pstFileInfo->key ^ 0x03020100);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Decoding Process
//
// Parameters:
//   - pbtTarget    - Data to be decrypted
//   - dwTargetSize - Decryption size
//   - dwOffset     - Offset of data to be decoded
//   - dwDecryptKey - Decryption key

DWORD CHachukano::OnDecrypt(BYTE* pbtTarget, DWORD dwTargetSize, DWORD dwOffset, DWORD dwDecryptKey)
{
	// Decrypt
	for (DWORD i = 0; i < dwTargetSize; i += 4)
	{
		if ((i & 255) == 0)
		{
			m_dwChangeDecryptKey = 0;
		}
		else
		{
			m_dwChangeDecryptKey += 0x04040404;
		}

		*(DWORD*)&pbtTarget[i] ^= dwDecryptKey ^ m_dwChangeDecryptKey;
	}

	return dwTargetSize;
}
