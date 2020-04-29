#include "stdafx.h"
#include "Player.h"
#include "CSpriteDib.h"



#define dfDELAY_STAND	5
#define dfDELAY_MOVE	4
#define dfDELAY_ATTACK1	3
#define dfDELAY_ATTACK2	4
#define dfDELAY_ATTACK3	4

#define dfRANGE_MOVE_TOP	0
#define dfRANGE_MOVE_LEFT	0
#define dfRANGE_MOVE_RIGHT	6399
#define dfRANGE_MOVE_BOTTOM	6399

#define dfSPEED_PLAYER_X	3
#define dfSPEED_PLAYER_Y	2

Player::Player(DWORD dwID, WORD wPosX, WORD wPosY, BYTE byDirection, BYTE byHP, int Type) {
	m_ID = dwID;
	m_X = wPosX;
	m_Y = wPosY;
	m_Direction = byDirection;
	m_type = Type;
	m_hp = byHP;
	//�ߺ��ȵǴ� ���ǰ�
	m_key = 253;
	m_oldKey = 254;

	m_action = PACKET;
	KeyAction(dfACTION_STAND);
}

Player::~Player() {
	
}


/*****************************
	Action
	�÷��̾� �׼� ó��
******************************/
bool Player::Action() {
	switch (m_key) {
	case dfACTION_STAND:
		if(Animation(dfDELAY_STAND))
			m_spriteIndex = m_animeStart;
		break;
	case dfACTION_MOVE_LU:
		if ((m_X - dfSPEED_PLAYER_X) >= dfRANGE_MOVE_LEFT && (m_Y - dfSPEED_PLAYER_Y) >= dfRANGE_MOVE_TOP) {
			m_X = m_X - dfSPEED_PLAYER_X;
			m_Y = m_Y - dfSPEED_PLAYER_Y;
		}
		if (Animation(dfDELAY_MOVE))
			m_spriteIndex = m_animeStart;
		break;
	case dfACTION_MOVE_LD:
		if ((m_X - dfSPEED_PLAYER_X) >= dfRANGE_MOVE_LEFT && (m_Y + dfSPEED_PLAYER_Y) <= dfRANGE_MOVE_BOTTOM) {
			m_X = m_X - dfSPEED_PLAYER_X;
			m_Y = m_Y + dfSPEED_PLAYER_Y;
		}
		if (Animation(dfDELAY_MOVE))
			m_spriteIndex = m_animeStart;
		break;
	case dfACTION_MOVE_LL:
		if ((m_X - dfSPEED_PLAYER_X) >= dfRANGE_MOVE_LEFT) {
			m_X = m_X - dfSPEED_PLAYER_X;
		}
		if (Animation(dfDELAY_MOVE))
			m_spriteIndex = m_animeStart;
		break;
	case dfACTION_MOVE_RU:
		if ((m_X + dfSPEED_PLAYER_X) <= dfRANGE_MOVE_RIGHT && (m_Y - dfSPEED_PLAYER_Y) >= dfRANGE_MOVE_TOP) {
			m_X = m_X + dfSPEED_PLAYER_X;
			m_Y = m_Y - dfSPEED_PLAYER_Y;
		}
		if (Animation(dfDELAY_MOVE))
			m_spriteIndex = m_animeStart;
		break;
	case dfACTION_MOVE_RD:
		if ((m_X + dfSPEED_PLAYER_X) <= dfRANGE_MOVE_RIGHT && (m_Y + dfSPEED_PLAYER_Y) <= dfRANGE_MOVE_BOTTOM) {
			m_X = m_X + dfSPEED_PLAYER_X;
			m_Y = m_Y + dfSPEED_PLAYER_Y;
		}
		if (Animation(dfDELAY_MOVE))
			m_spriteIndex = m_animeStart;
		break;
	case dfACTION_MOVE_RR:
		if ((m_X + dfSPEED_PLAYER_X) <= dfRANGE_MOVE_RIGHT) {
			m_X = m_X + dfSPEED_PLAYER_X;
		}
		if (Animation(dfDELAY_MOVE))
			m_spriteIndex = m_animeStart;
		break;
	case dfACTION_MOVE_UU:
		if ((m_Y - dfSPEED_PLAYER_Y) >= dfRANGE_MOVE_TOP) {
			m_Y = m_Y - dfSPEED_PLAYER_Y;
		}
		if (Animation(dfDELAY_MOVE))
			m_spriteIndex = m_animeStart;
		break;
	case dfACTION_MOVE_DD:
		if ((m_Y + dfSPEED_PLAYER_Y) <= dfRANGE_MOVE_BOTTOM) {
			m_Y = m_Y + dfSPEED_PLAYER_Y;
		}
		if(Animation(dfDELAY_MOVE))
			m_spriteIndex = m_animeStart;
		break;
	case dfACTION_ATTACK1:
		if (Animation(dfDELAY_ATTACK1)) {
			m_action = ENDATTACK;
			KeyAction(dfACTION_STAND);
			m_oldKey = m_key;
			/*if (m_Direction) {
				m_spriteIndex = 4;
				m_animeStart = 4;
				m_animeEnd = 6;
				m_animeCount = 0;
			}
			else {
				m_spriteIndex = 1;
				m_animeStart = 1;
				m_animeEnd = 3;
				m_animeCount = 0;
			}
			m_key = dfACTION_STAND;*/
		}
		break;
	case dfACTION_ATTACK2:
		if (Animation(dfDELAY_ATTACK1)) {
			m_action = ENDATTACK;
			KeyAction(dfACTION_STAND);
			m_oldKey = m_key;
			/*if (m_Direction) {
				m_spriteIndex = 4;
				m_animeStart = 4;
				m_animeEnd = 6;
				m_animeCount = 0;
			}
			else {
				m_spriteIndex = 1;
				m_animeStart = 1;
				m_animeEnd = 3;
				m_animeCount = 0;
			}
			m_key = dfACTION_STAND;*/
		}
		break;
	case dfACTION_ATTACK3:
		if (Animation(dfDELAY_ATTACK1)) {
			m_action = ENDATTACK;
			KeyAction(dfACTION_STAND);
			m_oldKey = m_key;
			/*if (m_Direction) {
				m_spriteIndex = 4;
				m_animeStart = 4;
				m_animeEnd = 6;
				m_animeCount = 0;
			}
			else {
				m_spriteIndex = 1;
				m_animeStart = 1;
				m_animeEnd = 3;
				m_animeCount = 0;
			}
			m_key = dfACTION_STAND;*/
		}
		break;
	}

	//if (m_hp <= 0) {
	//	return true;
	//}
	
	return false;
}

