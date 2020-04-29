#pragma once

#include "BaseObject.h"

class Map : public BaseObject {
public:
	Map(CSpriteDib *Sprite, int iWidth, int iHeight, int iColorBit, int iRight, int iBottom, int iSize, int iType);
	virtual ~Map();

	virtual bool Action();
	virtual void Draw(CSpriteDib *Sprite, BYTE *bypDest, int iDestWidth, int iDestHeight, int iDestPitch, HWND hWnd);
	void SetCameraPosition(int iPosX, int iPosY);		//ī�޶� ��ġ

private:
	// �� �����
	void CreateMap(CSpriteDib *Sprite, int iWidth, int iHeight, int iColorBit, int iRight, int iBottom, int iTileSize, int iType);
	void ReleaseMap();				// ������ ��
	void SetArrayMap();
	void UpdateMapBuffer();

private:
	int	m_iWidth;			// ȭ�� ����
	int	m_iHeight;			// ȭ�� ����
	int	m_iBuffWidth;		// ȭ�� ���� ����
	int	m_iBuffHeight;		// ȭ�� ���� ����
	int	m_iBuffPitch;		// ȭ�� ���� ��ġ
	int m_iRight;			// ȭ�� ������
	int m_iBottom;			// ȭ�� �Ʒ�
	int m_iTileSize;		// Ÿ�� ������

	int testX;
	int testXX;
	int testY;
	int testYY;
	
	//�÷��̾� ī�޶� ���� �� ������ 0,0 �κ� ��ǥ
	int	m_iCameraPosX;
	int m_iCameraPosY;
	CSpriteDib *m_Sprite;				// ��������Ʈ Ŭ���� ������
	BYTE m_byArrWorldMap[102][102];		// ����� Ÿ�� ���� �迭, +2ĭ
	// ȭ�� Ÿ�� �迭
	// ȭ�� ����, ���� / 64 + Ÿ�� 1ĭ
	BYTE m_byArrMap[9][11];
	BYTE *m_byMapBuffer;				// �� ȭ��

};