#include "stdafx.h"
#include <windows.h>
#include "CSpriteDib.h"
#include "stdio.h"

////////////////////////////////////////////////////////////////////
// ������, �ı���.
//
// Parameters: (int)�ִ� ��������Ʈ ����. (DWORD)����Į��.
////////////////////////////////////////////////////////////////////
CSpriteDib::CSpriteDib(int iMaxSprite, DWORD dwColorKey)
{
	//-----------------------------------------------------------------
	// �ִ� �о�� ���� ��ŭ �̸� �Ҵ� �޴´�.
	//-----------------------------------------------------------------
	m_iMaxSprite = iMaxSprite;
	m_dwColorKey = dwColorKey;
	m_stpSprite = new st_SPRITE[iMaxSprite];

	//�⺻ ī�޶� ��ġ
	m_iCameraPosX = 0;
	m_iCameraPosY = 0;
}

CSpriteDib::~CSpriteDib()
{
	int iCount;
	//-----------------------------------------------------------------
	// ��ü�� ���鼭 ��� ������.
	//-----------------------------------------------------------------
	for (iCount = 0; iCount < m_iMaxSprite; iCount++)
	{
		ReleaseSprite(iCount);
	}
	delete[] m_stpSprite;
}

///////////////////////////////////////////////////////
// LoadDibSprite. 
// BMP������ �о �ϳ��� ���������� �����Ѵ�.
//
// Parameters: (int)SpriteIndex. (char *)FileName. (int)CenterPointX. (int)CenterPointY.
// Return: (BOOL)TRUE, FALSE.
///////////////////////////////////////////////////////
BOOL CSpriteDib::LoadDibSprite(int iSpriteIndex, WCHAR *szFileName, int iCenterPointX,
	int iCenterPointY)
{
	//HANDLE		hFile;
	//DWORD		dwRead;
	int		iPitch;
	size_t		iImageSize;
	BITMAPFILEHEADER	stFileHeader;
	BITMAPINFOHEADER	stInfoHeader;
	
	FILE *fp;
	int err;
	int iCount;
	//-----------------------------------------------------------------
	// ��Ʈ�� ����� ���� BMP ���� Ȯ��.
	//-----------------------------------------------------------------
	err = _wfopen_s(&fp, szFileName, L"rb");
	if (err != 0) {
		return 0;
	}
	fread(&stFileHeader, sizeof(BITMAPFILEHEADER), 1, fp);
	if (stFileHeader.bfType != 0x4d42) {
		return 0;
	}
	
	//-----------------------------------------------------------------
	// ����, ������ ��ġ���� ���Ѵ�.
	//-----------------------------------------------------------------
	fread(&stInfoHeader, sizeof(BITMAPINFOHEADER), 1, fp);
	iPitch = (stInfoHeader.biWidth * (stInfoHeader.biBitCount / 8) + 3) & ~3;

	//-----------------------------------------------------------------
	// ��������Ʈ ����ü�� ũ�� ����.
	//-----------------------------------------------------------------
	m_stpSprite[iSpriteIndex].iWidth = stInfoHeader.biWidth;
	m_stpSprite[iSpriteIndex].iHeight = stInfoHeader.biHeight;
	m_stpSprite[iSpriteIndex].iPitch = iPitch;
	m_stpSprite[iSpriteIndex].iCenterPointX = iCenterPointX;
	m_stpSprite[iSpriteIndex].iCenterPointY = iCenterPointY;

	//-----------------------------------------------------------------
	// �̹����� ���� ��ü ũ�⸦ ���ϰ�, �޸� �Ҵ�.
	//-----------------------------------------------------------------
	iImageSize = iPitch * stInfoHeader.biHeight;
	m_stpSprite[iSpriteIndex].bypImage = new BYTE[iImageSize];

	//-----------------------------------------------------------------
	// �̹��� �κ��� ��������Ʈ ���۷� �о�´�.
	// DIB �� �������� �����Ƿ� �̸� �ٽ� ������.
	// �ӽ� ���ۿ� ���� �ڿ� �������鼭 �����Ѵ�.
	//-----------------------------------------------------------------
	BYTE *buffer = new BYTE[iImageSize];
	BYTE *tempBuffer;
	BYTE *tempSprite = m_stpSprite[iSpriteIndex].bypImage;
	fread(buffer, iImageSize, 1, fp);
	tempBuffer = buffer +(iPitch * (stInfoHeader.biHeight - 1));

	for (iCount = 0; iCount < stInfoHeader.biHeight; iCount++) {
		memcpy(tempSprite, tempBuffer, stInfoHeader.biWidth * 4);
		tempBuffer = tempBuffer - iPitch;
		tempSprite = tempSprite + iPitch;
	}

	delete[] buffer;
	fclose(fp);
	//CloseHandle(hFile);
	
	return FALSE;
}
///////////////////////////////////////////////////////
// ReleaseSprite. 
// �ش� ��������Ʈ ����.
//
// Parameters: (int)SpriteIndex.
// Return: (BOOL)TRUE, FALSE.
///////////////////////////////////////////////////////
void CSpriteDib::ReleaseSprite(int iSpriteIndex)
{
	//-----------------------------------------------------------------
	// �ִ� �Ҵ�� ��������Ʈ�� �Ѿ�� �ȵ�����
	//-----------------------------------------------------------------
	if (m_iMaxSprite <= iSpriteIndex)
		return;

	if (NULL != m_stpSprite[iSpriteIndex].bypImage)
	{
		//-----------------------------------------------------------------
		// ���� �� �ʱ�ȭ.
		//-----------------------------------------------------------------
		delete[] m_stpSprite[iSpriteIndex].bypImage;
		//memset(&m_stpSprite[iSpriteIndex], 0, sizeof(st_SPRITE));
	}
}

