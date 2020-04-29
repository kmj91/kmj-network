#pragma once

#include "BaseObject.h"

class Player;

class EffectObject : public BaseObject {
public:
	EffectObject(Player *Player, int Damage, int Delay, bool DrawRed,int Type);
	virtual ~EffectObject();

	virtual bool Action();
	virtual void Draw(CSpriteDib *Sprite, BYTE *bypDest, int iDestWidth, int iDestHeight, int iDestPitch, HWND hWnd);
	virtual void Hit(int AttackType);		//피격 처리

private:
	Player *m_player;
	int m_delay;				//딜레이 시간
	int m_damage;				//데미지
	bool m_drawRed;
	bool m_delayFlag;			//정해진 시간 대기후 이펙트 터짐
};