/*****************************
	Draw
	�÷��̾� ĳ����, 
	�׸���, 
	ü�¹� �׸��ϴ�
******************************/
void Player::Draw(CSpriteDib *Sprite, BYTE *bypDest, int iDestWidth, int iDestHeight, int iDestPitch, HWND hWnd) {
	//�÷��� ĳ����
	if (m_type == 100) {
		Sprite->DrawSpriteRed(m_spriteIndex, m_X, m_Y, bypDest, iDestWidth, iDestHeight, iDestPitch);
	}
	else if(m_type == 300){
		Sprite->DrawSprite(m_spriteIndex, m_X, m_Y, bypDest, iDestWidth, iDestHeight, iDestPitch);
	}
	//�׸���
	Sprite->DrawSpriteCompose(63, m_X, m_Y, bypDest, iDestWidth, iDestHeight, iDestPitch);
	//ü�¹�
	Sprite->DrawPercentage(64, m_X - 35, m_Y + 10, bypDest, iDestWidth, iDestHeight, iDestPitch, m_hp);

	//ID�� ����
	Sprite->DrawTextID(64, m_X - 35, m_Y + 25, bypDest, iDestWidth, iDestHeight, iDestPitch, hWnd, m_ID);
}

/*****************************
	KeyAction
	�÷��̾� Ű �Է� ó��
******************************/
void Player::KeyAction(BYTE Key) {
	////�������̸� ����
	//if (m_key == dfACTION_ATTACK1) {
	//	return true;
	//}
	//else if (m_key == dfACTION_ATTACK2) {
	//	return true;
	//}
	//else if (m_key == dfACTION_ATTACK3) {
	//	return true;
	//}


	//�������̸� ����
	if (m_action == ATTACK) {
		return;
	}

	switch (Key) {
	case dfACTION_STAND:
		if (Key != m_key) {
			m_key = Key;
			//���� ���°� �ƴϸ�
			if (m_action != STOP) {
				m_action = STOP;
				if (m_Direction) {
					m_spriteIndex = 4;
					m_animeStart = 4;
					m_animeEnd = 6;
				}
				else {
					m_spriteIndex = 1;
					m_animeStart = 1;
					m_animeEnd = 3;	
				}
				m_animeCount = 0;
			}
		}
		return;
	case dfACTION_MOVE_LU:
		if (Key != m_key) {
			m_key = Key;
			//�������� �ȴ� ���� �ƴ϶��
			if (m_action != LMOVE) {
				m_action = LMOVE;
				m_spriteIndex = 7;
				m_animeStart = 7;
				m_animeEnd = 18;
				m_Direction = 0;
				m_animeCount = 0;
			}
		}
		return;
	case dfACTION_MOVE_RU:
		if (Key != m_key) {
			m_key = Key;
			//���������� �ȴ� ���� �ƴ϶��
			if (m_action != RMOVE) {
				m_action = RMOVE;
				m_spriteIndex = 19;
				m_animeStart = 19;
				m_animeEnd = 30;
				m_Direction = 4;
				m_animeCount = 0;
			}
		}
		return;
	case dfACTION_MOVE_LD:
		if (Key != m_key) {
			m_key = Key;
			//�������� �ȴ� ���� �ƴ϶��
			if (m_action != LMOVE) {
				m_action = LMOVE;
				m_spriteIndex = 7;
				m_animeStart = 7;
				m_animeEnd = 18;
				m_Direction = 0;
				m_animeCount = 0;
			}
		}
		return;
	case dfACTION_MOVE_RD:
		if (Key != m_key) {
			m_key = Key;
			//���������� �ȴ� ���� �ƴ϶��
			if (m_action != RMOVE) {
				m_action = RMOVE;
				m_spriteIndex = 19;
				m_animeStart = 19;
				m_animeEnd = 30;
				m_Direction = 4;
				m_animeCount = 0;
			}
		}
		return;
	case dfACTION_MOVE_LL:
		if (Key != m_key) {
			m_key = Key;
			//�������� �ȴ� ���� �ƴ϶��
			if (m_action != LMOVE) {
				m_action = LMOVE;
				m_spriteIndex = 7;
				m_animeStart = 7;
				m_animeEnd = 18;
				m_Direction = 0;
				m_animeCount = 0;
			}
		}
		return;
	case dfACTION_MOVE_UU:
		if (Key != m_key) {
			m_key = Key;
			if (m_Direction) {
				//���������� �ȴ� ���� �ƴ϶��
				if (m_action != RMOVE) {
					m_action = RMOVE;
					m_spriteIndex = 19;
					m_animeStart = 19;
					m_animeEnd = 30;
					m_Direction = 4;
					m_animeCount = 0;
				}
			}
			else {
				//�������� �ȴ� ���� �ƴ϶��
				if (m_action != LMOVE) {
					m_action = LMOVE;
					m_spriteIndex = 7;
					m_animeStart = 7;
					m_animeEnd = 18;
					m_Direction = 0;
					m_animeCount = 0;
				}
			}
		}
		return;
	case dfACTION_MOVE_RR:
		if (Key != m_key) {
			m_key = Key;
			//���������� �ȴ� ���� �ƴ϶��
			if (m_action != RMOVE) {
				m_action = RMOVE;
				m_spriteIndex = 19;
				m_animeStart = 19;
				m_animeEnd = 30;
				m_Direction = 4;
				m_animeCount = 0;
			}
		}
		return;
	case dfACTION_MOVE_DD:
		if (Key != m_key) {
			m_key = Key;
			if (m_Direction) {
				//���������� �ȴ� ���� �ƴϸ�
				if (m_action != RMOVE) {
					m_action = RMOVE;
					m_spriteIndex = 19;
					m_animeStart = 19;
					m_animeEnd = 30;
					m_Direction = 4;
					m_animeCount = 0;
				}
			}
			else {
				//�������� �ȴ� ���� �ƴϸ�
				if (m_action != LMOVE) {
					m_action = LMOVE;
					m_spriteIndex = 7;
					m_animeStart = 7;
					m_animeEnd = 18;
					m_Direction = 0;
					m_animeCount = 0;
				}
			}
		}
		return;
	case dfACTION_ATTACK1:
		m_key = Key;
		//���� ���°� �ƴϸ�
		if (m_action != ATTACK) {
			m_action = ATTACK;
			if (m_Direction) {
				m_spriteIndex = 45;
				m_animeStart = 45;
				m_animeEnd = 48;
			}
			else {
				m_spriteIndex = 31;
				m_animeStart = 31;
				m_animeEnd = 34;
			}
			m_animeCount = 0;
		}
		return;
	case dfACTION_ATTACK2:
		m_key = Key;
		//���� ���°� �ƴϸ�
		if (m_action != ATTACK) {
			m_action = ATTACK;
			if (m_Direction) {
				m_spriteIndex = 49;
				m_animeStart = 49;
				m_animeEnd = 52;
			}
			else {
				m_spriteIndex = 35;
				m_animeStart = 35;
				m_animeEnd = 38;
			}
			m_animeCount = 0;
		}
		return;
	case dfACTION_ATTACK3:
		m_key = Key;
		//���� ���°� �ƴϸ�
		if (m_action != ATTACK) {
			m_action = ATTACK;
			if (m_Direction) {
				m_spriteIndex = 53;
				m_animeStart = 53;
				m_animeEnd = 58;
			}
			else {
				m_spriteIndex = 39;
				m_animeStart = 39;
				m_animeEnd = 44;
			}
			m_animeCount = 0;
		}
		return;
	}
	return;
}