///////////////////////////////////////////////////////
// DrawSprite. 
// Ư�� �޸� ��ġ�� ��������Ʈ�� ����Ѵ�. (Ŭ���� ó��)
// ü�¹� ó��
//
///////////////////////////////////////////////////////
void CSpriteDib::DrawSprite(int iSpriteIndex, int iDrawX, int iDrawY, BYTE *bypDest, int iDestWidth,
	int iDestHeight, int iDestPitch)
{
	//-----------------------------------------------------------------
	// �ִ� ��������Ʈ ������ �ʰ� �ϰų�, �ε���� �ʴ� ��������Ʈ��� ����
	//-----------------------------------------------------------------
	if (m_iMaxSprite < iSpriteIndex)
		return;

	if (m_stpSprite[iSpriteIndex].bypImage == NULL)
		return;

	//-----------------------------------------------------------------
	// ���������� �ʿ����� ����
	//-----------------------------------------------------------------
	st_SPRITE *stpSprite = &m_stpSprite[iSpriteIndex];

	int iCountY;
	int iCountX;
	int iSpriteHeight = stpSprite->iHeight;
	int iSpriteWidth = stpSprite->iWidth;
	int iSpritePitch = stpSprite->iPitch;
	int iSpriteCenterX = stpSprite->iCenterPointX;
	int iSpriteCenterY = stpSprite->iCenterPointY;
	DWORD *dwpSprite = (DWORD *)stpSprite->bypImage;
	DWORD *dwpDest = (DWORD *)bypDest;
	BYTE *bypTempSprite;
	BYTE *bypTempDest;

	//-----------------------------------------------------------------
	// ��� �������� ��ǥ ���
	//-----------------------------------------------------------------
	iDrawX = iDrawX - iSpriteCenterX;
	iDrawY = iDrawY - iSpriteCenterY;

	//-----------------------------------------------------------------
	// ��� �� ���� ��������Ʈ ��� ��ġ ���. (��� Ŭ����)
	// �ϴܿ� ������ ���. (�ϴ� Ŭ����)
	// ���� ��� ��ġ ���. (���� Ŭ����)
	// ������ ��� ��ġ ���. (���� Ŭ����)
	//-----------------------------------------------------------------

	//2018.02.08
	//ī�޶� �������� ��������Ʈ �� Ŭ���� ó��
	int clippingY;
	int clippingX;

	if (iDrawY < m_iCameraPosY) {
		//2018.02.08
		clippingY = m_iCameraPosY - iDrawY;
		iSpriteHeight = iSpriteHeight - clippingY;
		if (iSpriteHeight > 0) {
			dwpSprite = (DWORD *)((BYTE *)dwpSprite + iSpritePitch * clippingY);
		}
		iDrawY = m_iCameraPosY;
	}

	if (iDrawY + iSpriteHeight >= m_iCameraPosY + iDestHeight) {
		iSpriteHeight = iSpriteHeight - ((iDrawY + iSpriteHeight) - (m_iCameraPosY + iDestHeight));
	}

	if (iDrawX < m_iCameraPosX) {
		//2018.02.08
		clippingX = m_iCameraPosX - iDrawX;
		iSpriteWidth = iSpriteWidth - clippingX;
		if (iSpriteWidth > 0) {
			dwpSprite = dwpSprite + clippingX;
		}
		iDrawX = m_iCameraPosX;
	}

	if (iDrawX + iSpriteWidth >= m_iCameraPosX + iDestWidth) {
		iSpriteWidth = iSpriteWidth - ((iDrawX + iSpriteWidth) - (m_iCameraPosX + iDestWidth));
	}


	if (iSpriteWidth <= 0 || iSpriteHeight <= 0) {
		return;
	}

	//2018.02.08
	//ī�޶� �������� iDrawX iDrawY �� ��ǥ ���߱�
	iDrawX = iDrawX - m_iCameraPosX;
	iDrawY = iDrawY - m_iCameraPosY;

	//-----------------------------------------------------------------
	// ȭ�鿡 ���� ��ġ�� �̵��Ѵ�.
	//-----------------------------------------------------------------
	dwpDest = (DWORD *)((BYTE *)(dwpDest + iDrawX) + (iDrawY * iDestPitch));
	bypTempSprite = (BYTE *)dwpSprite;
	bypTempDest = (BYTE *)dwpDest;

	//-----------------------------------------------------------------
	// ��ü ũ�⸦ ���鼭 �ȼ����� ����� ó���� �ϸ� �׸� ���.
	//-----------------------------------------------------------------
	for (iCountY = 0; iSpriteHeight > iCountY; iCountY++)
	{
		for (iCountX = 0; iSpriteWidth > iCountX; iCountX++)
		{
			if (m_dwColorKey != (*dwpSprite & 0x00ffffff)) {
				*dwpDest = *dwpSprite;
			}
			
			//����ĭ �̵�
			++dwpDest;
			++dwpSprite;
		}
		//-----------------------------------------------------------------
		// �����ٷ� �̵�, ȭ��� ��������Ʈ ���..
		//-----------------------------------------------------------------
		bypTempDest = bypTempDest + iDestPitch;
		bypTempSprite = bypTempSprite + iSpritePitch;

		dwpDest = (DWORD *)bypTempDest;
		dwpSprite = (DWORD *)bypTempSprite;
	}
}


