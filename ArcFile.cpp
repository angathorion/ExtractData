#include "stdafx.h"
#include "Common.h"
#include "Dialog/ExistsDialog.h"
#include "ExtractBase.h"
#include "Extract/Standard.h"
#include "MD5.h"
#include "ArcFile.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Constructor

CArcFile::CArcFile()
{
	m_dwArcsID = -1;
	m_pEnt = NULL;
	m_hFile = INVALID_HANDLE_VALUE;
	m_ctEnt = 0;
	m_bState = FALSE;

	m_bMountWasSusie = FALSE;
	m_bSetMD5 = FALSE;
	m_bWork = FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Destructor

CArcFile::~CArcFile()
{
	Close();
}

//////////////////////////////////////////////////////////////////////////////////////////
// Mount

BOOL CArcFile::Mount()
{
	CStandard clStandard;

	return clStandard.Mount(this);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Decode

BOOL CArcFile::Decode()
{
	CStandard clStandard;

	return clStandard.Decode(this);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Extract

BOOL CArcFile::Extract()
{
	CStandard clStandard;

	return clStandard.Extract(this);
}

////////////////////////////////////////////////////////////////////////
//
// Archive file manipulation
//
////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
// Open archive file
//
// Parameters:
//   - pszPathToArc - Path to the archive file

BOOL CArcFile::Open(LPCTSTR pszPathToArc)
{
	HANDLE hArc = CreateFile(pszPathToArc, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hArc == INVALID_HANDLE_VALUE)
	{
		CError error;
		if (PathFileExists(pszPathToArc))
			error.Message(GetForegroundWindow(), _T("%s could not be opened."), PathFindFileName(pszPathToArc));
		else
			error.Message(GetForegroundWindow(), _T("%s does not exist."), PathFindFileName(pszPathToArc));
	}
	else
	{
		m_dwArcsID++;
		m_hArcs.push_back(hArc);
		m_pclArcPaths.push_back(pszPathToArc);
		m_pclArcNames.push_back(PathFindFileName(pszPathToArc));
		m_pclArcExtens.push_back(PathFindExtension(pszPathToArc));
	}

	return (hArc != INVALID_HANDLE_VALUE);
}

void CArcFile::Close()
{
	for (size_t i = 0; i < m_hArcs.size(); i++)
	{
		if (m_hArcs[i] != INVALID_HANDLE_VALUE)
		{
			::CloseHandle(m_hArcs[i]);
		}
	}

	m_dwArcsID = -1;
	m_hArcs.clear();
	m_pclArcPaths.clear();
	m_pclArcNames.clear();
	m_pclArcExtens.clear();
	m_vcFileInfoOfFileNameSorted.clear();
}

DWORD CArcFile::Read(LPVOID buf, DWORD size)
{
	DWORD dwReadSize;

	::ReadFile(m_hArcs[m_dwArcsID], buf, size, &dwReadSize, NULL);

	return dwReadSize;
}

BYTE *CArcFile::ReadHed()
{
	Read(m_pHeader, sizeof(m_pHeader));
	return m_pHeader;
}

QWORD CArcFile::Seek(INT64 offset, DWORD SeekMode)
{
	LARGE_INTEGER li;
	li.QuadPart = offset;
	li.LowPart = SetFilePointer(m_hArcs[m_dwArcsID], li.LowPart, &li.HighPart, SeekMode);

	if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
		li.QuadPart = -1;

	return (QWORD)(li.QuadPart);
/*
	LARGE_INTEGER li_offset;
	li_offset.QuadPart = offset;
	LARGE_INTEGER new_fp;
	SetFilePointerEx(m_hArcs[m_dwArcsID], li_offset, &new_fp, SeekMode);
	return (QWORD)(new_fp.QuadPart);
*/
}

QWORD CArcFile::SeekHed(INT64 offset)
{
	return Seek(offset, FILE_BEGIN);
}

QWORD CArcFile::SeekEnd(INT64 offset)
{
	return Seek(-offset, FILE_END);
}

QWORD CArcFile::SeekCur(INT64 offset)
{
	return Seek(offset, FILE_CURRENT);
}

QWORD CArcFile::GetArcPointer()
{
	return Seek(0, FILE_CURRENT);
/*
	LARGE_INTEGER zero;
	zero.QuadPart = 0;
	LARGE_INTEGER offset;
	SetFilePointerEx(m_hArcs[m_dwArcsID], zero, &offset, FILE_CURRENT);
	return (QWORD)(offset.QuadPart);
*/
}

QWORD CArcFile::GetArcSize()
{
	LARGE_INTEGER li;
	li.LowPart = GetFileSize(m_hArcs[m_dwArcsID], &(DWORD&)li.HighPart);
	return (QWORD)(li.QuadPart);
/*
	LARGE_INTEGER ArcSize;
	GetFileSizeEx(m_hArcs[m_dwArcsID], &ArcSize);
	return (QWORD)(ArcSize.QuadPart);
*/
}

////////////////////////////////////////////////////////////////////////
//
// Output file operation
//
////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
// Open File
//
// Parameters:
//   - pszReFileExt - Extension after rename

BOOL CArcFile::OpenFile(LPCTSTR pszReFileExt)
{
	BOOL bReturn = FALSE;

	// Create file path
	CreateFileName(pszReFileExt);

	// Open file
	if (m_clfOutput.Open(m_clsPathToFile, (YCFile::modeCreate | YCFile::modeWrite | YCFile::shareDenyWrite)))
	{
		// Opened file successfully

		if (m_clsPathToFile.Find(m_pOption->TmpDir) >= 0)
		{
			m_pInfFile->sTmpFilePath.insert(m_clsPathToFile);
		}

		bReturn = TRUE;
	}
	else
	{
		// Failed to open file.

		CError clError;

		clError.Message(m_pProg->GetHandle(), _T("Failed to write to %s"), m_clsPathToFile);

		throw (int) -1;
	}

	// Prepare simple decoding
	// InitDecrypt();

	return bReturn;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Opening the script file for writing

BOOL CArcFile::OpenScriptFile()
{
	YCString clsFileExt;

	if (m_pOption->bRenameScriptExt)
	{
		clsFileExt = _T(".txt");
	}

	return OpenFile(clsFileExt);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Close File

void CArcFile::CloseFile()
{
	m_clfOutput.Close();

	if (m_pProg->OnCancel())
	{
		// If cancel is pressed

		::DeleteFile(m_clfOutput.GetFilePath());
	}
/*
	if( m_hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
		if ((m_pProg->OnCancel() == TRUE) && (m_clsPathToFile != _T("")))
			DeleteFile(m_clsPathToFile);
	}
	if (m_clsPathToFile != _T(""))
		m_clsPathToFile = _T("");
*/
}

//////////////////////////////////////////////////////////////////////////////////////////
// Simple initialization of the decryption key
//
// Remark: Obtains the decryption key from a file

DWORD CArcFile::InitDecrypt()
{
	m_deckey = 0;

	if (!m_pOption->bEasyDecrypt)
	{
		return m_deckey;
	}

	SFileInfo* pInfFile = GetOpenFileInfo();

	BYTE  abtHeader[12];
	QWORD u64FilePtrSave = GetArcPointer();

	if (u64FilePtrSave != pInfFile->start)
	{
		SeekHed(pInfFile->start);
	}

	ZeroMemory(abtHeader, sizeof(abtHeader));

	Read(abtHeader, sizeof(abtHeader));
	SeekHed(u64FilePtrSave);

	return InitDecrypt(abtHeader);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Simple initialization of the decryption key
//
// Remark: Gets the decryption key from pvData

DWORD CArcFile::InitDecrypt(const void* pvData)
{
	m_deckey = 0;

	if (!m_pOption->bEasyDecrypt)
	{
		return m_deckey;
	}

	SFileInfo*  pstFileInfo = GetOpenFileInfo();
	const BYTE* pbtData = (const BYTE*) pvData;

	if (pstFileInfo->format == _T("OGG"))
	{
		// Ogg Vorbis
		m_deckey = *(DWORD*) &pbtData[0] ^ 0x5367674F;
	}
	else if (pstFileInfo->format == _T("PNG"))
	{
		// PNG
		m_deckey = *(DWORD*) &pbtData[0] ^ 0x474E5089;
	}
	else if (pstFileInfo->format == _T("BMP"))
	{
		// BMP
		m_deckey = (*(WORD*) &pbtData[6] << 16) | *(WORD*) &pbtData[8];
	}
	else if ((pstFileInfo->format == _T("JPG")) || (pstFileInfo->format == _T("JPEG")))
	{
		// JPEG
		m_deckey = *(DWORD*) &pbtData[0] ^ 0xE0FFD8FF;
	}
	else if ((pstFileInfo->format == _T("MPG")) || (pstFileInfo->format == _T("MPEG")))
	{
		// MPEG
		m_deckey = *(DWORD*) &pbtData[0] ^ 0xBA010000;
	}
	else if (pstFileInfo->format == _T("TLG"))
	{
		// TLG
		m_deckey = *(DWORD*) &pbtData[4] ^ 0x7200302E;

		// Try to decode
		*(DWORD*) &pbtData[0] ^= m_deckey;

		if ((memcmp(pbtData, "TLG5", 4) != 0) && (memcmp(pbtData, "TLG6", 4) != 0))
		{
			// TLG0 Decision
			m_deckey = *(DWORD*) &pbtData[4] ^ 0x7300302E;
		}
	}

	return m_deckey;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Simple initialization for the decryption key
//
// Note: Text-only data
//
// Parameters:
//   - pvText     - Text Data
//   - dwTextSize - Text data size

DWORD CArcFile::InitDecryptForText(const void* pvText, DWORD dwTextSize)
{
	const BYTE* pbtText = (const BYTE*) pvText;

	m_deckey = *(WORD*) &pbtText[dwTextSize - 2] ^ 0x0A0D;
	m_deckey = (m_deckey << 16) | m_deckey;

	return m_deckey;
}

// Simple decoding
void CArcFile::Decrypt(LPVOID buf, DWORD size)
{
	if (m_deckey == 0)
		return;

	LPBYTE pbyBuf = (LPBYTE)buf;

	for (DWORD i = 0; i < size; i += 4)
		*(LPDWORD)&pbyBuf[i] ^= m_deckey;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Write File

DWORD CArcFile::WriteFile(const void* pvBuffer, DWORD dwWriteSize, DWORD dwSizeOrg)
{
	DWORD dwResult = m_clfOutput.Write(pvBuffer, dwWriteSize);

	if (dwResult != dwWriteSize)
	{
		// Write Failure
		CError clError;

		// Check disk space
		ULARGE_INTEGER stuiFreeBytesAvailable;
		ULARGE_INTEGER stuiTotalNumberOfBytes;
		ULARGE_INTEGER stuiTotalNumberOfFreeBytes;

		if (::GetDiskFreeSpaceEx(m_clsPathToFile, &stuiFreeBytesAvailable, &stuiTotalNumberOfBytes, &stuiTotalNumberOfFreeBytes))
		{
			if (stuiTotalNumberOfFreeBytes.QuadPart < dwWriteSize)
			{

				clError.Message(m_pProg->GetHandle(), _T("Failed to write %s \n Not enough disk space"), m_clsPathToFile);
				return dwWriteSize;
			}
		}

		// Other causes
		clError.Message(m_pProg->GetHandle(), _T("Failed to write %s"), m_clsPathToFile);
	}
/*
	if( !::WriteFile(m_hFile, buf, size, &dwWriteSize, NULL) )
	{
		// Write failure

		CError clError;

		// Check disk space

		ULARGE_INTEGER stuiFreeBytesAvailable;
		ULARGE_INTEGER stuiTotalNumberOfBytes;
		ULARGE_INTEGER stuiTotalNumberOfFreeBytes;

		if( ::GetDiskFreeSpaceEx( m_clsPathToFile, &stuiFreeBytesAvailable, &stuiTotalNumberOfBytes, &stuiTotalNumberOfFreeBytes ) )
		{
			if( stuiTotalNumberOfFreeBytes.QuadPart < size )
			{

				clError.Message( m_pProg->GetHandle(), _T("Failed to write %s \n Not enough disk space"), m_clsPathToFile );
				return dwWriteSize;
			}
		}

		// Other causes

		clError.Message( m_pProg->GetHandle(), _T("Failed to write %s"), m_clsPathToFile );
	}
*/
	// Progress update

	if (dwSizeOrg != 0xFFFFFFFF)
	{
		dwWriteSize = dwSizeOrg;
	}

	if (dwWriteSize != 0)
	{
		m_pProg->UpdatePercent(dwWriteSize);
	}

	return dwResult;
}

void CArcFile::ReadWrite()
{
	ReadWrite(m_pInfFile->sizeCmp);
}

void CArcFile::ReadWrite(DWORD FileSize)
{
	DWORD BufSize = GetBufSize();
	YCMemory<BYTE> buf(BufSize);

	for (DWORD WriteSize = 0; WriteSize != FileSize; WriteSize += BufSize)
	{
		// Adjust buffer size
		SetBufSize(&BufSize, WriteSize, FileSize);

		// Output
		Read(&buf[0], BufSize);
		Decrypt(&buf[0], BufSize);
		WriteFile(&buf[0], BufSize);
	}
}

DWORD CArcFile::GetBufSize()
{
	return (m_pOption->BufSize << 10);
}

void CArcFile::SetBufSize(LPDWORD BufSize, DWORD WriteSize)
{
	SetBufSize(BufSize, WriteSize, GetOpenFileInfo()->sizeOrg);
}

void CArcFile::SetBufSize(LPDWORD BufSize, DWORD WriteSize, DWORD FileSize)
{
	if (WriteSize + *BufSize > FileSize)
		*BufSize = FileSize - WriteSize;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Additional file information
//
// Parameters:
//   - rfstFileInfo - File info to be added.

void CArcFile::AddFileInfo(SFileInfo& rfstFileInfo)
{
	if (rfstFileInfo.format == _T(""))
	{
		rfstFileInfo.format = SetFileFormat(rfstFileInfo.name);
	}

	rfstFileInfo.arcName = m_pclArcNames[m_dwArcsID];
	rfstFileInfo.arcID = m_dwArcID;
	rfstFileInfo.arcsID = m_dwArcsID;
	rfstFileInfo.sSizeOrg = SetCommaFormat(rfstFileInfo.sizeOrg);
	rfstFileInfo.sSizeCmp = SetCommaFormat(rfstFileInfo.sizeCmp);

	m_pEnt->push_back(rfstFileInfo);

	// Progress Update
	m_pProg->UpdatePercent(rfstFileInfo.sizeCmp);

	m_ctEnt++;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Additional file information
//
// Parameters:
//   - rfstFileInfo - File info to be added.
//   - dwFile       - File number
//   - pszFileExt   - Extension

void CArcFile::AddFileInfo(SFileInfo& rfstFileInfo, DWORD& dwFile, LPCTSTR pszFileExt)
{
	// Set filename
	TCHAR szFileName[_MAX_FNAME];
	_stprintf(szFileName, _T("%s_%06d%s"), GetArcName(), dwFile++, pszFileExt);
	rfstFileInfo.name = szFileName;

	if (rfstFileInfo.format == _T(""))
	{
		rfstFileInfo.format = SetFileFormat(rfstFileInfo.name);
	}

	rfstFileInfo.arcName = m_pclArcNames[m_dwArcsID];
	rfstFileInfo.arcID = m_dwArcID;
	rfstFileInfo.arcsID = m_dwArcsID;
	rfstFileInfo.sSizeOrg = SetCommaFormat(rfstFileInfo.sizeOrg);
	rfstFileInfo.sSizeCmp = SetCommaFormat(rfstFileInfo.sizeCmp);

	m_pEnt->push_back(rfstFileInfo);

	m_ctEnt++;
}

YCString CArcFile::SetFileFormat(const YCString& sFilePath)
{
	TCHAR szFileFormat[256];
	lstrcpy(szFileFormat, PathFindExtension(sFilePath));
	if (lstrcmp(szFileFormat, _T("")) == 0)
		return _T("");

	LPTSTR pszFileFormat = &szFileFormat[1];
	CharUpper(pszFileFormat);
	YCString FileFormat(pszFileFormat);

	return FileFormat;
}

// Function that separates every three digits in the filesize by commas
YCString CArcFile::SetCommaFormat(DWORD dwSize)
{
	TCHAR buf[256];
	_stprintf(buf, _T("%u"), dwSize);
	YCString sSize(buf);

	int len = sSize.GetLength();
	int comma_num = (len - 1) / 3;
	int comma_pos = len % 3;

	if (comma_pos == 0)
		comma_pos = 3;

	if (comma_num == 0)
		comma_pos = 0;

	for (int i = 0; i < comma_num; i++)
		sSize.Insert(comma_pos + 3 * i + i, _T(','));

	return sSize;
}

// Function to create the directories leading up to the lowest level you want to create
void CArcFile::MakeDirectory(LPCTSTR pFilePath)
{
	std::vector<YCString> sDirPathList;
	LPCTSTR pFilePathBase = pFilePath;

	while ((pFilePath = PathFindNextComponent(pFilePath)) != NULL)
	{
		YCString sDirPath(pFilePathBase, pFilePath - pFilePathBase - 1); // You do not put a '\' at the end just to be sure to -1
		sDirPathList.push_back(sDirPath);
	}

	// Create a directory in the order from the root
	for (size_t i = 0; i < sDirPathList.size() - 1; i++) // -1 so as not to create a directory of the file name
	{
		CreateDirectory(sDirPathList[i], NULL);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//

void CArcFile::ReplaceBackslash(LPTSTR pszPath)
{
	while (*pszPath != _T('\0'))
	{
		if (!::IsDBCSLeadByte(*pszPath))
		{
			// Half-width characters

			// \\ is back substitution

			if (*pszPath == _T('/'))
			{
				*pszPath = _T('\\');
			}
		}

		// To next character
		
		pszPath = CharNext(pszPath);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// Create output filename
//
// Parameters:
//   - pszRenameFileExt - Extension after rename

YCString CArcFile::CreateFileName(LPCTSTR pszRenameFileExt)
{
	// Get filename
	YCString clsFileName;

	// Extraction for each folder
	if (m_pOption->bCreateFolder)
	{
		clsFileName = m_pInfFile->name;

		// Replace '/' to '\'
		clsFileName.Replace(_T('/'), _T('\\'));
	}
	else // Copy only the filename
	{
		clsFileName = m_pInfFile->name.GetFileName();
	}

	// Get the file path
	YCString clsPathToFile;
	clsPathToFile.Format(_T("%s\\%s"), m_pSaveDir, clsFileName);

//  lstrcpy(szFilePath, m_pSaveDir);
//  PathAddBackslash(szFilePath);

//  lstrcat(szFilePath, szFileName);

	// Changing the extension
	if (pszRenameFileExt != NULL)
	{
		clsPathToFile.RenameExtension(pszRenameFileExt);
		clsPathToFile.Replace(_T('/'), _T('\\'));
	}

	// Limited to MAX_PATH
	clsPathToFile.Delete(MAX_PATH, clsPathToFile.GetLength());

	// Create directory
	MakeDirectory(clsPathToFile);

	// Overwrite confirmation
	if (PathFileExists(clsPathToFile))
	{
		CExistsDialog clExistsDlg;

		clExistsDlg.DoModal(m_pProg->GetHandle(), clsPathToFile);
	}

	m_clsPathToFile = clsPathToFile;

	return m_clsPathToFile;
}

DWORD CArcFile::ConvEndian(DWORD value)
{
	_asm
	{
		mov   eax, value
		bswap eax
		mov   value, eax
	}

	return value;
}

WORD CArcFile::ConvEndian(WORD value)
{
	_asm
	{
		ror value, 8
	}

	return value;
}

void CArcFile::ConvEndian(LPDWORD value)
{
	*value = ConvEndian(*value);
}

void CArcFile::ConvEndian(LPWORD value)
{
	*value = ConvEndian(*value);
}

BOOL CArcFile::CheckExe(LPCTSTR pExeName)
{
	// Gets the path to the executable file
	TCHAR szExePath[MAX_PATH];
	lstrcpy(szExePath, GetArcPath());
	PathRemoveFileSpec(szExePath);
	PathAppend(szExePath, pExeName);

	return PathFileExists(szExePath);
}

BOOL CArcFile::CheckDir(LPCTSTR pDirName)
{
	// Get directory name
	TCHAR szDirPath[MAX_PATH];
	lstrcpy(szDirPath, GetArcPath());
	PathRemoveFileSpec(szDirPath);

	if (lstrcmp(PathFindFileName(szDirPath), pDirName) == 0)
		return TRUE;

	return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Return information to the appropriate file from the file name
//
// Note: Linear search
//
// Parameters:
//   - pszFileName      - Target file name
//   - bCmpFileNameOnly - Whether or not to compare just the file names

SFileInfo* CArcFile::GetFileInfo(LPCTSTR pszFileName, BOOL bCmpFileNameOnly)
{
	if (bCmpFileNameOnly)
	{
		// Comparison in the requested file name only

		pszFileName = PathFindFileName(pszFileName);
	}

	for (size_t i = 0; i < m_pEnt->size(); i++)
	{
		LPCTSTR pszWork = (*m_pEnt)[i].name;

		if (bCmpFileNameOnly)
		{
			// Comparison in the requested file name only

			pszWork = PathFindFileName(pszWork);
		}

		if (lstrcmpi(pszWork, pszFileName) == 0)
		{
			return &(*m_pEnt)[i];
		}
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Return information to the appropriate file from the file name
//
// Note: Binary Search
//
// Parameters:
//   - pszFileName - Target file name

const SFileInfo* CArcFile::GetFileInfoForBinarySearch(LPCTSTR pszFileName)
{
	// Information is stored in files that are sorted by file name

	if (m_vcFileInfoOfFileNameSorted.empty())
	{
		// Uninitialized

		size_t uFileInfoStartIndex = GetStartEnt();
		size_t uFileInfoCount = GetCtEnt();

		m_vcFileInfoOfFileNameSorted.resize(uFileInfoCount);

		for (size_t i = 0, j = uFileInfoStartIndex; i < uFileInfoCount; i++, j++)
		{
			m_vcFileInfoOfFileNameSorted[i] = (*m_pEnt)[j];
		}

		std::sort(m_vcFileInfoOfFileNameSorted.begin(), m_vcFileInfoOfFileNameSorted.end(), CompareForFileInfo);
	}

	// Binary Search

	return SearchForFileInfo(m_vcFileInfoOfFileNameSorted, pszFileName);
}

void CArcFile::SetMD5(SMD5 stmd5File)
{
	m_vtstmd5File.push_back(stmd5File);
}

void CArcFile::ClearMD5()
{
	m_bSetMD5 = FALSE;
	m_vtstmd5File.clear();
}

//////////////////////////////////////////////////////////////////////////////////////////
// Comparison function for sorting
//
// Parameters:
//   - rstfiTarget1 - Left 
//   - rstfiTarget2 - Right

BOOL CArcFile::CompareForFileInfo(const SFileInfo& rstfiTarget1, const SFileInfo& rstfiTarget2)
{
	return (lstrcmpi(rstfiTarget1.name, rstfiTarget2.name) < 0);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Binary Search
//
// Parameters:
//   - rvcFileInfo - File info list
//   - pszFileName - Target filename

SFileInfo* CArcFile::SearchForFileInfo(std::vector<SFileInfo>& rvcFileInfo, LPCTSTR pszFileName)
{
	int nLow = 0;
	int nHigh = (rvcFileInfo.size() - 1);

	while (nLow <= nHigh)
	{
		int nMid = ((nHigh + nLow) / 2);
		int nResult = lstrcmpi(pszFileName, rvcFileInfo[nMid].name);

		if (nResult == 0)
		{
			return &rvcFileInfo[nMid];
		}

		if (nResult > 0)
		{
			nLow = (nMid + 1);
			continue;
		}

		if (nResult < 0)
		{
			nHigh = (nMid - 1);
			continue;
		}
	}

	return NULL;
}
