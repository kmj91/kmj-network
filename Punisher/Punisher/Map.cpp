#include "stdafx.h"
#include "Map.h"
#include "CSpriteDib.h"

#define dfDELAY_EFFECT	3

// ��������Ʈ, ȭ�� ����, ȭ�� ����, �÷���Ʈ, �� ���� ����, �� ���� ����, Ÿ�� ������, ������Ʈ Ÿ��,
Map::Map(CSpriteDib *Sprite, int iWidth, int iHeight, int iColorBit, int iRight, int iBottom, int iTileSize, int iType) {
	CreateMap(Sprite, iWidth, iHeight, iColorBit, iRight, iBottom, iTileSize, iType);
}

Map::~Map() {
	ReleaseMap();
}


bool Map::Action() {
	if (m_iCameraPosX < testX) {
		SetArrayMap();
		return false;
	}

	if (testXX <= m_iCameraPosX) {
		SetArrayMap();
		return false;
	}

	if (m_iCameraPosY < testY) {
		SetArrayMap();
		return false;
	}

	if (testYY <= m_iCameraPosY) {
		SetArrayMap();
		return false;
	}

	return false;
}

void Map::Draw(CSpriteDib *Sprite, BYTE *bypDest, int iDestWidth, int iDestHeight, int iDestPitch, HWND hWnd) {
	//Sprite->DrawImage(m_spriteIndex, m_X, m_Y, bypDest, iDestWidth, iDestHeight, iDestPitch);
	//Sprite->DrawImage(0, 0, 0, bypDest, iDestWidth, iDestHeight, iDestPitch);

	//memset(bypDest, 0xff, iDestHeight * iDestPitch);

	int iCountY;
	int iCountX;
	int iDrawY = testY;
	int iDrawX = testX;
	int iSpriteHeight = m_iBuffHeight;
	int iSpriteWidth = m_iBuffWidth;
	int iSpritePitch = m_iBuffPitch;
	DWORD *dwpSprite = (DWORD *)m_byMapBuffer;
	DWORD *dwpDest = (DWORD *)bypDest;
	BYTE *bypTempSprite;
	BYTE *bypTempDest;

	//-----------------------------------------------------------------
	// ��� �� ���� ��������Ʈ ��� ��ġ ���. (��� Ŭ����)
	// �ϴܿ� ������ ���. (�ϴ� Ŭ����)
	// ���� ��� ��ġ ���. (���� Ŭ����)
	// ������ ��� ��ġ ���. (���� Ŭ����)
	//-----------------------------------------------------------------

	//ī�޶� �������� ��������Ʈ �� Ŭ���� ó��
	int clippingY;
	int clippingX;

	if (iDrawY < m_iCameraPosY) {
		clippingY = m_iCameraPosY - iDrawY;
		iSpriteHeight = iSpriteHeight - clippingY;
		if (iSpriteHeight > 0) {
			dwpSprite = (DWORD *)((BYTE *)dwpSprite + m_iBuffPitch * clippingY);
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


void Map::SetCameraPosition(int iPosX, int iPosY) {
	m_iCameraPosX = iPosX;
	m_iCameraPosY = iPosY;
}

//�� ����
void Map::CreateMap(CSpriteDib *Sprite, int iWidth, int iHeight, int iColorBit, int iRight, int iBottom, int iTileSize, int iType) {
	int iBufferSize;
	m_Sprite = Sprite;
	m_X = 0;
	m_Y = 0;
	m_type = iType;
	m_iWidth = iWidth;
	m_iHeight = iHeight;
	m_iBuffWidth = iWidth + iTileSize;
	m_iBuffHeight = m_iHeight + (iTileSize * 2);
	m_iBuffPitch = (m_iBuffWidth * (iColorBit / 8) + 3) & ~3;
	iBufferSize = m_iBuffPitch * m_iBuffHeight;

	m_byMapBuffer = new BYTE[iBufferSize];
	m_iRight = iRight;
	m_iBottom = iBottom;
	m_iTileSize = iTileSize;

	testX = 0;
	testXX = iTileSize;
	testY = 0;
	testYY = iTileSize;
	m_iCameraPosX = 0;
	m_iCameraPosY = 0;

	// �ϵ��ڵ���
	memset(m_byArrWorldMap, 65, sizeof(m_byArrWorldMap));
	// �迭�� Ÿ�Ϲ�ȣ ����
	SetArrayMap();
}

// ������ ��
void Map::ReleaseMap() {
	delete[] m_byMapBuffer;
}

// �迭�� Ÿ�Ϲ�ȣ ����
void Map::SetArrayMap() {
	int iCntX;
	int iCntY;
	int iCntXX;
	int iCntYY;
	int iY;
	int iX;
	int iTempX;

	

	iY = 0;
	iCntY = m_iCameraPosY / m_iTileSize;
	iTempX = m_iCameraPosX / m_iTileSize;
	iCntYY = iCntY + 9;
	iCntXX = iTempX + 11;

	testX = iTempX * m_iTileSize;
	testXX = testX + m_iTileSize;
	testY = iCntY * m_iTileSize;
	testYY = testY + m_iTileSize;

	while (iCntY < iCntYY) {
		iX = 0;
		iCntX = iTempX;
		while (iCntX < iCntXX) {
			m_byArrMap[iY][iX] = m_byArrWorldMap[iCntY][iCntX];
			++iCntX;
			++iX;
		}
		++iCntY;
		++iY;
	}

	// �� �׸���
	UpdateMapBuffer();
}

// �� �׸���
void Map::UpdateMapBuffer() {
	int iCntY;
	int iCntX;

	int iCountY;
	int iCountX;
	int iSpriteHeight;
	int iSpriteWidth;
	int iSpritePitch;
	DWORD *dwpSprite;
	DWORD *dwpDest;
	BYTE *bypTempSprite;
	BYTE *bypTempDest;
	CSpriteDib::st_SPRITE *stpSprite;

	iCntY = 0;
	while (iCntY < 9) {
		iCntX = 0;
		while (iCntX < 11) {
			stpSprite = &m_Sprite->m_stpSprite[m_byArrMap[iCntY][iCntX]];
			
			iSpriteHeight = stpSprite->iHeight;
			iSpriteWidth = stpSprite->iWidth;
			iSpritePitch = stpSprite->iPitch;
			dwpSprite = (DWORD *)stpSprite->bypImage;
			//dwpDest = (DWORD *)((BYTE *)(dwpDest + iDrawX) + (iDrawY * iDestPitch));
			dwpDest = (DWORD *)((m_byMapBuffer + (iCntX * m_iTileSize * 4)) + ((iCntY * m_iTileSize) * m_iBuffPitch));
			bypTempSprite = (BYTE *)dwpSprite;
			bypTempDest = (BYTE *)dwpDest;

			//-----------------------------------------------------------------
			// ��ü ũ�⸦ ���鼭 �ȼ����� ����� ó���� �ϸ� �׸� ���.
			//-----------------------------------------------------------------
			iCountY = 0;
			while (iSpriteHeight > iCountY) {
				iCountX = 0;
				while (iSpriteWidth > iCountX) {
					*dwpDest = *dwpSprite;

					//����ĭ �̵�
					++dwpDest;
					++dwpSprite;

					++iCountX;
				}
				//-----------------------------------------------------------------
				// �����ٷ� �̵�, ȭ��� ��������Ʈ ���..
				//-----------------------------------------------------------------
				bypTempDest = bypTempDest + m_iBuffPitch;
				bypTempSprite = bypTempSprite + iSpritePitch;

				dwpDest = (DWORD *)bypTempDest;
				dwpSprite = (DWORD *)bypTempSprite;

				++iCountY;
			}//while ��������Ʈ

			++iCntX;
		}
		++iCntY;
	}//while Ÿ�� �迭
}