void CSpriteDib::DrawImage(int iSpriteIndex, int iDrawX, int iDrawY, BYTE *bypDest, int iDestWidth,
	int iDestHeight, int iDestPitch)
{
	//-----------------------------------------------------------------
	// �ִ� ��������Ʈ ������ �ʰ� �ϰų�, �ε���� �ʴ� ��������Ʈ��� ����
	//-----------------------------------------------------------------
	if (m_iMaxSprite < iSpriteIndex)
		return;

	if (m_stpSprite[iSpriteIndex].bypImage == NULL)
		return;
		

	//-----------------------------------------------------------------
	// ���������� �ʿ����� ����
	//-----------------------------------------------------------------
	st_SPRITE *stpSprite = &m_stpSprite[iSpriteIndex];

	int iCountY;
	int iCountX;
	int iSpriteHeight = stpSprite->iHeight;
	int iSpriteWidth = stpSprite->iWidth;
	int iSpritePitch = stpSprite->iPitch;
	int iSpriteCenterX = stpSprite->iCenterPointX;
	int iSpriteCenterY = stpSprite->iCenterPointY;
	DWORD *dwpSprite = (DWORD *)stpSprite->bypImage;
	DWORD *dwpDest = (DWORD *)bypDest;
	BYTE *bypTempSprite;
	BYTE *bypTempDest;

	//-----------------------------------------------------------------
	// ��� �������� ��ǥ ���
	//-----------------------------------------------------------------
	iDrawX = iDrawX - iSpriteCenterX;
	iDrawY = iDrawY - iSpriteCenterY;
	
	//-----------------------------------------------------------------
	// ��� �� ���� ��������Ʈ ��� ��ġ ���. (��� Ŭ����)
	// �ϴܿ� ������ ���. (�ϴ� Ŭ����)
	// ���� ��� ��ġ ���. (���� Ŭ����)
	// ������ ��� ��ġ ���. (���� Ŭ����)
	//-----------------------------------------------------------------
	
	//2018.02.08
	//ī�޶� �������� ��������Ʈ �� Ŭ���� ó��
	int clippingY;
	int clippingX;

	if (iDrawY < m_iCameraPosY) {
		//2018.02.08
		clippingY = m_iCameraPosY - iDrawY;
		iSpriteHeight = iSpriteHeight - clippingY;
		if (iSpriteHeight > 0) {
			dwpSprite = (DWORD *)((BYTE *)dwpSprite + iSpritePitch * clippingY);
		}
		iDrawY = m_iCameraPosY;
	}

	if (iDrawY + iSpriteHeight >= m_iCameraPosY + iDestHeight) {
		iSpriteHeight = iSpriteHeight - ((iDrawY + iSpriteHeight) - (m_iCameraPosY + iDestHeight));
	}

	if (iDrawX < m_iCameraPosX) {
		//2018.02.08
		clippingX = m_iCameraPosX - iDrawX;
		iSpriteWidth = iSpriteWidth - clippingX;
		if (iSpriteWidth > 0) {
			dwpSprite = dwpSprite + clippingX;
		}
		iDrawX = m_iCameraPosX;
	}

	if (iDrawX + iSpriteWidth >= m_iCameraPosX + iDestWidth) {
		iSpriteWidth = iSpriteWidth - ((iDrawX + iSpriteWidth) - (m_iCameraPosX + iDestWidth));
	}


	if (iSpriteWidth <= 0 || iSpriteHeight <= 0) {
		return;
	}

	//2018.02.08
	//ī�޶� �������� iDrawX iDrawY �� ��ǥ ���߱�
	iDrawX = iDrawX - m_iCameraPosX;
	iDrawY = iDrawY - m_iCameraPosY;

	//-----------------------------------------------------------------
	// ȭ�鿡 ���� ��ġ�� �̵��Ѵ�.
	//-----------------------------------------------------------------
	dwpDest = (DWORD *)((BYTE *)(dwpDest + iDrawX) + (iDrawY * iDestPitch));
	bypTempSprite = (BYTE *)dwpSprite;
	bypTempDest = (BYTE *)dwpDest;

	//-----------------------------------------------------------------
	// ��ü ũ�⸦ ���鼭 �ȼ����� �׸� ���.
	//-----------------------------------------------------------------
	for (iCountY = 0; iSpriteHeight > iCountY; iCountY++)
	{
		for (iCountX = 0; iSpriteWidth > iCountX; iCountX++)
		{
			*dwpDest = *dwpSprite;
			//����ĭ �̵�
			++dwpDest;
			++dwpSprite;
		}
		//-----------------------------------------------------------------
		// �����ٷ� �̵�, ȭ��� ��������Ʈ ���..
		//-----------------------------------------------------------------
		bypTempDest = bypTempDest + iDestPitch;
		bypTempSprite = bypTempSprite + iSpritePitch;
		
		dwpDest = (DWORD *)bypTempDest;
		dwpSprite = (DWORD *)bypTempSprite;
	}
	
	
}



