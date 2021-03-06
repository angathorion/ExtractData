#include "stdafx.h"
#include "Common.h"
#include "File.h"

CFile::CFile()
{
	m_hFile = INVALID_HANDLE_VALUE;
}

CFile::~CFile()
{
	Close();
}

HANDLE CFile::Open(LPCTSTR pFileName, DWORD Mode)
{
	if (Mode == FILE_READ)
	{
		m_hFile = CreateFile(pFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	else if (Mode == FILE_WRITE)
	{
		m_hFile = CreateFile(pFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	}

	return m_hFile;
}

BOOL CFile::OpenForRead(LPCTSTR pszFileName)
{
	return (Open(pszFileName, FILE_READ) != INVALID_HANDLE_VALUE);
}

BOOL CFile::OpenForWrite(LPCTSTR pszFileName)
{
	return (Open(pszFileName, FILE_WRITE) != INVALID_HANDLE_VALUE);
}

void CFile::Close()
{
	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}
}

DWORD CFile::Read(LPVOID buf, DWORD size)
{
	DWORD dwReadSize;
	ReadFile(m_hFile, buf, size, &dwReadSize, NULL);
	return dwReadSize;
}

DWORD CFile::ReadLine(LPVOID buf, DWORD dwBufSize, BOOL bDeleteLineCode)
{
	LPBYTE pbyBuf = (LPBYTE)buf;
	LPBYTE pbyBufEnd = pbyBuf + dwBufSize;
	DWORD dwTotalReadSize = 0;

	while (true)
	{
		// Read one byte
		DWORD dwReadSize = Read(pbyBuf, 1);
		dwTotalReadSize += dwReadSize;

		if (*pbyBuf == '\n')
		{
			// Reached newline character

			if (bDeleteLineCode)
			{
				// Remove carriage return character
				if (*(pbyBuf - 1) == '\r')
					*(pbyBuf - 1) = '\0';
				*pbyBuf = '\0';
			}

			break;
		}

		if (dwReadSize == 0)
		{
			// Read until the end of the file (EOF).
			break;
		}

		pbyBuf++;

		if (pbyBuf >= pbyBufEnd)
		{
			// Filled the entire buffer.
			break;
		}
	}

	return dwTotalReadSize;
}

DWORD CFile::Write(LPCVOID buf, DWORD size)
{
	DWORD dwWriteSize;
	WriteFile(m_hFile, buf, size, &dwWriteSize, NULL);
	return dwWriteSize;
}

void CFile::WriteLine(LPCVOID buf)
{
	LPBYTE pbyBuf = (LPBYTE)buf;

	while (true)
	{
		if (*pbyBuf == '\0')
		{
			// Has reached the null termination character
			break;
		}

		// Writing one byte
		DWORD dwWriteSize = Write(pbyBuf, 1);

		if (dwWriteSize == 0)
		{
			// Could not be written
			break;
		}

		if (*pbyBuf == '\n')
		{
			// Reached newline character
			break;
		}

		pbyBuf++;
	}
}

QWORD CFile::Seek(INT64 offset, DWORD SeekMode)
{
	LARGE_INTEGER li;
	li.QuadPart = offset;
	li.LowPart = SetFilePointer(m_hFile, li.LowPart, &li.HighPart, SeekMode);

	if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
		li.QuadPart = -1;

	return (QWORD)(li.QuadPart);
/*
	LARGE_INTEGER li_offset;
	li_offset.QuadPart = offset;
	LARGE_INTEGER new_fp;
	SetFilePointerEx(m_hFile, li_offset, &new_fp, SeekMode);
	return (QWORD)(new_fp.QuadPart);
*/
}

QWORD CFile::SeekHed(INT64 offset)
{
	return Seek(offset, FILE_BEGIN);
}

QWORD CFile::SeekEnd(INT64 offset)
{
	return Seek(-offset, FILE_END);
}

QWORD CFile::SeekCur(INT64 offset)
{
	return Seek(offset, FILE_CURRENT);
}

QWORD CFile::GetFilePointer()
{
	return Seek(0, FILE_CURRENT);
/*
	LARGE_INTEGER zero;
	zero.QuadPart = 0;
	LARGE_INTEGER offset;
	SetFilePointerEx(m_hFile, zero, &offset, FILE_CURRENT);
	return (QWORD)(offset.QuadPart);
*/
}

QWORD CFile::GetFileSize()
{
	LARGE_INTEGER li;
	li.LowPart = ::GetFileSize(m_hFile, &(DWORD&)li.HighPart);
	return (QWORD)(li.QuadPart);
/*
	LARGE_INTEGER FileSize;
	GetFileSizeEx(m_hFile, &FileSize);
	return (QWORD)(FileSize.QuadPart);
*/
}
