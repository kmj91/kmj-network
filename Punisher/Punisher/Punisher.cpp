// Punisher.cpp: 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include <Windows.h>
#include <TimeAPI.h >
#include <windowsx.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <mmsystem.h>
#include <list>

#include "Punisher.h"
#include "CScreenDib.h"
#include "CSpriteDib.h"
#include "MYList.h"
#include "CSerializationBuffer.h"

#include "Player.h"
#include "EffectObject.h"
#include "PacketDefine.h"
#include "CRingBuffer.h"
#include "Map.h"
#include "CSerializationBuffer.h"

#pragma comment( lib, "Imm32.lib") 
#pragma comment( lib, "Winmm.lib")
#pragma comment(lib, "ws2_32")

using namespace std;

#define INPUT_ARRAY_SIZE 32
#define INPUT_DATA_SIZE 24

#define SERVERPORT	20000
#define SERVERIP L"127.0.0.1"
#define UM_SOCKET	(WM_USER+1)
//#define FRAME_DELEY 1000/40
#define FRAME_DELEY 26

#define df_WIN_RIGHT			640
#define df_WIN_BOTTOM			480

#define dfRANGE_MOVE_TOP		0
#define dfRANGE_MOVE_LEFT		0
#define dfRANGE_MOVE_RIGHT		6400
#define dfRANGE_MOVE_BOTTOM		6400

#define df_USER_ZERO			0
#define df_USER_LOGIN			1
#define df_USER_LOGOUT			2
#define df_USER_PLAY_KANOU		4

#define df_HEADER_SIZE			4
#define df_END_CODE_SIZE		1
#define dfNETWORK_PACKET_CODE	((BYTE)0x89)
#define dfNETWORK_PACKET_END	((BYTE)0x79)

#define df_OBJECT_TYPE_PLAYER					100
#define df_OBJECT_TYPE_OTHERPLAYER				300

#define df_OBJECT_TYPE_EFFECT_NUMBER_MIN		0
#define df_OBJECT_TYPE_EFFECT_NUMBER_MAX		99

#define df_LOG_LEVEL_DEBUG			1
#define df_LOG_LEVEL_DEBUG2			2
#define df_LOG_LEVEL_WARNING		4
#define df_LOG_LEVEL_ERROR			8
#define df_LOG_LEVEL_NOTICE			16
#define df_LOG_LEVEL_ALL			31		//df_LOG_LEVEL_DEBUG | df_LOG_LEVEL_DEBUG2 | df_LOG_LEVEL_WARNING | df_LOG_LEVEL_ERROR | df_LOG_LEVEL_NOTICE

#define _LOG(LogLevel, fmt, ...)\
do{\
	if((g_dwLogLevel & LogLevel) == LogLevel){\
		wsprintf(g_szLogBuff, fmt, ##__VA_ARGS__);\
		Log(g_szLogBuff, LogLevel);\
	}\
} while (0)




//패킷의 해더
struct st_PACKET_HEADER {
	BYTE	byCode;			// 패킷코드 0x89 고정.
	BYTE	bySize;			// 패킷 사이즈.
	BYTE	byType;			// 패킷타입.
	BYTE	byTemp;			// 사용안함.
};

class BaseObject;

// 이 코드 모듈에 들어 있는 함수의 정방향 선언입니다.
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    ConnectProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
BOOL ProcessSocketMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);		//유저 메시지 처리
void RecvProc(WPARAM wParam);												//리시브 처리
BOOL RecvPacketProc(BYTE byType, CSerializationBuffer *pPacket);			//리시브 패킷 처리
void PacketSCCreateMyCharacter(CSerializationBuffer *pPacket);				//패킷) 내 캐릭터 생성
void PacketSCCreateOtherCharacter(CSerializationBuffer *pPacket);			//패킷) 다른 사용자 캐릭터 생성
void PacketSCStopCharacter(CSerializationBuffer *pPacket);					//패킷) 캐릭터 정지
void PacketSCMoveCharacter(CSerializationBuffer *pPacket);					//패킷) 캐릭터 이동
void PacketSCCharacterAttack_1(CSerializationBuffer *pPacket);				//패킷) 캐릭터 공격 1				
void PacketSCCharacterAttack_2(CSerializationBuffer *pPacket);				//패킷) 캐릭터 공격 2
void PacketSCCharacterAttack_3(CSerializationBuffer *pPacket);				//패킷) 캐릭터 공격 3
void PacketSCDeleteCharcter(CSerializationBuffer *pPacket);					//패킷) 캐릭터 삭제
void PacketSCDamage(CSerializationBuffer *pPacket);							//패킷) 데미지
void PacketSCSync(CSerializationBuffer *pPacket);

BOOL Connect(WCHAR * szServerIP, int iServerPort);				//서버에 접속
BOOL GameInit(void);
BOOL FrameSkip();												//프레임 스킵
void Update(void);												//게임 처리
void Network();													//네트워크 처리
void KeyProcess();												//키 입력 처리
void CameraProc();												//카메라 처리
void PacketCSMoveStop();										//패킷) 이동 정지
void PacketCSMoveStart(BYTE byKey);								//패킷) 이동 시작
void PacketCSAttack(BYTE byKey);								//패킷) 공격

void CreateHeader(CSerializationBuffer &SerialBuffer, BYTE bySize, BYTE byType);		//헤더 만들기
void CreateMoveStopPacket(CSerializationBuffer &SerialBuffer);							//이동 정지 패킷 만들기
void CreateMoveStartPacket(CSerializationBuffer &SerialBuffer, BYTE byDirection);		//이동 시작 패킷 만들기
void CreateAttackPacket(CSerializationBuffer &SerialBuffer);							//공격 패킷 만들기

void insertionSort(int Num, int Max);													//오브젝트 리스트 정렬
void Log(WCHAR *szString, int iLogLevel);												//로그 출력용 함수

/*
bool Attack(int AttackType, bool Target, Player *Attacker);
void Ai();
bool SelectPlayer(int *PlayerPosX, int *PlayerPosY);
*/

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.	
HWND g_hWnd;									// 윈도우 핸들
HWND g_hDlg;									// 다이얼로그 핸들
bool g_connectFalg = false;						// 컨넥션 성공했는지 여부
bool g_sendFalg = false;						// 샌드 가능한지 여부
CRingBuffer g_readBuffer;						// 리시브 버퍼
CRingBuffer g_writeBuffer;						// 샌드 버퍼

CScreenDib g_cScreenDib(df_WIN_RIGHT, df_WIN_BOTTOM, 32);		// 전역으로 스크린DIB 생성
CSpriteDib g_cSprite(66, 0x00ffffff);			// 최대 스프라이트 개수와 칼라키 값 입력.
list<BaseObject *> *g_list;						// 오브젝트 리스트
Map *g_map;										// 맵

SOCKET g_sock;									// 소켓
Player *g_myPlayer = NULL;						// 플레이어
BYTE g_byPlayerState = df_USER_ZERO;			// 플레이어 상태
__int64 g_time;
__int64 g_fps;
__int64 g_oldTime = 0;
int g_skipFrame = 0;
int g_FramePerSecond = 0;
bool g_bActiveApp;								// 윈도우 활성화 비활성화 상태 체크
bool g_bKeyOnaji = true;
HIMC g_hOldIMC;									// 한글 입력 변환

WORD g_dwLogLevel = 0;							//출력, 저장 대상의 로그 레벨
WCHAR g_szLogBuff[1024];						//로그 저장시 필요한 임시 버퍼


//프로젝트 속성
//c++ 전처리기 전처리기 정의 NDEBUG;_WINDOWS;%(PreprocessorDefinitions)
//링커 시스템 하위시스템 윈도우