/*********************************
	DrawSpriteRed
	������ ������ �̹��� ���
**********************************/
void CSpriteDib::DrawSpriteRed(int iSpriteIndex, int iDrawX, int iDrawY, BYTE *bypDest, int iDestWidth,
	int iDestHeight, int iDestPitch)
{
	//-----------------------------------------------------------------
	// �ִ� ��������Ʈ ������ �ʰ� �ϰų�, �ε���� �ʴ� ��������Ʈ��� ����
	//-----------------------------------------------------------------
	if (m_iMaxSprite < iSpriteIndex)
		return;

	if (m_stpSprite[iSpriteIndex].bypImage == NULL)
		return;

	//-----------------------------------------------------------------
	// ���������� �ʿ����� ����
	//-----------------------------------------------------------------
	st_SPRITE *stpSprite = &m_stpSprite[iSpriteIndex];

	int iCountY;
	int iCountX;
	int iSpriteHeight = stpSprite->iHeight;
	int iSpriteWidth = stpSprite->iWidth;
	int iSpritePitch = stpSprite->iPitch;
	int iSpriteCenterX = stpSprite->iCenterPointX;
	int iSpriteCenterY = stpSprite->iCenterPointY;
	DWORD *dwpSprite = (DWORD *)stpSprite->bypImage;
	DWORD *dwpDest = (DWORD *)bypDest;
	BYTE *bypTempSprite;
	BYTE *bypTempDest;

	//-----------------------------------------------------------------
	// ��� �������� ��ǥ ���
	//-----------------------------------------------------------------
	iDrawX = iDrawX - iSpriteCenterX;
	iDrawY = iDrawY - iSpriteCenterY;

	//-----------------------------------------------------------------
	// ��� �� ���� ��������Ʈ ��� ��ġ ���. (��� Ŭ����)
	// �ϴܿ� ������ ���. (�ϴ� Ŭ����)
	// ���� ��� ��ġ ���. (���� Ŭ����)
	// ������ ��� ��ġ ���. (���� Ŭ����)
	//-----------------------------------------------------------------

	//2018.02.08
	//ī�޶� �������� ��������Ʈ �� Ŭ���� ó��
	int clippingY;
	int clippingX;

	if (iDrawY < m_iCameraPosY) {
		//2018.02.08
		clippingY = m_iCameraPosY - iDrawY;
		iSpriteHeight = iSpriteHeight - clippingY;
		if (iSpriteHeight > 0) {
			dwpSprite = (DWORD *)((BYTE *)dwpSprite + iSpritePitch * clippingY);
		}
		iDrawY = m_iCameraPosY;
	}

	if (iDrawY + iSpriteHeight >= m_iCameraPosY + iDestHeight) {
		iSpriteHeight = iSpriteHeight - ((iDrawY + iSpriteHeight) - (m_iCameraPosY + iDestHeight));
	}

	if (iDrawX < m_iCameraPosX) {
		//2018.02.08
		clippingX = m_iCameraPosX - iDrawX;
		iSpriteWidth = iSpriteWidth - clippingX;
		if (iSpriteWidth > 0) {
			dwpSprite = dwpSprite + clippingX;
		}
		iDrawX = m_iCameraPosX;
	}

	if (iDrawX + iSpriteWidth >= m_iCameraPosX + iDestWidth) {
		iSpriteWidth = iSpriteWidth - ((iDrawX + iSpriteWidth) - (m_iCameraPosX + iDestWidth));
	}


	if (iSpriteWidth <= 0 || iSpriteHeight <= 0) {
		return;
	}

	//2018.02.08
	//ī�޶� �������� iDrawX iDrawY �� ��ǥ ���߱�
	iDrawX = iDrawX - m_iCameraPosX;
	iDrawY = iDrawY - m_iCameraPosY;

	//-----------------------------------------------------------------
	// ȭ�鿡 ���� ��ġ�� �̵��Ѵ�.
	//-----------------------------------------------------------------
	dwpDest = (DWORD *)((BYTE *)(dwpDest + iDrawX) + (iDrawY * iDestPitch));
	bypTempSprite = (BYTE *)dwpSprite;
	bypTempDest = (BYTE *)dwpDest;

	//-----------------------------------------------------------------
	// ��ü ũ�⸦ ���鼭 �ȼ����� ����� ó���� �ϸ� �׸� ���.
	//-----------------------------------------------------------------
	for (iCountY = 0; iSpriteHeight > iCountY; iCountY++)
	{
		for (iCountX = 0; iSpriteWidth > iCountX; iCountX++)
		{
			if (m_dwColorKey != (*dwpSprite & 0x00ffffff)) {
				*((BYTE *)dwpDest + 2) = *((BYTE *)dwpSprite + 2);
				*((BYTE *)dwpDest + 1) = *((BYTE *)dwpSprite + 1) / 2;
				*((BYTE *)dwpDest + 0) = *((BYTE *)dwpSprite + 0) / 2;
			}

			//����ĭ �̵�
			++dwpDest;
			++dwpSprite;
		}
		//-----------------------------------------------------------------
		// �����ٷ� �̵�, ȭ��� ��������Ʈ ���..
		//-----------------------------------------------------------------
		bypTempDest = bypTempDest + iDestPitch;
		bypTempSprite = bypTempSprite + iSpritePitch;

		dwpDest = (DWORD *)bypTempDest;
		dwpSprite = (DWORD *)bypTempSprite;
	}
}