//������ ����
void Player::Damage(int Damage) {
	m_hp = m_hp - Damage;
}

//���̵�
DWORD Player::GetID() {
	return m_ID;
}

//����
BYTE Player::GetDirection() {
	return m_Direction;
}

//�Էµ� Ű
BYTE Player::GetKey() {
	return m_key;
}

//ü��
BYTE Player::GetHp(){
	return m_hp;
}

//��ġ ��ǥ ����
//wPosX : X��ǥ ��, wPosY : Y��ǥ ��
void Player::SetPosition(WORD wPosX, WORD wPosY) {
	m_X = wPosX;
	m_Y = wPosY;
}

//��Ŷ ���
//Action : Ű��, Direction : ���Ⱚ
void Player::PacketAction(BYTE Action, BYTE Direction) {
	m_Direction = Direction;
	m_action = PACKET;
	KeyAction(Action);
}

//��Ŷ ���
//Action : Ű��
void Player::PacketAction(BYTE Action) {
	m_action = PACKET;
	KeyAction(Action);
}

//�÷��̾� ������ ó��
//AnimeDelay : ������ ������ ��
bool Player::Animation(int AnimeDelay) {
	if (m_animeCount < AnimeDelay) {
		++m_animeCount;
	}
	else {
		m_animeCount = 0;
		++m_spriteIndex;
		if (m_spriteIndex > m_animeEnd) {
			return true;
		}
	}
	return false;
}

//Ű �Է� ���� �� �ִ� �������� �˻�
//���� ������ ��� false;
BOOL Player::CheckActionState() {
	//��������
	if (m_action == ATTACK) {
		return false;
	}
	return true;
}


///*****************************
//	HitCheck
//	�ǰ� ����
//******************************/
//void Player::Hit(int AttackType) {
//
//	switch (AttackType) {
//	case dfACTION_ATTACK1:
//		m_hp = m_hp - 10;
//		break;
//	case dfACTION_ATTACK2:
//		m_hp = m_hp - 15;
//		break;
//	case dfACTION_ATTACK3:
//		m_hp = m_hp - 20;
//		break;
//	}
//	
//}