//int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
//	_In_opt_ HINSTANCE hPrevInstance,
//	_In_ LPWSTR    lpCmdLine,
//	_In_ int       nCmdShow)
int main()
{
	//UNREFERENCED_PARAMETER(hPrevInstance);
	//UNREFERENCED_PARAMETER(lpCmdLine);

	timeBeginPeriod(1);

	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = NULL;
	wcex.hIcon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_PUNISHER));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDI_PUNISHER);
	wcex.lpszClassName = L"Punisher";
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	RegisterClassExW(&wcex);

	g_hWnd = CreateWindowW(L"Punisher", L"Punisher", WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, nullptr, nullptr, nullptr, nullptr);

	if (!g_hWnd)
	{
		return FALSE;
	}

	ShowWindow(g_hWnd, SW_SHOWNORMAL);
	UpdateWindow(g_hWnd);

	SetFocus(g_hWnd);

	RECT WindowRect;
	WindowRect.top = 0;
	WindowRect.left = 0;
	WindowRect.right = df_WIN_RIGHT;
	WindowRect.bottom = df_WIN_BOTTOM;

	AdjustWindowRectEx(&WindowRect, GetWindowStyle(g_hWnd), GetMenu(g_hWnd) != NULL, GetWindowExStyle(g_hWnd));

	//화면의 크기를 얻어서 중앙으로
	int iX = (GetSystemMetrics(SM_CXSCREEN) / 2) - (df_WIN_RIGHT / 2);
	int iY = (GetSystemMetrics(SM_CYSCREEN) / 2) - (df_WIN_BOTTOM / 2);

	//윈도우 크기 변경
	MoveWindow(g_hWnd, iX, iY, WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top, TRUE);

	//한글 입력 IME창 없애기
	g_hOldIMC = ImmAssociateContext(g_hWnd, NULL);





	//HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDI_PUNISHER));

	MSG msg;

	// TODO: 여기에 코드를 입력합니다.
	//오브젝트 리스트 동적 할당
	g_list = new list<BaseObject*>;

	//서버 IP, 포트번호 입력 받기
	DialogBox(hInst, MAKEINTRESOURCE(IDD_CONNECT), g_hWnd, ConnectProc);

	//게임 초기화
	GameInit();

	//맵 생성
	BaseObject * map = new Map(&g_cSprite, df_WIN_RIGHT, df_WIN_BOTTOM, 32, dfRANGE_MOVE_RIGHT, dfRANGE_MOVE_BOTTOM, 64, 400);
	g_list->push_front(map);
	g_map = (Map*)map;

	g_fps = GetTickCount();
	g_oldTime = g_fps;

	//게임을 위한 메시지 루프
	while (1) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			if (FrameSkip()) {
				//게임 처리 함수
				Update();
			}
		}
	}

	timeEndPeriod(1);
	return 0;
}



//
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  목적:  주 창의 메시지를 처리합니다.
//
//  WM_COMMAND  - 응용 프로그램 메뉴를 처리합니다.
//  WM_PAINT    - 주 창을 그립니다.
//  WM_DESTROY  - 종료 메시지를 게시하고 반환합니다.
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_ACTIVATEAPP:
		g_bActiveApp = (bool)wParam;
		break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// 메뉴 선택을 구문 분석합니다.
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	}
	case UM_SOCKET:
		ProcessSocketMessage(hWnd, message, wParam, lParam);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		//한글 입력 IME 복구
		ImmAssociateContext(hWnd, g_hOldIMC);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// 접속 처리
INT_PTR CALLBACK ConnectProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int iSize;							//문자열 크기
	int iServerPort;					//서버 포트
	WCHAR *szServerIP;					//서버 아이피
	WCHAR szData[INPUT_ARRAY_SIZE];		//문자열 체크용
	WCHAR *szNext;						//다음 문자열

	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		wsprintfW(szData, L"%s:%d", SERVERIP, SERVERPORT);

		SetDlgItemTextW(hDlg, IDC_INSERT_EDIT, szData);
		SetFocus(GetDlgItem(hDlg, IDC_INSERT_EDIT));
		//다이얼로그 핸들 전역변수에 저장
		g_hDlg = hDlg;
		return (INT_PTR)FALSE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			GetDlgItemText(hDlg, IDC_INSERT_EDIT, szData, INPUT_ARRAY_SIZE);
			iSize = wcsnlen_s(szData, INPUT_ARRAY_SIZE);
			if (iSize > INPUT_DATA_SIZE) {
				MessageBox(hDlg, L"자리 수 입력제한", L"Error", MB_OK | MB_ICONERROR);
				SetWindowText(GetDlgItem(hDlg, IDC_INSERT_EDIT), szData);
				return FALSE;
			}
			else if (iSize == 0) {
				MessageBox(hDlg, L"값을 입력해주세요", L"Error", MB_OK | MB_ICONERROR);
				return FALSE;
			}
			
			//입력 값
			GetDlgItemTextW(hDlg, IDC_INSERT_EDIT, szData, INPUT_DATA_SIZE);
			szServerIP = wcstok_s(szData, L":", &szNext);
			iServerPort = _wtoi(szNext);

			if (iServerPort == 0) {
				MessageBox(hDlg, L"접속 실패", L"Error", MB_OK | MB_ICONERROR);
				return FALSE;
			}

			//서버에 접속
			if (!Connect(szServerIP, iServerPort)) {
				MessageBox(hDlg, L"접속 실패", L"Error", MB_OK | MB_ICONERROR);
				return FALSE;
			}

			return (INT_PTR)TRUE;
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			DestroyWindow(g_hWnd);
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


//유저 메시지 처리
BOOL ProcessSocketMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	int iretval;

	// 오류 발생 여부 확인
	if (WSAGETSELECTERROR(lParam)) {
		iretval = WSAGETSELECTERROR(lParam);
		SendMessage(g_hWnd, UM_SOCKET, wParam, FD_CLOSE);
		return TRUE;
	}

	// 메시지 처리
	switch (WSAGETSELECTEVENT(lParam)) {
	case FD_CONNECT:
		//접속 성공
		g_connectFalg = true;
		//다이얼로그 끄기
		EndDialog(g_hDlg, LOWORD(wParam));
		return TRUE;
	case FD_READ:
		if (!g_connectFalg) {
			return TRUE;
		}
		RecvProc(wParam);
		return TRUE;
	case FD_WRITE:
		g_sendFalg = true;
		return TRUE;
	case FD_CLOSE:
		closesocket(wParam);
		return FALSE;
	}

	return TRUE;
}