/*********************************
	DrawSpriteCompose
	�ռ� ���
**********************************/
void CSpriteDib::DrawSpriteCompose(int iSpriteIndex, int iDrawX, int iDrawY, BYTE *bypDest, int iDestWidth,
	int iDestHeight, int iDestPitch)
{
	//-----------------------------------------------------------------
	// �ִ� ��������Ʈ ������ �ʰ� �ϰų�, �ε���� �ʴ� ��������Ʈ��� ����
	//-----------------------------------------------------------------
	if (m_iMaxSprite < iSpriteIndex)
		return;

	if (m_stpSprite[iSpriteIndex].bypImage == NULL)
		return;

	//-----------------------------------------------------------------
	// ���������� �ʿ����� ����
	//-----------------------------------------------------------------
	st_SPRITE *stpSprite = &m_stpSprite[iSpriteIndex];

	int iCountY;
	int iCountX;
	int iSpriteHeight = stpSprite->iHeight;
	int iSpriteWidth = stpSprite->iWidth;
	int iSpritePitch = stpSprite->iPitch;
	int iSpriteCenterX = stpSprite->iCenterPointX;
	int iSpriteCenterY = stpSprite->iCenterPointY;
	DWORD *dwpSprite = (DWORD *)stpSprite->bypImage;
	DWORD *dwpDest = (DWORD *)bypDest;
	BYTE *bypTempSprite;
	BYTE *bypTempDest;

	//-----------------------------------------------------------------
	// ��� �������� ��ǥ ���
	//-----------------------------------------------------------------
	iDrawX = iDrawX - iSpriteCenterX;
	iDrawY = iDrawY - iSpriteCenterY;

	//-----------------------------------------------------------------
	// ��� �� ���� ��������Ʈ ��� ��ġ ���. (��� Ŭ����)
	// �ϴܿ� ������ ���. (�ϴ� Ŭ����)
	// ���� ��� ��ġ ���. (���� Ŭ����)
	// ������ ��� ��ġ ���. (���� Ŭ����)
	//-----------------------------------------------------------------

	//2018.02.08
	//ī�޶� �������� ��������Ʈ �� Ŭ���� ó��
	int clippingY;
	int clippingX;

	if (iDrawY < m_iCameraPosY) {
		//2018.02.08
		clippingY = m_iCameraPosY - iDrawY;
		iSpriteHeight = iSpriteHeight - clippingY;
		if (iSpriteHeight > 0) {
			dwpSprite = (DWORD *)((BYTE *)dwpSprite + iSpritePitch * clippingY);
		}
		iDrawY = m_iCameraPosY;
	}

	if (iDrawY + iSpriteHeight >= m_iCameraPosY + iDestHeight) {
		iSpriteHeight = iSpriteHeight - ((iDrawY + iSpriteHeight) - (m_iCameraPosY + iDestHeight));
	}

	if (iDrawX < m_iCameraPosX) {
		//2018.02.08
		clippingX = m_iCameraPosX - iDrawX;
		iSpriteWidth = iSpriteWidth - clippingX;
		if (iSpriteWidth > 0) {
			dwpSprite = dwpSprite + clippingX;
		}
		iDrawX = m_iCameraPosX;
	}

	if (iDrawX + iSpriteWidth >= m_iCameraPosX + iDestWidth) {
		iSpriteWidth = iSpriteWidth - ((iDrawX + iSpriteWidth) - (m_iCameraPosX + iDestWidth));
	}


	if (iSpriteWidth <= 0 || iSpriteHeight <= 0) {
		return;
	}

	//2018.02.08
	//ī�޶� �������� iDrawX iDrawY �� ��ǥ ���߱�
	iDrawX = iDrawX - m_iCameraPosX;
	iDrawY = iDrawY - m_iCameraPosY;

	//-----------------------------------------------------------------
	// ȭ�鿡 ���� ��ġ�� �̵��Ѵ�.
	//-----------------------------------------------------------------
	dwpDest = (DWORD *)((BYTE *)(dwpDest + iDrawX) + (iDrawY * iDestPitch));
	bypTempSprite = (BYTE *)dwpSprite;
	bypTempDest = (BYTE *)dwpDest;

	//-----------------------------------------------------------------
	// ��ü ũ�⸦ ���鼭 �ȼ����� ����� ó���� �ϸ� �׸� ���.
	//-----------------------------------------------------------------
	for (iCountY = 0; iSpriteHeight > iCountY; iCountY++)
	{
		for (iCountX = 0; iSpriteWidth > iCountX; iCountX++)
		{
			if (m_dwColorKey != (*dwpSprite & 0x00ffffff)) {
				*((BYTE *)dwpDest + 2) = (*((BYTE *)dwpSprite + 2) / 2) + (*((BYTE *)dwpDest + 2) / 2);
				*((BYTE *)dwpDest + 1) = (*((BYTE *)dwpSprite + 1) / 2) + (*((BYTE *)dwpDest + 1) / 2);
				*((BYTE *)dwpDest + 0) = (*((BYTE *)dwpSprite + 0) / 2) + (*((BYTE *)dwpDest + 0) / 2);
			}

			//����ĭ �̵�
			++dwpDest;
			++dwpSprite;
		}
		//-----------------------------------------------------------------
		// �����ٷ� �̵�, ȭ��� ��������Ʈ ���..
		//-----------------------------------------------------------------
		bypTempDest = bypTempDest + iDestPitch;
		bypTempSprite = bypTempSprite + iSpritePitch;

		dwpDest = (DWORD *)bypTempDest;
		dwpSprite = (DWORD *)bypTempSprite;
	}
}

