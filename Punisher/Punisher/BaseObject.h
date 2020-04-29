#pragma once

class CSpriteDib;

class BaseObject {
public:
	BaseObject();
	virtual ~BaseObject();

	virtual bool Action() = 0;
	virtual void Draw(CSpriteDib *Sprite, BYTE *bypDest, int iDestWidth, int iDestHeight, int iDestPitch, HWND hWnd) = 0;

public:
	int m_type;		//오브젝트 타입 : 0~99 이펙트, 100~199 플레이어, 200~299 동맹, 300~399 적 그외 중립
	WORD m_X;		//좌표 X
	WORD m_Y;		//좌표 Y

	int m_spriteIndex;
	int m_animeStart;
	int m_animeEnd;
	int m_animeCount;



	//int m_moveCount = 0;		//Ai용
	//int m_moveRandCount = 0;	//Ai용
	//int m_moveDelay = 0;		//Ai용
	//int m_moveRandDelay = 0;	//Ai용
	//int m_attack = 0;			//Ai용
};