//리시브 처리
void RecvProc(WPARAM wParam) {
	int iretval;
	int iReadSize;
	char *chpRear;
	BYTE byEndCode;
	st_PACKET_HEADER header;
	CSerializationBuffer serialBuffer;

	chpRear = g_readBuffer.GetWriteBufferPtr();
	iReadSize = g_readBuffer.GetNotBrokenPutSize();

	//데이터 받기
	iretval = recv(wParam, chpRear, iReadSize, 0);
	if (iretval == SOCKET_ERROR) {
		if (iretval == WSAEWOULDBLOCK) {
			return;
		}
		iretval = GetLastError();
		_LOG(df_LOG_LEVEL_ERROR, L"RecvProc(), recv error[%d]", iretval);
		SendMessage(g_hWnd, UM_SOCKET, wParam, FD_CLOSE);
		return;
	}
	else if (iretval == 0) {
		_LOG(df_LOG_LEVEL_ERROR, L"RecvProc(), recv error 0");
		SendMessage(g_hWnd, UM_SOCKET, wParam, FD_CLOSE);
		return;
	}

	//받은만큼 이동
	g_readBuffer.MoveRear(iretval);

	//완성된 패킷 전부 처리
	while (1) {
		//헤더 읽기
		iretval = g_readBuffer.Peek((char *)&header, df_HEADER_SIZE);
		if (iretval < df_HEADER_SIZE) {
			return;
		}

		//패킷 코드 확인
		if (header.byCode != dfNETWORK_PACKET_CODE) {
			_LOG(df_LOG_LEVEL_ERROR, L"RecvProc(), packet code error");
			//패킷코드 에러
			SendMessage(g_hWnd, UM_SOCKET, wParam, FD_CLOSE);
			return;
		}

		//패킷 완성됬나
		iretval = g_readBuffer.GetUseSize();
		if (iretval < header.bySize + df_HEADER_SIZE + df_END_CODE_SIZE) {
			return;
		}

		g_readBuffer.MoveFront(df_HEADER_SIZE);
		//패킷
		g_readBuffer.Dequeue(serialBuffer.GetWriteBufferPtr(), header.bySize);
		serialBuffer.MoveRear(header.bySize);
		//EndCode 검사
		g_readBuffer.Dequeue((char *)&byEndCode, df_END_CODE_SIZE);
		if (byEndCode != dfNETWORK_PACKET_END) {
			_LOG(df_LOG_LEVEL_ERROR, L"RecvProc(), packet end code error");
			//패킷코드 에러
			SendMessage(g_hWnd, UM_SOCKET, wParam, FD_CLOSE);
			return;
		}

		//패킷 처리
		if (!RecvPacketProc(header.byType, &serialBuffer)) {
			_LOG(df_LOG_LEVEL_ERROR, L"RecvProc(), packet type error type:%d", header.byType);
			//잘못된 패킷 타입 에러
			SendMessage(g_hWnd, UM_SOCKET, wParam, FD_CLOSE);
			return;
		}

		//직렬버퍼 초기화
		serialBuffer.ClearBuffer();
	}//while(1)
	

	return;
}

