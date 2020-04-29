#pragma once

#include "BaseObject.h"

class Map : public BaseObject {
public:
	Map(CSpriteDib *Sprite, int iWidth, int iHeight, int iColorBit, int iRight, int iBottom, int iSize, int iType);
	virtual ~Map();

	virtual bool Action();
	virtual void Draw(CSpriteDib *Sprite, BYTE *bypDest, int iDestWidth, int iDestHeight, int iDestPitch, HWND hWnd);
	void SetCameraPosition(int iPosX, int iPosY);		//카메라 위치

private:
	// 맵 만들기
	void CreateMap(CSpriteDib *Sprite, int iWidth, int iHeight, int iColorBit, int iRight, int iBottom, int iTileSize, int iType);
	void ReleaseMap();				// 릴리즈 맵
	void SetArrayMap();
	void UpdateMapBuffer();

private:
	int	m_iWidth;			// 화면 가로
	int	m_iHeight;			// 화면 세로
	int	m_iBuffWidth;		// 화면 버퍼 가로
	int	m_iBuffHeight;		// 화면 버퍼 세로
	int	m_iBuffPitch;		// 화면 버퍼 피치
	int m_iRight;			// 화면 오른쪽
	int m_iBottom;			// 화면 아래
	int m_iTileSize;		// 타일 사이즈

	int testX;
	int testXX;
	int testY;
	int testYY;
	
	//플레이어 카메라 왼쪽 위 꼭지점 0,0 부분 좌표
	int	m_iCameraPosX;
	int m_iCameraPosY;
	CSpriteDib *m_Sprite;				// 스프라이트 클래스 포인터
	BYTE m_byArrWorldMap[102][102];		// 월드맵 타일 정보 배열, +2칸
	// 화면 타일 배열
	// 화면 세로, 가로 / 64 + 타일 1칸
	BYTE m_byArrMap[9][11];
	BYTE *m_byMapBuffer;				// 맵 화면

};