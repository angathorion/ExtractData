#include "stdafx.h"
#include "../ExtractBase.h"
#include "../Image.h"
#include "Tga.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Decoding
//
// Parameters:
//   - pclArc            - Archive
//   - pvSrc             - TGA data
//   - dwSrcSize         - TGA data size
//   - rfclsFileLastName - End of the filename
//
BOOL CTga::Decode(CArcFile* pclArc, const void* pvSrc, DWORD dwSrcSize, const YCString& rfclsFileLastName)
{
	const BYTE*       pbtSrc = (const BYTE*)pvSrc;
	const STGAHeader* psttgahSrc = (STGAHeader*)pvSrc;

	pbtSrc += sizeof(STGAHeader);
	dwSrcSize -= sizeof(STGAHeader);

	// Decompression

	YCMemory<BYTE> clmbtSrc2;

	switch (psttgahSrc->btImageType)
	{
	case 9:
	case 10: // RLE Compression

		DWORD dwSrcSize2 = ((psttgahSrc->wWidth * (psttgahSrc->btDepth >> 3) + 3) & 0xFFFFFFFC) * psttgahSrc->wHeight;
		clmbtSrc2.resize(dwSrcSize2);

		DecompRLE(&clmbtSrc2[0], dwSrcSize2, pbtSrc, dwSrcSize, psttgahSrc->btDepth);

		pbtSrc = &clmbtSrc2[0];
		dwSrcSize = dwSrcSize2;
		break;
	}

	CImage clImage;

	if (psttgahSrc->btDepth == 0)
	{
		clImage.Init(pclArc, psttgahSrc->wWidth, psttgahSrc->wHeight, 32, NULL, 0, rfclsFileLastName);
		clImage.WriteReverse(pbtSrc, dwSrcSize);
		clImage.Close();
	}
	else
	{
		clImage.Init(pclArc, psttgahSrc->wWidth, psttgahSrc->wHeight, psttgahSrc->btDepth, NULL, 0, rfclsFileLastName);
		clImage.Write(pbtSrc, dwSrcSize);
		clImage.Close();
	}

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Decompression of compressed TGAs
//
// Parameters:
//   - pvDst     - Storage location
//   - dwDstSize - Storage location size
//   - pvSrc     - Compressed TGA data
//   - dwSrcSize - Compressed TGA data size
//
BOOL CTga::Decomp(void* pvDst, DWORD dwDstSize, const void* pvSrc, DWORD dwSrcSize)
{
	BYTE*             pbtDst = (BYTE*)pvDst;
	const BYTE*       pbtSrc = (const BYTE*)pvSrc;
	const STGAHeader* psttgahSrc = (STGAHeader*)pvSrc;

	pbtSrc += sizeof(STGAHeader);
	dwSrcSize -= sizeof(STGAHeader);

	// Decompression
	switch (psttgahSrc->btDepth)
	{
	case 9:
	case 10: // RLE
		DecompRLE(pbtDst, dwDstSize, pbtSrc, dwSrcSize, psttgahSrc->btDepth);
		break;

	default: // No Compression
		memcpy(pbtDst, pbtSrc, dwSrcSize);
		break;
	}

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////
// RLE Decompression
//
// Parameters:
//   - pvDst     - Storage location
//   - dwDstSize - Storage location size
//   - pvSrc     - Compressed data
//   - dwSrcSize - Compressed data size
//   - wBpp      - Number of bits
//
BOOL CTga::DecompRLE(void* pvDst, DWORD dwDstSize, const void* pvSrc, DWORD dwSrcSize, BYTE wBpp)
{
	const BYTE* pbtSrc = (const BYTE*)pvSrc;
	BYTE*       pbtDst = (BYTE*)pvDst;
	DWORD       dwSrcPtr = 0;
	DWORD       dwDstPtr = 0;
	WORD        wByteCount = (wBpp >> 3);

	while ((dwSrcPtr < dwSrcSize) && (dwDstPtr < dwDstSize))
	{
		DWORD dwLength = pbtSrc[dwSrcPtr++];

		if (dwLength & 0x80)
		{
			dwLength = (dwLength & 0x7F) + 1;

			for (DWORD i = 0; i < dwLength; i++)
			{
				memcpy(&pbtDst[dwDstPtr], &pbtSrc[dwSrcPtr], wByteCount);

				dwDstPtr += wByteCount;
			}

			dwSrcPtr += wByteCount;
		}
		else
		{
			dwLength++;

			dwLength *= wByteCount;

			memcpy(&pbtDst[dwDstPtr], &pbtSrc[dwSrcPtr], dwLength);

			dwSrcPtr += dwLength;
			dwDstPtr += dwLength;
		}

		if (memcmp(&pbtSrc[dwSrcPtr + 8], "TRUEVISION-XFILE", 16) == 0)
		{
			// End

			break;
		}
	}

	return TRUE;
}