//리시브 패킷 처리
BOOL RecvPacketProc(BYTE byType, CSerializationBuffer *pPacket) {
	//패킷 처리
	switch (byType) {
	case dfPACKET_SC_CREATE_MY_CHARACTER:
		PacketSCCreateMyCharacter(pPacket);
		break;
	case dfPACKET_SC_CREATE_OTHER_CHARACTER:
		PacketSCCreateOtherCharacter(pPacket);
		break;
	case dfPACKET_SC_MOVE_STOP:
		PacketSCStopCharacter(pPacket);
		break;
	case dfPACKET_SC_MOVE_START:
		PacketSCMoveCharacter(pPacket);
		break;
	case dfPACKET_SC_ATTACK1:
		PacketSCCharacterAttack_1(pPacket);
		break;
	case dfPACKET_SC_ATTACK2:
		PacketSCCharacterAttack_2(pPacket);
		break;
	case dfPACKET_SC_ATTACK3:
		PacketSCCharacterAttack_3(pPacket);
		break;
	case dfPACKET_SC_DELETE_CHARACTER:
		PacketSCDeleteCharcter(pPacket);
		break;
	case dfPACKET_SC_DAMAGE:
		PacketSCDamage(pPacket);
		break;
	case dfPACKET_SC_SYNC:
		PacketSCSync(pPacket);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}


//패킷) 내 캐릭터 생성
//iPacketSize : 패킷 사이즈
void PacketSCCreateMyCharacter(CSerializationBuffer *pPacket) {
	BaseObject *myPlayer;
	UINT uiID;				//유저 아이디
	WORD wPosX;				//위치 좌표 x
	WORD wPosY;				//위치 좌표 y
	BYTE byDirection;		//방향
	BYTE byHP;				//체력

	//패킷 읽기
	*pPacket >> &uiID >> &byDirection >> &wPosX >> &wPosY >> &byHP;

	//플레이어 생성
	myPlayer = new Player(uiID , wPosX, wPosY, byDirection, byHP, df_OBJECT_TYPE_PLAYER);
	//전역변수로 가지고있음
	g_myPlayer = (Player *)myPlayer;
	g_byPlayerState = g_byPlayerState | df_USER_LOGIN | df_USER_PLAY_KANOU;
	//오브젝트 리스트에 보관
	g_list->push_back(myPlayer);
}

//패킷) 다른 사용자 캐릭터 생성
//iPacketSize : 패킷 사이즈
void PacketSCCreateOtherCharacter(CSerializationBuffer *pPacket) {
	BaseObject *player;
	UINT uiID;				//유저 아이디
	WORD wPosX;				//위치 좌표 x
	WORD wPosY;				//위치 좌표 y
	BYTE byDirection;		//방향
	BYTE byHP;				//체력

	//패킷 읽기
	*pPacket >> &uiID >> &byDirection >> &wPosX >> &wPosY >> &byHP;
	//다른 플레이어 생성
	player = new Player(uiID, wPosX, wPosY, byDirection, byHP, df_OBJECT_TYPE_OTHERPLAYER);
	//오브젝트 리스트에 보관
	g_list->push_back(player);
}

//패킷) 캐릭터 정지
//iPacketSize : 패킷 사이즈
void PacketSCStopCharacter(CSerializationBuffer *pPacket) {
	Player *player;							//리스트 오브젝트
	UINT uiID;								//유저 아이디
	WORD wPosX;								//위치 좌표 x
	WORD wPosY;								//위치 좌표 y
	BYTE byDirection;						//방향
	list<BaseObject *>::iterator iter;		//이터레이터
	list<BaseObject *>::iterator iter_end;	//이터레이터 끝

	//패킷 읽기
	*pPacket >> &uiID >> &byDirection >> &wPosX >> &wPosY;

	iter = g_list->begin();
	iter_end = g_list->end();
	while (iter != iter_end) {
		player = (Player *)*iter;
		// 2018.05.28
		// 오브젝트 타입 검사
		if (player->m_type == df_OBJECT_TYPE_OTHERPLAYER || player->m_type == df_OBJECT_TYPE_PLAYER) {
			if (player->GetID() == uiID) {
				//위치 좌표 동기화
				player->SetPosition(wPosX, wPosY);
				//정지
				player->PacketAction(dfACTION_STAND, byDirection);
				return;
			}
		}
		++iter;
	}
	//없는 사용자
	//에러 처리
	return;
}

//패킷) 캐릭터 이동
//iPacketSize : 패킷 사이즈
void PacketSCMoveCharacter(CSerializationBuffer *pPacket) {
	Player *player;							//리스트 오브젝트
	UINT uiID;								//유저 아이디
	WORD wPosX;								//위치 좌표 x
	WORD wPosY;								//위치 좌표 y
	BYTE byDirection;						//방향
	list<BaseObject *>::iterator iter;		//이터레이터
	list<BaseObject *>::iterator iter_end;	//이터레이터 끝

	//패킷 읽기
	*pPacket >> &uiID >> &byDirection >> &wPosX >> &wPosY;

	iter = g_list->begin();
	iter_end = g_list->end();
	while (iter != iter_end) {
		player = (Player *)*iter;
		// 2018.05.28
		// 오브젝트 타입 검사
		if (player->m_type == df_OBJECT_TYPE_OTHERPLAYER || player->m_type == df_OBJECT_TYPE_PLAYER) {
			if (player->GetID() == uiID) {
				//위치 좌표 동기화
				player->SetPosition(wPosX, wPosY);
				//이동
				player->PacketAction(byDirection);
				return;
			}
		}
		++iter;
	}
	//없는 사용자
	//에러 처리
	return;
}

//패킷) 캐릭터 공격 1
//iPacketSize : 패킷 사이즈
void PacketSCCharacterAttack_1(CSerializationBuffer *pPacket) {
	Player *player;							//리스트 오브젝트
	UINT uiID;								//유저 아이디
	WORD wPosX;								//위치 좌표 x
	WORD wPosY;								//위치 좌표 y
	BYTE byDirection;						//방향
	list<BaseObject *>::iterator iter;		//이터레이터
	list<BaseObject *>::iterator iter_end;	//이터레이터 끝

	//패킷 읽기
	*pPacket >> &uiID >> &byDirection >> &wPosX >> &wPosY;

	iter = g_list->begin();
	iter_end = g_list->end();
	while (iter != iter_end) {
		player = (Player *)*iter;
		// 2018.05.28
		// 오브젝트 타입 검사
		if (player->m_type == df_OBJECT_TYPE_OTHERPLAYER || player->m_type == df_OBJECT_TYPE_PLAYER) {
			if (player->GetID() == uiID) {
				//위치 좌표 동기화
				player->SetPosition(wPosX, wPosY);
				//정지
				player->PacketAction(dfACTION_ATTACK1, byDirection);
				return;
			}
		}
		++iter;
	}
	//없는 사용자
	//에러 처리
	return;
}


//패킷) 캐릭터 공격 2
//iPacketSize : 패킷 사이즈
void PacketSCCharacterAttack_2(CSerializationBuffer *pPacket) {
	Player *player;							//리스트 오브젝트
	UINT uiID;								//유저 아이디
	WORD wPosX;								//위치 좌표 x
	WORD wPosY;								//위치 좌표 y
	BYTE byDirection;						//방향
	list<BaseObject *>::iterator iter;		//이터레이터
	list<BaseObject *>::iterator iter_end;	//이터레이터 끝

	//패킷 읽기
	*pPacket >> &uiID >> &byDirection >> &wPosX >> &wPosY;

	iter = g_list->begin();
	iter_end = g_list->end();
	while (iter != iter_end) {
		player = (Player *)*iter;
		// 2018.05.28
		// 오브젝트 타입 검사
		if (player->m_type == df_OBJECT_TYPE_OTHERPLAYER || player->m_type == df_OBJECT_TYPE_PLAYER) {
			if (player->GetID() == uiID) {
				//위치 좌표 동기화
				player->SetPosition(wPosX, wPosY);
				//정지
				player->PacketAction(dfACTION_ATTACK2, byDirection);
				return;
			}
		}
		++iter;
	}
	//없는 사용자
	//에러 처리
	return;
}


//패킷) 캐릭터 공격 3
//iPacketSize : 패킷 사이즈
void PacketSCCharacterAttack_3(CSerializationBuffer *pPacket) {
	Player *player;							//리스트 오브젝트
	UINT uiID;								//유저 아이디
	WORD wPosX;								//위치 좌표 x
	WORD wPosY;								//위치 좌표 y
	BYTE byDirection;						//방향
	list<BaseObject *>::iterator iter;		//이터레이터
	list<BaseObject *>::iterator iter_end;	//이터레이터 끝

	//패킷 읽기
	*pPacket >> &uiID >> &byDirection >> &wPosX >> &wPosY;

	iter = g_list->begin();
	iter_end = g_list->end();
	while (iter != iter_end) {
		player = (Player *)*iter;
		// 2018.05.28
		// 오브젝트 타입 검사
		if (player->m_type == df_OBJECT_TYPE_OTHERPLAYER || player->m_type == df_OBJECT_TYPE_PLAYER) {
			if (player->GetID() == uiID) {
				//위치 좌표 동기화
				player->SetPosition(wPosX, wPosY);
				//정지
				player->PacketAction(dfACTION_ATTACK3, byDirection);
				return;
			}
		}
		++iter;
	}
	//없는 사용자
	//에러 처리
	return;
}

//패킷) 캐릭터 삭제
//iPacketSize : 패킷 사이즈
void PacketSCDeleteCharcter(CSerializationBuffer *pPacket) {
	Player *player;							//리스트 오브젝트
	UINT uiID;								//유저 아이디
	list<BaseObject *>::iterator iter;		//이터레이터
	list<BaseObject *>::iterator iter_end;	//이터레이터 끝

	//패킷 읽기
	*pPacket >> &uiID;

	iter = g_list->begin();
	iter_end = g_list->end();
	while (iter != iter_end) {
		player = (Player *)*iter;
		// 2018.05.28
		// 오브젝트 리스트에서 이상한거 삭제됨 추가
		if (player->m_type == df_OBJECT_TYPE_OTHERPLAYER || player->m_type == df_OBJECT_TYPE_PLAYER) {
			if (player->GetID() == uiID) {
				//삭제
				g_list->erase(iter);
				delete player;
				return;
			}
		}
		++iter;
	}
	//없는 사용자
	//에러 처리
	return;
}


//패킷) 데미지
//iPacketSize : 패킷 사이즈
void PacketSCDamage(CSerializationBuffer *pPacket) {
	CSerializationBuffer serialBuffer;
	Player *temp;
	Player *attacker = NULL;				//때린놈
	Player *target = NULL;					//맞는놈
	BaseObject *effect;						//이펙트
	UINT uiAttackID;						//공격자 ID
	UINT uiDamageID;						//피해자 ID
	int iDelay = 0;							//딜레이
	int iDamage;							//딜량
	BYTE DamageHP;							//피해자 HP
	bool drawRed = false;					//이거 이펙트 내꺼임?
	list<BaseObject *>::iterator iter;		//이터레이터
	list<BaseObject *>::iterator iter_end;	//이터레이터 끝

	//패킷 읽기
	*pPacket >> &uiAttackID >> &uiDamageID >> &DamageHP;

	iter = g_list->begin();
	iter_end = g_list->end();
	while (iter != iter_end) {
		temp = (Player *)*iter;
		// 2018.05.28
		// 오브젝트 타입 검사
		if (temp->m_type == df_OBJECT_TYPE_OTHERPLAYER || temp->m_type == df_OBJECT_TYPE_PLAYER) {
			//때린놈
			if (temp->GetID() == uiAttackID) {
				attacker = temp;
			}
			//맞는놈
			if (temp->GetID() == uiDamageID) {
				target = temp;
			}
		}
		++iter;
	}
	
	//공격자 피해자가 없는가?
	if (attacker == NULL || target == NULL) {
		//없는 사용자
		//에러 처리
		_LOG(df_LOG_LEVEL_ERROR, L"PacketSCDamage(), Character Not Found!");
		return;
	}

	//내가 공격자인가?
	if (attacker->GetID() == g_myPlayer->GetID()) {
		//이펙트 빨간색톤으로
		drawRed = true;
	}

	switch (attacker->GetKey()) {
	case dfACTION_ATTACK1:
		iDelay = 0;
		break;
	case dfACTION_ATTACK2:
		iDelay = 5;
		break;
	case dfACTION_ATTACK3:
		iDelay = 15;
		break;
	}

	//딜량 계산
	iDamage = target->GetHp() - DamageHP;

	//이펙트 오브젝트 생성
	effect = new EffectObject(target, iDamage, iDelay, drawRed, 0);
	g_list->push_back(effect);

	_LOG(df_LOG_LEVEL_DEBUG2, L"PacketSCDamage(), attacker:%d, target:%d", attacker->GetID(), target->GetID());

	return;
}

void PacketSCSync(CSerializationBuffer *pPacket) {
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

	CSerializationBuffer serialBuffer;
	Player *player;							//리스트 오브젝트
	UINT uiID;								//유저 아이디
	WORD wPosX;
	WORD wPosY;
	list<BaseObject *>::iterator iter;		//이터레이터
	list<BaseObject *>::iterator iter_end;	//이터레이터 끝

	//패킷 읽기
	*pPacket >> &uiID >> &wPosX >> &wPosY;

	iter = g_list->begin();
	iter_end = g_list->end();
	while (iter != iter_end) {
		player = (Player *)*iter;
		// 2018.05.28
		// 오브젝트 타입 검사
		if (player->m_type == df_OBJECT_TYPE_OTHERPLAYER || player->m_type == df_OBJECT_TYPE_PLAYER) {
			if (player->GetID() == uiID) {
				//위치 좌표 동기화
				player->SetPosition(wPosX, wPosY);
				return;
			}
		}
		++iter;
	}
	//없는 사용자
	//에러 처리
	return;
}

//서버에 접속
BOOL Connect(WCHAR * szServerIP, int iServerPort) {
	int iretval;

	//윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// socket()
	g_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (g_sock == INVALID_SOCKET) {
		iretval = GetLastError();
		return FALSE;
	}

	// WSAAsyncSelect()
	iretval = WSAAsyncSelect(g_sock, g_hWnd, UM_SOCKET,
		FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE);
	if (iretval == SOCKET_ERROR) {
		iretval = GetLastError();
		closesocket(g_sock);
		WSACleanup();
		return FALSE;
	}
	
	// connect()
	SOCKADDR_IN serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	InetPton(AF_INET, szServerIP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(iServerPort);
	connect(g_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));


	return TRUE;
}




/*****************************
	GameInit
	게임 초기화
******************************/
BOOL GameInit(void)
{
	//로그 레벨
	g_dwLogLevel = g_dwLogLevel | df_LOG_LEVEL_ERROR;

	g_cSprite.LoadDibSprite(0, L"Data\\_Map.bmp", 0, 0);

	g_cSprite.LoadDibSprite(1, L"Data\\Stand_L_01.bmp", 71, 90);
	g_cSprite.LoadDibSprite(2, L"Data\\Stand_L_02.bmp", 71, 90);
	g_cSprite.LoadDibSprite(3, L"Data\\Stand_L_03.bmp", 71, 90);

	g_cSprite.LoadDibSprite(4, L"Data\\Stand_R_01.bmp", 71, 90);
	g_cSprite.LoadDibSprite(5, L"Data\\Stand_R_02.bmp", 71, 90);
	g_cSprite.LoadDibSprite(6, L"Data\\Stand_R_03.bmp", 71, 90);

	g_cSprite.LoadDibSprite(7, L"Data\\Move_L_01.bmp", 71, 90);
	g_cSprite.LoadDibSprite(8, L"Data\\Move_L_02.bmp", 71, 90);
	g_cSprite.LoadDibSprite(9, L"Data\\Move_L_03.bmp", 71, 90);
	g_cSprite.LoadDibSprite(10, L"Data\\Move_L_04.bmp", 71, 90);
	g_cSprite.LoadDibSprite(11, L"Data\\Move_L_05.bmp", 71, 90);
	g_cSprite.LoadDibSprite(12, L"Data\\Move_L_06.bmp", 71, 90);
	g_cSprite.LoadDibSprite(13, L"Data\\Move_L_07.bmp", 71, 90);
	g_cSprite.LoadDibSprite(14, L"Data\\Move_L_08.bmp", 71, 90);
	g_cSprite.LoadDibSprite(15, L"Data\\Move_L_09.bmp", 71, 90);
	g_cSprite.LoadDibSprite(16, L"Data\\Move_L_10.bmp", 71, 90);
	g_cSprite.LoadDibSprite(17, L"Data\\Move_L_11.bmp", 71, 90);
	g_cSprite.LoadDibSprite(18, L"Data\\Move_L_12.bmp", 71, 90);

	g_cSprite.LoadDibSprite(19, L"Data\\Move_R_01.bmp", 71, 90);
	g_cSprite.LoadDibSprite(20, L"Data\\Move_R_02.bmp", 71, 90);
	g_cSprite.LoadDibSprite(21, L"Data\\Move_R_03.bmp", 71, 90);
	g_cSprite.LoadDibSprite(22, L"Data\\Move_R_04.bmp", 71, 90);
	g_cSprite.LoadDibSprite(23, L"Data\\Move_R_05.bmp", 71, 90);
	g_cSprite.LoadDibSprite(24, L"Data\\Move_R_06.bmp", 71, 90);
	g_cSprite.LoadDibSprite(25, L"Data\\Move_R_07.bmp", 71, 90);
	g_cSprite.LoadDibSprite(26, L"Data\\Move_R_08.bmp", 71, 90);
	g_cSprite.LoadDibSprite(27, L"Data\\Move_R_09.bmp", 71, 90);
	g_cSprite.LoadDibSprite(28, L"Data\\Move_R_10.bmp", 71, 90);
	g_cSprite.LoadDibSprite(29, L"Data\\Move_R_11.bmp", 71, 90);
	g_cSprite.LoadDibSprite(30, L"Data\\Move_R_12.bmp", 71, 90);

	g_cSprite.LoadDibSprite(31, L"Data\\Attack1_L_01.bmp", 71, 90);
	g_cSprite.LoadDibSprite(32, L"Data\\Attack1_L_02.bmp", 71, 90);
	g_cSprite.LoadDibSprite(33, L"Data\\Attack1_L_03.bmp", 71, 90);
	g_cSprite.LoadDibSprite(34, L"Data\\Attack1_L_04.bmp", 71, 90);

	g_cSprite.LoadDibSprite(35, L"Data\\Attack2_L_01.bmp", 71, 90);
	g_cSprite.LoadDibSprite(36, L"Data\\Attack2_L_02.bmp", 71, 90);
	g_cSprite.LoadDibSprite(37, L"Data\\Attack2_L_03.bmp", 71, 90);
	g_cSprite.LoadDibSprite(38, L"Data\\Attack2_L_04.bmp", 71, 90);

	g_cSprite.LoadDibSprite(39, L"Data\\Attack3_L_01.bmp", 71, 90);
	g_cSprite.LoadDibSprite(40, L"Data\\Attack3_L_02.bmp", 71, 90);
	g_cSprite.LoadDibSprite(41, L"Data\\Attack3_L_03.bmp", 71, 90);
	g_cSprite.LoadDibSprite(42, L"Data\\Attack3_L_04.bmp", 71, 90);
	g_cSprite.LoadDibSprite(43, L"Data\\Attack3_L_05.bmp", 71, 90);
	g_cSprite.LoadDibSprite(44, L"Data\\Attack3_L_06.bmp", 71, 90);

	g_cSprite.LoadDibSprite(45, L"Data\\Attack1_R_01.bmp", 71, 90);
	g_cSprite.LoadDibSprite(46, L"Data\\Attack1_R_02.bmp", 71, 90);
	g_cSprite.LoadDibSprite(47, L"Data\\Attack1_R_03.bmp", 71, 90);
	g_cSprite.LoadDibSprite(48, L"Data\\Attack1_R_04.bmp", 71, 90);

	g_cSprite.LoadDibSprite(49, L"Data\\Attack2_R_01.bmp", 71, 90);
	g_cSprite.LoadDibSprite(50, L"Data\\Attack2_R_02.bmp", 71, 90);
	g_cSprite.LoadDibSprite(51, L"Data\\Attack2_R_03.bmp", 71, 90);
	g_cSprite.LoadDibSprite(52, L"Data\\Attack2_R_04.bmp", 71, 90);

	g_cSprite.LoadDibSprite(53, L"Data\\Attack3_R_01.bmp", 71, 90);
	g_cSprite.LoadDibSprite(54, L"Data\\Attack3_R_02.bmp", 71, 90);
	g_cSprite.LoadDibSprite(55, L"Data\\Attack3_R_03.bmp", 71, 90);
	g_cSprite.LoadDibSprite(56, L"Data\\Attack3_R_04.bmp", 71, 90);
	g_cSprite.LoadDibSprite(57, L"Data\\Attack3_R_05.bmp", 71, 90);
	g_cSprite.LoadDibSprite(58, L"Data\\Attack3_R_06.bmp", 71, 90);

	g_cSprite.LoadDibSprite(59, L"Data\\xSpark_1.bmp", 70, 70);
	g_cSprite.LoadDibSprite(60, L"Data\\xSpark_2.bmp", 70, 70);
	g_cSprite.LoadDibSprite(61, L"Data\\xSpark_3.bmp", 70, 70);
	g_cSprite.LoadDibSprite(62, L"Data\\xSpark_4.bmp", 70, 70);

	g_cSprite.LoadDibSprite(63, L"Data\\Shadow.bmp", 32, 4);

	g_cSprite.LoadDibSprite(64, L"Data\\HPGauge.bmp", 0, 0);

	g_cSprite.LoadDibSprite(65, L"Data\\Map\\Tile_01.bmp", 0, 0);

	return TRUE;
}

//프레임 스킵
BOOL FrameSkip() {
	__int64 tempTime;
	WCHAR title[32];

	//스킵
	g_time = GetTickCount();
	tempTime = g_time - g_oldTime;
	g_oldTime = g_time;

	if (g_time - g_fps >= 1000) {
		wsprintfW(title, L"Punisher - %d fps", g_FramePerSecond);
		SetWindowTextW(g_hWnd, title);
		g_FramePerSecond = 0;
		g_fps = g_time;
	}

	if (tempTime > FRAME_DELEY) {
		g_skipFrame = g_skipFrame + tempTime;
		g_skipFrame = g_skipFrame - FRAME_DELEY;

		return FALSE;
	}
	else if (g_skipFrame > FRAME_DELEY) {
		g_skipFrame = g_skipFrame + tempTime;
		g_skipFrame = g_skipFrame - FRAME_DELEY;

		return FALSE;
	}
	else {
		tempTime = FRAME_DELEY - tempTime;
		Sleep(tempTime);
	}

	++g_FramePerSecond;

	return TRUE;
}

//게임 처리
void Update(void) {
	BaseObject *object;						//리스트 오브젝트
	int listSize;
	list<BaseObject *>::iterator iter;		//이터레이터
	list<BaseObject *>::iterator iter_end;	//이터레이터 끝

	//접속은 함?
	if (g_byPlayerState == df_USER_ZERO) {
		//응 안했어~
		return;
	}

	//출력 버퍼 포인터 및 정보 얻음
	BYTE *bypDest = g_cScreenDib.GetDibBuffer();
	int iDestWidth = g_cScreenDib.GetWidth();
	int iDestHeight = g_cScreenDib.GetHeight();
	int iDestPitch = g_cScreenDib.GetPitch();


	if (g_sendFalg) {
		if (g_writeBuffer.GetUseSize() != 0) {
			//네트워크
			Network();
		}
	}

	//윈도우 활성화 확인
	if (g_bActiveApp) {
		//키입력 해도됨?
		if (g_byPlayerState & df_USER_PLAY_KANOU) {
			//키 입력 처리
			KeyProcess();
		}
	}

	
	if (g_byPlayerState & df_USER_PLAY_KANOU) {
		//카메라 처리
		CameraProc();
	}
	

	//Ai();

	//리스트 정렬 후 오브젝트 액션 출력
	listSize = g_list->size();
	if (listSize >= 2) {
		insertionSort(0, listSize);
	}
	iter = g_list->begin();
	iter_end = g_list->end();
	while (iter != iter_end) {
		object = *iter;
		if (object->Action()) {
			iter = g_list->erase(iter);
		}
		else {
			++iter;
		}
		object->Draw(&g_cSprite, bypDest, iDestWidth, iDestHeight, iDestPitch, g_hWnd);
	}



	

	//------------------------------------------------------------
	// ScreenDib 를 화면으로 플립
	//------------------------------------------------------------
	g_cScreenDib.DrawBuffer(g_hWnd);
}


//키 입력 처리
void KeyProcess() {
	unsigned char key;
	unsigned char oldKey;
	oldKey = g_myPlayer->GetKey();		//이전에 입력한 값

	if (!g_myPlayer->CheckActionState()) {
		return;
	}

	if (GetAsyncKeyState(0x5A) & 0x8000) {
		key = dfACTION_ATTACK1;
	}
	else if (GetAsyncKeyState(0x58) & 0x8000) {
		key = dfACTION_ATTACK2;
	}
	else if (GetAsyncKeyState(0x43) & 0x8000) {
		key = dfACTION_ATTACK3;
	}
	else if ((GetAsyncKeyState(VK_LEFT) & 0x8000) && (GetAsyncKeyState(VK_UP) & 0x8000)) {
		key = dfACTION_MOVE_LU;
	}
	else if ((GetAsyncKeyState(VK_RIGHT) & 0x8000) && (GetAsyncKeyState(VK_UP) & 0x8000)) {
		key = dfACTION_MOVE_RU;
	}
	else if ((GetAsyncKeyState(VK_LEFT) & 0x8000) && (GetAsyncKeyState(VK_DOWN) & 0x8000)) {
		key = dfACTION_MOVE_LD;
	}
	else if ((GetAsyncKeyState(VK_RIGHT) & 0x8000) && (GetAsyncKeyState(VK_DOWN) & 0x8000)) {
		key = dfACTION_MOVE_RD;
	}
	else if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
		key = dfACTION_MOVE_LL;
	}
	else if (GetAsyncKeyState(VK_UP) & 0x8000) {
		key = dfACTION_MOVE_UU;
	}
	else if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
		key = dfACTION_MOVE_RR;
	}
	else if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
		key = dfACTION_MOVE_DD;
	}
	else {
		key = dfACTION_STAND;
	}

	//내꺼 움직임
	g_myPlayer->KeyAction(key);

	//이전 입력키값 비교함
	if (key == oldKey) {
		g_bKeyOnaji = false;
	}
	else {
		g_bKeyOnaji = true;
	}

	//이전 행동과 다른 행동을 할경우 패킷을 만든다
	if (g_bKeyOnaji) {
		switch (key)
		{
		case dfACTION_STAND:
			PacketCSMoveStop();
			break;
		case dfACTION_MOVE_LU:
		case dfACTION_MOVE_LD:
		case dfACTION_MOVE_LL:
		case dfACTION_MOVE_RU:
		case dfACTION_MOVE_RD:
		case dfACTION_MOVE_RR:
		case dfACTION_MOVE_UU:
		case dfACTION_MOVE_DD:
			PacketCSMoveStart(key);
			break;
		case dfACTION_ATTACK1:
		case dfACTION_ATTACK2:
		case dfACTION_ATTACK3:
			PacketCSMoveStop();
			PacketCSAttack(key);
			break;
		}
	}

	/*
	WCHAR buffer[8];
	wsprintfW(buffer, L"%d\n", key);
	OutputDebugString(buffer);
	
	
	if (!g_myPlayer->KeyAction(g_key)) {
		Attack(g_key, true, g_myPlayer);
	}
	*/
}

