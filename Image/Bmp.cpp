
#include	"stdafx.h"
#include	"../ExtractBase.h"
#include	"Bmp.h"

//////////////////////////////////////////////////////////////////////////////////////////
//	Mount

BOOL	CBmp::Mount(
	CArcFile*			pclArc							// Archive
	)
{
	if( lstrcmpi( pclArc->GetArcExten(), _T(".bmp") ) != 0 )
	{
		return	FALSE;
	}

	return	pclArc->Mount();
}

//////////////////////////////////////////////////////////////////////////////////////////
//	Initialization

BOOL	CBmp::OnInit(
	const YCString&		rfclsFileName					// Filename
	)
{
	// Set file header

	BITMAPFILEHEADER*	pstBMPFileHeader = &m_stBMPFileHeader;

	ZeroMemory( pstBMPFileHeader, sizeof(BITMAPFILEHEADER) );

	pstBMPFileHeader->bfType = 'MB';
	pstBMPFileHeader->bfSize = (54 + m_lPitch * m_lHeight);
	pstBMPFileHeader->bfReserved1 = 0;
	pstBMPFileHeader->bfReserved2 = 0;
	pstBMPFileHeader->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	if( m_wBpp == 8 )
	{
		// 8bit

		pstBMPFileHeader->bfSize += 1024;
		pstBMPFileHeader->bfOffBits += 1024;
	}

	// Set info header

	BITMAPINFOHEADER*	pstBMPInfoHeader = &m_stBMPInfoHeader;

	ZeroMemory( pstBMPInfoHeader, sizeof(BITMAPINFOHEADER) );

	pstBMPInfoHeader->biSize = sizeof(BITMAPINFOHEADER);
	pstBMPInfoHeader->biWidth = m_lWidth;
	pstBMPInfoHeader->biHeight = m_lHeight;
	pstBMPInfoHeader->biPlanes = 1;
	pstBMPInfoHeader->biBitCount = m_wBpp;
	pstBMPInfoHeader->biCompression = BI_RGB;
	pstBMPInfoHeader->biSizeImage = 0;
	pstBMPInfoHeader->biXPelsPerMeter = 0;
	pstBMPInfoHeader->biYPelsPerMeter = 0;
	pstBMPInfoHeader->biClrUsed = (m_wBpp == 8) ? 256 : 0;
	pstBMPInfoHeader->biClrImportant = 0;

	// Output BMP header

	WriteHed( rfclsFileName );

	return	TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////
//	Output BMP Header

void	CBmp::WriteHed(
	const YCString&		rfclsFileName					// Filename
	)
{
	CArcFile*			pclArc = m_pclArc;

	pclArc->OpenFile( rfclsFileName );

	// Output BMP header

	pclArc->WriteFile( &m_stBMPFileHeader, sizeof(m_stBMPFileHeader), 0 );
	pclArc->WriteFile( &m_stBMPInfoHeader, sizeof(m_stBMPInfoHeader), 0 );

	// Output palette

	if( m_stBMPInfoHeader.biBitCount == 8 )
	{
		// 8bit

		pclArc->WriteFile( m_astPallet, sizeof(m_astPallet), 0 );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//	Create Palette

BOOL	CBmp::OnCreatePallet(
	const void*			pvSrcPallet,					// Source (referenced) palette
	DWORD				dwSrcPalletSize					// Source (referenced) palette size
	)
{
	RGBQUAD*			pstPallet = m_astPallet;
	const BYTE*			pbtSrcPallet = (const BYTE*) pvSrcPallet;

	ZeroMemory( m_astPallet, sizeof(m_astPallet) );

	if( pbtSrcPallet == NULL )
	{
		// Use default palette (Grayscale)

		for( int i = 0 ; i < 256 ; i++ )
		{
			pstPallet[i].rgbBlue = i;
			pstPallet[i].rgbGreen = i;
			pstPallet[i].rgbRed = i;
			pstPallet[i].rgbReserved = 0;
		}
	}
	else
	{
		// Source palette

		if( dwSrcPalletSize == 1024 )
		{
			// 1024 byte palette

			for( int i = 0 ; i < 256 ; i++ )
			{
				pstPallet[i].rgbBlue = *pbtSrcPallet++;
				pstPallet[i].rgbGreen = *pbtSrcPallet++;
				pstPallet[i].rgbRed = *pbtSrcPallet++;
				pstPallet[i].rgbReserved = *pbtSrcPallet++;
			}
		}
		else if( dwSrcPalletSize == 768 )
		{
			// 768 byte palette (No alpha)

			for( int i = 0 ; i < 256 ; i++ )
			{
				pstPallet[i].rgbBlue = *pbtSrcPallet++;
				pstPallet[i].rgbGreen = *pbtSrcPallet++;
				pstPallet[i].rgbRed = *pbtSrcPallet++;
				pstPallet[i].rgbReserved = 0;
			}
		}
		else
		{
			// Other palette sizes

			for( DWORD i = 0 ; i < dwSrcPalletSize / 4 ; i++ )
			{
				pstPallet[i].rgbBlue = *pbtSrcPallet++;
				pstPallet[i].rgbGreen = *pbtSrcPallet++;
				pstPallet[i].rgbRed = *pbtSrcPallet++;
				pstPallet[i].rgbReserved = *pbtSrcPallet++;
			}
		}
	}

	return	TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////
//	Write 1 Line

void	CBmp::WriteLine(
	const void*			pvBuffer						// Buffer
	)
{
	// Output

	m_pclArc->WriteFile( pvBuffer, m_lLine, m_dwRowSize );

	// Output dummy data

	if( m_bOutputDummyFromBuffer )
	{
		// Output from buffer

		const BYTE*			pbtBuffer = (const BYTE*) pvBuffer;

		m_pclArc->WriteFile( &pbtBuffer[m_lLine], (m_lPitch - m_lLine), 0 );
	}
	else
	{
		for( long i = m_lLine ; i < m_lPitch ; i++ )
		{
			m_pclArc->WriteFile( "\0", 1, 0 );
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//	Write a line with alpha blending

void	CBmp::WriteLineWithAlphaBlend(
	void*				pvBuffer24,						// Data storage destination alpha blending
	const void*			pvBuffer32						// 32bit Data
	)
{
	// Fill with 0's (So that the dummy data can be outputted)

	ZeroMemory( pvBuffer24, m_lPitch );

	// Alpha blending

	AlphaBlend( pvBuffer24, pvBuffer32 );

	// Output

	m_pclArc->WriteFile( pvBuffer24, m_lPitch, m_dwRowSize );
}
