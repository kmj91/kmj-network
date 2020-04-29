//#pragma once

#ifndef __SPRITE_DIB__
#define __SPRITE_DIB__




class CSpriteDib
{
public:

	// **************************************************************** 
	// DIB ��������Ʈ ����ü. 
	//
	// ��������Ʈ �̹����� ������ ������ ����.
	// **************************************************************** 
	typedef struct st_SPRITE
	{
		BYTE	*bypImage;				// ��������Ʈ �̹��� ������.
		int		iWidth;					// Widht
		int		iHeight;				// Height
		int		iPitch;					// Pitch

		int		iCenterPointX;			// ���� X
		int		iCenterPointY;			// ���� Y
	}st_SPRITE;
	////////////////////////////////////////////////////////////////////
	// ������, �ı���.
	//
	// Parameters: (int)�ִ� ��������Ʈ ����. (DWORD)����Į��.
	////////////////////////////////////////////////////////////////////
	CSpriteDib(int iMaxSprite, DWORD dwColorKey);
	virtual ~CSpriteDib();

	///////////////////////////////////////////////////////
	// LoadDibSprite. 
	// BMP������ �о �ϳ��� ���������� �����Ѵ�.
	//
	///////////////////////////////////////////////////////
	BOOL LoadDibSprite(int iSpriteIndex, WCHAR *szFileName, int iCenterPointX, int iCenterPointY);

	///////////////////////////////////////////////////////
	// ReleaseSprite. 
	// �ش� ��������Ʈ ����.
	//
	///////////////////////////////////////////////////////
	void ReleaseSprite(int iSpriteIndex);

	///////////////////////////////////////////////////////
	// DrawSprite. 
	// Ư�� �޸� ��ġ�� ��������Ʈ�� ����Ѵ�. (Į��Ű, Ŭ���� ó��)
	//
	///////////////////////////////////////////////////////
	void DrawSprite(int iSpriteIndex, int iDrawX, int iDrawY, BYTE *bypDest, int iDestWidth,
		int iDestHeight, int iDestPitch);
	///////////////////////////////////////////////////////
	// DrawImage. 
	// Ư�� �޸� ��ġ�� �̹����� ����Ѵ�. (Ŭ���� ó��)
	//
	///////////////////////////////////////////////////////
	void DrawImage(int iSpriteIndex, int iDrawX, int iDrawY, BYTE *bypDest, int iDestWidth,
		int iDestHeight, int iDestPitch);

	//������ ������ ���
	void DrawSpriteRed(int iSpriteIndex, int iDrawX, int iDrawY, BYTE *bypDest, int iDestWidth,
		int iDestHeight, int iDestPitch);

	//�ռ� ���
	void DrawSpriteCompose(int iSpriteIndex, int iDrawX, int iDrawY, BYTE *bypDest, int iDestWidth,
		int iDestHeight, int iDestPitch);

	//ü�¹� ��
	void DrawPercentage(int iSpriteIndex, int iDrawX, int iDrawY, BYTE *bypDest, int iDestWidth,
		int iDestHeight, int iDestPitch, int iPercentage = 100);

	// �׽�Ʈ ��
	void DrawTextID(int iSpriteIndex, int iDrawX, int iDrawY, BYTE *bypDest, int iDestWidth,
		int iDestHeight, int iDestPitch, HWND hWnd, UINT id);


	//ī�޶� ��ġ
	void SetCameraPosition(int iPosX, int iPosY);

protected:

	//------------------------------------------------------------------
	// Sprite �迭 ����.
	//------------------------------------------------------------------
	int		m_iMaxSprite;

public:
	st_SPRITE	*m_stpSprite;

protected:
	//------------------------------------------------------------------
	// ���� �������� ����� �÷�.
	//------------------------------------------------------------------
	DWORD		m_dwColorKey;

	//�÷��̾� ī�޶� ���� �� ������ 0,0 �κ� ��ǥ
	int	m_iCameraPosX;
	int m_iCameraPosY;
};

#endif
