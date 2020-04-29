//#pragma once

#ifndef __SPRITE_DIB__
#define __SPRITE_DIB__




class CSpriteDib
{
public:

	// **************************************************************** 
	// DIB 스프라이트 구조체. 
	//
	// 스프라이트 이미지와 사이즈 정보를 가짐.
	// **************************************************************** 
	typedef struct st_SPRITE
	{
		BYTE	*bypImage;				// 스프라이트 이미지 포인터.
		int		iWidth;					// Widht
		int		iHeight;				// Height
		int		iPitch;					// Pitch

		int		iCenterPointX;			// 중점 X
		int		iCenterPointY;			// 중점 Y
	}st_SPRITE;
	////////////////////////////////////////////////////////////////////
	// 생성자, 파괴자.
	//
	// Parameters: (int)최대 스프라이트 개수. (DWORD)투명칼라.
	////////////////////////////////////////////////////////////////////
	CSpriteDib(int iMaxSprite, DWORD dwColorKey);
	virtual ~CSpriteDib();

	///////////////////////////////////////////////////////
	// LoadDibSprite. 
	// BMP파일을 읽어서 하나의 프레임으로 저장한다.
	//
	///////////////////////////////////////////////////////
	BOOL LoadDibSprite(int iSpriteIndex, WCHAR *szFileName, int iCenterPointX, int iCenterPointY);

	///////////////////////////////////////////////////////
	// ReleaseSprite. 
	// 해당 스프라이트 해제.
	//
	///////////////////////////////////////////////////////
	void ReleaseSprite(int iSpriteIndex);

	///////////////////////////////////////////////////////
	// DrawSprite. 
	// 특정 메모리 위치에 스프라이트를 출력한다. (칼라키, 클리핑 처리)
	//
	///////////////////////////////////////////////////////
	void DrawSprite(int iSpriteIndex, int iDrawX, int iDrawY, BYTE *bypDest, int iDestWidth,
		int iDestHeight, int iDestPitch);
	///////////////////////////////////////////////////////
	// DrawImage. 
	// 특정 메모리 위치에 이미지를 출력한다. (클리핑 처리)
	//
	///////////////////////////////////////////////////////
	void DrawImage(int iSpriteIndex, int iDrawX, int iDrawY, BYTE *bypDest, int iDestWidth,
		int iDestHeight, int iDestPitch);

	//빨간색 톤으로 출력
	void DrawSpriteRed(int iSpriteIndex, int iDrawX, int iDrawY, BYTE *bypDest, int iDestWidth,
		int iDestHeight, int iDestPitch);

	//합성 출력
	void DrawSpriteCompose(int iSpriteIndex, int iDrawX, int iDrawY, BYTE *bypDest, int iDestWidth,
		int iDestHeight, int iDestPitch);

	//체력바 용
	void DrawPercentage(int iSpriteIndex, int iDrawX, int iDrawY, BYTE *bypDest, int iDestWidth,
		int iDestHeight, int iDestPitch, int iPercentage = 100);

	// 테스트 용
	void DrawTextID(int iSpriteIndex, int iDrawX, int iDrawY, BYTE *bypDest, int iDestWidth,
		int iDestHeight, int iDestPitch, HWND hWnd, UINT id);


	//카메라 위치
	void SetCameraPosition(int iPosX, int iPosY);

protected:

	//------------------------------------------------------------------
	// Sprite 배열 정보.
	//------------------------------------------------------------------
	int		m_iMaxSprite;

public:
	st_SPRITE	*m_stpSprite;

protected:
	//------------------------------------------------------------------
	// 투명 색상으로 사용할 컬러.
	//------------------------------------------------------------------
	DWORD		m_dwColorKey;

	//플레이어 카메라 왼쪽 위 꼭지점 0,0 부분 좌표
	int	m_iCameraPosX;
	int m_iCameraPosY;
};

#endif
