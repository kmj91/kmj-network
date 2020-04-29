#include "stdafx.h"
#include <windows.h>
#include "CScreenDib.h"

CScreenDib::CScreenDib(int iWidth, int iHeight, int iColorBit) {
	//스크린 버퍼 생성
	CreateDibBuffer(iWidth, iHeight, iColorBit);
}

CScreenDib::~CScreenDib() {
	ReleaseDibBuffer();
}


void CScreenDib::CreateDibBuffer(int iWidth, int iHeight, int iColorBit) {
	m_iWidth = iWidth;
	m_iHeight = iHeight;

	m_stDibInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_stDibInfo.bmiHeader.biWidth = iWidth;
	m_stDibInfo.bmiHeader.biHeight = -iHeight;
	m_stDibInfo.bmiHeader.biPlanes = 1;
	m_stDibInfo.bmiHeader.biBitCount = iColorBit;
	m_stDibInfo.bmiHeader.biSizeImage = 0;
	m_stDibInfo.bmiHeader.biClrUsed = 0;
	m_stDibInfo.bmiHeader.biClrImportant = 0;
	/*
	m_stDibInfo.bmiColors[0].rgbRed = 0;
	m_stDibInfo.bmiColors[0].rgbGreen = 0;
	m_stDibInfo.bmiColors[0].rgbBlue = 0;
	m_stDibInfo.bmiColors[0].rgbReserved = 0;
	*/

	m_iPitch = (iWidth * (iColorBit / 8) + 3) & ~3;
	m_iBufferSize = m_iPitch * m_iHeight;

	m_bypBuffer = (BYTE*)malloc(m_iBufferSize);
}


void CScreenDib::ReleaseDibBuffer(void) {
	free(m_bypBuffer);
}


void CScreenDib::DrawBuffer(HWND hWnd, int iX, int iY) {
	HDC hdc;
	RECT DCRect;

	GetClientRect(hWnd, &DCRect);
	hdc = GetDC(hWnd);

	StretchDIBits(hdc, iX, iY, m_iWidth, m_iHeight, iX, iY, m_iWidth, m_iHeight, m_bypBuffer,
					&m_stDibInfo, DIB_RGB_COLORS, SRCCOPY);
	
	ReleaseDC(hWnd, hdc);
}


BYTE * CScreenDib::GetDibBuffer(void) {
	return m_bypBuffer;
}


int CScreenDib::GetWidth(void) {
	return m_iWidth;
}


int CScreenDib::GetHeight(void) {
	return m_iHeight;
}


int CScreenDib::GetPitch(void) {
	return m_iPitch;
}