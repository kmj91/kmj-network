#include "stdafx.h"
#include "EffectObject.h"
#include "Player.h"
#include "CSpriteDib.h"

#define dfDELAY_EFFECT	3

EffectObject::EffectObject(Player *Player, int Damage, int Delay, bool DrawRed, int Type) {
	m_X = Player->m_X;
	m_Y = Player->m_Y - 50;
	m_type = Type;
	m_player = Player;
	m_damage = Damage;
	m_delay = Delay;
	m_drawRed = DrawRed;
	m_delayFlag = false;

	m_spriteIndex = 59;
	m_animeStart = 59;
	m_animeEnd = 62;
	m_animeCount = 0;
}

EffectObject::~EffectObject() {

}

bool EffectObject::Action() {
	if (!m_delayFlag) {
		--m_delay;
		if (m_delay <= 0) {
			if (m_player != NULL) {
				m_player->Damage(m_damage);
			}
			m_delayFlag = true;
		}
		return false;
	}
	

	if (m_animeCount < dfDELAY_EFFECT) {
		++m_animeCount;
	}
	else {
		m_animeCount = 0;
		if (m_spriteIndex + 1 > m_animeEnd) {
			return true;
		}
		else {
			++m_spriteIndex;
		}
	}
	
	return false;
}

void EffectObject::Draw(CSpriteDib *Sprite, BYTE *bypDest, int iDestWidth, int iDestHeight, int iDestPitch, HWND hWnd) {
	if (!m_delayFlag) {
		return;
	}

	if (m_drawRed) {
		//³» ÀÌÆåÆ®
		Sprite->DrawSpriteRed(m_spriteIndex, m_X, m_Y, bypDest, iDestWidth, iDestHeight, iDestPitch);
	}
	else {
		//³²ÀÇ ÀÌÆåÆ®
		Sprite->DrawSprite(m_spriteIndex, m_X, m_Y, bypDest, iDestWidth, iDestHeight, iDestPitch);
	}
}

void EffectObject::Hit(int AttackType) {

}