//카메라 처리
void CameraProc() {
	int iPosX;
	int iPosY;
	int iPosXX;
	int iPosYY;
	int iWidth = df_WIN_RIGHT / 2;
	int iHeight = df_WIN_BOTTOM / 2;
	
	//플레이어 위치 기준
	iPosX = g_myPlayer->m_X - iWidth;
	iPosY = g_myPlayer->m_Y - iHeight;
	iPosXX = g_myPlayer->m_X + iWidth;
	iPosYY = g_myPlayer->m_Y + iHeight;

	//카메라 고정
	if (iPosX < dfRANGE_MOVE_LEFT) {
		iPosX = dfRANGE_MOVE_LEFT;
	}

	if (iPosXX >= dfRANGE_MOVE_RIGHT) {
		iPosX = dfRANGE_MOVE_RIGHT - df_WIN_RIGHT;
	}

	if (iPosY < dfRANGE_MOVE_TOP) {
		iPosY = dfRANGE_MOVE_TOP;
	}

	if (iPosYY >= dfRANGE_MOVE_BOTTOM) {
		iPosY = dfRANGE_MOVE_BOTTOM - df_WIN_BOTTOM;
	}

	g_cSprite.SetCameraPosition(iPosX, iPosY);
	g_map->SetCameraPosition(iPosX, iPosY);
}


