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
		STOP = 0,		//�÷��̾� ����
		LMOVE,			//�������� �̵���
		RMOVE,			//���������� �̵���
		ATTACK,			//������
		ENDATTACK,		//���ݳ���
		PACKET			//��Ŷ����
	};
public:
	Player(DWORD dwID, WORD wPosX, WORD wPosY, BYTE byDirection, BYTE byHP, int Type);
	virtual ~Player();

	virtual bool Action();
	virtual void Draw(CSpriteDib *Sprite, BYTE *bypDest, int iDestWidth, int iDestHeight, int iDestPitch, HWND hWnd);

	void KeyAction(BYTE Key);								//Ű�Է� ó��
	void Damage(int Damage);								//�ǰ� ó��
	DWORD GetID();											//ID��
	BYTE GetDirection();									//���� �� 1 or 4
	BYTE GetKey();											//���� �Էµ� Ű
	BYTE GetHp();											//ü��
	//��ġ ��ǥ ����
	//wPosX : X��ǥ ��, wPosY : Y��ǥ ��
	void SetPosition(WORD wPosX, WORD wPosY);				//��ġ ��ǥ ����
	//��Ŷ ���
	//Action : Ű��, Direction : ���Ⱚ
	void PacketAction(BYTE Action, BYTE Direction);
	//��Ŷ ���
	//Action : Ű��
	void PacketAction(BYTE Action);
	//Ű �Է� ���� �� �ִ� �������� �˻�
	//���� ������ ��� false;
	BOOL CheckActionState();

private:
	bool Animation(int AnimeDelay);			//�÷��̾� ������ ó��
	
private:
	UINT m_ID;				//ID;
	BYTE m_key;				//���� �Էµ� Ű��
	BYTE m_oldKey;			//���� Ű��
	BYTE m_Direction;		//�ٶ󺸴� ��, 0 ����, 4������
	BYTE m_action;			//0 ��������, 1 �������� �ȴ� ��, 2 ���������� �ȴ� ��, 3 �����ϴ� ��, 4 ���� ����, 5 ��Ŷ��;
	BYTE m_hp;				//ü��
};