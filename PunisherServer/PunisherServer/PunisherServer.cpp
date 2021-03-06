// PunisherServer.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <map>
#include <list>
#include <conio.h>
#include <time.h>				// time

#include "PacketDefine.h"
#include "CRingBuffer.h"
#include "CSerializationBuffer.h"
#include "MyProfile.h"

#pragma comment(lib, "ws2_32")
#pragma comment(lib, "Winmm.lib")

using namespace std;


#define dfNETWORK_PORT				20000

#define df_FDSET_SIZE				64
#define df_LOGINMAX_SIZE			5000

#define dfRANGE_MOVE_TOP			0
#define dfRANGE_MOVE_LEFT			0
#define dfRANGE_MOVE_RIGHT			6399
#define dfRANGE_MOVE_BOTTOM			6399

#define dfSPEED_PLAYER_X			6
#define dfSPEED_PLAYER_Y			4
#define dfRECKONING_SPEED_PLAYER_X	3
#define dfRECKONING_SPEED_PLAYER_Y	2
#define df_DAMAGE_ATTACK1			1
#define df_DAMAGE_ATTACK2			1
#define df_DAMAGE_ATTACK3			1

#define df_WIN_RIGHT				640
#define df_WIN_BOTTOM				480

#define df_SECTOR_WIDTH				320
#define df_SECTOR_HEIGHT			320
#define df_SECTOR_MIN_X				0
#define df_SECTOR_MIN_Y				0
#define df_SECTOR_MAX_X				6400 / df_SECTOR_WIDTH
#define df_SECTOR_MAX_Y				6400 / df_SECTOR_HEIGHT
#define df_SECTOR_BASIC				0
#define df_SECTOR_LL				1
#define df_SECTOR_LU				2
#define df_SECTOR_UU				4
#define df_SECTOR_RU				8
#define df_SECTOR_RR				16
#define df_SECTOR_RD				32
#define df_SECTOR_DD				64
#define df_SECTOR_LD				128
#define df_SECTOR_CENTER			256
#define df_SECTOR_ALL				511
#define df_MOVE_SECTOR_LU_REMOVE	248		//df_SECTOR_RU | df_SECTOR_RR | df_SECTOR_RD | df_SECTOR_DD | df_SECTOR_LD
#define df_MOVE_SECTOR_UU_REMOVE	224		//df_SECTOR_RD | df_SECTOR_DD | df_SECTOR_LD
#define df_MOVE_SECTOR_RU_REMOVE	227		//df_SECTOR_LU | df_SECTOR_LL | df_SECTOR_LD | df_SECTOR_DD | df_SECTOR_RD
#define df_MOVE_SECTOR_RR_REMOVE	131		//df_SECTOR_LU | df_SECTOR_LL | df_SECTOR_LD
#define df_MOVE_SECTOR_RD_REMOVE	143		//df_SECTOR_RU | df_SECTOR_UU | df_SECTOR_LU | df_SECTOR_LL | df_SECTOR_LD
#define df_MOVE_SECTOR_DD_REMOVE	14		//df_SECTOR_RU | df_SECTOR_UU | df_SECTOR_LU
#define df_MOVE_SECTOR_LD_REMOVE	62		//df_SECTOR_LU | df_SECTOR_UU | df_SECTOR_RU | df_SECTOR_RR | df_SECTOR_RD
#define df_MOVE_SECTOR_LL_REMOVE	56		//df_SECTOR_RU | df_SECTOR_RR | df_SECTOR_RD
#define df_MOVE_SECTOR_LU_ADD		143		//df_SECTOR_RU | df_SECTOR_UU | df_SECTOR_LU | df_SECTOR_LL | df_SECTOR_LD
#define df_MOVE_SECTOR_UU_ADD		14		//df_SECTOR_RU | df_SECTOR_UU | df_SECTOR_LU
#define df_MOVE_SECTOR_RU_ADD		62		//df_SECTOR_LU | df_SECTOR_UU | df_SECTOR_RU | df_SECTOR_RR | df_SECTOR_RD
#define df_MOVE_SECTOR_RR_ADD		56		//df_SECTOR_RU | df_SECTOR_RR | df_SECTOR_RD
#define df_MOVE_SECTOR_RD_ADD		248		//df_SECTOR_RU | df_SECTOR_RR | df_SECTOR_RD | df_SECTOR_DD | df_SECTOR_LD
#define df_MOVE_SECTOR_DD_ADD		224		//df_SECTOR_RD | df_SECTOR_DD | df_SECTOR_LD
#define df_MOVE_SECTOR_LD_ADD		227		//df_SECTOR_LU | df_SECTOR_LL | df_SECTOR_LD | df_SECTOR_DD | df_SECTOR_RD
#define df_MOVE_SECTOR_LL_ADD		131		//df_SECTOR_LU | df_SECTOR_LL | df_SECTOR_LD

#define df_CHARACTER_HP				100

#define df_USERSTATE_ZERO			0
#define df_USERSTATE_LOGIN			1
#define df_USERSTATE_CLOSED			2
#define df_USERSTATE_DELETE			4

#define df_LOG_LEVEL_DEBUG			1
#define df_LOG_LEVEL_DEBUG2			2
#define df_LOG_LEVEL_WARNING		4
#define df_LOG_LEVEL_ERROR			8
#define df_LOG_LEVEL_NOTICE			16
#define df_LOG_LEVEL_ALL			31		//df_LOG_LEVEL_DEBUG | df_LOG_LEVEL_DEBUG2 | df_LOG_LEVEL_WARNING | df_LOG_LEVEL_ERROR | df_LOG_LEVEL_NOTICE

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

#define df_HEADER_SIZE			4
#define df_END_CODE_SIZE		1
#define dfNETWORK_PACKET_CODE	((BYTE)0x89)
#define dfNETWORK_PACKET_END	((BYTE)0x79)
#define dfERROR_RANGE			50

#define df_SERVER_FRAME_DELEY	40