//네트워크 처리
void Network() {
	int iretval;
	int iSendSize;
	char *chpFront;

	while (1) {
		chpFront = g_writeBuffer.GetReadBufferPtr();
		iSendSize = g_writeBuffer.GetNotBrokenGetSize();

		//데이터 보내기
		iretval = send(g_sock, chpFront, iSendSize, 0);
		if (iretval == SOCKET_ERROR) {
			if (iretval == WSAEWOULDBLOCK) {
				g_sendFalg = false;
				return;
			}
			iretval = GetLastError();
			_LOG(df_LOG_LEVEL_ERROR, L"RecvProc(), send error[%d]", iretval);
			SendMessage(g_hWnd, UM_SOCKET, g_sock, FD_CLOSE);
			return;
		}
		else if (iretval == 0) {
			_LOG(df_LOG_LEVEL_ERROR, L"RecvProc(), send error 0");
			SendMessage(g_hWnd, UM_SOCKET, g_sock, FD_CLOSE);
			return;
		}

		if (iretval != iSendSize) {
			g_writeBuffer.MoveFront(iretval);
		}
		else {
			g_writeBuffer.MoveFront(iSendSize);
		}

		//빠져나감
		if (g_writeBuffer.GetUseSize() == 0) {
			return;
		}
	}

	return;
}


