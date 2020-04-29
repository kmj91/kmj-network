#pragma once

class CSpriteDib;

class BaseObject {
public:
	BaseObject();
	virtual ~BaseObject();

	virtual bool Action() = 0;
	virtual void Draw(CSpriteDib *Sprite, BYTE *bypDest, int iDestWidth, int iDestHeight, int iDestPitch, HWND hWnd) = 0;

public:
	int m_type;		//������Ʈ Ÿ�� : 0~99 ����Ʈ, 100~199 �÷��̾�, 200~299 ����, 300~399 �� �׿� �߸�
	WORD m_X;		//��ǥ X
	WORD m_Y;		//��ǥ Y

	int m_spriteIndex;
	int m_animeStart;
	int m_animeEnd;
	int m_animeCount;



	//int m_moveCount = 0;		//Ai��
	//int m_moveRandCount = 0;	//Ai��
	//int m_moveDelay = 0;		//Ai��
	//int m_moveRandDelay = 0;	//Ai��
	//int m_attack = 0;			//Ai��
};