// ü�¹� ��
void CSpriteDib::DrawPercentage(int iSpriteIndex, int iDrawX, int iDrawY, BYTE *bypDest, int iDestWidth,
	int iDestHeight, int iDestPitch, int iPercentage) {
	//-----------------------------------------------------------------
	// �ִ� ��������Ʈ ������ �ʰ� �ϰų�, �ε���� �ʴ� ��������Ʈ��� ����
	//-----------------------------------------------------------------
	if (m_iMaxSprite < iSpriteIndex)
		return;

	if (m_stpSprite[iSpriteIndex].bypImage == NULL)
		return;

	//-----------------------------------------------------------------
	// ���������� �ʿ����� ����
	//-----------------------------------------------------------------
	st_SPRITE *stpSprite = &m_stpSprite[iSpriteIndex];

	int iCountY;
	int iCountX;
	int iSpriteHeight = stpSprite->iHeight;
	int iSpriteWidth = stpSprite->iWidth;
	int iSpritePitch = stpSprite->iPitch;
	int iSpriteCenterX = stpSprite->iCenterPointX;
	int iSpriteCenterY = stpSprite->iCenterPointY;
	DWORD *dwpSprite = (DWORD *)stpSprite->bypImage;
	DWORD *dwpDest = (DWORD *)bypDest;
	BYTE *bypTempSprite;
	BYTE *bypTempDest;

	//-----------------------------------------------------------------
	// ��� �������� ��ǥ ���
	//-----------------------------------------------------------------
	iDrawX = iDrawX - iSpriteCenterX;
	iDrawY = iDrawY - iSpriteCenterY;

	//-----------------------------------------------------------------
	// ��� �� ���� ��������Ʈ ��� ��ġ ���. (��� Ŭ����)
	// �ϴܿ� ������ ���. (�ϴ� Ŭ����)
	// ���� ��� ��ġ ���. (���� Ŭ����)
	// ������ ��� ��ġ ���. (���� Ŭ����)
	//-----------------------------------------------------------------

	//2018.02.08
	//ī�޶� �������� ��������Ʈ �� Ŭ���� ó��
	int clippingY;
	int clippingX;

	if (iDrawY < m_iCameraPosY) {
		//2018.02.08
		clippingY = m_iCameraPosY - iDrawY;
		iSpriteHeight = iSpriteHeight - clippingY;
		if (iSpriteHeight > 0) {
			dwpSprite = (DWORD *)((BYTE *)dwpSprite + iSpritePitch * clippingY);
		}
		iDrawY = m_iCameraPosY;
	}

	if (iDrawY + iSpriteHeight >= m_iCameraPosY + iDestHeight) {
		iSpriteHeight = iSpriteHeight - ((iDrawY + iSpriteHeight) - (m_iCameraPosY + iDestHeight));
	}

	if (iDrawX < m_iCameraPosX) {
		//2018.02.08
		clippingX = m_iCameraPosX - iDrawX;
		iSpriteWidth = iSpriteWidth - clippingX;
		if (iSpriteWidth > 0) {
			dwpSprite = dwpSprite + clippingX;
		}
		iDrawX = m_iCameraPosX;
	}

	if (iDrawX + iSpriteWidth >= m_iCameraPosX + iDestWidth) {
		iSpriteWidth = iSpriteWidth - ((iDrawX + iSpriteWidth) - (m_iCameraPosX + iDestWidth));
	}


	if (iSpriteWidth <= 0 || iSpriteHeight <= 0) {
		return;
	}

	//2018.02.08
	//ī�޶� �������� iDrawX iDrawY �� ��ǥ ���߱�
	iDrawX = iDrawX - m_iCameraPosX;
	iDrawY = iDrawY - m_iCameraPosY;

	//-----------------------------------------------------------------
	// ȭ�鿡 ���� ��ġ�� �̵��Ѵ�.
	//-----------------------------------------------------------------
	dwpDest = (DWORD *)((BYTE *)(dwpDest + iDrawX) + (iDrawY * iDestPitch));
	bypTempSprite = (BYTE *)dwpSprite;
	bypTempDest = (BYTE *)dwpDest;

	//-----------------------------------------------------------------
	// ��ü ũ�⸦ ���鼭 �ȼ����� ����� ó���� �ϸ� �׸� ���.
	//-----------------------------------------------------------------
	for (iCountY = 0; iSpriteHeight > iCountY; iCountY++)
	{
		for (iCountX = 0; iSpriteWidth - ((100 - iPercentage) * ((double)iSpriteWidth / (double)100)) > iCountX; iCountX++)
		{
			if (m_dwColorKey != (*dwpSprite & 0x00ffffff)) {
				*dwpDest = *dwpSprite;
			}

			//����ĭ �̵�
			++dwpDest;
			++dwpSprite;
		}
		//-----------------------------------------------------------------
		// �����ٷ� �̵�, ȭ��� ��������Ʈ ���..
		//-----------------------------------------------------------------
		bypTempDest = bypTempDest + iDestPitch;
		bypTempSprite = bypTempSprite + iSpritePitch;

		dwpDest = (DWORD *)bypTempDest;
		dwpSprite = (DWORD *)bypTempSprite;
	}
}