//패킷) 이동 정지
void PacketCSMoveStop() {
	CSerializationBuffer serialBuffer;
	int iTotalSize;
	//패킷
	BYTE bySize;							//패킷 사이즈
	char *chpWrite;

	bySize = sizeof(WORD) + sizeof(WORD) + sizeof(BYTE);

	iTotalSize = df_HEADER_SIZE + bySize + df_END_CODE_SIZE;

	if (g_writeBuffer.GetFreeSize() < iTotalSize) {
		//쓰기 버퍼에 자리없음
		return;
	}

	//헤더 만들기
	CreateHeader(serialBuffer, bySize, dfPACKET_CS_MOVE_STOP);
	//패킷 만들기
	CreateMoveStopPacket(serialBuffer);
	//EndCode
	serialBuffer << dfNETWORK_PACKET_END;

	//쓰기 버퍼에 넣기
	chpWrite = serialBuffer.GetReadBufferPtr();
	g_writeBuffer.Enqueue(chpWrite, serialBuffer.GetUseSize());
}


//패킷) 이동 시작
//byKey : 입력한 키
void PacketCSMoveStart(BYTE byKey) {
	CSerializationBuffer serialBuffer;
	int iTotalSize;
	//패킷
	BYTE bySize;							//패킷 사이즈
	char *chpWrite;

	bySize = sizeof(WORD) + sizeof(WORD) + sizeof(BYTE);

	iTotalSize = df_HEADER_SIZE + bySize + df_END_CODE_SIZE;

	if (g_writeBuffer.GetFreeSize() < iTotalSize) {
		//쓰기 버퍼에 자리없음
		return;
	}

	//헤더 만들기
	CreateHeader(serialBuffer, bySize, dfPACKET_CS_MOVE_START);
	//패킷 만들기
	CreateMoveStartPacket(serialBuffer, byKey);
	//EndCode
	serialBuffer << dfNETWORK_PACKET_END;

	//쓰기 버퍼에 넣기
	chpWrite = serialBuffer.GetReadBufferPtr();
	g_writeBuffer.Enqueue(chpWrite, serialBuffer.GetUseSize());
}

//패킷) 공격
//byKey : 입력한 키
void PacketCSAttack(BYTE byKey) {
	CSerializationBuffer serialBuffer;
	int iTotalSize;
	//패킷
	BYTE bySize;							//패킷 사이즈
	BYTE byHeaderType;						//헤더 타입
	char *chpWrite;

	bySize = sizeof(WORD) + sizeof(WORD) + sizeof(BYTE);

	iTotalSize = df_HEADER_SIZE + bySize + df_END_CODE_SIZE;

	if (g_writeBuffer.GetFreeSize() < iTotalSize) {
		//쓰기 버퍼에 자리없음
		return;
	}

	//공격 타입
	switch (byKey) {
	case dfACTION_ATTACK1:
		byHeaderType = dfPACKET_CS_ATTACK1;
		break;
	case dfACTION_ATTACK2:
		byHeaderType = dfPACKET_CS_ATTACK2;
		break;
	case dfACTION_ATTACK3:
		byHeaderType = dfPACKET_CS_ATTACK3;
		break;
	default:
		break;
	}

	//헤더 만들기
	CreateHeader(serialBuffer, bySize, byHeaderType);
	//패킷 만들기
	CreateAttackPacket(serialBuffer);
	//EndCode
	serialBuffer << dfNETWORK_PACKET_END;

	//쓰기 버퍼에 넣기
	chpWrite = serialBuffer.GetReadBufferPtr();
	g_writeBuffer.Enqueue(chpWrite, serialBuffer.GetUseSize());
}


//헤더 만들기
//SerialBuffer : 직렬 버퍼, bySize : 패킷 사이즈, byType : 패킷 타입
void CreateHeader(CSerializationBuffer &SerialBuffer, BYTE bySize, BYTE byType) {
	//사용안함
	BYTE temp = 0;
	//패킷 코드, 사이즈, 타입, temp
	SerialBuffer << dfNETWORK_PACKET_CODE << bySize << byType << temp;
}

//이동 정지 패킷 만들기
//SerialBuffer : 직렬 버퍼
void CreateMoveStopPacket(CSerializationBuffer &SerialBuffer) {
	//방향, 좌표X, 좌표Y
	SerialBuffer << g_myPlayer->GetDirection() << g_myPlayer->m_X << g_myPlayer->m_Y;
}

//이동 시작 패킷 만들기
//SerialBuffer : 직렬 버퍼, byDirection : 이동 방향 값
void CreateMoveStartPacket(CSerializationBuffer &SerialBuffer, BYTE byDirection) {
	//이동 방향, 좌표X, 좌표Y
	SerialBuffer << byDirection << g_myPlayer->m_X << g_myPlayer->m_Y;
}

//공격 패킷 만들기
//SerialBuffer : 직렬 버퍼
void CreateAttackPacket(CSerializationBuffer &SerialBuffer) {
	//방향, 좌표X, 좌표Y
	SerialBuffer << g_myPlayer->GetDirection() << g_myPlayer->m_X << g_myPlayer->m_Y;
}

//오브젝트 리스트 정렬
void insertionSort(int Num, int Max) {
	BaseObject *tempObject;					//비교할 값
	BaseObject *sortObject;					//정렬할 값
	list<BaseObject *>::iterator iter;		//이터레이터
	int sortNum;

	while (1) {
		iter = g_list->begin();
		if (Num != Max - 1) {
			for (int index = 0; index < Num; index++) {
				++iter;
			}
			tempObject = iter._Ptr->_Next->_Myval;

		}
		else {
			return;
		}

		for (sortNum = Num; sortNum >= 0; sortNum--, iter--) {
			sortObject = *iter;
			//이펙트는 뒤로 보냄
			if (df_OBJECT_TYPE_EFFECT_NUMBER_MIN <= sortObject->m_type &&
				sortObject->m_type <= df_OBJECT_TYPE_EFFECT_NUMBER_MAX) {
				iter._Ptr->_Next->_Myval = sortObject;
				iter._Ptr->_Myval = tempObject;
			}
			//Y축 더 높은거 뒤로 보냄
			else if (sortObject->m_Y > tempObject->m_Y) {
				iter._Ptr->_Next->_Myval = sortObject;
				iter._Ptr->_Myval = tempObject;
			}
			else {
				break;
			}
		}
		//iter._Ptr->_Next->_Myval = tempObject;

		if (Num + 1 != Max - 1) {
			++Num;
			//insertionSort(Num, Max);
		}
		else {
			break;
		}
	}
}

