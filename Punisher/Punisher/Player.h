#pragma once

#include "BaseObject.h"

#define dfACTION_MOVE_LL		0
#define dfACTION_MOVE_LU		1
#define dfACTION_MOVE_UU		2
#define dfACTION_MOVE_RU		3
#define dfACTION_MOVE_RR		4
#define dfACTION_MOVE_RD		5
#define dfACTION_MOVE_DD		6
#define dfACTION_MOVE_LD		7

#define dfACTION_ATTACK1		11
#define dfACTION_ATTACK2		12
#define dfACTION_ATTACK3		13

#define dfACTION_STAND			255



class Player : public BaseObject {
private:
	enum PlayerState {
		STOP = 0,		//플레이어 정지
		LMOVE,			//왼쪽으로 이동중
		RMOVE,			//오른쪽으로 이동중
		ATTACK,			//공격중
		ENDATTACK,		//공격끝남
		PACKET			//패킷받음
	};
public:
	Player(DWORD dwID, WORD wPosX, WORD wPosY, BYTE byDirection, BYTE byHP, int Type);
	virtual ~Player();

	virtual bool Action();
	virtual void Draw(CSpriteDib *Sprite, BYTE *bypDest, int iDestWidth, int iDestHeight, int iDestPitch, HWND hWnd);

	void KeyAction(BYTE Key);								//키입력 처리
	void Damage(int Damage);								//피격 처리
	DWORD GetID();											//ID값
	BYTE GetDirection();									//방향 값 1 or 4
	BYTE GetKey();											//현제 입력된 키
	BYTE GetHp();											//체력
	//위치 좌표 지정
	//wPosX : X좌표 값, wPosY : Y좌표 값
	void SetPosition(WORD wPosX, WORD wPosY);				//위치 좌표 지정
	//패킷 명령
	//Action : 키값, Direction : 방향값
	void PacketAction(BYTE Action, BYTE Direction);
	//패킷 명령
	//Action : 키값
	void PacketAction(BYTE Action);
	//키 입력 받을 수 있는 상태인지 검사
	//없는 상태인 경우 false;
	BOOL CheckActionState();

private:
	bool Animation(int AnimeDelay);			//플레이어 움직임 처리
	
private:
	UINT m_ID;				//ID;
	BYTE m_key;				//현제 입력된 키값
	BYTE m_oldKey;			//이전 키값
	BYTE m_Direction;		//바라보는 곳, 0 왼쪽, 4오른쪽
	BYTE m_action;			//0 걷지않음, 1 왼쪽으로 걷는 중, 2 오른쪽으로 걷는 중, 3 공격하는 중, 4 공격 종료, 5 패킷용;
	BYTE m_hp;				//체력
};