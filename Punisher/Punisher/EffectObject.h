#pragma once

#include "BaseObject.h"

class Player;

class EffectObject : public BaseObject {
public:
	EffectObject(Player *Player, int Damage, int Delay, bool DrawRed,int Type);
	virtual ~EffectObject();

	virtual bool Action();
	virtual void Draw(CSpriteDib *Sprite, BYTE *bypDest, int iDestWidth, int iDestHeight, int iDestPitch, HWND hWnd);
	virtual void Hit(int AttackType);		//�ǰ� ó��

private:
	Player *m_player;
	int m_delay;				//������ �ð�
	int m_damage;				//������
	bool m_drawRed;
	bool m_delayFlag;			//������ �ð� ����� ����Ʈ ����
};