//로그 출력용 함수
void Log(WCHAR *szString, int iLogLevel) {
	wprintf(L"%s\n", szString);
}

///*****************************
//	Attack
//	공격 처리
//	공격 성공시 true 반환
//******************************/
//bool Attack(int AttackType, bool TargetType, Player *Attacker) {
//	CList<BaseObject *>::iterator iter;		//이터레이터
//	BaseObject *target;						//리스트 오브젝트
//	int objectType;							//오브젝트 타입
//	int attackerPosY;							//플레이어 위치 Y
//	int attackerPosX;							//플레이어 위치 X						
//	int targetPosY;							//오브젝트 위치 Y
//	int targetPosX;							//오브젝트 위치 X
//	int targetNumberMin;
//	int targetNumberMax;
//	bool attackSuccess = false;
//
//	if (TargetType) {
//		targetNumberMin = 300;
//		targetNumberMax = 399;
//	}
//	else {
//		targetNumberMin = 100;
//		targetNumberMax = 299;
//	}
//
//	//공격 히트 판정
//	switch (AttackType) {
//	case dfACTION_ATTACK1:
//	case dfACTION_ATTACK2:
//	case dfACTION_ATTACK3:
//
//		iter = g_list->begin();
//		for (; iter != g_list->end(); iter++) {
//			target = *iter;
//			objectType = target->m_type;
//			if (targetNumberMin <= objectType && objectType <= targetNumberMax) {
//				attackerPosY = Attacker->m_Y;
//				targetPosY = target->m_Y;
//				if ((attackerPosY - 20) < targetPosY && targetPosY < (attackerPosY + 20)) {
//					attackerPosX = Attacker->m_X;
//					targetPosX = target->m_X;
//					if (Attacker->m_Direction) {
//						if ((attackerPosX) < targetPosX && targetPosX < (attackerPosX + 100)) {
//							target->Hit(AttackType);
//							BaseObject *object = new EffectObject(targetPosX, targetPosY - 50, 0);
//							g_list->push_back(object);
//							attackSuccess = true;
//						}
//					}
//					else {
//						if ((attackerPosX) > targetPosX && targetPosX > (attackerPosX - 100)) {
//							target->Hit(AttackType);
//							BaseObject *object = new EffectObject(targetPosX, targetPosY - 50, 0);
//							g_list->push_back(object);
//							attackSuccess = true;
//						}
//					}
//				}
//			}
//		}
//		break;
//	}
//	return attackSuccess;
//}
//
//
///*****************************
//	Ai
//	컴퓨터 처리
//******************************/
//void Ai() {
//	CList<BaseObject *>::iterator iter;		//이터레이터
//	Player *enemy;						//리스트 오브젝트
//	int playerPosX;
//	int playerPosY;
//	int randAttack;
//	/*
//	int static moveCount = 0;
//	int static moveRandCount = 0;
//	int static moveDelay = 0;
//	int static moveRandDelay = 0;
//	int static attack = 0;
//	*/
//	iter = g_list->begin();
//	for (; iter != g_list->end(); iter++) {
//		enemy = (Player *)*iter;
//		if (300 <= enemy->m_type && enemy->m_type <= 399) {
//			//자신이 공격할 대상이 존재하는감
//			if (SelectPlayer(&playerPosX, &playerPosY)) {
//				//타켓 대상이 인식 범위내에 존재하는감
//				if (true) {
//					//고개 돌림
//					if (enemy->m_X > playerPosX) {
//						enemy->m_Direction = false;
//					}
//					else {
//						enemy->m_Direction = true;
//					}
//
//					//타켓 대상이 공격 범위안에 존재하는감
//					randAttack = rand() % 1000;
//					if (enemy->m_attack == 0) {
//						if (randAttack < 10) {
//							if (!enemy->KeyAction(dfACTION_ATTACK1)) {
//								if (Attack(dfACTION_ATTACK1, false, enemy)) {
//									enemy->m_attack = 1;
//								}
//								//break;
//								continue;
//							}
//
//						}
//					}
//					else if (enemy->m_attack == 1) {
//						if (!enemy->KeyAction(dfACTION_ATTACK2)) {
//							if (Attack(dfACTION_ATTACK2, false, enemy)) {
//								enemy->m_attack = 2;
//							}
//							else {
//								enemy->m_attack = 0;
//							}
//							//break;
//							continue;
//						}
//					}
//					else if (enemy->m_attack == 2) {
//						if (!enemy->KeyAction(dfACTION_ATTACK3)) {
//							if (Attack(dfACTION_ATTACK3, false, enemy)) {
//								enemy->m_attack = 0;
//							}
//							else {
//								enemy->m_attack = 0;
//							}
//							//break;
//							continue;
//						}
//					}
//					
//					
//					if (enemy->m_moveCount > 20 + enemy->m_moveRandCount) {
//						enemy->KeyAction(dfACTION_STAND);
//						enemy->m_moveDelay += 1;
//						if (enemy->m_moveDelay > 100 + enemy->m_moveRandDelay) {
//							enemy->m_moveRandCount = rand() % 50;
//							enemy->m_moveRandDelay = rand() % 200;
//							enemy->m_moveCount = 0;
//							enemy->m_moveDelay = 0;
//						}
//						//break;
//						continue;
//					}
//					//추적
//					if ((enemy->m_X - 100) > playerPosX && enemy->m_Y > playerPosY) {
//						enemy->KeyAction(dfACTION_MOVE_LU);
//					}
//					else if ((enemy->m_X + 100) < playerPosX && enemy->m_Y > playerPosY) {
//						enemy->KeyAction(dfACTION_MOVE_RU);
//					}
//					else if ((enemy->m_X - 100) > playerPosX && enemy->m_Y < playerPosY) {
//						enemy->KeyAction(dfACTION_MOVE_LD);
//					}
//					else if ((enemy->m_X + 100) < playerPosX && enemy->m_Y < playerPosY) {
//						enemy->KeyAction(dfACTION_MOVE_RD);
//					}
//					else if ((enemy->m_X - 100) > playerPosX) {
//						enemy->KeyAction(dfACTION_MOVE_LL);
//					}
//					else if ((enemy->m_X + 100) < playerPosX) {
//						enemy->KeyAction(dfACTION_MOVE_RR);
//					}
//					else if (enemy->m_Y > playerPosY) {
//						enemy->KeyAction(dfACTION_MOVE_UU);
//					}
//					else if (enemy->m_Y < playerPosY) {
//						enemy->KeyAction(dfACTION_MOVE_DD);
//					}
//					else {
//						enemy->KeyAction(dfACTION_STAND);
//					}
//					enemy->m_moveCount += 1;
//				}
//				else {
//					//대기
//				}
//			}
//			else {
//				//대기
//			}
//		}//if 타입이 적인지 아닌지
//	}//for
//	
//}
//
//
///*****************************
//	SelectPlayer
//	플레이어를 찾음
//******************************/
//bool SelectPlayer(int *PlayerPosX, int *PlayerPosY) {
//	CList<BaseObject *>::iterator iter;		//이터레이터
//	BaseObject *player;						//리스트 오브젝트
//
//
//	iter = g_list->begin();
//	for (; iter != g_list->end(); iter++) {
//		player = *iter;
//		if (100 <= player->m_type && player->m_type < 200) {
//			*PlayerPosX = player->m_X;
//			*PlayerPosY = player->m_Y;
//			return true;
//		}
//	}
//
//	return false;
//}