#define _LOG(LogLevel, fmt, ...)\
do{\
	if(g_wLogLevel & LogLevel){\
		wsprintfW(g_szLogBuff, fmt, ##__VA_ARGS__);\
		Log();\
	}\
} while (0)

//패킷의 해더
struct st_PACKET_HEADER {
	BYTE	byCode;			// 패킷코드 0x89 고정.
	BYTE	bySize;			// 패킷 사이즈.
	BYTE	byType;			// 패킷타입.
	BYTE	byTemp;			// 사용안함.
};

//세션
struct st_SESSION {
	SOCKET sock;									// 소켓
	SOCKADDR_IN addr;								// 주소
	CRingBuffer * readBuffer;						// 리시브 버퍼
	CRingBuffer * writeBuffer;						// 샌드 버퍼
	DWORD dwSessionID;								// 세션 ID
	DWORD dwTrafficTick;							// 트래픽 체크를 위한 틱
	DWORD dwTrafficCount;							// 1초마다 송신된 트래픽 수
	DWORD dwTrafficSecondTick;						// 트래픽 카운트 1초 확인용 Tick
	BYTE state;										// 상태
};

//섹터 하나의 좌표
struct st_SECTOR_POS {
	int iX;
	int iY;
};

//특정 위치 주변의 9개 섹터 정보
struct st_SECTOR_AROUND {
	int iCount;
	st_SECTOR_POS Around[9];
};

//캐릭터 정보
struct st_CHARACTER {
	st_SESSION *pSession;			//세션 포인터
	DWORD dwSessionID;				//세션 ID
	DWORD dwAction;					//행동
	DWORD dwActionTick;				//현재 액션을 취한 시간, 데드레커닝용
	BYTE byDirection;				//방향
	BYTE byMoveDirection;			//이동 방향
	short shX;
	short shY;
	short shActionX;				//액션이 변경되었을 때의 좌표
	short shActionY;
	st_SECTOR_POS CurSector;
	st_SECTOR_POS OldSector;
	char chHp;
};

bool InitNetwork();				//네트워크 초기화
void NetworkService();				//네트워크 서비스
bool FrameSkip();					//프레임 스킵
void Update();						//게임 로직
void ServerControl();				//키보드 입력 제어
void AcceptProc();					//억셉트 처리
void Accept();						//억셉트
void NetworkProc();					//네트워크 처리
void ReceiveProc(st_SESSION * SessionUser);												//리시브 처리
void SandProc(st_SESSION * SessionUser);												//샌드 처리
bool PacketProc(st_SESSION *pSession, BYTE byPacketType, CSerializationBuffer *pPacket);//패킷 처리
void netPacketProc_MoveStart(st_SESSION *pSession, CSerializationBuffer *pPacket);		//이동 패킷 처리
void netPacketProc_MoveStop(st_SESSION *pSession, CSerializationBuffer *pPacket);		//정지 패킷 처리
void netPacketProc_Attack1(st_SESSION *pSession, CSerializationBuffer *pPacket);		//공격1 패킷 처리
void netPacketProc_Attack2(st_SESSION *pSession, CSerializationBuffer *pPacket);		//공격2 패킷 처리
void netPacketProc_Attack3(st_SESSION *pSession, CSerializationBuffer *pPacket);		//공격3 패킷 처리
void netPacketProc_Echo(st_SESSION *pSession, CSerializationBuffer *pPacket);			//에코 패킷 처리

void CreateMyCharacterService(st_CHARACTER *pCharacter);								//캐릭터 생성 서비스
void RemoveSectorService(const int iSectorX, const int iSectorY, st_CHARACTER *pCharacter,
						CSerializationBuffer &pPacketSector,
						CSerializationBuffer &pPacketClient);							//섹터 삭제 서비스
void AddSectorService(const int iSectorX, const int iSectorY, st_CHARACTER *pCharacter,
					CSerializationBuffer &pPacketSector,
					CSerializationBuffer &pPacketSector2,
					CSerializationBuffer &pPacketClient);								//섹터 추가 서비스
void AddSectorService(const int iSectorX, const int iSectorY, st_CHARACTER *pCharacter,
					CSerializationBuffer &pPacketSector,
					CSerializationBuffer &pPacketClient);								//섹터 추가 서비스
void DamageService_Attack1(st_CHARACTER *pCharacter, CSerializationBuffer *pPacketSector);//공격1 데미지 서비스
void DamageService_Attack2(st_CHARACTER *pCharacter, CSerializationBuffer *pPacketSector);//공격2 데미지 서비스
void DamageService_Attack3(st_CHARACTER *pCharacter, CSerializationBuffer *pPacketSector);//공격3 데미지 서비스

void CreateSessionUser(st_SESSION * NewSessionUser, SOCKET ClientSocket);				//신규 유저 세션 만들기
void CreateCharacter(st_SESSION * NewSessionUser);										//신규 캐릭터 정보 만들기
st_CHARACTER * FindCharacter(DWORD dwSessionID);										//ID로 캐릭터 찾기
int DeadReckoningPos(st_CHARACTER *pCharacter, int *pPosX, int *pPosY);					//데드 레커닝
bool CharacterMoveCheck(short shX, short shY);											//캐릭터 이동 좌표 검사
void Sector_AddCharacter(st_CHARACTER *pCharacter);										//섹터에 신규 캐릭터 추가
void Sector_RemoveCharacter(st_CHARACTER *pCharacter);									//섹터의 캐릭터 삭제
bool Sector_UpdateCharacter(st_CHARACTER *pCharacter);									//섹터 갱신
void GetUpdateSectorAround(const st_CHARACTER * const pCharacter,
							DWORD &pRemoveSector, DWORD &pAddSector);					//주변 섹터 갱신
void CharacterSectorUpdatePacket(st_CHARACTER *pCharacter);								//섹터 갱신 패킷 처리

void mpDeleteCharacter(CSerializationBuffer * pPacket, const DWORD dwSessionID);		//캐릭터 삭제 패킷 만들기
void mpCreateOtherCharacter(CSerializationBuffer * pPacket, const DWORD dwSessionID,
						const BYTE byDirection,
						const short shX, const short shY, const char chHp);				//다른 캐릭터 생성 패킷 만들기
void mpCreateMyCharacter(CSerializationBuffer * pPacket, const DWORD dwSessionID,
						const BYTE byDirection,
						const short shX, const short shY, const char chHp);				//내 캐릭터 생성 패킷 만들기
void mpMoveStart(CSerializationBuffer * pPacket, const DWORD dwSessionID,
				const BYTE byDirection, const short shX, const short shY);				//캐릭터 이동 패킷 만들기
void mpMoveStop(CSerializationBuffer * pPacket, const DWORD dwSessionID,
				const BYTE byDirection, const short shX, const short shY);				//캐릭터 이동 중지 패킷 만들기
void mpAttack1(CSerializationBuffer * pPacket, const DWORD dwSessionID,
				const BYTE byDirection, const short shX, const short shY);				//캐릭터 공격1 패킷 만들기
void mpAttack2(CSerializationBuffer * pPacket, const DWORD dwSessionID,
				const BYTE byDirection, const short shX, const short shY);				//캐릭터 공격2 패킷 만들기
void mpAttack3(CSerializationBuffer * pPacket, const DWORD dwSessionID,
				const BYTE byDirection, const short shX, const short shY);				//캐릭터 공격3 패킷 만들기
void mpDamage(CSerializationBuffer * pPacket, const DWORD dwAttackerID,
					const DWORD dwDamageID, const char chDamageHp);						//데미지 패킷 만들기
void mpEcho(CSerializationBuffer * pPacket, const DWORD dwTime);						//에코 패킷 만들기
void mpSync(CSerializationBuffer *pPacket, DWORD dwSessionID, short shX, short shY);	//싱크 패킷 만들기
void SendPacket_SectorOne(int iSectorX, int iSectorY,
						CSerializationBuffer *pPacket, st_SESSION *pExceptSession);		//특정 섹터에 보내기
void SendPacket_Unicast(st_SESSION *pSession, CSerializationBuffer *pPacket);			//1명에게만 보내기
void SendPacket_Around(int iSectorX, int iSectorY, st_SESSION *pSession,
						CSerializationBuffer *pPacket, bool bSendMe = false);			//클라이언트 기준 주변 섹터에 보내기
bool CheckSessionClosed(st_SESSION * SessionUser);										//세션 삭제 검사
void RemoveCaracter(DWORD dwKey);														//캐릭터 삭제
void RemoveSession(st_SESSION * SessionUser);											//세션 삭제
void RemoveSession(DWORD dwKey);
void Log();																				//로그 출력용 함수
void DebugSecter();
void PrintLogLevel();

typedef pair <DWORD, st_SESSION *> SessionUser_Pair;
typedef pair <DWORD, st_CHARACTER *> Character_Pair;


SOCKET g_listen_sock;							// 리슨 소켓
map<DWORD, st_SESSION *> g_mapSessionUser;		// 세션
map<DWORD, st_CHARACTER *> g_mapCharacter;		// 캐릭터 객체 메인 관리
list<st_CHARACTER *> g_listSector[df_SECTOR_MAX_Y][df_SECTOR_MAX_X];		// 월드맵 캐릭터 섹터

DWORD g_dwSessionID = 0;						// 세션 ID

WORD g_wLogLevel = 0;							// 출력, 저장 대상의 로그 레벨
WCHAR g_szLogBuff[1024];						// 로그 저장시 필요한 임시 버퍼
bool g_bShutdown = true;						// 서버 메인 루프 플래그

DWORD g_dwFrameTime;
DWORD g_dwOldTime;
DWORD g_dwSkipFrame = 0;
int g_iFramePerSecond = 0;
int g_iLoop = 0;

FILE *g_fp = NULL;								// 로그 출력 파일
tm g_today;										// 날짜

int main()
{
	timeBeginPeriod(1);

	if (!InitNetwork()) {
		return 0;
	}

	//로직
	while (g_bShutdown) {
		NetworkService();
		if (FrameSkip()) {
			Update();
		}
		ServerControl();
		//DebugSecter();
		++g_iLoop;
		Sleep(1);
	}

	fclose(g_fp);

	//printProfile("log\\profile.txt");
	timeEndPeriod(1);
	return 0;
}

//네트워크 초기화
bool InitNetwork() {
	int iRetval;
	int iSize;				// 파일 사이즈
	char * chpFileBuff;		// 버퍼
	__time64_t t64Time;
	WCHAR FileName[32];		// 파일 이름

	g_dwOldTime = timeGetTime();
	g_dwFrameTime = g_dwOldTime;

	// 로그 레벨
	g_wLogLevel = g_wLogLevel | df_LOG_LEVEL_ALL;

	// 현제 날짜 얻기
	_time64(&t64Time);
	_localtime64_s(&g_today, &t64Time);

	g_today.tm_year = g_today.tm_year + 1900;
	g_today.tm_mon = g_today.tm_mon + 1;

	wsprintfW(FileName, L"log\\LOG_%d_%d_%d.txt", g_today.tm_year, g_today.tm_mon, g_today.tm_mday);

	// 읽기 모드로 파일 오픈
	iRetval = _wfopen_s(&g_fp, FileName, L"rt");
	if (iRetval != 0) {
		switch (iRetval) {
		case ENOENT:
			// 파일이 없으면 새로 만듬
			iRetval = _wfopen_s(&g_fp, FileName, L"wt");
			if (iRetval != 0) {
				_LOG(df_LOG_LEVEL_ERROR, L"fopen error [%d]", iRetval);
				return false;
			}
			break;
		default:
			_LOG(df_LOG_LEVEL_ERROR, L"fopen error [%d]", iRetval);
			return false;
		}
	}
	else {
		fseek(g_fp, 0, SEEK_END);
		iSize = ftell(g_fp);
		chpFileBuff = new char[iSize];
		rewind(g_fp);
		// 파일 읽기
		fread_s(chpFileBuff, iSize, iSize, 1, g_fp);
		fclose(g_fp);
		
		// 쓰기 모드로 파일 오픈
		iRetval = _wfopen_s(&g_fp, FileName, L"wt");
		if (iRetval != 0) {
			_LOG(df_LOG_LEVEL_ERROR, L"fopen error [%d]", iRetval);
			return false;
		}
		
		// 사이즈 다시 "\r" 뺀 문자열 사이즈
		iSize = _scprintf("%s\n", chpFileBuff);
		// 파일 덮어쓰기
		fwrite(chpFileBuff, iSize, 1, g_fp);
		delete[] chpFileBuff;
	}

	//윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return false;

	//리슨 소켓
	g_listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (g_listen_sock == INVALID_SOCKET) {
		iRetval = GetLastError();
		_LOG(df_LOG_LEVEL_ERROR, L"InitNetwork() listen socket error [%d]\n", iRetval);
		return false;
	}

	//넌블로킹 전환
	u_long on = 1;
	iRetval = ioctlsocket(g_listen_sock, FIONBIO, &on);
	if (iRetval == SOCKET_ERROR) {
		iRetval = GetLastError();
		_LOG(df_LOG_LEVEL_ERROR, L"InitNetwork() ioctlsocket error [%d]\n", iRetval);
		return false;
	}

	//bind()
	SOCKADDR_IN serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(dfNETWORK_PORT);
	iRetval = bind(g_listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (iRetval == SOCKET_ERROR) {
		iRetval = GetLastError();
		_LOG(df_LOG_LEVEL_ERROR, L"InitNetwork() bind error [%d]\n", iRetval);
		return false;
	}

	//listen()
	iRetval = listen(g_listen_sock, SOMAXCONN);
	if (iRetval == SOCKET_ERROR) {
		iRetval = GetLastError();
		_LOG(df_LOG_LEVEL_ERROR, L"InitNetwork() listen error [%d]\n", iRetval);
		return false;
	}

	_LOG(df_LOG_LEVEL_NOTICE, L"Server open	port:%d\n", dfNETWORK_PORT);

	return true;
}

//네트워크
void NetworkService() {
	AcceptProc();
	NetworkProc();
}

//프레임 스킵
bool FrameSkip() {
	DWORD dwTempTime;
	DWORD dwTime;

	//스킵
	dwTime = timeGetTime();
	dwTempTime = dwTime - g_dwOldTime;
	g_dwOldTime = dwTime;

	if (dwTime - g_dwFrameTime >= 1000) {
		// 만약 1초 동안 25프레임을 돌지 못했다면 현제 프레임과 루프를 기록합니다
		if (g_iFramePerSecond != 25) {
			_LOG(df_LOG_LEVEL_NOTICE, L"Frame : %d, Roop : %d", g_iFramePerSecond, g_iLoop);
		}
		g_iFramePerSecond = 0;
		g_iLoop = 0;
		g_dwFrameTime = dwTime;
	}

	if (dwTempTime > df_SERVER_FRAME_DELEY) {
		g_dwSkipFrame = g_dwSkipFrame + dwTempTime;
		g_dwSkipFrame = g_dwSkipFrame - df_SERVER_FRAME_DELEY;

		return false;
	}
	else if (g_dwSkipFrame > df_SERVER_FRAME_DELEY) {
		g_dwSkipFrame = g_dwSkipFrame + dwTempTime;
		g_dwSkipFrame = g_dwSkipFrame - df_SERVER_FRAME_DELEY;

		return false;
	}
	else {
		dwTempTime = df_SERVER_FRAME_DELEY - dwTempTime;
		Sleep(dwTempTime);
	}

	if (g_iFramePerSecond >= 25) {
		return false;
	}

	++g_iFramePerSecond;

	return true;
}

//게임 로직
void Update() {
	st_CHARACTER *pCharacter = NULL;
	map<DWORD, st_CHARACTER *>::iterator iter;
	map<DWORD, st_CHARACTER *>::iterator iter_end;

	iter = g_mapCharacter.begin();
	iter_end = g_mapCharacter.end();
	while (iter != iter_end) {
		pCharacter = iter->second;
		
		if (pCharacter->chHp <= 0) {
			//사망처리
			//
		}
		else {
			////일정 시간동안 수신이 없으면 종료처리
			//if (dwCurrentTick - pCharacter->pSession->dwTrafficTick) {
			//	
			//}

			//현재 동작 처리
			switch (pCharacter->dwAction) {
			case dfACTION_MOVE_LL:
				if (CharacterMoveCheck(pCharacter->shX - dfSPEED_PLAYER_X, pCharacter->shY)) {
					pCharacter->shX = pCharacter->shX - dfSPEED_PLAYER_X;
				}
				break;
			case dfACTION_MOVE_LU:
				if (CharacterMoveCheck(pCharacter->shX - dfSPEED_PLAYER_X, pCharacter->shY - dfSPEED_PLAYER_Y)) {
					pCharacter->shX = pCharacter->shX - dfSPEED_PLAYER_X;
					pCharacter->shY = pCharacter->shY - dfSPEED_PLAYER_Y;
				}
				break;
			case dfACTION_MOVE_UU:
				if (CharacterMoveCheck(pCharacter->shX, pCharacter->shY - dfSPEED_PLAYER_Y)) {
					pCharacter->shY = pCharacter->shY - dfSPEED_PLAYER_Y;
				}
				break;
			case dfACTION_MOVE_RU:
				if (CharacterMoveCheck(pCharacter->shX + dfSPEED_PLAYER_X, pCharacter->shY - dfSPEED_PLAYER_Y)) {
					pCharacter->shX = pCharacter->shX + dfSPEED_PLAYER_X;
					pCharacter->shY = pCharacter->shY - dfSPEED_PLAYER_Y;
				}
				break;
			case dfACTION_MOVE_RR:
				if (CharacterMoveCheck(pCharacter->shX + dfSPEED_PLAYER_X, pCharacter->shY)) {
					pCharacter->shX = pCharacter->shX + dfSPEED_PLAYER_X;
				}
				break;
			case dfACTION_MOVE_RD:
				if (CharacterMoveCheck(pCharacter->shX + dfSPEED_PLAYER_X, pCharacter->shY + dfSPEED_PLAYER_Y)) {
					pCharacter->shX = pCharacter->shX + dfSPEED_PLAYER_X;
					pCharacter->shY = pCharacter->shY + dfSPEED_PLAYER_Y;
				}
				break;
			case dfACTION_MOVE_DD:
				if (CharacterMoveCheck(pCharacter->shX, pCharacter->shY + dfSPEED_PLAYER_Y)) {
					pCharacter->shY = pCharacter->shY + dfSPEED_PLAYER_Y;
				}
				break;
			case dfACTION_MOVE_LD:
				if (CharacterMoveCheck(pCharacter->shX - dfSPEED_PLAYER_X, pCharacter->shY + dfSPEED_PLAYER_Y)) {
					pCharacter->shX = pCharacter->shX - dfSPEED_PLAYER_X;
					pCharacter->shY = pCharacter->shY + dfSPEED_PLAYER_Y;
				}
				break;
			case dfACTION_STAND:
			case dfACTION_ATTACK1:
			case dfACTION_ATTACK2:
			case dfACTION_ATTACK3:
				break;
			default:
				_LOG(df_LOG_LEVEL_ERROR, L"Update(), character action error ID:%d", pCharacter->dwSessionID);
				pCharacter->pSession->state |= df_USERSTATE_DELETE;
				break;
			}

			if (pCharacter->dwAction >= dfACTION_MOVE_LL && pCharacter->dwAction <= dfACTION_MOVE_LD) {

				_LOG(df_LOG_LEVEL_DEBUG, L"Update(), ID:%d, Action:%d, Direction:%d, X:%d, Y:%d",
					pCharacter->dwSessionID, pCharacter->dwAction, pCharacter->byDirection, pCharacter->shX, pCharacter->shY);

				//이동인 경우 섹터 업데이트를 함
				if (Sector_UpdateCharacter(pCharacter)) {
					//섹터가 변경된 경우는 클라에게 관련 정보를 쏜다.
					CharacterSectorUpdatePacket(pCharacter);
				}
			}
		}
		
		++iter;
	}
}

//키보드 입력 제어
void ServerControl() {
	//키보드 컨트롤 잠금, 풀림 변수
	static bool bControlMode = false;

	if (_kbhit()) {
		WCHAR controlKey = _getch();

		if (bControlMode) {
			if (L'l' == controlKey || L'L' == controlKey) {
				wprintf(L"Control Lock..! Press U - Control Unlock\n");

				bControlMode = false;
			}

			if (L'q' == controlKey || L'Q' == controlKey) {
				g_bShutdown = false;
			}

			if (L'1' == controlKey) {
				g_wLogLevel = g_wLogLevel ^ df_LOG_LEVEL_DEBUG;
				PrintLogLevel();
			}

			if (L'2' == controlKey) {
				g_wLogLevel = g_wLogLevel ^ df_LOG_LEVEL_DEBUG2;
				PrintLogLevel();
			}

			if (L'3' == controlKey) {
				g_wLogLevel = g_wLogLevel ^ df_LOG_LEVEL_WARNING;
			}

			if (L'4' == controlKey) {
				g_wLogLevel = g_wLogLevel ^ df_LOG_LEVEL_ERROR;
			}

			if (L'5' == controlKey) {
				g_wLogLevel = g_wLogLevel ^ df_LOG_LEVEL_NOTICE;
			}
		}
		else {
			if (L'u' == controlKey || L'U' == controlKey) {
				bControlMode = true;
				//관련 키 도움말
				wprintf(L"Control Mode : Press Q - Quit\n");
				wprintf(L"Control Mode : Press L - Key Lock\n");
				wprintf(L"Control Mode : Press 1 - Log Debug On/Off\n");
				wprintf(L"Control Mode : Press 2 - Log Debug2 On/Off\n");
				wprintf(L"Control Mode : Press 3 - Log Warning On/Off\n");
				wprintf(L"Control Mode : Press 4 - Log Error On/Off\n");
				wprintf(L"Control Mode : Press 5 - Log Notice On/Off\n");
			}
		}
	}
}

//억셉트 처리
void AcceptProc() {
	FD_SET rset;
	timeval time;
	time.tv_sec = 0;
	time.tv_usec = 0;
	int iretcnt = 0;
	int err;
	static int icnt;
	icnt = 0;

	while (icnt < df_FDSET_SIZE) {
		//초기화
		FD_ZERO(&rset);
		//FD 셋
		FD_SET(g_listen_sock, &rset);
		//select
		iretcnt = select(0, &rset, NULL, NULL, &time);
		if (iretcnt == SOCKET_ERROR) {
			err = GetLastError();
			_LOG(df_LOG_LEVEL_ERROR, L"AcceptProc() listen_sock [%d], select error [%d]", g_listen_sock, err);
			return;
		}
		if (FD_ISSET(g_listen_sock, &rset)) {
			//억셉트
			Accept();
			++icnt;
		}
		else {
			break;
		}
	}
}

//억셉트
void Accept() {
	st_SESSION *newSessionUser = new st_SESSION;
	SOCKET clientSocket;		//신규 소켓
	int addrlen;
	int err;
	WCHAR szClientIP[16];

	addrlen = sizeof(newSessionUser->addr);
	//accept()
	clientSocket = accept(g_listen_sock, (SOCKADDR *)&newSessionUser->addr, &addrlen);
	if (clientSocket == INVALID_SOCKET) {
		err = GetLastError();
		if (err == WSAEWOULDBLOCK) {
			//연결 요청 받지 못함
			_LOG(df_LOG_LEVEL_WARNING, L"Accept() WOULDBLOCK");
			delete newSessionUser;
			return;
		}
		_LOG(df_LOG_LEVEL_ERROR, L"Accept() accept error [%d]", err);
		delete newSessionUser;
		return;
	}

	//신규 세션 생성
	CreateSessionUser(newSessionUser, clientSocket);
	//캐릭터 생성
	CreateCharacter(newSessionUser);

	_LOG(df_LOG_LEVEL_NOTICE, L"Aceept %s::%d [UserID:%d]",
		InetNtop(AF_INET, &newSessionUser->addr.sin_addr, szClientIP, 16),
		ntohs(newSessionUser->addr.sin_port),
		g_dwSessionID);

	return;
}

//네트워크 처리
void NetworkProc() {
	map<DWORD, st_SESSION *>::iterator iter;
	map<DWORD, st_SESSION *>::iterator iterTemp;
	map<DWORD, st_SESSION *>::iterator iter_end;
	FD_SET rset;
	FD_SET wset;
	timeval time;
	time.tv_sec = 0;
	time.tv_usec = 0;
	int iFDcnt;
	int icnt;
	int iretcnt = 0;
	int err;
	st_SESSION * sessionUser;
	st_SESSION * sessionArray[df_FDSET_SIZE];
	WCHAR szClientIP[16];
	bool iterFlag = true;

	if (g_mapSessionUser.size() == 0) {
		return;
	}

	iter = g_mapSessionUser.begin();
	iter_end = g_mapSessionUser.end();
	while (iter != iter_end)
	{
		//초기화
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		//FD 셋
		//클라 소켓
		iFDcnt = 0;
		while (iter != iter_end) {
			sessionArray[iFDcnt] = iter->second;
			FD_SET(sessionArray[iFDcnt]->sock, &rset);
			FD_SET(sessionArray[iFDcnt]->sock, &wset);
			++iter;
			++iFDcnt;
			if (iFDcnt >= df_FDSET_SIZE) {
				break;
			}
		}

		//select
		iretcnt = select(0, &rset, &wset, NULL, &time);
		if (iretcnt == SOCKET_ERROR) {
			err = GetLastError();
			_LOG(df_LOG_LEVEL_ERROR, L"select error [%d]", err);
			return;
		}

		//클라 소켓
		icnt = 0;
		while (icnt < iFDcnt) {
			sessionUser = sessionArray[icnt];
			++icnt;
			if (FD_ISSET(sessionUser->sock, &rset)) {
				ReceiveProc(sessionUser);
				--iretcnt;
			}

			if (FD_ISSET(sessionUser->sock, &wset)) {
				if (sessionUser->writeBuffer->GetUseSize() != 0) {
					SandProc(sessionUser);
				}
				--iretcnt;
			}

			//세션 삭제 검사
			if (CheckSessionClosed(sessionUser)) {
				_LOG(df_LOG_LEVEL_NOTICE, L"Disconnect %s::%d  [ID:%d]",
					InetNtop(AF_INET, &sessionUser->addr.sin_addr, szClientIP, 16),
					ntohs(sessionUser->addr.sin_port),
					sessionUser->dwSessionID);
				//캐릭터 삭제
				RemoveCaracter(sessionUser->dwSessionID);
				
				//세션 삭제
				iterTemp = g_mapSessionUser.find(sessionUser->dwSessionID);
				if (icnt == iFDcnt) {
					
					iter = g_mapSessionUser.erase(iterTemp);
				}
				else {
					g_mapSessionUser.erase(iterTemp);
				}
				
				RemoveSession(sessionUser);
			}

			//루프 정지
			if (iretcnt <= 0) {
				break;
			}
		}//while (icnt < iFDcnt)
	}//while (iterTemp != iter_end)



	//map<DWORD, st_SESSION *>::iterator iter;
	//map<DWORD, st_SESSION *>::iterator iterTemp;
	//map<DWORD, st_SESSION *>::iterator iter_begin;
	//map<DWORD, st_SESSION *>::iterator iter_end;
	//FD_SET rset;
	//FD_SET wset;
	//timeval time;
	//time.tv_sec = 0;
	//time.tv_usec = 0;
	//int iFDcnt;
	//int iretcnt = 0;
	//int err;
	//st_SESSION * sessionUser;
	//SOCKET sock;
	//WCHAR szClientIP[16];
	//bool iterFlag = true;

	//if (g_mapSessionUser.size() == 0) {
	//	return;
	//}

	//iter_begin = g_mapSessionUser.begin();
	//iter_end = g_mapSessionUser.end();
	//while (iter_begin != iter_end)
	//{
	//	//초기화
	//	FD_ZERO(&rset);
	//	FD_ZERO(&wset);
	//	//FD 셋
	//	//클라 소켓
	//	iter = iter_begin;
	//	iFDcnt = 0;
	//	while (iter != iter_end) {
	//		sock = iter->second->sock;
	//		FD_SET(sock, &rset);
	//		FD_SET(sock, &wset);
	//		++iter;
	//		++iFDcnt;
	//		if (iFDcnt >= df_FDSET_SIZE) {
	//			break;
	//		}
	//	}
	//	iterTemp = iter_begin;
	//	iter_begin = iter;
	//	iter = iterTemp;

	//	//select
	//	iretcnt = select(0, &rset, &wset, NULL, &time);
	//	if (iretcnt == SOCKET_ERROR) {
	//		err = GetLastError();
	//		printf("[%d] select error\n", err);
	//		return;
	//	}
	//	//클라 소켓
	//	while (iretcnt > 0) {
	//		sessionUser = iter->second;

	//		if (FD_ISSET(sessionUser->sock, &rset)) {
	//			ReceiveProc(sessionUser);
	//			--iretcnt;
	//		}

	//		if (FD_ISSET(sessionUser->sock, &wset)) {
	//			if (sessionUser->writeBuffer->GetUseSize() != 0) {
	//				SandProc(sessionUser);
	//			}
	//			--iretcnt;
	//		}

	//		//세션 삭제 검사
	//		if (CheckSessionClosed(sessionUser)) {
	//			_LOG(df_LOG_LEVEL_NOTICE, L"Disconnect %s::%d  [ID:%d]",
	//				InetNtop(AF_INET, &sessionUser->addr.sin_addr, szClientIP, 16),
	//				ntohs(sessionUser->addr.sin_port),
	//				sessionUser->dwSessionID);
	//			//캐릭터 삭제
	//			RemoveCaracter(sessionUser->dwSessionID);
	//			//세션 삭제
	//			iter = g_mapSessionUser.erase(iter);
	//			RemoveSession(sessionUser);

	//			iterFlag = false;
	//		}

	//		if (iterFlag) {
	//			++iter;
	//		}
	//		else {
	//			iterFlag = true;
	//		}
	//	}//while (iretcnt > 0)
	//}//while (iterTemp != iter_end)
}


//리시브 처리
void ReceiveProc(st_SESSION * SessionUser) {
	int iretval;
	int iReadSize;
	char *chpRear;
	BYTE byEndCode;
	CRingBuffer * readBuffer = SessionUser->readBuffer;
	st_PACKET_HEADER header;
	CSerializationBuffer Packet;


	chpRear = readBuffer->GetWriteBufferPtr();
	iReadSize = readBuffer->GetNotBrokenPutSize();

	//데이터 받기
	iretval = recv(SessionUser->sock, chpRear, iReadSize, 0);
	if (iretval == SOCKET_ERROR) {
		if (iretval == WSAEWOULDBLOCK) {
			return;
		}

		iretval = GetLastError();
		_LOG(df_LOG_LEVEL_ERROR, L"ReceiveProc(), recv error[%d] ID:%d", iretval, SessionUser->dwSessionID);
		SessionUser->state = SessionUser->state | df_USERSTATE_CLOSED;
		return;
	}
	else if (iretval == 0) {
		_LOG(df_LOG_LEVEL_ERROR, L"ReceiveProc(), recv retval 0 ID:%d", SessionUser->dwSessionID);
		SessionUser->state = SessionUser->state | df_USERSTATE_CLOSED;
		return;
	}

	//받은만큼 이동
	readBuffer->MoveRear(iretval);

	//완성된 패킷 전부 처리
	while (1) {

		//헤더 읽기
		iretval = readBuffer->Peek((char *)&header, df_HEADER_SIZE);
		if (iretval < df_HEADER_SIZE) {
			return;
		}

		//패킷 코드 확인
		if (header.byCode != dfNETWORK_PACKET_CODE) {
			SessionUser->state = SessionUser->state | df_USERSTATE_DELETE;
			_LOG(df_LOG_LEVEL_ERROR, L"ReceiveProc(), packet code error ID:%d", SessionUser->dwSessionID);
			return;
		}

		//패킷 완성됬나
		iretval = readBuffer->GetUseSize();
		if (iretval < header.bySize + df_HEADER_SIZE + df_END_CODE_SIZE) {
			return;
		}

		readBuffer->MoveFront(df_HEADER_SIZE);
		//패킷
		readBuffer->Dequeue(Packet.GetWriteBufferPtr(), header.bySize);
		Packet.MoveRear(header.bySize);
		//EndCode
		readBuffer->Dequeue((char *)&byEndCode, df_END_CODE_SIZE);
		if (byEndCode != dfNETWORK_PACKET_END) {
			SessionUser->state = SessionUser->state | df_USERSTATE_DELETE;
			_LOG(df_LOG_LEVEL_ERROR, L"ReceiveProc(), packet end code error ID:%d", SessionUser->dwSessionID);
			return;
		}

		//_LOG(df_LOG_LEVEL_DEBUG, L"type : %d", header.byType);

		// 패킷 처리
		if (!PacketProc(SessionUser, header.byType, &Packet)) {
			return;
		}
		
		Packet.ClearBuffer();
	}//while(1)
}

//샌드 처리
void SandProc(st_SESSION * SessionUser) {
	int iretval;
	char *chpRear;
	int iWriteSize;
	int iNotBrokenSize;
	CRingBuffer * writeBuffer = SessionUser->writeBuffer;

	chpRear = writeBuffer->GetReadBufferPtr();
	iWriteSize = writeBuffer->GetUseSize();
	iNotBrokenSize = writeBuffer->GetNotBrokenGetSize();

	//링버퍼 끝 체크
	if (iWriteSize > iNotBrokenSize) {
		iWriteSize = iNotBrokenSize;
	}

	iretval = send(SessionUser->sock, chpRear, iWriteSize, 0);
	if (iretval == SOCKET_ERROR) {
		if (iretval == WSAEWOULDBLOCK) {
			return;
		}

		//iretval = GetLastError();
		//printf("[%d] send error\n", iretval);
		SessionUser->state = SessionUser->state | df_USERSTATE_DELETE;

		return;
	}

	writeBuffer->MoveFront(iretval);
}

//패킷 처리
bool PacketProc(st_SESSION *pSession, BYTE byPacketType, CSerializationBuffer *pPacket) {
	//패킷 타입
	switch (byPacketType) {
	case dfPACKET_CS_MOVE_START:
		netPacketProc_MoveStart(pSession, pPacket);
		return true;
	case dfPACKET_CS_MOVE_STOP:
		netPacketProc_MoveStop(pSession, pPacket);
		return true;
	case dfPACKET_CS_ATTACK1:
		netPacketProc_Attack1(pSession, pPacket);
		return true;
	case dfPACKET_CS_ATTACK2:
		netPacketProc_Attack2(pSession, pPacket);
		return true;
	case dfPACKET_CS_ATTACK3:
		netPacketProc_Attack3(pSession, pPacket);
		return true;
	case dfPACKET_CS_ECHO:
		netPacketProc_Echo(pSession, pPacket);
		return true;
	default:
		pSession->state = pSession->state | df_USERSTATE_DELETE;
		_LOG(df_LOG_LEVEL_ERROR, L"PacketProc(), packet type error ID:%d", pSession->dwSessionID);
		return false;
	}
}

//이동 패킷 처리
void netPacketProc_MoveStart(st_SESSION *pSession, CSerializationBuffer *pPacket) {
	//---------------------------------------------------------------
	// 10. 캐릭터 이동시작 패킷						Client -> Server
	//
	// 자신의 캐릭터 이동시작시 이 패킷을 보낸다.
	// 이동 중에는 본 패킷을 보내지 않으며, 키 입력이 변경되었을 경우에만
	// 보내줘야 한다.
	//
	// (왼쪽 이동중 위로 이동 / 왼쪽 이동중 왼쪽 위로 이동... 등등)
	//
	//	1	-	Direction	( 방향 디파인 값 8방향 사용 )
	//	2	-	X
	//	2	-	Y
	//
	//---------------------------------------------------------------
	
	BYTE byDirection;
	short shX;
	short shY;

	*pPacket >> &byDirection >> &shX >> &shY;

	_LOG(df_LOG_LEVEL_DEBUG, L"move start, ID:%d, Action:%d, X:%d, Y:%d", pSession->dwSessionID, byDirection, shX, shY);

	//ID로 캐릭터를 검색한다.
	st_CHARACTER *pCharacter = FindCharacter(pSession->dwSessionID);

	if (pCharacter == NULL) {
		_LOG(df_LOG_LEVEL_ERROR, L"netPacketProc_MoveStart(), Character Not Found! ID:%d", pSession->dwSessionID);
		pSession->state = pSession->state | df_USERSTATE_DELETE;
		return;
	}

	//서버의 위치와 받은 패킷의 위치값이 너무 큰 차이가 난다면 데드 레커닝으로 재위치 확인
	//그래도 좌표가 다르다면 싱크 패킷을 보내어 좌표 보정
	if (abs(pCharacter->shX - shX) > dfERROR_RANGE || abs(pCharacter->shY - shY) > dfERROR_RANGE) {
		int idrX = shX;
		int idrY = shY;
		int iDeadFrame = DeadReckoningPos(pCharacter, &idrX, &idrY);

		//서버측 수정된 좌표로 보정
		if (abs(idrX - shX) > dfERROR_RANGE || abs(idrY - shY) > dfERROR_RANGE) {
			pPacket->ClearBuffer();
			mpSync(pPacket, pCharacter->dwSessionID, idrX, idrY);
			SendPacket_Around(idrX / df_SECTOR_WIDTH, idrY / df_SECTOR_HEIGHT, pCharacter->pSession, pPacket, true);
			_LOG(df_LOG_LEVEL_DEBUG2, L"move start sync packet, ID:%d, Direction:%d, X:%d, Y:%d",
				pSession->dwSessionID, byDirection, idrX, idrY);
		}
		shX = idrX;
		shY = idrY;
	}

	//동작을 변경
	pCharacter->dwAction = byDirection;
	pCharacter->byMoveDirection = byDirection;

	//방향을 변경
	switch (byDirection) {
	case dfPACKET_MOVE_DIR_RR:
	case dfPACKET_MOVE_DIR_RU:
	case dfPACKET_MOVE_DIR_RD:
		pCharacter->byDirection = dfPACKET_MOVE_DIR_RR;
		break;
	case dfPACKET_MOVE_DIR_LL:
	case dfPACKET_MOVE_DIR_LU:
	case dfPACKET_MOVE_DIR_LD:
		pCharacter->byDirection = dfPACKET_MOVE_DIR_LL;
		break;
	}

	//좌표 범위 검사
	if (shX > dfRANGE_MOVE_RIGHT) {
		shX = dfRANGE_MOVE_RIGHT;
	}
	else if (shX < dfRANGE_MOVE_LEFT) {
		shX = dfRANGE_MOVE_LEFT;
	}

	if (shY > dfRANGE_MOVE_BOTTOM) {
		shY = dfRANGE_MOVE_BOTTOM;
	}
	else if (shY < dfRANGE_MOVE_TOP) {
		shY = dfRANGE_MOVE_TOP;
	}
	pCharacter->shX = shX;
	pCharacter->shY = shY;

	//정지를 하면서 좌표가 약간 조절된 경우 섹터 업데이트를 함
	if (Sector_UpdateCharacter(pCharacter)) {
		CharacterSectorUpdatePacket(pCharacter);
	}

	//이동 액션이 변경되는 시점에서 데드래커닝을 위한 정보 저장
	pCharacter->dwActionTick = timeGetTime();
	pCharacter->shActionX = pCharacter->shX;
	pCharacter->shActionY = pCharacter->shY;

	pPacket->ClearBuffer();
	mpMoveStart(pPacket, pSession->dwSessionID, byDirection, pCharacter->shX, pCharacter->shY);
	//주변에 보낸다
	SendPacket_Around(pCharacter->CurSector.iX, pCharacter->CurSector.iY, pSession, pPacket);
}

//정지 패킷 처리
void netPacketProc_MoveStop(st_SESSION *pSession, CSerializationBuffer *pPacket) {
	//---------------------------------------------------------------
	// 캐릭터 이동중지 패킷						Client -> Server
	//
	// 이동중 키보드 입력이 없어서 정지되었을 때, 이 패킷을 서버에 보내준다.
	//
	//	1	-	Direction	( 방향 디파인 값 좌/우만 사용 )
	//	2	-	X
	//	2	-	Y
	//
	//---------------------------------------------------------------

	BYTE byDirection;
	short shX;
	short shY;

	*pPacket >> &byDirection >> &shX >> &shY;

	_LOG(df_LOG_LEVEL_DEBUG, L"move stop, ID:%d, Direction:%d, X:%d, Y:%d", pSession->dwSessionID, byDirection, shX, shY);

	//ID로 캐릭터를 검색한다.
	st_CHARACTER *pCharacter = FindCharacter(pSession->dwSessionID);

	if (pCharacter == NULL) {
		_LOG(df_LOG_LEVEL_ERROR, L"netPacketProc_MoveStop(), Character Not Found! ID:%d", pSession->dwSessionID);
		pSession->state = pSession->state | df_USERSTATE_DELETE;
		return;
	}

	//서버의 위치와 받은 패킷의 위치값이 너무 큰 차이가 난다면 데드 레커닝으로 재위치 확인
	//그래도 좌표가 다르다면 싱크 패킷을 보내어 좌표 보정
	if (abs(pCharacter->shX - shX) > dfERROR_RANGE || abs(pCharacter->shY - shY) > dfERROR_RANGE) {
		int idrX = shX;
		int idrY = shY;
		int iDeadFrame = DeadReckoningPos(pCharacter, &idrX, &idrY);

		//서버측 수정된 좌표로 보정
		if (abs(idrX - shX) > dfERROR_RANGE || abs(idrY - shY) > dfERROR_RANGE) {
			pPacket->ClearBuffer();
			mpSync(pPacket, pCharacter->dwSessionID, idrX, idrY);
			SendPacket_Around(idrX / df_SECTOR_WIDTH, idrY / df_SECTOR_HEIGHT, pCharacter->pSession, pPacket, true);
			_LOG(df_LOG_LEVEL_DEBUG, L"move stop sync packet, ID:%d, Direction:%d, X:%d, Y:%d",
				pSession->dwSessionID, byDirection, idrX, idrY);
		}
		shX = idrX;
		shY = idrY;
	}

	//동작을 변경
	pCharacter->dwAction = dfACTION_STAND;
	pCharacter->byMoveDirection = dfACTION_STAND;

	//방향을 변경
	pCharacter->byDirection = byDirection;

	//좌표 범위 검사
	if (shX > dfRANGE_MOVE_RIGHT) {
		shX = dfRANGE_MOVE_RIGHT;
	}
	else if (shX < dfRANGE_MOVE_LEFT) {
		shX = dfRANGE_MOVE_LEFT;
	}

	if (shY > dfRANGE_MOVE_BOTTOM) {
		shY = dfRANGE_MOVE_BOTTOM;
	}
	else if (shY < dfRANGE_MOVE_TOP) {
		shY = dfRANGE_MOVE_TOP;
	}
	pCharacter->shX = shX;
	pCharacter->shY = shY;

	//정지를 하면서 좌표가 약간 조절된 경우 섹터 업데이트를 함
	if (Sector_UpdateCharacter(pCharacter)) {
		CharacterSectorUpdatePacket(pCharacter);
	}

	//이동 액션이 변경되는 시점에서 데드래커닝을 위한 정보 저장
	pCharacter->dwActionTick = timeGetTime();
	pCharacter->shActionX = pCharacter->shX;
	pCharacter->shActionY = pCharacter->shY;

	pPacket->ClearBuffer();
	mpMoveStop(pPacket, pSession->dwSessionID, byDirection, pCharacter->shX, pCharacter->shY);
	//주변에 보낸다
	SendPacket_Around(pCharacter->CurSector.iX, pCharacter->CurSector.iY, pSession, pPacket);
}

//공격1 패킷 처리
void netPacketProc_Attack1(st_SESSION *pSession, CSerializationBuffer *pPacket) {
	//---------------------------------------------------------------
	// 캐릭터 공격 패킷							Client -> Server
	//
	// 공격 키 입력시 본 패킷을 서버에게 보낸다.
	// 충돌 및 데미지에 대한 결과는 서버에서 알려 줄 것이다.
	//
	// 공격 동작 시작시 한번만 서버에게 보내줘야 한다.
	//
	//	1	-	Direction	( 방향 디파인 값. 좌/우만 사용 )
	//	2	-	X
	//	2	-	Y	
	//
	//---------------------------------------------------------------

	BYTE byDirection;
	short shX;
	short shY;

	*pPacket >> &byDirection >> &shX >> &shY;

	_LOG(df_LOG_LEVEL_DEBUG, L"attack1, ID:%d, Direction:%d, X:%d, Y:%d", pSession->dwSessionID, byDirection, shX, shY);

	//ID로 캐릭터를 검색한다.
	st_CHARACTER *pCharacter = FindCharacter(pSession->dwSessionID);

	if (pCharacter == NULL) {
		_LOG(df_LOG_LEVEL_ERROR, L"netPacketProc_Attack1(), Character Not Found! ID:%d", pSession->dwSessionID);
		pSession->state = pSession->state | df_USERSTATE_DELETE;
		return;
	}

	//서버의 위치와 받은 패킷의 위치값이 너무 큰 차이가 난다면 데드 레커닝으로 재위치 확인
	//그래도 좌표가 다르다면 싱크 패킷을 보내어 좌표 보정
	if (abs(pCharacter->shX - shX) > dfERROR_RANGE || abs(pCharacter->shY - shY) > dfERROR_RANGE) {
		int idrX = shX;
		int idrY = shY;
		int iDeadFrame = DeadReckoningPos(pCharacter, &idrX, &idrY);

		//서버측 수정된 좌표로 보정
		if (abs(idrX - shX) > dfERROR_RANGE || abs(idrY - shY) > dfERROR_RANGE) {
			pPacket->ClearBuffer();
			mpSync(pPacket, pCharacter->dwSessionID, idrX, idrY);
			SendPacket_Around(idrX / df_SECTOR_WIDTH, idrY / df_SECTOR_HEIGHT, pCharacter->pSession, pPacket, true);
			_LOG(df_LOG_LEVEL_DEBUG, L"attack1 sync packet, ID:%d, Direction:%d, X:%d, Y:%d",
				pSession->dwSessionID, byDirection, idrX, idrY);
		}
		shX = idrX;
		shY = idrY;
	}

	//동작을 변경
	pCharacter->dwAction = dfACTION_ATTACK1;
	pCharacter->byMoveDirection = dfACTION_STAND;

	//방향을 변경
	pCharacter->byDirection = byDirection;

	//좌표 범위 검사
	if (shX > dfRANGE_MOVE_RIGHT) {
		shX = dfRANGE_MOVE_RIGHT;
	}
	else if (shX < dfRANGE_MOVE_LEFT) {
		shX = dfRANGE_MOVE_LEFT;
	}

	if (shY > dfRANGE_MOVE_BOTTOM) {
		shY = dfRANGE_MOVE_BOTTOM;
	}
	else if (shY < dfRANGE_MOVE_TOP) {
		shY = dfRANGE_MOVE_TOP;
	}
	pCharacter->shX = shX;
	pCharacter->shY = shY;

	//정지를 하면서 좌표가 약간 조절된 경우 섹터 업데이트를 함
	if (Sector_UpdateCharacter(pCharacter)) {
		CharacterSectorUpdatePacket(pCharacter);
	}

	//이동 액션이 변경되는 시점에서 데드래커닝을 위한 정보 저장
	pCharacter->dwActionTick = timeGetTime();
	pCharacter->shActionX = pCharacter->shX;
	pCharacter->shActionY = pCharacter->shY;

	pPacket->ClearBuffer();
	mpAttack1(pPacket, pSession->dwSessionID, byDirection, pCharacter->shX, pCharacter->shY);
	//주변에 보낸다
	SendPacket_Around(pCharacter->CurSector.iX, pCharacter->CurSector.iY, pSession, pPacket);

	//공격1 데미지 서비스
	DamageService_Attack1(pCharacter, pPacket);
}

//공격2 패킷 처리
void netPacketProc_Attack2(st_SESSION *pSession, CSerializationBuffer *pPacket) {
	//---------------------------------------------------------------
	// 캐릭터 공격 패킷							Client -> Server
	//
	// 공격 키 입력시 본 패킷을 서버에게 보낸다.
	// 충돌 및 데미지에 대한 결과는 서버에서 알려 줄 것이다.
	//
	// 공격 동작 시작시 한번만 서버에게 보내줘야 한다.
	//
	//	1	-	Direction	( 방향 디파인 값. 좌/우만 사용 )
	//	2	-	X
	//	2	-	Y	
	//
	//---------------------------------------------------------------

	BYTE byDirection;
	short shX;
	short shY;

	*pPacket >> &byDirection >> &shX >> &shY;

	_LOG(df_LOG_LEVEL_DEBUG, L"attack2, ID:%d, Direction:%d, X:%d, Y:%d", pSession->dwSessionID, byDirection, shX, shY);

	//ID로 캐릭터를 검색한다.
	st_CHARACTER *pCharacter = FindCharacter(pSession->dwSessionID);

	if (pCharacter == NULL) {
		_LOG(df_LOG_LEVEL_ERROR, L"netPacketProc_Attack2(), Character Not Found! ID:%d", pSession->dwSessionID);
		pSession->state = pSession->state | df_USERSTATE_DELETE;
		return;
	}

	//서버의 위치와 받은 패킷의 위치값이 너무 큰 차이가 난다면 데드 레커닝으로 재위치 확인
	//그래도 좌표가 다르다면 싱크 패킷을 보내어 좌표 보정
	if (abs(pCharacter->shX - shX) > dfERROR_RANGE || abs(pCharacter->shY - shY) > dfERROR_RANGE) {
		int idrX = shX;
		int idrY = shY;
		int iDeadFrame = DeadReckoningPos(pCharacter, &idrX, &idrY);

		//서버측 수정된 좌표로 보정
		if (abs(idrX - shX) > dfERROR_RANGE || abs(idrY - shY) > dfERROR_RANGE) {
			pPacket->ClearBuffer();
			mpSync(pPacket, pCharacter->dwSessionID, idrX, idrY);
			SendPacket_Around(idrX / df_SECTOR_WIDTH, idrY / df_SECTOR_HEIGHT, pCharacter->pSession, pPacket, true);
			_LOG(df_LOG_LEVEL_DEBUG, L"attack2 sync packet, ID:%d, Direction:%d, X:%d, Y:%d",
				pSession->dwSessionID, byDirection, idrX, idrY);
		}
		shX = idrX;
		shY = idrY;
	}

	//동작을 변경
	pCharacter->dwAction = dfACTION_ATTACK2;
	pCharacter->byMoveDirection = dfACTION_STAND;

	//방향을 변경
	pCharacter->byDirection = byDirection;

	//좌표 범위 검사
	if (shX > dfRANGE_MOVE_RIGHT) {
		shX = dfRANGE_MOVE_RIGHT;
	}
	else if (shX < dfRANGE_MOVE_LEFT) {
		shX = dfRANGE_MOVE_LEFT;
	}

	if (shY > dfRANGE_MOVE_BOTTOM) {
		shY = dfRANGE_MOVE_BOTTOM;
	}
	else if (shY < dfRANGE_MOVE_TOP) {
		shY = dfRANGE_MOVE_TOP;
	}
	pCharacter->shX = shX;
	pCharacter->shY = shY;

	//정지를 하면서 좌표가 약간 조절된 경우 섹터 업데이트를 함
	if (Sector_UpdateCharacter(pCharacter)) {
		CharacterSectorUpdatePacket(pCharacter);
	}

	//이동 액션이 변경되는 시점에서 데드래커닝을 위한 정보 저장
	pCharacter->dwActionTick = timeGetTime();
	pCharacter->shActionX = pCharacter->shX;
	pCharacter->shActionY = pCharacter->shY;

	pPacket->ClearBuffer();
	mpAttack2(pPacket, pSession->dwSessionID, byDirection, pCharacter->shX, pCharacter->shY);
	//주변에 보낸다
	SendPacket_Around(pCharacter->CurSector.iX, pCharacter->CurSector.iY, pSession, pPacket);

	//공격2 데미지 서비스
	DamageService_Attack2(pCharacter, pPacket);
}

//공격3 패킷 처리
void netPacketProc_Attack3(st_SESSION *pSession, CSerializationBuffer *pPacket) {
	//---------------------------------------------------------------
	// 캐릭터 공격 패킷							Client -> Server
	//
	// 공격 키 입력시 본 패킷을 서버에게 보낸다.
	// 충돌 및 데미지에 대한 결과는 서버에서 알려 줄 것이다.
	//
	// 공격 동작 시작시 한번만 서버에게 보내줘야 한다.
	//
	//	1	-	Direction	( 방향 디파인 값. 좌/우만 사용 )
	//	2	-	X
	//	2	-	Y	
	//
	//---------------------------------------------------------------

	BYTE byDirection;
	short shX;
	short shY;

	*pPacket >> &byDirection >> &shX >> &shY;

	_LOG(df_LOG_LEVEL_DEBUG, L"attack3, ID:%d, Direction:%d, X:%d, Y:%d", pSession->dwSessionID, byDirection, shX, shY);

	//ID로 캐릭터를 검색한다.
	st_CHARACTER *pCharacter = FindCharacter(pSession->dwSessionID);

	if (pCharacter == NULL) {
		_LOG(df_LOG_LEVEL_ERROR, L"netPacketProc_Attack3(), Character Not Found! ID:%d", pSession->dwSessionID);
		pSession->state = pSession->state | df_USERSTATE_DELETE;
		return;
	}

	//서버의 위치와 받은 패킷의 위치값이 너무 큰 차이가 난다면 데드 레커닝으로 재위치 확인
	//그래도 좌표가 다르다면 싱크 패킷을 보내어 좌표 보정
	if (abs(pCharacter->shX - shX) > dfERROR_RANGE || abs(pCharacter->shY - shY) > dfERROR_RANGE) {
		int idrX = shX;
		int idrY = shY;
		int iDeadFrame = DeadReckoningPos(pCharacter, &idrX, &idrY);

		//서버측 수정된 좌표로 보정
		if (abs(idrX - shX) > dfERROR_RANGE || abs(idrY - shY) > dfERROR_RANGE) {
			pPacket->ClearBuffer();
			mpSync(pPacket, pCharacter->dwSessionID, idrX, idrY);
			SendPacket_Around(idrX / df_SECTOR_WIDTH, idrY / df_SECTOR_HEIGHT, pCharacter->pSession, pPacket, true);
			_LOG(df_LOG_LEVEL_DEBUG, L"attack3 sync packet, ID:%d, Direction:%d, X:%d, Y:%d",
				pSession->dwSessionID, byDirection, idrX, idrY);
		}
		shX = idrX;
		shY = idrY;
	}

	//동작을 변경
	pCharacter->dwAction = dfACTION_ATTACK3;
	pCharacter->byMoveDirection = dfACTION_STAND;

	//방향을 변경
	pCharacter->byDirection = byDirection;

	//좌표 범위 검사
	if (shX > dfRANGE_MOVE_RIGHT) {
		shX = dfRANGE_MOVE_RIGHT;
	}
	else if (shX < dfRANGE_MOVE_LEFT) {
		shX = dfRANGE_MOVE_LEFT;
	}

	if (shY > dfRANGE_MOVE_BOTTOM) {
		shY = dfRANGE_MOVE_BOTTOM;
	}
	else if (shY < dfRANGE_MOVE_TOP) {
		shY = dfRANGE_MOVE_TOP;
	}
	pCharacter->shX = shX;
	pCharacter->shY = shY;

	//정지를 하면서 좌표가 약간 조절된 경우 섹터 업데이트를 함
	if (Sector_UpdateCharacter(pCharacter)) {
		CharacterSectorUpdatePacket(pCharacter);
	}

	//이동 액션이 변경되는 시점에서 데드래커닝을 위한 정보 저장
	pCharacter->dwActionTick = timeGetTime();
	pCharacter->shActionX = pCharacter->shX;
	pCharacter->shActionY = pCharacter->shY;

	pPacket->ClearBuffer();
	mpAttack3(pPacket, pSession->dwSessionID, byDirection, pCharacter->shX, pCharacter->shY);
	//주변에 보낸다
	SendPacket_Around(pCharacter->CurSector.iX, pCharacter->CurSector.iY, pSession, pPacket);

	//공격3 데미지 서비스
	DamageService_Attack3(pCharacter, pPacket);
}

//캐릭터 생성 서비스
void CreateMyCharacterService(st_CHARACTER *pCharacter) {
	int iSectorX = pCharacter->CurSector.iX;
	int iSectorY = pCharacter->CurSector.iY;
	CSerializationBuffer PacketSector;
	CSerializationBuffer PacketClient;

	//다른 캐릭터 생성 패킷 만들기
	mpCreateOtherCharacter(&PacketSector, pCharacter->dwSessionID, pCharacter->byDirection,
							pCharacter->shX, pCharacter->shY, pCharacter->chHp);

	AddSectorService(iSectorX, iSectorY, pCharacter, PacketSector, PacketClient);

	if (iSectorX - 1 >= df_SECTOR_MIN_X) {
		AddSectorService(iSectorX - 1, iSectorY, pCharacter, PacketSector, PacketClient);
		if (iSectorY - 1 >= df_SECTOR_MIN_Y) {
			AddSectorService(iSectorX - 1, iSectorY - 1, pCharacter, PacketSector, PacketClient);
		}
		if (iSectorY + 1 < df_SECTOR_MAX_Y) {
			AddSectorService(iSectorX - 1, iSectorY + 1, pCharacter, PacketSector, PacketClient);
		}
	}

	if (iSectorX + 1 < df_SECTOR_MAX_X) {
		AddSectorService(iSectorX + 1, iSectorY, pCharacter, PacketSector, PacketClient);
		if (iSectorY - 1 >= df_SECTOR_MIN_Y) {
			AddSectorService(iSectorX + 1, iSectorY - 1, pCharacter, PacketSector, PacketClient);
		}
		if (iSectorY + 1 < df_SECTOR_MAX_Y) {
			AddSectorService(iSectorX + 1, iSectorY + 1, pCharacter, PacketSector, PacketClient);
		}
	}
	
	if (iSectorY - 1 >= df_SECTOR_MIN_Y) {
		AddSectorService(iSectorX, iSectorY - 1, pCharacter, PacketSector, PacketClient);
	}

	if (iSectorY + 1 < df_SECTOR_MAX_Y) {
		AddSectorService(iSectorX, iSectorY + 1, pCharacter, PacketSector, PacketClient);
	}

	//내 캐릭터 생성 패킷 만들기
	mpCreateMyCharacter(&PacketClient, pCharacter->dwSessionID, pCharacter->byDirection,
							pCharacter->shX, pCharacter->shY, pCharacter->chHp);
	//내 캐릭터 생성 패킷 보내기
	SendPacket_Unicast(pCharacter->pSession, &PacketClient);
}

//에코 패킷 처리
void netPacketProc_Echo(st_SESSION *pSession, CSerializationBuffer *pPacket) {
	//---------------------------------------------------------------
	// Echo 용 패킷					Client -> Server
	//
	//	4	-	Time
	//
	//---------------------------------------------------------------

	DWORD dwTime;

	*pPacket >> &dwTime;

	pPacket->ClearBuffer();
	//에코 패킷 만들기
	mpEcho(pPacket, dwTime);
	//에코 패킷 보내기
	SendPacket_Unicast(pSession, pPacket);
}


//섹터 삭제 서비스
void RemoveSectorService(const int iSectorX, const int iSectorY, st_CHARACTER *pCharacter,
						CSerializationBuffer &pPacketSector, CSerializationBuffer &pPacketClient) {
	list<st_CHARACTER *> *pSectorList;
	list<st_CHARACTER *>::iterator iter;
	list<st_CHARACTER *>::iterator iter_end;

	//1. RemoveSector에 캐릭터 삭제 패킷 보내기
	SendPacket_SectorOne(iSectorX, iSectorY, &pPacketSector, NULL);


	pSectorList = &g_listSector[iSectorY][iSectorX];

	iter = pSectorList->begin();
	iter_end = pSectorList->end();

	while (iter != iter_end) {
		//캐릭터 삭제 패킷 만들기
		mpDeleteCharacter(&pPacketClient, (*iter)->dwSessionID);

		//2. 지금 움직이는 녀석에게, RemoveSector의 캐릭터들 삭제 패킷 보내기
		SendPacket_Unicast(pCharacter->pSession, &pPacketClient);
		pPacketClient.ClearBuffer();
		++iter;
	}
}

//섹터 추가 서비스
void AddSectorService(const int iSectorX, const int iSectorY, st_CHARACTER *pCharacter,
		CSerializationBuffer &pPacketSector, CSerializationBuffer &pPacketSector2, CSerializationBuffer &pPacketClient) {
	st_CHARACTER *pExistCharacter;
	list<st_CHARACTER *> *pSectorList;
	list<st_CHARACTER *>::iterator iter;
	list<st_CHARACTER *>::iterator iter_end;

	//3. AddSector에 캐릭터 생성 패킷 보내기
	SendPacket_SectorOne(iSectorX, iSectorY, &pPacketSector, NULL);

	//3-1 AddSector에 생성된 캐릭터 이동 패킷 보내기
	SendPacket_SectorOne(iSectorX, iSectorY, &pPacketSector2, NULL);


	pSectorList = &g_listSector[iSectorY][iSectorX];

	iter = pSectorList->begin();
	iter_end = pSectorList->end();

	while (iter != iter_end) {
		pExistCharacter = *iter;

		//내가 아닌 경우에만
		if (pExistCharacter != pCharacter) {
			//다른 캐릭터 생성 패킷 만들기
			mpCreateOtherCharacter(&pPacketClient, pExistCharacter->dwSessionID, pExistCharacter->byDirection,
				pExistCharacter->shX, pExistCharacter->shY, pExistCharacter->chHp);

			//4. 이동한 녀석에게 AddSector에 있는 캐릭터 생성 패킷 보내기
			SendPacket_Unicast(pCharacter->pSession, &pPacketClient);
			pPacketClient.ClearBuffer();

			//새 AddSector의 캐릭터가 걷고 있다면 이동 패킷 만들어서 보냄
			switch (pExistCharacter->dwAction) {
			case dfACTION_MOVE_LL:
			case dfACTION_MOVE_LU:
			case dfACTION_MOVE_UU:
			case dfACTION_MOVE_RU:
			case dfACTION_MOVE_RR:
			case dfACTION_MOVE_RD:
			case dfACTION_MOVE_DD:
			case dfACTION_MOVE_LD:
				//캐릭터 이동 패킷 만들기
				mpMoveStart(&pPacketClient, pExistCharacter->dwSessionID, pExistCharacter->byMoveDirection,
					pExistCharacter->shX, pExistCharacter->shY);
				SendPacket_Unicast(pCharacter->pSession, &pPacketClient);
				pPacketClient.ClearBuffer();
				break;
			case dfACTION_ATTACK1:
				//mpAttack1(&Packet, pExistCharacter->dwSessionID, pExistCharacter->byDirection,
				//	pExistCharacter->shX, pExistCharacter->shY);
				break;
			case dfACTION_ATTACK2:
				//mpAttack2(&Packet, pExistCharacter->dwSessionID, pExistCharacter->byDirection,
				//	pExistCharacter->shX, pExistCharacter->shY);
				break;
			case dfACTION_ATTACK3:
				//mpAttack3(&Packet, pExistCharacter->dwSessionID, pExistCharacter->byDirection,
				//	pExistCharacter->shX, pExistCharacter->shY);
				break;
			default:

				break;
			}
		}
		++iter;
	}
}

//섹터 추가 서비스
void AddSectorService(const int iSectorX, const int iSectorY, st_CHARACTER *pCharacter,
					CSerializationBuffer &pPacketSector, CSerializationBuffer &pPacketClient) {
	st_CHARACTER *pExistCharacter;
	list<st_CHARACTER *> *pSectorList;
	list<st_CHARACTER *>::iterator iter;
	list<st_CHARACTER *>::iterator iter_end;

	//섹터에 캐릭터 생성 패킷 보내기
	SendPacket_SectorOne(iSectorX, iSectorY, &pPacketSector, pCharacter->pSession);

	pSectorList = &g_listSector[iSectorY][iSectorX];

	iter = pSectorList->begin();
	iter_end = pSectorList->end();

	while (iter != iter_end) {
		pExistCharacter = *iter;

		//내가 아닌 경우에만
		if (pExistCharacter != pCharacter) {
			//다른 캐릭터 생성 패킷 만들기
			mpCreateOtherCharacter(&pPacketClient, pExistCharacter->dwSessionID, pExistCharacter->byDirection,
									pExistCharacter->shX, pExistCharacter->shY, pExistCharacter->chHp);

			//생성된 캐릭터에게 섹터에 있는 캐릭터 생성 패킷 보내기
			SendPacket_Unicast(pCharacter->pSession, &pPacketClient);
			pPacketClient.ClearBuffer();

			//섹터의 캐릭터가 걷고 있다면 이동 패킷 만들어서 보냄
			switch (pExistCharacter->dwAction) {
			case dfACTION_MOVE_LL:
			case dfACTION_MOVE_LU:
			case dfACTION_MOVE_UU:
			case dfACTION_MOVE_RU:
			case dfACTION_MOVE_RR:
			case dfACTION_MOVE_RD:
			case dfACTION_MOVE_DD:
			case dfACTION_MOVE_LD:
				//캐릭터 이동 패킷 만들기
				mpMoveStart(&pPacketClient, pExistCharacter->dwSessionID, pExistCharacter->byMoveDirection,
					pExistCharacter->shX, pExistCharacter->shY);
				SendPacket_Unicast(pCharacter->pSession, &pPacketClient);
				pPacketClient.ClearBuffer();
				break;
			case dfACTION_ATTACK1:
				//mpAttack1(&Packet, pExistCharacter->dwSessionID, pExistCharacter->byDirection,
				//	pExistCharacter->shX, pExistCharacter->shY);
				break;
			case dfACTION_ATTACK2:
				//mpAttack2(&Packet, pExistCharacter->dwSessionID, pExistCharacter->byDirection,
				//	pExistCharacter->shX, pExistCharacter->shY);
				break;
			case dfACTION_ATTACK3:
				//mpAttack3(&Packet, pExistCharacter->dwSessionID, pExistCharacter->byDirection,
				//	pExistCharacter->shX, pExistCharacter->shY);
				break;
			default:

				break;
			}
		}
		++iter;
	}
}

//공격1 데미지 서비스
//pCharacter : 캐릭터 정보 구조체, pPacketSector : 직렬 버퍼
void DamageService_Attack1(st_CHARACTER *pCharacter, CSerializationBuffer *pPacketSector) {
	st_CHARACTER *pExistCharacter;
	list<st_CHARACTER *> *pSectorList;
	list<st_CHARACTER *>::iterator iter;
	list<st_CHARACTER *>::iterator iter_end;
	short shX = pCharacter->shX;
	short shY = pCharacter->shY;
	BYTE byDirection = pCharacter->byDirection;

	pSectorList = &g_listSector[pCharacter->CurSector.iY][pCharacter->CurSector.iX];

	iter = pSectorList->begin();
	iter_end = pSectorList->end();

	while (iter != iter_end) {
		pExistCharacter = *iter;

		//내가 아닌 경우에만
		if (pExistCharacter != pCharacter) {
			//Y축
			if ((shY - 20) <  pExistCharacter->shY && pExistCharacter->shY < (shY + 20)) {
				//오른쪽
				if (byDirection == dfACTION_MOVE_RR) {
					//X축
					if (shX < pExistCharacter->shX && pExistCharacter->shX < (shX + 100)) {
						// 2018.05.25
						// 0 이하면 0고정임
						if (pExistCharacter->chHp - df_DAMAGE_ATTACK1 < 0) {
							pExistCharacter->chHp = 0;
						}
						else {
							//체력 깎음
							pExistCharacter->chHp = pExistCharacter->chHp - df_DAMAGE_ATTACK1;
						}
						
						//데미지 패킷 만들기
						pPacketSector->ClearBuffer();
						mpDamage(pPacketSector, pCharacter->dwSessionID, pExistCharacter->dwSessionID,
									pExistCharacter->chHp);
						//맞는놈 기준으로 패킷 보냄
						SendPacket_Around(pExistCharacter->CurSector.iX, pExistCharacter->CurSector.iY,
											pExistCharacter->pSession, pPacketSector, true);
					}
				}
				//왼쪽
				else {
					//X축
					if (shX > pExistCharacter->shX && pExistCharacter->shX > (shX - 100)) {
						// 2018.05.25
						// 0 이하면 0고정임
						if (pExistCharacter->chHp - df_DAMAGE_ATTACK1 < 0) {
							pExistCharacter->chHp = 0;
						}
						else {
							//체력 깎음
							pExistCharacter->chHp = pExistCharacter->chHp - df_DAMAGE_ATTACK1;
						}

						//데미지 패킷 만들기
						pPacketSector->ClearBuffer();
						mpDamage(pPacketSector, pCharacter->dwSessionID, pExistCharacter->dwSessionID,
									pExistCharacter->chHp);
						//맞는놈 기준으로 패킷 보냄
						SendPacket_Around(pExistCharacter->CurSector.iX, pExistCharacter->CurSector.iY,
											pExistCharacter->pSession, pPacketSector, true);
					}
				}
			}
		}
		++iter;
	}//while
}

//공격2 데미지 서비스
void DamageService_Attack2(st_CHARACTER *pCharacter, CSerializationBuffer *pPacketSector) {
	st_CHARACTER *pExistCharacter;
	list<st_CHARACTER *> *pSectorList;
	list<st_CHARACTER *>::iterator iter;
	list<st_CHARACTER *>::iterator iter_end;
	short shX = pCharacter->shX;
	short shY = pCharacter->shY;
	BYTE byDirection = pCharacter->byDirection;

	pSectorList = &g_listSector[pCharacter->CurSector.iY][pCharacter->CurSector.iX];

	iter = pSectorList->begin();
	iter_end = pSectorList->end();

	while (iter != iter_end) {
		pExistCharacter = *iter;

		//내가 아닌 경우에만
		if (pExistCharacter != pCharacter) {
			//Y축
			if ((shY - 20) <  pExistCharacter->shY && pExistCharacter->shY < (shY + 20)) {
				//오른쪽
				if (byDirection == dfACTION_MOVE_RR) {
					//X축
					if (shX < pExistCharacter->shX && pExistCharacter->shX < (shX + 100)) {
						// 2018.05.25
						// 0 이하면 0고정임
						if (pExistCharacter->chHp - df_DAMAGE_ATTACK2 < 0) {
							pExistCharacter->chHp = 0;
						}
						else {
							//체력 깎음
							pExistCharacter->chHp = pExistCharacter->chHp - df_DAMAGE_ATTACK2;
						}

						//데미지 패킷 만들기
						pPacketSector->ClearBuffer();
						mpDamage(pPacketSector, pCharacter->dwSessionID, pExistCharacter->dwSessionID,
									pExistCharacter->chHp);
						//맞는놈 기준으로 패킷 보냄
						SendPacket_Around(pExistCharacter->CurSector.iX, pExistCharacter->CurSector.iY,
											pExistCharacter->pSession, pPacketSector, true);
					}
				}
				//왼쪽
				else {
					//X축
					if (shX > pExistCharacter->shX && pExistCharacter->shX > (shX - 100)) {
						// 2018.05.25
						// 0 이하면 0고정임
						if (pExistCharacter->chHp - df_DAMAGE_ATTACK2 < 0) {
							pExistCharacter->chHp = 0;
						}
						else {
							//체력 깎음
							pExistCharacter->chHp = pExistCharacter->chHp - df_DAMAGE_ATTACK2;
						}

						//데미지 패킷 만들기
						pPacketSector->ClearBuffer();
						mpDamage(pPacketSector, pCharacter->dwSessionID, pExistCharacter->dwSessionID,
									pExistCharacter->chHp);
						//맞는놈 기준으로 패킷 보냄
						SendPacket_Around(pExistCharacter->CurSector.iX, pExistCharacter->CurSector.iY,
											pExistCharacter->pSession, pPacketSector, true);
					}
				}
			}
		}
		++iter;
	}//while
}

//공격3 데미지 서비스
void DamageService_Attack3(st_CHARACTER *pCharacter, CSerializationBuffer *pPacketSector) {
	st_CHARACTER *pExistCharacter;
	list<st_CHARACTER *> *pSectorList;
	list<st_CHARACTER *>::iterator iter;
	list<st_CHARACTER *>::iterator iter_end;
	short shX = pCharacter->shX;
	short shY = pCharacter->shY;
	BYTE byDirection = pCharacter->byDirection;

	pSectorList = &g_listSector[pCharacter->CurSector.iY][pCharacter->CurSector.iX];

	iter = pSectorList->begin();
	iter_end = pSectorList->end();

	while (iter != iter_end) {
		pExistCharacter = *iter;

		//내가 아닌 경우에만
		if (pExistCharacter != pCharacter) {
			//Y축
			if ((shY - 20) <  pExistCharacter->shY && pExistCharacter->shY < (shY + 20)) {
				//오른쪽
				if (byDirection == dfACTION_MOVE_RR) {
					//X축
					if (shX < pExistCharacter->shX && pExistCharacter->shX < (shX + 100)) {
						// 2018.05.25
						// 0 이하면 0고정임
						if (pExistCharacter->chHp - df_DAMAGE_ATTACK3 < 0) {
							pExistCharacter->chHp = 0;
						}
						else {
							//체력 깎음
							pExistCharacter->chHp = pExistCharacter->chHp - df_DAMAGE_ATTACK3;
						}

						//데미지 패킷 만들기
						pPacketSector->ClearBuffer();
						mpDamage(pPacketSector, pCharacter->dwSessionID, pExistCharacter->dwSessionID,
									pExistCharacter->chHp);
						//맞는놈 기준으로 패킷 보냄
						SendPacket_Around(pExistCharacter->CurSector.iX, pExistCharacter->CurSector.iY,
											pExistCharacter->pSession, pPacketSector, true);
					}
				}
				//왼쪽
				else {
					//X축
					if (shX > pExistCharacter->shX && pExistCharacter->shX > (shX - 100)) {
						// 2018.05.25
						// 0 이하면 0고정임
						if (pExistCharacter->chHp - df_DAMAGE_ATTACK3 < 0) {
							pExistCharacter->chHp = 0;
						}
						else {
							//체력 깎음
							pExistCharacter->chHp = pExistCharacter->chHp - df_DAMAGE_ATTACK3;
						}

						//데미지 패킷 만들기
						pPacketSector->ClearBuffer();
						mpDamage(pPacketSector, pCharacter->dwSessionID, pExistCharacter->dwSessionID,
									pExistCharacter->chHp);
						//맞는놈 기준으로 패킷 보냄
						SendPacket_Around(pExistCharacter->CurSector.iX, pExistCharacter->CurSector.iY,
											pExistCharacter->pSession, pPacketSector, true);
					}
				}
			}
		}
		++iter;
	}//while
}

//신규 세션 유저 만들기
//NewSessionUser : 유저 세션 구조체
void CreateSessionUser(st_SESSION * NewSessionUser, SOCKET ClientSocket) {
	//SOCKET sock;							// 소켓
	//SOCKADDR_IN addr;						// 주소
	//CRingBuffer * readBuffer;				// 리시브 버퍼
	//CRingBuffer * writeBuffer;			// 샌드 버퍼
	//DWORD dwSessionID;					// 세션 ID
	//DWORD dwTrafficTick;					// 트래픽 체크를 위한 틱
	//DWORD dwTrafficCount;					// 1초마다 송신된 트래픽 수
	//DWORD dwTrafficSecondTick;			// 트래픽 카운트 1초 확인용 Tick
	//BYTE state;							// 상태

	//세션 셋
	NewSessionUser->sock = ClientSocket;
	NewSessionUser->readBuffer = new CRingBuffer(10000);
	NewSessionUser->writeBuffer = new CRingBuffer(10000);
	NewSessionUser->dwSessionID = ++g_dwSessionID;
	NewSessionUser->state = df_USERSTATE_LOGIN;

	//map에 추가
	g_mapSessionUser.insert(SessionUser_Pair(g_dwSessionID, NewSessionUser));
}

//신규 캐릭터 정보 만들기
//NewSessionUser : 유저 세션 구조체
void CreateCharacter(st_SESSION * NewSessionUser) {
	st_CHARACTER *newCharacter = new st_CHARACTER;

	//st_SESSION *pSession;			//세션 포인터
	//DWORD dwSessionID;			//세션 ID
	//DWORD dwAction;				//행동
	//DWORD dwActionTick;			//현재 액션을 취한 시간, 데드레커닝용
	//BYTE byDirection;				//방향
	//BYTE byMoveDirection;			//이동 방향
	//short shX;
	//short shY;
	//short shActionX;				//액션이 변경되었을 때의 좌표
	//short shActionY;
	//st_SECTOR_POS CurSector;
	//st_SECTOR_POS OldSector;
	//char chHp;

	static int iCount = 0;
	static int iSectorX = 0;
	static int iSectorY = 0;

	if (iCount >= 5) {
		iCount = 0;
		++iSectorX;
	}

	if (iSectorX >= df_SECTOR_MAX_X) {
		iSectorX = 0;
		++iSectorY;
	}

	if (iSectorY >= df_SECTOR_MAX_Y) {
		iSectorY = 0;
	}

	++iCount;

	newCharacter->pSession = NewSessionUser;
	newCharacter->dwSessionID = NewSessionUser->dwSessionID;
	newCharacter->dwAction = dfACTION_STAND;
	newCharacter->dwActionTick = timeGetTime();
	newCharacter->byDirection = dfACTION_MOVE_RR;
	newCharacter->byMoveDirection = dfACTION_STAND;
	newCharacter->shX = rand() % (df_SECTOR_WIDTH - 10) + (df_SECTOR_WIDTH * iSectorX);
	newCharacter->shY = rand() % (df_SECTOR_HEIGHT - 10) + (df_SECTOR_HEIGHT * iSectorY);
	newCharacter->CurSector.iX = -1;
	newCharacter->CurSector.iY = -1;
	newCharacter->chHp = df_CHARACTER_HP;
	//섹터에 신규 캐릭터 추가
	Sector_AddCharacter(newCharacter);

	//map에 추가
	g_mapCharacter.insert(Character_Pair(g_dwSessionID, newCharacter));

	//섹터에 캐릭터 생성
	CreateMyCharacterService(newCharacter);

	_LOG(df_LOG_LEVEL_DEBUG, L"create charater, ID:%d, Direction:%d, X:%d, Y:%d",
		newCharacter->dwSessionID, newCharacter->byDirection, newCharacter->shX, newCharacter->shY);
}

//ID로 캐릭터 찾기
st_CHARACTER * FindCharacter(DWORD dwSessionID) {
	map<DWORD, st_CHARACTER *>::iterator iter;
	iter = g_mapCharacter.find(dwSessionID);

	//못찾음
	if (iter == g_mapCharacter.end()) {
		return NULL;
	}

	return iter->second;
}

//데드 레커닝
int DeadReckoningPos(st_CHARACTER *pCharacter, int *pPosX, int *pPosY) {
	int iOldPosX = pCharacter->shActionX;
	int iOldPosY = pCharacter->shActionY;

	//시간차를 구해서 몇 프레임이 지났는지 계산
	DWORD dwIntervalTick = timeGetTime() - pCharacter->dwActionTick;;
	int iActionFrame = dwIntervalTick / 20;
	int iRemoveFrame = 0;
	int iValue;

	int iRPosX = iOldPosX;
	int iRPosY = iOldPosY;

	//1. 계산된 프레임으로 X축, Y축의 좌표 이동값을 구함
	int iDX = iActionFrame * dfRECKONING_SPEED_PLAYER_X;
	int iDY = iActionFrame * dfRECKONING_SPEED_PLAYER_Y;

	switch (pCharacter->dwAction) {
	case dfACTION_MOVE_LL:
		iRPosX = iOldPosX - iDX;
		break;
	case dfACTION_MOVE_LU:
		iRPosX = iOldPosX - iDX;
		iRPosY = iOldPosY - iDY;
		break;
	case dfACTION_MOVE_UU:
		iRPosY = iOldPosY - iDY;
		break;
	case dfACTION_MOVE_RU:
		iRPosX = iOldPosX + iDX;
		iRPosY = iOldPosY - iDY;
		break;
	case dfACTION_MOVE_RR:
		iRPosX = iOldPosX + iDX;
		break;
	case dfACTION_MOVE_RD:
		iRPosX = iOldPosX + iDX;
		iRPosY = iOldPosY + iDY;
		break;
	case dfACTION_MOVE_DD:
		iRPosY = iOldPosY + iDY;
		break;
	case dfACTION_MOVE_LD:
		iRPosX = iOldPosX - iDX;
		iRPosY = iOldPosY + iDY;
		break;
	case dfACTION_STAND:
		// 좌표 변동 없음
		break;
	default:
		_LOG(df_LOG_LEVEL_ERROR, L"DeadReckoningPos() character action error ID:%d, action:%d, oldX:%d, oldY:%d, packetX:%d, packetY:%d",
			pCharacter->dwSessionID, pCharacter->dwAction, iOldPosX, iOldPosY, *pPosX, *pPosY);
		break;
	}

	//여기까지가 iRPosX, iRPosY에 계산된 좌표가 완료 되었음
	//이 아래 부분은 계산된 좌표가 화면의 이동 영역을 벗어난 경우 그 액션을 잘라내기 위해서
	//영억을 벗어난 이후의 프레임을 계산하는 과정
	
	if (iRPosX < dfRANGE_MOVE_LEFT) {
		iValue = abs(dfRANGE_MOVE_LEFT + abs(iRPosX)) / dfRECKONING_SPEED_PLAYER_X;
		iRemoveFrame = max(iValue, iRemoveFrame);
	}
	if (iRPosX > dfRANGE_MOVE_RIGHT) {
		iValue = abs(dfRANGE_MOVE_RIGHT - iRPosX) / dfRECKONING_SPEED_PLAYER_X;
		iRemoveFrame = max(iValue, iRemoveFrame);
	}
	if (iRPosY < dfRANGE_MOVE_TOP) {
		iValue = abs(dfRANGE_MOVE_TOP + abs(iRPosY)) / dfRECKONING_SPEED_PLAYER_Y;
		iRemoveFrame = max(iValue, iRemoveFrame);
	}
	if (iRPosY > dfRANGE_MOVE_BOTTOM) {
		iValue = abs(dfRANGE_MOVE_BOTTOM - iRPosY) / dfRECKONING_SPEED_PLAYER_Y;
		iRemoveFrame = max(iValue, iRemoveFrame);
	}

	//위에서 계산된 결과 삭제 되어야 할 프레임이 나타났다면 좌표를 다시 재 계산 한다.
	if (iRemoveFrame > 0) {
		iActionFrame -= iRemoveFrame;
		//보정된 좌표로 다시 계산
		iDX = iActionFrame * dfRECKONING_SPEED_PLAYER_X;
		iDY = iActionFrame * dfRECKONING_SPEED_PLAYER_Y;

		switch (pCharacter->dwAction)
		{
		case dfACTION_MOVE_LL:
			iRPosX = iOldPosX - iDX;
			break;
		case dfACTION_MOVE_LU:
			iRPosX = iOldPosX - iDX;
			iRPosY = iOldPosY - iDY;
			break;
		case dfACTION_MOVE_UU:
			iRPosY = iOldPosY - iDY;
			break;
		case dfACTION_MOVE_RU:
			iRPosX = iOldPosX + iDX;
			iRPosY = iOldPosY - iDY;
			break;
		case dfACTION_MOVE_RR:
			iRPosX = iOldPosX + iDX;
			break;
		case dfACTION_MOVE_RD:
			iRPosX = iOldPosX + iDX;
			iRPosY = iOldPosY + iDY;
			break;
		case dfACTION_MOVE_DD:
			iRPosY = iOldPosY + iDY;
			break;
		case dfACTION_MOVE_LD:
			iRPosX = iOldPosX - iDX;
			iRPosY = iOldPosY + iDY;
			break;
		case dfACTION_STAND:
			// 좌표 변동 없음
			break;
		default:
			_LOG(df_LOG_LEVEL_ERROR, L"DeadReckoningPos() character action error ID:%d, action:%d",
				pCharacter->dwSessionID, pCharacter->dwAction);
			break;
		}
	}

	//iRPosX = min(iRPosX, dfRANGE_MOVE_RIGHT);
	//iRPosX = max(iRPosX, dfRANGE_MOVE_LEFT);
	//iRPosY = min(iRPosY, dfRANGE_MOVE_BOTTOM);
	//iRPosY = max(iRPosY, dfRANGE_MOVE_TOP);

	if (iRPosX < dfRANGE_MOVE_LEFT) {
		iRPosX = dfRANGE_MOVE_LEFT;
	}
	else if (iRPosX > dfRANGE_MOVE_RIGHT) {
		iRPosX = dfRANGE_MOVE_RIGHT;
	}
	if (iRPosY < dfRANGE_MOVE_TOP) {
		iRPosY = dfRANGE_MOVE_TOP;
	}
	else if (iRPosY > dfRANGE_MOVE_BOTTOM) {
		iRPosY = dfRANGE_MOVE_BOTTOM;
	}


	*pPosX = iRPosX;
	*pPosY = iRPosY;

	return iActionFrame;
}

//캐릭터 이동 좌표 검사
//shX : 좌표 X, shY : 좌표 Y
bool CharacterMoveCheck(short shX, short shY) {
	if (shX <= dfRANGE_MOVE_LEFT || shY <= dfRANGE_MOVE_TOP) {
		return false;
	}
	if (shX >= dfRANGE_MOVE_RIGHT || shY >= dfRANGE_MOVE_BOTTOM) {
		return false;
	}
	return true;
}



//캐릭터의 현재 좌표 shX, shY으로 섹터 위치를 계산하여 해당 섹터에 넣음
//pCharacter : 캐릭터 정보 구조체
void Sector_AddCharacter(st_CHARACTER *pCharacter) {
	//신규 추가는 기존에 들어간 적이 없는 캐릭터만 가능
	if (pCharacter->CurSector.iX != -1 || pCharacter->CurSector.iY != -1) {
		return;
	}

	int iSectorX = pCharacter->shX / df_SECTOR_WIDTH;
	int iSectorY = pCharacter->shY / df_SECTOR_HEIGHT;

	if (iSectorX >= df_SECTOR_MAX_X || iSectorY >= df_SECTOR_MAX_Y) {
		_LOG(df_LOG_LEVEL_ERROR, L"Sector_AddCharacter(), sector position error, ID:%d, SectorX:%d, SectorY:%d",
				pCharacter->dwSessionID, iSectorX, iSectorY);
		return;
	}
	else if (iSectorX < df_SECTOR_MIN_X || iSectorY < df_SECTOR_MIN_Y) {
		_LOG(df_LOG_LEVEL_ERROR, L"Sector_AddCharacter(), sector position error, ID:%d, SectorX:%d, SectorY:%d",
				pCharacter->dwSessionID, iSectorX, iSectorY);
		return;
	}

	g_listSector[iSectorY][iSectorX].push_front(pCharacter);
	//pCharacter->OldSector.iX = pCharacter->CurSector.iX = iSectorX;
	//pCharacter->OldSector.iY = pCharacter->CurSector.iY = iSectorY;
	pCharacter->CurSector.iX = iSectorX;
	pCharacter->CurSector.iY = iSectorY;
}

//캐릭터의 현재 좌표 shX, shY으로 섹터를 계산하여 해당 섹터에서 삭제
//pCharacter : 캐릭터 정보 구조체
void Sector_RemoveCharacter(st_CHARACTER *pCharacter) {
	st_SECTOR_POS &sector = pCharacter->CurSector;

	//캐릭터에 섹터 정보가 없다면 문제 있는 상황
	if (pCharacter->CurSector.iX == -1 || pCharacter->CurSector.iY == -1) {
		_LOG(df_LOG_LEVEL_ERROR, L"Sector_RemoveCharacter(), sector error, ID:%d", pCharacter->dwSessionID);
		pCharacter->pSession->state = pCharacter->pSession->state | df_USERSTATE_DELETE;
		return;
	}

	list<st_CHARACTER *> &pSectorList = g_listSector[sector.iY][sector.iX];
	list<st_CHARACTER *>::iterator iter = pSectorList.begin();
	list<st_CHARACTER *>::iterator iter_end = pSectorList.end();

	while (iter != iter_end) {
		if (*iter == pCharacter) {
			pSectorList.erase(iter);

			pCharacter->OldSector = sector;
			//뭐지
			sector.iX = -1;
			sector.iY = -1;
			return;
		}
		++iter;
	}
	_LOG(df_LOG_LEVEL_ERROR, L"Sector_RemoveCharacter(), character is not on the list, ID:%d", pCharacter->dwSessionID);
}

//위의 RemoveCharacter, AddCharacter를 사용하여
//현재 위치한 섹터에서 삭제 후 현재의 좌표로 섹터를 새롭게 계산하여 해당 섹터에 넣음
//pCharacter : 캐릭터 정보 구조체
bool Sector_UpdateCharacter(st_CHARACTER *pCharacter) {
	int iBeforSectorX = pCharacter->CurSector.iX;
	int iBeforSectorY = pCharacter->CurSector.iY;

	int iNewSectorX = pCharacter->shX / df_SECTOR_WIDTH;
	int iNewSectorY = pCharacter->shY / df_SECTOR_HEIGHT;

	if (iBeforSectorX == iNewSectorX && iBeforSectorY == iNewSectorY) {
		return false;
	}

	Sector_RemoveCharacter(pCharacter);
	Sector_AddCharacter(pCharacter);

	_LOG(df_LOG_LEVEL_DEBUG, L"Sector_UpdateCharacter(), ID:%d, curX:%d, curY:%d, oldX:%d, oldY:%d",
			pCharacter->dwSessionID, pCharacter->CurSector.iX, pCharacter->CurSector.iY, pCharacter->OldSector.iX, pCharacter->OldSector.iY);

	return true;
}

//섹터에서 섹터를 이동 하였을 때 섹터 영향권에서 빠진 섹터, 새로 추가된 섹터의 정보 구하는 함수
//pCharacter : 캐릭터 정보 구조체, pRemoveSector : 섹터 삭제 비트, pAddSector : 섹터 추가 비트
void GetUpdateSectorAround(const st_CHARACTER * const pCharacter, DWORD &byRemoveSector, DWORD &byAddSector) {
	int curX = pCharacter->CurSector.iX;
	int curY = pCharacter->CurSector.iY;
	int oldX = pCharacter->OldSector.iX;
	int oldY = pCharacter->OldSector.iY;

	_LOG(df_LOG_LEVEL_DEBUG, L"GetUpdateSectorAround(), ID:%d, curX:%d, curY:%d, oldX:%d, oldY:%d",
			pCharacter->dwSessionID, curX, curY, oldX, oldY);

	//old, cur 섹터 거리
	int errX = abs(curX - oldX);
	int errY = abs(curY - oldY);

	if (errX >= 3 || errY >= 3) {
		byRemoveSector = byRemoveSector | df_SECTOR_ALL;
		byAddSector = byAddSector | df_SECTOR_ALL;
		return;
	}

	//작음
	if (curX < oldX) {
		//작음
		if (curY < oldY) {
			//↖방향임
			//byRemoveSector = byRemoveSector | df_SECTOR_RU | df_SECTOR_RR | df_SECTOR_RD | df_SECTOR_DD | df_SECTOR_LD;
			byRemoveSector = byRemoveSector | df_MOVE_SECTOR_LU_REMOVE;
			byAddSector = byAddSector | df_MOVE_SECTOR_LU_ADD;
			if (errX == 2) {
				byRemoveSector = byRemoveSector | df_SECTOR_UU | df_SECTOR_CENTER;
				byAddSector = byAddSector | df_SECTOR_DD | df_SECTOR_CENTER;
			}
			if (errY == 2) {
				byRemoveSector = byRemoveSector | df_SECTOR_LL | df_SECTOR_CENTER;
				byAddSector = byAddSector | df_SECTOR_RR | df_SECTOR_CENTER;
			}
		}
		//큼
		else if (curY > oldY) {
			//↙방향임
			byRemoveSector = byRemoveSector | df_MOVE_SECTOR_LD_REMOVE;
			byAddSector = byAddSector | df_MOVE_SECTOR_LD_ADD;
			if (errX == 2) {
				byRemoveSector = byRemoveSector | df_SECTOR_DD | df_SECTOR_CENTER;
				byAddSector = byAddSector | df_SECTOR_UU | df_SECTOR_CENTER;
			}
			if (errY == 2) {
				byRemoveSector = byRemoveSector | df_SECTOR_RR | df_SECTOR_CENTER;
				byAddSector = byAddSector | df_SECTOR_LL | df_SECTOR_CENTER;
			}
		}
		//같음
		else {
			//←방향임
			byRemoveSector = byRemoveSector | df_MOVE_SECTOR_LL_REMOVE;
			byAddSector = byAddSector | df_MOVE_SECTOR_LL_ADD;
			if (errX == 2) {
				byRemoveSector = byRemoveSector | df_SECTOR_UU | df_SECTOR_CENTER | df_SECTOR_DD;
				byAddSector = byAddSector | df_SECTOR_UU | df_SECTOR_CENTER | df_SECTOR_DD;
			}
		}
	}
	//큼
	else if (curX > oldX) {
		//작음
		if (curY < oldY) {
			//↗방향임
			byRemoveSector = byRemoveSector | df_MOVE_SECTOR_RU_REMOVE;
			byAddSector = byAddSector | df_MOVE_SECTOR_RU_ADD;
			if (errX == 2) {
				byRemoveSector = byRemoveSector | df_SECTOR_UU | df_SECTOR_CENTER;
				byAddSector = byAddSector | df_SECTOR_DD | df_SECTOR_CENTER;
			}
			if (errY == 2) {
				byRemoveSector = byRemoveSector | df_SECTOR_RR | df_SECTOR_CENTER;
				byAddSector = byAddSector | df_SECTOR_LL | df_SECTOR_CENTER;
			}
		}
		//큼
		else if (curY > oldY) {
			//↘방향임
			byRemoveSector = byRemoveSector | df_MOVE_SECTOR_RD_REMOVE;
			byAddSector = byAddSector | df_MOVE_SECTOR_RD_ADD;
			if (errX == 2) {
				byRemoveSector = byRemoveSector | df_SECTOR_DD | df_SECTOR_CENTER;
				byAddSector = byAddSector | df_SECTOR_UU | df_SECTOR_CENTER;
			}
			if (errY == 2) {
				byRemoveSector = byRemoveSector | df_SECTOR_RR | df_SECTOR_CENTER;
				byAddSector = byAddSector | df_SECTOR_LL | df_SECTOR_CENTER;
			}
		}
		//같음
		else {
			//→방향임
			byRemoveSector = byRemoveSector | df_MOVE_SECTOR_RR_REMOVE;
			byAddSector = byAddSector | df_MOVE_SECTOR_RR_ADD;
			if (errX == 2) {
				byRemoveSector = byRemoveSector | df_SECTOR_UU | df_SECTOR_CENTER | df_SECTOR_DD;
				byAddSector = byAddSector | df_SECTOR_UU | df_SECTOR_CENTER | df_SECTOR_DD;
			}
		}
	}
	//같음
	else {
		//작음
		if (curY < oldY) {
			//↑방향임
			byRemoveSector = byRemoveSector | df_MOVE_SECTOR_UU_REMOVE;
			byAddSector = byAddSector | df_MOVE_SECTOR_UU_ADD;
			if (errY == 2) {
				byRemoveSector = byRemoveSector | df_SECTOR_RR | df_SECTOR_CENTER | df_SECTOR_LL;
				byAddSector = byAddSector | df_SECTOR_RR | df_SECTOR_CENTER | df_SECTOR_LL;
			}
		}
		//큼
		else if (curY > oldY) {
			//↓방향임
			byRemoveSector = byRemoveSector | df_MOVE_SECTOR_DD_REMOVE;
			byAddSector = byAddSector | df_MOVE_SECTOR_DD_ADD;
			if (errY == 2) {
				byRemoveSector = byRemoveSector | df_SECTOR_RR | df_SECTOR_CENTER | df_SECTOR_LL;
				byAddSector = byAddSector | df_SECTOR_RR | df_SECTOR_CENTER | df_SECTOR_LL;
			}
		}
		//같음
		else {
			_LOG(df_LOG_LEVEL_WARNING, L"GetUpdateSectorAround() onaji sector ID:%d, curX:%d, curY:%d, oldX:%d, oldY:%d",
				pCharacter->dwSessionID, curX, curY, oldX, oldY);
		}
	}
}

//섹터 갱신 패킷
//pCharacter : 캐릭터 정보 구조체
void CharacterSectorUpdatePacket(st_CHARACTER *pCharacter) {
	
	//1. 이전 섹터에서 없어진 부분에 - 캐릭터 삭제 메시지
	//2. 이동하는 캐릭터에게 이전 섹터에서 제외된 섹터의 캐릭터를 삭제 시키는 메시지
	//3. 새로 추가된 섹터에 - 캐릭터 생성 메시지 & 이동 메시지
	//4. 이동하는 캐릭터에게 - 새로 진입한 섹터의 캐릭터들 생성 메시지

	CSerializationBuffer PacketSector;
	CSerializationBuffer PacketSector2;
	CSerializationBuffer PacketClient;
	int iSectorX = pCharacter->OldSector.iX;
	int iSectorY = pCharacter->OldSector.iY;
	DWORD byRemoveSector = df_SECTOR_BASIC;
	DWORD byAddSector = df_SECTOR_BASIC;

	//삭제된곳 추가된곳 얻기
	GetUpdateSectorAround(pCharacter, byRemoveSector, byAddSector);

	//캐릭터 삭제 패킷 만들기
	mpDeleteCharacter(&PacketSector, pCharacter->dwSessionID);

	if (byRemoveSector & df_SECTOR_LL) {
		if (iSectorX - 1 >= df_SECTOR_MIN_X) {
			RemoveSectorService(iSectorX - 1, iSectorY, pCharacter, PacketSector, PacketClient);
		}
	}
	if (byRemoveSector & df_SECTOR_LU) {
		if (iSectorX - 1 >= df_SECTOR_MIN_X && iSectorY - 1 >= df_SECTOR_MIN_Y) {
			RemoveSectorService(iSectorX - 1, iSectorY - 1, pCharacter, PacketSector, PacketClient);
		}
	}
	if (byRemoveSector & df_SECTOR_UU) {
		if (iSectorY - 1 >= df_SECTOR_MIN_Y) {
			RemoveSectorService(iSectorX, iSectorY - 1, pCharacter, PacketSector, PacketClient);
		}
	}
	if (byRemoveSector & df_SECTOR_RU) {
		if (iSectorX + 1 < df_SECTOR_MAX_X && iSectorY - 1 >= df_SECTOR_MIN_Y) {
			RemoveSectorService(iSectorX + 1, iSectorY - 1, pCharacter, PacketSector, PacketClient);
		}
	}
	if (byRemoveSector & df_SECTOR_RR) {
		if (iSectorX + 1 < df_SECTOR_MAX_X) {
			RemoveSectorService(iSectorX + 1, iSectorY, pCharacter, PacketSector, PacketClient);
		}
	}
	if (byRemoveSector & df_SECTOR_RD) {
		if (iSectorX + 1 < df_SECTOR_MAX_X && iSectorY + 1 < df_SECTOR_MAX_Y) {
			RemoveSectorService(iSectorX + 1, iSectorY + 1, pCharacter, PacketSector, PacketClient);
		}
	}
	if (byRemoveSector & df_SECTOR_DD) {
		if (iSectorY + 1 < df_SECTOR_MAX_Y) {
			RemoveSectorService(iSectorX, iSectorY + 1, pCharacter, PacketSector, PacketClient);
		}
	}
	if (byRemoveSector & df_SECTOR_LD) {
		if (iSectorX - 1 >= df_SECTOR_MIN_X && iSectorY + 1 < df_SECTOR_MAX_Y) {
			RemoveSectorService(iSectorX - 1, iSectorY + 1, pCharacter, PacketSector, PacketClient);
		}
	}
	if (byRemoveSector & df_SECTOR_CENTER) {
		RemoveSectorService(iSectorX, iSectorY, pCharacter, PacketSector, PacketClient);
	}



	iSectorX = pCharacter->CurSector.iX;
	iSectorY = pCharacter->CurSector.iY;

	PacketSector.ClearBuffer();

	//캐릭터 생성 패킷 만들기
	mpCreateOtherCharacter(&PacketSector, pCharacter->dwSessionID, pCharacter->byDirection,
							pCharacter->shX, pCharacter->shY, pCharacter->chHp);

	//캐릭터 이동 패킷 만들기
	mpMoveStart(&PacketSector2, pCharacter->dwSessionID, pCharacter->byMoveDirection,
				pCharacter->shX, pCharacter->shY);

	if (byAddSector & df_SECTOR_LL) {
		if (iSectorX - 1 >= df_SECTOR_MIN_X) {
			AddSectorService(iSectorX - 1, iSectorY, pCharacter, PacketSector, PacketSector2, PacketClient);
		}
	}
	if (byAddSector & df_SECTOR_LU) {
		if (iSectorX - 1 >= df_SECTOR_MIN_X && iSectorY - 1 >= df_SECTOR_MIN_Y) {
			AddSectorService(iSectorX - 1, iSectorY - 1, pCharacter, PacketSector, PacketSector2, PacketClient);
		}
	}
	if (byAddSector & df_SECTOR_UU) {
		if (iSectorY - 1 >= df_SECTOR_MIN_Y) {
			AddSectorService(iSectorX, iSectorY - 1, pCharacter, PacketSector, PacketSector2, PacketClient);
		}
	}
	if (byAddSector & df_SECTOR_RU) {
		if (iSectorX + 1 < df_SECTOR_MAX_X && iSectorY - 1 >= df_SECTOR_MIN_Y) {
			AddSectorService(iSectorX + 1, iSectorY - 1, pCharacter, PacketSector, PacketSector2, PacketClient);
		}
	}
	if (byAddSector & df_SECTOR_RR) {
		if (iSectorX + 1 < df_SECTOR_MAX_X) {
			AddSectorService(iSectorX + 1, iSectorY, pCharacter, PacketSector, PacketSector2, PacketClient);
		}
	}
	if (byAddSector & df_SECTOR_RD) {
		if (iSectorX + 1 < df_SECTOR_MAX_X && iSectorY + 1 < df_SECTOR_MAX_Y) {
			AddSectorService(iSectorX + 1, iSectorY + 1, pCharacter, PacketSector, PacketSector2, PacketClient);
		}
	}
	if (byAddSector & df_SECTOR_DD) {
		if (iSectorY + 1 < df_SECTOR_MAX_Y) {
			AddSectorService(iSectorX, iSectorY + 1, pCharacter, PacketSector, PacketSector2, PacketClient);
		}
	}
	if (byAddSector & df_SECTOR_LD) {
		if (iSectorX - 1 >= df_SECTOR_MIN_X && iSectorY + 1 < df_SECTOR_MAX_Y) {
			AddSectorService(iSectorX - 1, iSectorY + 1, pCharacter, PacketSector, PacketSector2, PacketClient);
		}
	}
	if (byAddSector & df_SECTOR_CENTER) {
		AddSectorService(iSectorX, iSectorY, pCharacter, PacketSector, PacketSector2, PacketClient);
	}
}


//캐릭터 삭제 패킷 만들기
//Packet : 직렬 버퍼, dwSessionID : 세션 ID
void mpDeleteCharacter(CSerializationBuffer * pPacket, const DWORD dwSessionID) {
	/*
	BYTE	byCode;			// 패킷코드 0x89 고정.
	BYTE	bySize;			// 패킷 사이즈.
	BYTE	byType;			// 패킷타입.
	BYTE	byTemp;			// 사용안함.
	*/
	//---------------------------------------------------------------
	// 2. 캐릭터 삭제 패킷						Server -> Client
	//
	// 캐릭터의 접속해제 또는 캐릭터가 죽었을때 전송됨.
	//
	//	4	-	ID
	//
	//---------------------------------------------------------------
	
	BYTE bySize = sizeof(dwSessionID);

	//헤더 부분
	*pPacket << dfNETWORK_PACKET_CODE << bySize << (BYTE)dfPACKET_SC_DELETE_CHARACTER;
	pPacket->MoveRear(1);
	//패킷 부분
	*pPacket << dwSessionID << dfNETWORK_PACKET_END;
}

//다른 캐릭터 생성 패킷 만들기
void mpCreateOtherCharacter(CSerializationBuffer * pPacket, const DWORD dwSessionID, const BYTE byDirection,
							const short shX, const short shY, const char chHp) {
	/*
	BYTE	byCode;			// 패킷코드 0x89 고정.
	BYTE	bySize;			// 패킷 사이즈.
	BYTE	byType;			// 패킷타입.
	BYTE	byTemp;			// 사용안함.
	*/
	//---------------------------------------------------------------
	// 1. 다른 클라이언트의 캐릭터 생성 패킷		Server -> Client
	//
	// 처음 서버에 접속시 이미 접속되어 있던 캐릭터들의 정보
	// 또는 게임중에 접속된 클라이언트들의 생성 용 정보.
	//
	//
	//	4	-	ID
	//	1	-	Direction
	//	2	-	X
	//	2	-	Y
	//	1	-	HP
	//
	//---------------------------------------------------------------

	BYTE bySize = sizeof(dwSessionID) + sizeof(byDirection) + sizeof(shX) + sizeof(shY) + sizeof(chHp);

	//헤더 부분
	*pPacket << dfNETWORK_PACKET_CODE << bySize << (BYTE)dfPACKET_SC_CREATE_OTHER_CHARACTER;
	pPacket->MoveRear(1);
	//패킷 부분
	*pPacket << dwSessionID << byDirection << shX << shY << chHp << dfNETWORK_PACKET_END;
}

//내 캐릭터 생성 패킷 만들기
void mpCreateMyCharacter(CSerializationBuffer * pPacket, const DWORD dwSessionID, const BYTE byDirection,
	const short shX, const short shY, const char chHp) {
	/*
	BYTE	byCode;			// 패킷코드 0x89 고정.
	BYTE	bySize;			// 패킷 사이즈.
	BYTE	byType;			// 패킷타입.
	BYTE	byTemp;			// 사용안함.
	*/
	//---------------------------------------------------------------
	// 0 - 클라이언트 자신의 캐릭터 할당		Server -> Client
	//
	// 서버에 접속시 최초로 받게되는 패킷으로 자신이 할당받은 ID 와
	// 자신의 최초 위치, HP 를 받게 된다. (처음에 한번 받게 됨)
	// 
	// 이 패킷을 받으면 자신의 ID,X,Y,HP 를 저장하고 캐릭터를 생성시켜야 한다.
	//
	//	4	-	ID
	//	1	-	Direction
	//	2	-	X
	//	2	-	Y
	//	1	-	HP
	//
	//---------------------------------------------------------------

	BYTE bySize = sizeof(dwSessionID) + sizeof(byDirection) + sizeof(shX) + sizeof(shY) + sizeof(chHp);

	//헤더 부분
	*pPacket << dfNETWORK_PACKET_CODE << bySize << (BYTE)dfPACKET_SC_CREATE_MY_CHARACTER;
	pPacket->MoveRear(1);
	//패킷 부분
	*pPacket << dwSessionID << byDirection << shX << shY << chHp << dfNETWORK_PACKET_END;
}

//캐릭터 이동 패킷 만들기
void mpMoveStart(CSerializationBuffer * pPacket, const DWORD dwSessionID,
				const BYTE byDirection, const short shX, const short shY) {
	/*
	BYTE	byCode;			// 패킷코드 0x89 고정.
	BYTE	bySize;			// 패킷 사이즈.
	BYTE	byType;			// 패킷타입.
	BYTE	byTemp;			// 사용안함.
	*/
	//---------------------------------------------------------------
	// 캐릭터 이동시작 패킷						Server -> Client
	//
	// 다른 유저의 캐릭터 이동시 본 패킷을 받는다.
	// 패킷 수신시 해당 캐릭터를 찾아 이동처리를 해주도록 한다.
	// 
	// 패킷 수신 시 해당 키가 계속해서 눌린것으로 생각하고
	// 해당 방향으로 계속 이동을 하고 있어야만 한다.
	//
	//	4	-	ID
	//	1	-	Direction	( 방향 디파인 값 8방향 )
	//	2	-	X
	//	2	-	Y
	//
	//---------------------------------------------------------------

	BYTE bySize = sizeof(dwSessionID) + sizeof(byDirection) + sizeof(shX) + sizeof(shY);

	//헤더 부분
	*pPacket << dfNETWORK_PACKET_CODE << bySize << (BYTE)dfPACKET_SC_MOVE_START;
	pPacket->MoveRear(1);
	//패킷 부분
	*pPacket << dwSessionID << byDirection << shX << shY << dfNETWORK_PACKET_END;
}

//캐릭터 이동 중지 패킷 만들기
void mpMoveStop(CSerializationBuffer * pPacket, const DWORD dwSessionID,
				const BYTE byDirection, const short shX, const short shY) {
	/*
	BYTE	byCode;			// 패킷코드 0x89 고정.
	BYTE	bySize;			// 패킷 사이즈.
	BYTE	byType;			// 패킷타입.
	BYTE	byTemp;			// 사용안함.
	*/
	//---------------------------------------------------------------
	// 캐릭터 이동중지 패킷						Server -> Client
	//
	// ID 에 해당하는 캐릭터가 이동을 멈춘것이므로 
	// 캐릭터를 찾아서 방향과, 좌표를 입력해주고 멈추도록 처리한다.
	//
	//	4	-	ID
	//	1	-	Direction	( 방향 디파인 값. 좌/우만 사용 )
	//	2	-	X
	//	2	-	Y
	//
	//---------------------------------------------------------------

	BYTE bySize = sizeof(dwSessionID) + sizeof(byDirection) + sizeof(shX) + sizeof(shY);

	//헤더 부분
	*pPacket << dfNETWORK_PACKET_CODE << bySize << (BYTE)dfPACKET_SC_MOVE_STOP;
	pPacket->MoveRear(1);
	//패킷 부분
	*pPacket << dwSessionID << byDirection << shX << shY << dfNETWORK_PACKET_END;
}

//캐릭터 공격1 패킷 만들기
void mpAttack1(CSerializationBuffer * pPacket, const DWORD dwSessionID,
				const BYTE byDirection, const short shX, const short shY) {
	/*
	BYTE	byCode;			// 패킷코드 0x89 고정.
	BYTE	bySize;			// 패킷 사이즈.
	BYTE	byType;			// 패킷타입.
	BYTE	byTemp;			// 사용안함.
	*/
	//---------------------------------------------------------------
	// 캐릭터 공격 패킷							Server -> Client
	//
	// 패킷 수신시 해당 캐릭터를 찾아서 공격1번 동작으로 액션을 취해준다.
	// 방향이 다를 경우에는 해당 방향으로 바꾼 후 해준다.
	//
	//	4	-	ID
	//	1	-	Direction	( 방향 디파인 값. 좌/우만 사용 )
	//	2	-	X
	//	2	-	Y
	//
	//---------------------------------------------------------------

	BYTE bySize = sizeof(dwSessionID) + sizeof(byDirection) + sizeof(shX) + sizeof(shY);

	//헤더 부분
	*pPacket << dfNETWORK_PACKET_CODE << bySize << (BYTE)dfPACKET_SC_ATTACK1;
	pPacket->MoveRear(1);
	//패킷 부분
	*pPacket << dwSessionID << byDirection << shX << shY << dfNETWORK_PACKET_END;
}

//캐릭터 공격2 패킷 만들기
void mpAttack2(CSerializationBuffer * pPacket, const DWORD dwSessionID,
				const BYTE byDirection, const short shX, const short shY) {
	/*
	BYTE	byCode;			// 패킷코드 0x89 고정.
	BYTE	bySize;			// 패킷 사이즈.
	BYTE	byType;			// 패킷타입.
	BYTE	byTemp;			// 사용안함.
	*/
	//---------------------------------------------------------------
	// 캐릭터 공격 패킷							Server -> Client
	//
	// 패킷 수신시 해당 캐릭터를 찾아서 공격2번 동작으로 액션을 취해준다.
	// 방향이 다를 경우에는 해당 방향으로 바꾼 후 해준다.
	//
	//	4	-	ID
	//	1	-	Direction	( 방향 디파인 값. 좌/우만 사용 )
	//	2	-	X
	//	2	-	Y
	//
	//---------------------------------------------------------------

	BYTE bySize = sizeof(dwSessionID) + sizeof(byDirection) + sizeof(shX) + sizeof(shY);

	//헤더 부분
	*pPacket << dfNETWORK_PACKET_CODE << bySize << (BYTE)dfPACKET_SC_ATTACK2;
	pPacket->MoveRear(1);
	//패킷 부분
	*pPacket << dwSessionID << byDirection << shX << shY << dfNETWORK_PACKET_END;
}

//캐릭터 공격3 패킷 만들기
void mpAttack3(CSerializationBuffer * pPacket, const DWORD dwSessionID,
				const BYTE byDirection, const short shX, const short shY) {
	/*
	BYTE	byCode;			// 패킷코드 0x89 고정.
	BYTE	bySize;			// 패킷 사이즈.
	BYTE	byType;			// 패킷타입.
	BYTE	byTemp;			// 사용안함.
	*/
	//---------------------------------------------------------------
	// 캐릭터 공격 패킷							Server -> Client
	//
	// 패킷 수신시 해당 캐릭터를 찾아서 공격3번 동작으로 액션을 취해준다.
	// 방향이 다를 경우에는 해당 방향으로 바꾼 후 해준다.
	//
	//	4	-	ID
	//	1	-	Direction	( 방향 디파인 값. 좌/우만 사용 )
	//	2	-	X
	//	2	-	Y
	//
	//---------------------------------------------------------------

	BYTE bySize = sizeof(dwSessionID) + sizeof(byDirection) + sizeof(shX) + sizeof(shY);

	//헤더 부분
	*pPacket << dfNETWORK_PACKET_CODE << bySize << (BYTE)dfPACKET_SC_ATTACK3;
	pPacket->MoveRear(1);
	//패킷 부분
	*pPacket << dwSessionID << byDirection << shX << shY << dfNETWORK_PACKET_END;
}

//데미지 패킷 만들기
void mpDamage(CSerializationBuffer * pPacket,
					const DWORD dwAttackerID, const DWORD dwDamageID, const char chDamageHp){
	/*
	BYTE	byCode;			// 패킷코드 0x89 고정.
	BYTE	bySize;			// 패킷 사이즈.
	BYTE	byType;			// 패킷타입.
	BYTE	byTemp;			// 사용안함.
	*/
	//---------------------------------------------------------------
	// 캐릭터 데미지 패킷							Server -> Client
	//
	// 공격에 맞은 캐릭터의 정보를 보냄.
	//
	//	4	-	AttackID	( 공격자 ID )
	//	4	-	DamageID	( 피해자 ID )
	//	1	-	DamageHP	( 피해자 HP )
	//
	//---------------------------------------------------------------

	BYTE bySize = sizeof(dwAttackerID) + sizeof(dwDamageID) + sizeof(chDamageHp);

	//헤더 부분
	*pPacket << dfNETWORK_PACKET_CODE << bySize << (BYTE)dfPACKET_SC_DAMAGE;
	pPacket->MoveRear(1);
	//패킷 부분
	*pPacket << dwAttackerID << dwDamageID << chDamageHp << dfNETWORK_PACKET_END;
}

//에코 패킷 만들기
void mpEcho(CSerializationBuffer * pPacket, const DWORD dwTime) {
	//---------------------------------------------------------------
	// Echo 응답 패킷				Server -> Client
	//
	//	4	-	Time
	//
	//---------------------------------------------------------------

	BYTE bySize = sizeof(dwTime);

	//헤더 부분
	*pPacket << dfNETWORK_PACKET_CODE << bySize << (BYTE)dfPACKET_SC_ECHO;
	pPacket->MoveRear(1);
	//패킷 부분
	*pPacket << dwTime << dfNETWORK_PACKET_END;
}

//싱크 패킷 만들기
void mpSync(CSerializationBuffer *pPacket, DWORD dwSessionID, short shX, short shY) {
	/*
	BYTE	byCode;			// 패킷코드 0x89 고정.
	BYTE	bySize;			// 패킷 사이즈.
	BYTE	byType;			// 패킷타입.
	BYTE	byTemp;			// 사용안함.
	*/
	//---------------------------------------------------------------
	// 동기화를 위한 패킷					Server -> Client
	//
	// 서버로부터 동기화 패킷을 받으면 해당 캐릭터를 찾아서
	// 캐릭터 좌표를 보정해준다.
	//
	//	4	-	ID
	//	2	-	X
	//	2	-	Y
	//
	//---------------------------------------------------------------

	BYTE bySize = sizeof(dwSessionID) + sizeof(shX) + sizeof(shY);

	//헤더 부분
	*pPacket << dfNETWORK_PACKET_CODE << bySize << (BYTE)dfPACKET_SC_SYNC;
	pPacket->MoveRear(1);
	//패킷 부분
	*pPacket << dwSessionID << shX << shY << dfNETWORK_PACKET_END;
}

//특정 섹터에 보내기
void SendPacket_SectorOne(int iSectorX, int iSectorY, CSerializationBuffer *pPacket, st_SESSION *pExceptSession) {
	st_SESSION *pSession;
	list<st_CHARACTER *> *pSectorList;
	list<st_CHARACTER *>::iterator iter;
	list<st_CHARACTER *>::iterator iter_end;

	pSectorList = &g_listSector[iSectorY][iSectorX];

	iter = pSectorList->begin();
	iter_end = pSectorList->end();

	while (iter != iter_end) {
		pSession = (*iter)->pSession;

		//예외 검사
		if (pSession != pExceptSession) {
			pSession->writeBuffer->Enqueue(pPacket->GetReadBufferPtr(), pPacket->GetUseSize());
		}
		++iter;
	}
}

//1명에게만 보내기
void SendPacket_Unicast(st_SESSION *pSession, CSerializationBuffer *pPacket) {
	pSession->writeBuffer->Enqueue(pPacket->GetReadBufferPtr(), pPacket->GetUseSize());
}

//클라이언트 기준 주변 섹터에 보내기
void SendPacket_Around(int iSectorX, int iSectorY, st_SESSION *pSession, CSerializationBuffer *pPacket, bool bSendMe) {
	st_SESSION *pExceptSession = NULL;

	if (!bSendMe) {
		pExceptSession = pSession;
	}

	SendPacket_SectorOne(iSectorX, iSectorY, pPacket, pExceptSession);

	if (iSectorX - 1 >= df_SECTOR_MIN_X) {
		SendPacket_SectorOne(iSectorX - 1, iSectorY, pPacket, pExceptSession);
		if (iSectorY - 1 >= df_SECTOR_MIN_Y) {
			SendPacket_SectorOne(iSectorX - 1, iSectorY - 1, pPacket, pExceptSession);
		}
		if (iSectorY + 1 < df_SECTOR_MAX_Y) {
			SendPacket_SectorOne(iSectorX - 1, iSectorY + 1, pPacket, pExceptSession);
		}
	}

	if (iSectorX + 1 < df_SECTOR_MAX_X) {
		SendPacket_SectorOne(iSectorX + 1, iSectorY, pPacket, pExceptSession);
		if (iSectorY - 1 >= df_SECTOR_MIN_Y) {
			SendPacket_SectorOne(iSectorX + 1, iSectorY - 1, pPacket, pExceptSession);
		}
		if (iSectorY + 1 < df_SECTOR_MAX_Y) {
			SendPacket_SectorOne(iSectorX + 1, iSectorY + 1, pPacket, pExceptSession);
		}
	}

	if (iSectorY - 1 >= df_SECTOR_MIN_Y) {
		SendPacket_SectorOne(iSectorX, iSectorY - 1, pPacket, pExceptSession);
	}

	if (iSectorY + 1 < df_SECTOR_MAX_Y) {
		SendPacket_SectorOne(iSectorX, iSectorY + 1, pPacket, pExceptSession);
	}
}

//세션 삭제 검사
bool CheckSessionClosed(st_SESSION * SessionUser) {
	if (SessionUser->state & df_USERSTATE_CLOSED) {
		if (SessionUser->writeBuffer->GetUseSize() == 0) {
			_LOG(df_LOG_LEVEL_DEBUG2, L"CheckSessionClosed(), session closed ID:%d", SessionUser->dwSessionID);
			return true;
		}
	}

	if (SessionUser->state & df_USERSTATE_DELETE) {
		_LOG(df_LOG_LEVEL_DEBUG2, L"CheckSessionClosed(), session delete ID:%d", SessionUser->dwSessionID);
		return true;
	}
	return false;
}

//캐릭터 삭제
void RemoveCaracter(DWORD dwKey) {
	st_CHARACTER * character;
	CSerializationBuffer PacketSector;
	list<st_CHARACTER *> *pSectorList;
	map<DWORD, st_CHARACTER *>::iterator iter;
	list<st_CHARACTER *>::iterator sector_iter;
	list<st_CHARACTER *>::iterator sector_iter_end;

	iter = g_mapCharacter.find(dwKey);

	if (iter == g_mapCharacter.end()) {
		_LOG(df_LOG_LEVEL_ERROR, L"RemoveCaracter(), character is not on the map, ID:%d", dwKey);
		return;
	}


	character = iter->second;

	pSectorList = &g_listSector[character->CurSector.iY][character->CurSector.iX];
	sector_iter = pSectorList->begin();
	sector_iter_end = pSectorList->end();

	while (sector_iter != sector_iter_end) {
		if (*sector_iter == character) {
			//섹터에서 삭제
			pSectorList->erase(sector_iter);
			//캐릭터 삭제 패킷 만들기
			mpDeleteCharacter(&PacketSector, character->dwSessionID);
			//주변에 보낸다
			SendPacket_Around(character->CurSector.iX, character->CurSector.iY,
				character->pSession, &PacketSector, true);
			break;
		}
		++sector_iter;
	}

	//맵에서 삭제
	g_mapCharacter.erase(iter);
	delete character;
}

//세션 삭제
void RemoveSession(st_SESSION * SessionUser) {
	closesocket(SessionUser->sock);
	delete SessionUser->readBuffer;
	delete SessionUser->writeBuffer;
	delete SessionUser;
}

void RemoveSession(DWORD dwKey) {
	map<DWORD, st_SESSION *>::iterator iter;
	map<DWORD, st_SESSION *>::iterator iter_end;
	st_SESSION *sessionUser;

	iter = g_mapSessionUser.begin();
	iter_end = g_mapSessionUser.end();
	while (iter != iter_end) {
		if (iter->first == dwKey) {
			sessionUser = iter->second;
			g_mapSessionUser.erase(iter);
			RemoveSession(sessionUser);
			return;
		}
		++iter;
	}
}

//로그 출력용 함수
void Log() {
	int iRetval;
	__time64_t t64Time;
	tm localTime;
	WCHAR szFileName[32];		// 파일 이름
	WCHAR szBuff[1024];			// 시간 + 로그 버퍼

	// 현제 시간을 얻어봅시다
	_time64(&t64Time);
	_localtime64_s(&localTime, &t64Time);

	localTime.tm_year = localTime.tm_year + 1900;
	localTime.tm_mon = localTime.tm_mon + 1;

	// 날짜가 달라졌는지 비교
	if (g_today.tm_mday != localTime.tm_mday) {
		// 현제 로그 파일 닫고
		fclose(g_fp);
		// 로그 파일 새로 만듭니다
		wsprintfW(szFileName, L"log\\LOG_%d_%d_%d.txt", localTime.tm_year, localTime.tm_mon, localTime.tm_mday);
		iRetval = _wfopen_s(&g_fp, szFileName, L"wt");
		if (iRetval != 0) {
			// 로그 파일이 안만들어짐
			_LOG(df_LOG_LEVEL_ERROR, L"fopen error [%d]", iRetval);
			// 서버 종료
			g_bShutdown = false;
		}

		g_today = localTime;
	}

	// 년_월_일_시:분:초 + 로그
	wsprintfW(szBuff, L"[%d_%d_%d_%d:%d:%d] %s\n",
		localTime.tm_year, localTime.tm_mon, localTime.tm_mday, localTime.tm_hour, localTime.tm_min, localTime.tm_sec, g_szLogBuff);

	wprintf(L"%s", szBuff);
	//fwrite(buff, 1024, 1, g_fp);

	fwprintf_s(g_fp, L"%s", szBuff);
}

void DebugSecter() {
	int iSectorY;
	int iSectorX;
	list<st_CHARACTER *> *pSectorList;
	list<st_CHARACTER *>::iterator iter;
	list<st_CHARACTER *>::iterator iter_end;
	WCHAR wchString[32];


	for (iSectorY = 0; iSectorY < df_SECTOR_MAX_Y; iSectorY++) {
		for (iSectorX = 0; iSectorX < df_SECTOR_MAX_X; iSectorX++) {
			pSectorList = &g_listSector[iSectorY][iSectorX];

			if (pSectorList->size() != 0) {
				wsprintfW(wchString, L"Secter[%d][%d] : %d명", iSectorY, iSectorX, pSectorList->size());
				OutputDebugString(wchString);

				iter = pSectorList->begin();
				iter_end = pSectorList->end();
				OutputDebugString(L" {");
				while (iter != iter_end) {
					wsprintfW(wchString, L"%d ,", (*iter)->dwSessionID);
					OutputDebugString(wchString);
					++iter;
				}
				OutputDebugString(L"}\n");
			}
		}//for
	}//for
}


void PrintLogLevel() {
	wprintf(L"Log Level : ");
	if (g_wLogLevel & df_LOG_LEVEL_DEBUG) {
		wprintf(L"Debug On / ");
	}
	else {
		wprintf(L"Debug Off / ");
	}

	if (g_wLogLevel & df_LOG_LEVEL_DEBUG2) {
		wprintf(L"Debug2 On / ");
	}
	else {
		wprintf(L"Debug2 Off / ");
	}

	if (g_wLogLevel & df_LOG_LEVEL_WARNING) {
		wprintf(L"Warning On / ");
	}
	else {
		wprintf(L"Warning Off / ");
	}

	if (g_wLogLevel & df_LOG_LEVEL_ERROR) {
		wprintf(L"Error On / ");
	}
	else {
		wprintf(L"Error Off / ");
	}

	if (g_wLogLevel & df_LOG_LEVEL_NOTICE) {
		wprintf(L"Notice On\n");
	}
	else {
		wprintf(L"Notice Off\n");
	}
}