// �׽�Ʈ ��
void CSpriteDib::DrawTextID(int iSpriteIndex, int iDrawX, int iDrawY, BYTE *bypDest, int iDestWidth,
	int iDestHeight, int iDestPitch, HWND hWnd, UINT id) {
	//-----------------------------------------------------------------
	// �ִ� ��������Ʈ ������ �ʰ� �ϰų�, �ε���� �ʴ� ��������Ʈ��� ����
	//-----------------------------------------------------------------
	if (m_iMaxSprite < iSpriteIndex)
		return;

	if (m_stpSprite[iSpriteIndex].bypImage == NULL)
		return;

	//-----------------------------------------------------------------
	// ���������� �ʿ����� ����
	//-----------------------------------------------------------------
	st_SPRITE *stpSprite = &m_stpSprite[iSpriteIndex];

	int iSpriteHeight = stpSprite->iHeight;
	int iSpriteWidth = stpSprite->iWidth;
	int iSpritePitch = stpSprite->iPitch;
	int iSpriteCenterX = stpSprite->iCenterPointX;
	int iSpriteCenterY = stpSprite->iCenterPointY;
	DWORD *dwpSprite = (DWORD *)stpSprite->bypImage;
	DWORD *dwpDest = (DWORD *)bypDest;

	//-----------------------------------------------------------------
	// ��� �������� ��ǥ ���
	//-----------------------------------------------------------------
	iDrawX = iDrawX - iSpriteCenterX;
	iDrawY = iDrawY - iSpriteCenterY;

	//-----------------------------------------------------------------
	// ��� �� ���� ��������Ʈ ��� ��ġ ���. (��� Ŭ����)
	// �ϴܿ� ������ ���. (�ϴ� Ŭ����)
	// ���� ��� ��ġ ���. (���� Ŭ����)
	// ������ ��� ��ġ ���. (���� Ŭ����)
	//-----------------------------------------------------------------

	//2018.02.08
	//ī�޶� �������� ��������Ʈ �� Ŭ���� ó��
	int clippingY;
	int clippingX;

	if (iDrawY < m_iCameraPosY) {
		//2018.02.08
		clippingY = m_iCameraPosY - iDrawY;
		iSpriteHeight = iSpriteHeight - clippingY;
		if (iSpriteHeight > 0) {
			dwpSprite = (DWORD *)((BYTE *)dwpSprite + iSpritePitch * clippingY);
		}
		iDrawY = m_iCameraPosY;
	}

	if (iDrawY + iSpriteHeight >= m_iCameraPosY + iDestHeight) {
		iSpriteHeight = iSpriteHeight - ((iDrawY + iSpriteHeight) - (m_iCameraPosY + iDestHeight));
	}

	if (iDrawX < m_iCameraPosX) {
		//2018.02.08
		clippingX = m_iCameraPosX - iDrawX;
		iSpriteWidth = iSpriteWidth - clippingX;
		if (iSpriteWidth > 0) {
			dwpSprite = dwpSprite + clippingX;
		}
		iDrawX = m_iCameraPosX;
	}

	if (iDrawX + iSpriteWidth >= m_iCameraPosX + iDestWidth) {
		iSpriteWidth = iSpriteWidth - ((iDrawX + iSpriteWidth) - (m_iCameraPosX + iDestWidth));
	}


	if (iSpriteWidth <= 0 || iSpriteHeight <= 0) {
		return;
	}

	//2018.02.08
	//ī�޶� �������� iDrawX iDrawY �� ��ǥ ���߱�
	iDrawX = iDrawX - m_iCameraPosX;
	iDrawY = iDrawY - m_iCameraPosY;

	HDC hdc;
	RECT DCRect;

	GetClientRect(hWnd, &DCRect);
	hdc = GetDC(hWnd);

	WCHAR wchString[32];
	wsprintfW(wchString, L"%d", id);
	TextOut(hdc, iDrawX, iDrawY, wchString, wcslen(wchString));

	ReleaseDC(hWnd, hdc);
}



//ī�޶� ��ġ
void CSpriteDib::SetCameraPosition(int iPosX, int iPosY) {
	m_iCameraPosX = iPosX;
	m_iCameraPosY = iPosY;
}