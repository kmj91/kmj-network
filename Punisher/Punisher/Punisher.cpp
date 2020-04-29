// Punisher.cpp: ���� ���α׷��� �������� �����մϴ�.
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




//��Ŷ�� �ش�
struct st_PACKET_HEADER {
	BYTE	byCode;			// ��Ŷ�ڵ� 0x89 ����.
	BYTE	bySize;			// ��Ŷ ������.
	BYTE	byType;			// ��ŶŸ��.
	BYTE	byTemp;			// ������.
};

class BaseObject;

// �� �ڵ� ��⿡ ��� �ִ� �Լ��� ������ �����Դϴ�.
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    ConnectProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
BOOL ProcessSocketMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);		//���� �޽��� ó��
void RecvProc(WPARAM wParam);												//���ú� ó��
BOOL RecvPacketProc(BYTE byType, CSerializationBuffer *pPacket);			//���ú� ��Ŷ ó��
void PacketSCCreateMyCharacter(CSerializationBuffer *pPacket);				//��Ŷ) �� ĳ���� ����
void PacketSCCreateOtherCharacter(CSerializationBuffer *pPacket);			//��Ŷ) �ٸ� ����� ĳ���� ����
void PacketSCStopCharacter(CSerializationBuffer *pPacket);					//��Ŷ) ĳ���� ����
void PacketSCMoveCharacter(CSerializationBuffer *pPacket);					//��Ŷ) ĳ���� �̵�
void PacketSCCharacterAttack_1(CSerializationBuffer *pPacket);				//��Ŷ) ĳ���� ���� 1				
void PacketSCCharacterAttack_2(CSerializationBuffer *pPacket);				//��Ŷ) ĳ���� ���� 2
void PacketSCCharacterAttack_3(CSerializationBuffer *pPacket);				//��Ŷ) ĳ���� ���� 3
void PacketSCDeleteCharcter(CSerializationBuffer *pPacket);					//��Ŷ) ĳ���� ����
void PacketSCDamage(CSerializationBuffer *pPacket);							//��Ŷ) ������
void PacketSCSync(CSerializationBuffer *pPacket);

BOOL Connect(WCHAR * szServerIP, int iServerPort);				//������ ����
BOOL GameInit(void);
BOOL FrameSkip();												//������ ��ŵ
void Update(void);												//���� ó��
void Network();													//��Ʈ��ũ ó��
void KeyProcess();												//Ű �Է� ó��
void CameraProc();												//ī�޶� ó��
void PacketCSMoveStop();										//��Ŷ) �̵� ����
void PacketCSMoveStart(BYTE byKey);								//��Ŷ) �̵� ����
void PacketCSAttack(BYTE byKey);								//��Ŷ) ����

void CreateHeader(CSerializationBuffer &SerialBuffer, BYTE bySize, BYTE byType);		//��� �����
void CreateMoveStopPacket(CSerializationBuffer &SerialBuffer);							//�̵� ���� ��Ŷ �����
void CreateMoveStartPacket(CSerializationBuffer &SerialBuffer, BYTE byDirection);		//�̵� ���� ��Ŷ �����
void CreateAttackPacket(CSerializationBuffer &SerialBuffer);							//���� ��Ŷ �����

void insertionSort(int Num, int Max);													//������Ʈ ����Ʈ ����
void Log(WCHAR *szString, int iLogLevel);												//�α� ��¿� �Լ�

/*
bool Attack(int AttackType, bool Target, Player *Attacker);
void Ai();
bool SelectPlayer(int *PlayerPosX, int *PlayerPosY);
*/

// ���� ����:
HINSTANCE hInst;                                // ���� �ν��Ͻ��Դϴ�.	
HWND g_hWnd;									// ������ �ڵ�
HWND g_hDlg;									// ���̾�α� �ڵ�
bool g_connectFalg = false;						// ���ؼ� �����ߴ��� ����
bool g_sendFalg = false;						// ���� �������� ����
CRingBuffer g_readBuffer;						// ���ú� ����
CRingBuffer g_writeBuffer;						// ���� ����

CScreenDib g_cScreenDib(df_WIN_RIGHT, df_WIN_BOTTOM, 32);		// �������� ��ũ��DIB ����
CSpriteDib g_cSprite(66, 0x00ffffff);			// �ִ� ��������Ʈ ������ Į��Ű �� �Է�.
list<BaseObject *> *g_list;						// ������Ʈ ����Ʈ
Map *g_map;										// ��

SOCKET g_sock;									// ����
Player *g_myPlayer = NULL;						// �÷��̾�
BYTE g_byPlayerState = df_USER_ZERO;			// �÷��̾� ����
__int64 g_time;
__int64 g_fps;
__int64 g_oldTime = 0;
int g_skipFrame = 0;
int g_FramePerSecond = 0;
bool g_bActiveApp;								// ������ Ȱ��ȭ ��Ȱ��ȭ ���� üũ
bool g_bKeyOnaji = true;
HIMC g_hOldIMC;									// �ѱ� �Է� ��ȯ

WORD g_dwLogLevel = 0;							//���, ���� ����� �α� ����
WCHAR g_szLogBuff[1024];						//�α� ����� �ʿ��� �ӽ� ����


//������Ʈ �Ӽ�
//c++ ��ó���� ��ó���� ���� NDEBUG;_WINDOWS;%(PreprocessorDefinitions)
//��Ŀ �ý��� �����ý��� ������

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

	//ȭ���� ũ�⸦ �� �߾�����
	int iX = (GetSystemMetrics(SM_CXSCREEN) / 2) - (df_WIN_RIGHT / 2);
	int iY = (GetSystemMetrics(SM_CYSCREEN) / 2) - (df_WIN_BOTTOM / 2);

	//������ ũ�� ����
	MoveWindow(g_hWnd, iX, iY, WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top, TRUE);

	//�ѱ� �Է� IMEâ ���ֱ�
	g_hOldIMC = ImmAssociateContext(g_hWnd, NULL);





	//HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDI_PUNISHER));

	MSG msg;

	// TODO: ���⿡ �ڵ带 �Է��մϴ�.
	//������Ʈ ����Ʈ ���� �Ҵ�
	g_list = new list<BaseObject*>;

	//���� IP, ��Ʈ��ȣ �Է� �ޱ�
	DialogBox(hInst, MAKEINTRESOURCE(IDD_CONNECT), g_hWnd, ConnectProc);

	//���� �ʱ�ȭ
	GameInit();

	//�� ����
	BaseObject * map = new Map(&g_cSprite, df_WIN_RIGHT, df_WIN_BOTTOM, 32, dfRANGE_MOVE_RIGHT, dfRANGE_MOVE_BOTTOM, 64, 400);
	g_list->push_front(map);
	g_map = (Map*)map;

	g_fps = GetTickCount();
	g_oldTime = g_fps;

	//������ ���� �޽��� ����
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
				//���� ó�� �Լ�
				Update();
			}
		}
	}

	timeEndPeriod(1);
	return 0;
}



//
//  �Լ�: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  ����:  �� â�� �޽����� ó���մϴ�.
//
//  WM_COMMAND  - ���� ���α׷� �޴��� ó���մϴ�.
//  WM_PAINT    - �� â�� �׸��ϴ�.
//  WM_DESTROY  - ���� �޽����� �Խ��ϰ� ��ȯ�մϴ�.
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
		// �޴� ������ ���� �м��մϴ�.
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
		//�ѱ� �Է� IME ����
		ImmAssociateContext(hWnd, g_hOldIMC);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// ���� ó��
INT_PTR CALLBACK ConnectProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int iSize;							//���ڿ� ũ��
	int iServerPort;					//���� ��Ʈ
	WCHAR *szServerIP;					//���� ������
	WCHAR szData[INPUT_ARRAY_SIZE];		//���ڿ� üũ��
	WCHAR *szNext;						//���� ���ڿ�

	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		wsprintfW(szData, L"%s:%d", SERVERIP, SERVERPORT);

		SetDlgItemTextW(hDlg, IDC_INSERT_EDIT, szData);
		SetFocus(GetDlgItem(hDlg, IDC_INSERT_EDIT));
		//���̾�α� �ڵ� ���������� ����
		g_hDlg = hDlg;
		return (INT_PTR)FALSE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			GetDlgItemText(hDlg, IDC_INSERT_EDIT, szData, INPUT_ARRAY_SIZE);
			iSize = wcsnlen_s(szData, INPUT_ARRAY_SIZE);
			if (iSize > INPUT_DATA_SIZE) {
				MessageBox(hDlg, L"�ڸ� �� �Է�����", L"Error", MB_OK | MB_ICONERROR);
				SetWindowText(GetDlgItem(hDlg, IDC_INSERT_EDIT), szData);
				return FALSE;
			}
			else if (iSize == 0) {
				MessageBox(hDlg, L"���� �Է����ּ���", L"Error", MB_OK | MB_ICONERROR);
				return FALSE;
			}
			
			//�Է� ��
			GetDlgItemTextW(hDlg, IDC_INSERT_EDIT, szData, INPUT_DATA_SIZE);
			szServerIP = wcstok_s(szData, L":", &szNext);
			iServerPort = _wtoi(szNext);

			if (iServerPort == 0) {
				MessageBox(hDlg, L"���� ����", L"Error", MB_OK | MB_ICONERROR);
				return FALSE;
			}

			//������ ����
			if (!Connect(szServerIP, iServerPort)) {
				MessageBox(hDlg, L"���� ����", L"Error", MB_OK | MB_ICONERROR);
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

// ���� ��ȭ ������ �޽��� ó�����Դϴ�.
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


//���� �޽��� ó��
BOOL ProcessSocketMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	int iretval;

	// ���� �߻� ���� Ȯ��
	if (WSAGETSELECTERROR(lParam)) {
		iretval = WSAGETSELECTERROR(lParam);
		SendMessage(g_hWnd, UM_SOCKET, wParam, FD_CLOSE);
		return TRUE;
	}

	// �޽��� ó��
	switch (WSAGETSELECTEVENT(lParam)) {
	case FD_CONNECT:
		//���� ����
		g_connectFalg = true;
		//���̾�α� ����
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

//���ú� ó��
void RecvProc(WPARAM wParam) {
	int iretval;
	int iReadSize;
	char *chpRear;
	BYTE byEndCode;
	st_PACKET_HEADER header;
	CSerializationBuffer serialBuffer;

	chpRear = g_readBuffer.GetWriteBufferPtr();
	iReadSize = g_readBuffer.GetNotBrokenPutSize();

	//������ �ޱ�
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

	//������ŭ �̵�
	g_readBuffer.MoveRear(iretval);

	//�ϼ��� ��Ŷ ���� ó��
	while (1) {
		//��� �б�
		iretval = g_readBuffer.Peek((char *)&header, df_HEADER_SIZE);
		if (iretval < df_HEADER_SIZE) {
			return;
		}

		//��Ŷ �ڵ� Ȯ��
		if (header.byCode != dfNETWORK_PACKET_CODE) {
			_LOG(df_LOG_LEVEL_ERROR, L"RecvProc(), packet code error");
			//��Ŷ�ڵ� ����
			SendMessage(g_hWnd, UM_SOCKET, wParam, FD_CLOSE);
			return;
		}

		//��Ŷ �ϼ��糪
		iretval = g_readBuffer.GetUseSize();
		if (iretval < header.bySize + df_HEADER_SIZE + df_END_CODE_SIZE) {
			return;
		}

		g_readBuffer.MoveFront(df_HEADER_SIZE);
		//��Ŷ
		g_readBuffer.Dequeue(serialBuffer.GetWriteBufferPtr(), header.bySize);
		serialBuffer.MoveRear(header.bySize);
		//EndCode �˻�
		g_readBuffer.Dequeue((char *)&byEndCode, df_END_CODE_SIZE);
		if (byEndCode != dfNETWORK_PACKET_END) {
			_LOG(df_LOG_LEVEL_ERROR, L"RecvProc(), packet end code error");
			//��Ŷ�ڵ� ����
			SendMessage(g_hWnd, UM_SOCKET, wParam, FD_CLOSE);
			return;
		}

		//��Ŷ ó��
		if (!RecvPacketProc(header.byType, &serialBuffer)) {
			_LOG(df_LOG_LEVEL_ERROR, L"RecvProc(), packet type error type:%d", header.byType);
			//�߸��� ��Ŷ Ÿ�� ����
			SendMessage(g_hWnd, UM_SOCKET, wParam, FD_CLOSE);
			return;
		}

		//���Ĺ��� �ʱ�ȭ
		serialBuffer.ClearBuffer();
	}//while(1)
	

	return;
}

//���ú� ��Ŷ ó��
BOOL RecvPacketProc(BYTE byType, CSerializationBuffer *pPacket) {
	//��Ŷ ó��
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


//��Ŷ) �� ĳ���� ����
//iPacketSize : ��Ŷ ������
void PacketSCCreateMyCharacter(CSerializationBuffer *pPacket) {
	BaseObject *myPlayer;
	UINT uiID;				//���� ���̵�
	WORD wPosX;				//��ġ ��ǥ x
	WORD wPosY;				//��ġ ��ǥ y
	BYTE byDirection;		//����
	BYTE byHP;				//ü��

	//��Ŷ �б�
	*pPacket >> &uiID >> &byDirection >> &wPosX >> &wPosY >> &byHP;

	//�÷��̾� ����
	myPlayer = new Player(uiID , wPosX, wPosY, byDirection, byHP, df_OBJECT_TYPE_PLAYER);
	//���������� ����������
	g_myPlayer = (Player *)myPlayer;
	g_byPlayerState = g_byPlayerState | df_USER_LOGIN | df_USER_PLAY_KANOU;
	//������Ʈ ����Ʈ�� ����
	g_list->push_back(myPlayer);
}

//��Ŷ) �ٸ� ����� ĳ���� ����
//iPacketSize : ��Ŷ ������
void PacketSCCreateOtherCharacter(CSerializationBuffer *pPacket) {
	BaseObject *player;
	UINT uiID;				//���� ���̵�
	WORD wPosX;				//��ġ ��ǥ x
	WORD wPosY;				//��ġ ��ǥ y
	BYTE byDirection;		//����
	BYTE byHP;				//ü��

	//��Ŷ �б�
	*pPacket >> &uiID >> &byDirection >> &wPosX >> &wPosY >> &byHP;
	//�ٸ� �÷��̾� ����
	player = new Player(uiID, wPosX, wPosY, byDirection, byHP, df_OBJECT_TYPE_OTHERPLAYER);
	//������Ʈ ����Ʈ�� ����
	g_list->push_back(player);
}

//��Ŷ) ĳ���� ����
//iPacketSize : ��Ŷ ������
void PacketSCStopCharacter(CSerializationBuffer *pPacket) {
	Player *player;							//����Ʈ ������Ʈ
	UINT uiID;								//���� ���̵�
	WORD wPosX;								//��ġ ��ǥ x
	WORD wPosY;								//��ġ ��ǥ y
	BYTE byDirection;						//����
	list<BaseObject *>::iterator iter;		//���ͷ�����
	list<BaseObject *>::iterator iter_end;	//���ͷ����� ��

	//��Ŷ �б�
	*pPacket >> &uiID >> &byDirection >> &wPosX >> &wPosY;

	iter = g_list->begin();
	iter_end = g_list->end();
	while (iter != iter_end) {
		player = (Player *)*iter;
		// 2018.05.28
		// ������Ʈ Ÿ�� �˻�
		if (player->m_type == df_OBJECT_TYPE_OTHERPLAYER || player->m_type == df_OBJECT_TYPE_PLAYER) {
			if (player->GetID() == uiID) {
				//��ġ ��ǥ ����ȭ
				player->SetPosition(wPosX, wPosY);
				//����
				player->PacketAction(dfACTION_STAND, byDirection);
				return;
			}
		}
		++iter;
	}
	//���� �����
	//���� ó��
	return;
}

//��Ŷ) ĳ���� �̵�
//iPacketSize : ��Ŷ ������
void PacketSCMoveCharacter(CSerializationBuffer *pPacket) {
	Player *player;							//����Ʈ ������Ʈ
	UINT uiID;								//���� ���̵�
	WORD wPosX;								//��ġ ��ǥ x
	WORD wPosY;								//��ġ ��ǥ y
	BYTE byDirection;						//����
	list<BaseObject *>::iterator iter;		//���ͷ�����
	list<BaseObject *>::iterator iter_end;	//���ͷ����� ��

	//��Ŷ �б�
	*pPacket >> &uiID >> &byDirection >> &wPosX >> &wPosY;

	iter = g_list->begin();
	iter_end = g_list->end();
	while (iter != iter_end) {
		player = (Player *)*iter;
		// 2018.05.28
		// ������Ʈ Ÿ�� �˻�
		if (player->m_type == df_OBJECT_TYPE_OTHERPLAYER || player->m_type == df_OBJECT_TYPE_PLAYER) {
			if (player->GetID() == uiID) {
				//��ġ ��ǥ ����ȭ
				player->SetPosition(wPosX, wPosY);
				//�̵�
				player->PacketAction(byDirection);
				return;
			}
		}
		++iter;
	}
	//���� �����
	//���� ó��
	return;
}

//��Ŷ) ĳ���� ���� 1
//iPacketSize : ��Ŷ ������
void PacketSCCharacterAttack_1(CSerializationBuffer *pPacket) {
	Player *player;							//����Ʈ ������Ʈ
	UINT uiID;								//���� ���̵�
	WORD wPosX;								//��ġ ��ǥ x
	WORD wPosY;								//��ġ ��ǥ y
	BYTE byDirection;						//����
	list<BaseObject *>::iterator iter;		//���ͷ�����
	list<BaseObject *>::iterator iter_end;	//���ͷ����� ��

	//��Ŷ �б�
	*pPacket >> &uiID >> &byDirection >> &wPosX >> &wPosY;

	iter = g_list->begin();
	iter_end = g_list->end();
	while (iter != iter_end) {
		player = (Player *)*iter;
		// 2018.05.28
		// ������Ʈ Ÿ�� �˻�
		if (player->m_type == df_OBJECT_TYPE_OTHERPLAYER || player->m_type == df_OBJECT_TYPE_PLAYER) {
			if (player->GetID() == uiID) {
				//��ġ ��ǥ ����ȭ
				player->SetPosition(wPosX, wPosY);
				//����
				player->PacketAction(dfACTION_ATTACK1, byDirection);
				return;
			}
		}
		++iter;
	}
	//���� �����
	//���� ó��
	return;
}


//��Ŷ) ĳ���� ���� 2
//iPacketSize : ��Ŷ ������
void PacketSCCharacterAttack_2(CSerializationBuffer *pPacket) {
	Player *player;							//����Ʈ ������Ʈ
	UINT uiID;								//���� ���̵�
	WORD wPosX;								//��ġ ��ǥ x
	WORD wPosY;								//��ġ ��ǥ y
	BYTE byDirection;						//����
	list<BaseObject *>::iterator iter;		//���ͷ�����
	list<BaseObject *>::iterator iter_end;	//���ͷ����� ��

	//��Ŷ �б�
	*pPacket >> &uiID >> &byDirection >> &wPosX >> &wPosY;

	iter = g_list->begin();
	iter_end = g_list->end();
	while (iter != iter_end) {
		player = (Player *)*iter;
		// 2018.05.28
		// ������Ʈ Ÿ�� �˻�
		if (player->m_type == df_OBJECT_TYPE_OTHERPLAYER || player->m_type == df_OBJECT_TYPE_PLAYER) {
			if (player->GetID() == uiID) {
				//��ġ ��ǥ ����ȭ
				player->SetPosition(wPosX, wPosY);
				//����
				player->PacketAction(dfACTION_ATTACK2, byDirection);
				return;
			}
		}
		++iter;
	}
	//���� �����
	//���� ó��
	return;
}


//��Ŷ) ĳ���� ���� 3
//iPacketSize : ��Ŷ ������
void PacketSCCharacterAttack_3(CSerializationBuffer *pPacket) {
	Player *player;							//����Ʈ ������Ʈ
	UINT uiID;								//���� ���̵�
	WORD wPosX;								//��ġ ��ǥ x
	WORD wPosY;								//��ġ ��ǥ y
	BYTE byDirection;						//����
	list<BaseObject *>::iterator iter;		//���ͷ�����
	list<BaseObject *>::iterator iter_end;	//���ͷ����� ��

	//��Ŷ �б�
	*pPacket >> &uiID >> &byDirection >> &wPosX >> &wPosY;

	iter = g_list->begin();
	iter_end = g_list->end();
	while (iter != iter_end) {
		player = (Player *)*iter;
		// 2018.05.28
		// ������Ʈ Ÿ�� �˻�
		if (player->m_type == df_OBJECT_TYPE_OTHERPLAYER || player->m_type == df_OBJECT_TYPE_PLAYER) {
			if (player->GetID() == uiID) {
				//��ġ ��ǥ ����ȭ
				player->SetPosition(wPosX, wPosY);
				//����
				player->PacketAction(dfACTION_ATTACK3, byDirection);
				return;
			}
		}
		++iter;
	}
	//���� �����
	//���� ó��
	return;
}

//��Ŷ) ĳ���� ����
//iPacketSize : ��Ŷ ������
void PacketSCDeleteCharcter(CSerializationBuffer *pPacket) {
	Player *player;							//����Ʈ ������Ʈ
	UINT uiID;								//���� ���̵�
	list<BaseObject *>::iterator iter;		//���ͷ�����
	list<BaseObject *>::iterator iter_end;	//���ͷ����� ��

	//��Ŷ �б�
	*pPacket >> &uiID;

	iter = g_list->begin();
	iter_end = g_list->end();
	while (iter != iter_end) {
		player = (Player *)*iter;
		// 2018.05.28
		// ������Ʈ ����Ʈ���� �̻��Ѱ� ������ �߰�
		if (player->m_type == df_OBJECT_TYPE_OTHERPLAYER || player->m_type == df_OBJECT_TYPE_PLAYER) {
			if (player->GetID() == uiID) {
				//����
				g_list->erase(iter);
				delete player;
				return;
			}
		}
		++iter;
	}
	//���� �����
	//���� ó��
	return;
}


//��Ŷ) ������
//iPacketSize : ��Ŷ ������
void PacketSCDamage(CSerializationBuffer *pPacket) {
	CSerializationBuffer serialBuffer;
	Player *temp;
	Player *attacker = NULL;				//������
	Player *target = NULL;					//�´³�
	BaseObject *effect;						//����Ʈ
	UINT uiAttackID;						//������ ID
	UINT uiDamageID;						//������ ID
	int iDelay = 0;							//������
	int iDamage;							//����
	BYTE DamageHP;							//������ HP
	bool drawRed = false;					//�̰� ����Ʈ ������?
	list<BaseObject *>::iterator iter;		//���ͷ�����
	list<BaseObject *>::iterator iter_end;	//���ͷ����� ��

	//��Ŷ �б�
	*pPacket >> &uiAttackID >> &uiDamageID >> &DamageHP;

	iter = g_list->begin();
	iter_end = g_list->end();
	while (iter != iter_end) {
		temp = (Player *)*iter;
		// 2018.05.28
		// ������Ʈ Ÿ�� �˻�
		if (temp->m_type == df_OBJECT_TYPE_OTHERPLAYER || temp->m_type == df_OBJECT_TYPE_PLAYER) {
			//������
			if (temp->GetID() == uiAttackID) {
				attacker = temp;
			}
			//�´³�
			if (temp->GetID() == uiDamageID) {
				target = temp;
			}
		}
		++iter;
	}
	
	//������ �����ڰ� ���°�?
	if (attacker == NULL || target == NULL) {
		//���� �����
		//���� ó��
		_LOG(df_LOG_LEVEL_ERROR, L"PacketSCDamage(), Character Not Found!");
		return;
	}

	//���� �������ΰ�?
	if (attacker->GetID() == g_myPlayer->GetID()) {
		//����Ʈ ������������
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

	//���� ���
	iDamage = target->GetHp() - DamageHP;

	//����Ʈ ������Ʈ ����
	effect = new EffectObject(target, iDamage, iDelay, drawRed, 0);
	g_list->push_back(effect);

	_LOG(df_LOG_LEVEL_DEBUG2, L"PacketSCDamage(), attacker:%d, target:%d", attacker->GetID(), target->GetID());

	return;
}

void PacketSCSync(CSerializationBuffer *pPacket) {
	//---------------------------------------------------------------
	// ����ȭ�� ���� ��Ŷ					Server -> Client
	//
	// �����κ��� ����ȭ ��Ŷ�� ������ �ش� ĳ���͸� ã�Ƽ�
	// ĳ���� ��ǥ�� �������ش�.
	//
	//	4	-	ID
	//	2	-	X
	//	2	-	Y
	//
	//---------------------------------------------------------------

	CSerializationBuffer serialBuffer;
	Player *player;							//����Ʈ ������Ʈ
	UINT uiID;								//���� ���̵�
	WORD wPosX;
	WORD wPosY;
	list<BaseObject *>::iterator iter;		//���ͷ�����
	list<BaseObject *>::iterator iter_end;	//���ͷ����� ��

	//��Ŷ �б�
	*pPacket >> &uiID >> &wPosX >> &wPosY;

	iter = g_list->begin();
	iter_end = g_list->end();
	while (iter != iter_end) {
		player = (Player *)*iter;
		// 2018.05.28
		// ������Ʈ Ÿ�� �˻�
		if (player->m_type == df_OBJECT_TYPE_OTHERPLAYER || player->m_type == df_OBJECT_TYPE_PLAYER) {
			if (player->GetID() == uiID) {
				//��ġ ��ǥ ����ȭ
				player->SetPosition(wPosX, wPosY);
				return;
			}
		}
		++iter;
	}
	//���� �����
	//���� ó��
	return;
}

//������ ����
BOOL Connect(WCHAR * szServerIP, int iServerPort) {
	int iretval;

	//���� �ʱ�ȭ
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
	���� �ʱ�ȭ
******************************/
BOOL GameInit(void)
{
	//�α� ����
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

//������ ��ŵ
BOOL FrameSkip() {
	__int64 tempTime;
	WCHAR title[32];

	//��ŵ
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

//���� ó��
void Update(void) {
	BaseObject *object;						//����Ʈ ������Ʈ
	int listSize;
	list<BaseObject *>::iterator iter;		//���ͷ�����
	list<BaseObject *>::iterator iter_end;	//���ͷ����� ��

	//������ ��?
	if (g_byPlayerState == df_USER_ZERO) {
		//�� ���߾�~
		return;
	}

	//��� ���� ������ �� ���� ����
	BYTE *bypDest = g_cScreenDib.GetDibBuffer();
	int iDestWidth = g_cScreenDib.GetWidth();
	int iDestHeight = g_cScreenDib.GetHeight();
	int iDestPitch = g_cScreenDib.GetPitch();


	if (g_sendFalg) {
		if (g_writeBuffer.GetUseSize() != 0) {
			//��Ʈ��ũ
			Network();
		}
	}

	//������ Ȱ��ȭ Ȯ��
	if (g_bActiveApp) {
		//Ű�Է� �ص���?
		if (g_byPlayerState & df_USER_PLAY_KANOU) {
			//Ű �Է� ó��
			KeyProcess();
		}
	}

	
	if (g_byPlayerState & df_USER_PLAY_KANOU) {
		//ī�޶� ó��
		CameraProc();
	}
	

	//Ai();

	//����Ʈ ���� �� ������Ʈ �׼� ���
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
	// ScreenDib �� ȭ������ �ø�
	//------------------------------------------------------------
	g_cScreenDib.DrawBuffer(g_hWnd);
}


//Ű �Է� ó��
void KeyProcess() {
	unsigned char key;
	unsigned char oldKey;
	oldKey = g_myPlayer->GetKey();		//������ �Է��� ��

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

	//���� ������
	g_myPlayer->KeyAction(key);

	//���� �Է�Ű�� ����
	if (key == oldKey) {
		g_bKeyOnaji = false;
	}
	else {
		g_bKeyOnaji = true;
	}

	//���� �ൿ�� �ٸ� �ൿ�� �Ұ�� ��Ŷ�� �����
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

//ī�޶� ó��
void CameraProc() {
	int iPosX;
	int iPosY;
	int iPosXX;
	int iPosYY;
	int iWidth = df_WIN_RIGHT / 2;
	int iHeight = df_WIN_BOTTOM / 2;
	
	//�÷��̾� ��ġ ����
	iPosX = g_myPlayer->m_X - iWidth;
	iPosY = g_myPlayer->m_Y - iHeight;
	iPosXX = g_myPlayer->m_X + iWidth;
	iPosYY = g_myPlayer->m_Y + iHeight;

	//ī�޶� ����
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


//��Ʈ��ũ ó��
void Network() {
	int iretval;
	int iSendSize;
	char *chpFront;

	while (1) {
		chpFront = g_writeBuffer.GetReadBufferPtr();
		iSendSize = g_writeBuffer.GetNotBrokenGetSize();

		//������ ������
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

		//��������
		if (g_writeBuffer.GetUseSize() == 0) {
			return;
		}
	}

	return;
}


//��Ŷ) �̵� ����
void PacketCSMoveStop() {
	CSerializationBuffer serialBuffer;
	int iTotalSize;
	//��Ŷ
	BYTE bySize;							//��Ŷ ������
	char *chpWrite;

	bySize = sizeof(WORD) + sizeof(WORD) + sizeof(BYTE);

	iTotalSize = df_HEADER_SIZE + bySize + df_END_CODE_SIZE;

	if (g_writeBuffer.GetFreeSize() < iTotalSize) {
		//���� ���ۿ� �ڸ�����
		return;
	}

	//��� �����
	CreateHeader(serialBuffer, bySize, dfPACKET_CS_MOVE_STOP);
	//��Ŷ �����
	CreateMoveStopPacket(serialBuffer);
	//EndCode
	serialBuffer << dfNETWORK_PACKET_END;

	//���� ���ۿ� �ֱ�
	chpWrite = serialBuffer.GetReadBufferPtr();
	g_writeBuffer.Enqueue(chpWrite, serialBuffer.GetUseSize());
}


//��Ŷ) �̵� ����
//byKey : �Է��� Ű
void PacketCSMoveStart(BYTE byKey) {
	CSerializationBuffer serialBuffer;
	int iTotalSize;
	//��Ŷ
	BYTE bySize;							//��Ŷ ������
	char *chpWrite;

	bySize = sizeof(WORD) + sizeof(WORD) + sizeof(BYTE);

	iTotalSize = df_HEADER_SIZE + bySize + df_END_CODE_SIZE;

	if (g_writeBuffer.GetFreeSize() < iTotalSize) {
		//���� ���ۿ� �ڸ�����
		return;
	}

	//��� �����
	CreateHeader(serialBuffer, bySize, dfPACKET_CS_MOVE_START);
	//��Ŷ �����
	CreateMoveStartPacket(serialBuffer, byKey);
	//EndCode
	serialBuffer << dfNETWORK_PACKET_END;

	//���� ���ۿ� �ֱ�
	chpWrite = serialBuffer.GetReadBufferPtr();
	g_writeBuffer.Enqueue(chpWrite, serialBuffer.GetUseSize());
}

//��Ŷ) ����
//byKey : �Է��� Ű
void PacketCSAttack(BYTE byKey) {
	CSerializationBuffer serialBuffer;
	int iTotalSize;
	//��Ŷ
	BYTE bySize;							//��Ŷ ������
	BYTE byHeaderType;						//��� Ÿ��
	char *chpWrite;

	bySize = sizeof(WORD) + sizeof(WORD) + sizeof(BYTE);

	iTotalSize = df_HEADER_SIZE + bySize + df_END_CODE_SIZE;

	if (g_writeBuffer.GetFreeSize() < iTotalSize) {
		//���� ���ۿ� �ڸ�����
		return;
	}

	//���� Ÿ��
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

	//��� �����
	CreateHeader(serialBuffer, bySize, byHeaderType);
	//��Ŷ �����
	CreateAttackPacket(serialBuffer);
	//EndCode
	serialBuffer << dfNETWORK_PACKET_END;

	//���� ���ۿ� �ֱ�
	chpWrite = serialBuffer.GetReadBufferPtr();
	g_writeBuffer.Enqueue(chpWrite, serialBuffer.GetUseSize());
}


//��� �����
//SerialBuffer : ���� ����, bySize : ��Ŷ ������, byType : ��Ŷ Ÿ��
void CreateHeader(CSerializationBuffer &SerialBuffer, BYTE bySize, BYTE byType) {
	//������
	BYTE temp = 0;
	//��Ŷ �ڵ�, ������, Ÿ��, temp
	SerialBuffer << dfNETWORK_PACKET_CODE << bySize << byType << temp;
}

//�̵� ���� ��Ŷ �����
//SerialBuffer : ���� ����
void CreateMoveStopPacket(CSerializationBuffer &SerialBuffer) {
	//����, ��ǥX, ��ǥY
	SerialBuffer << g_myPlayer->GetDirection() << g_myPlayer->m_X << g_myPlayer->m_Y;
}

//�̵� ���� ��Ŷ �����
//SerialBuffer : ���� ����, byDirection : �̵� ���� ��
void CreateMoveStartPacket(CSerializationBuffer &SerialBuffer, BYTE byDirection) {
	//�̵� ����, ��ǥX, ��ǥY
	SerialBuffer << byDirection << g_myPlayer->m_X << g_myPlayer->m_Y;
}

//���� ��Ŷ �����
//SerialBuffer : ���� ����
void CreateAttackPacket(CSerializationBuffer &SerialBuffer) {
	//����, ��ǥX, ��ǥY
	SerialBuffer << g_myPlayer->GetDirection() << g_myPlayer->m_X << g_myPlayer->m_Y;
}

//������Ʈ ����Ʈ ����
void insertionSort(int Num, int Max) {
	BaseObject *tempObject;					//���� ��
	BaseObject *sortObject;					//������ ��
	list<BaseObject *>::iterator iter;		//���ͷ�����
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
			//����Ʈ�� �ڷ� ����
			if (df_OBJECT_TYPE_EFFECT_NUMBER_MIN <= sortObject->m_type &&
				sortObject->m_type <= df_OBJECT_TYPE_EFFECT_NUMBER_MAX) {
				iter._Ptr->_Next->_Myval = sortObject;
				iter._Ptr->_Myval = tempObject;
			}
			//Y�� �� ������ �ڷ� ����
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

//�α� ��¿� �Լ�
void Log(WCHAR *szString, int iLogLevel) {
	wprintf(L"%s\n", szString);
}

///*****************************
//	Attack
//	���� ó��
//	���� ������ true ��ȯ
//******************************/
//bool Attack(int AttackType, bool TargetType, Player *Attacker) {
//	CList<BaseObject *>::iterator iter;		//���ͷ�����
//	BaseObject *target;						//����Ʈ ������Ʈ
//	int objectType;							//������Ʈ Ÿ��
//	int attackerPosY;							//�÷��̾� ��ġ Y
//	int attackerPosX;							//�÷��̾� ��ġ X						
//	int targetPosY;							//������Ʈ ��ġ Y
//	int targetPosX;							//������Ʈ ��ġ X
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
//	//���� ��Ʈ ����
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
//	��ǻ�� ó��
//******************************/
//void Ai() {
//	CList<BaseObject *>::iterator iter;		//���ͷ�����
//	Player *enemy;						//����Ʈ ������Ʈ
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
//			//�ڽ��� ������ ����� �����ϴ°�
//			if (SelectPlayer(&playerPosX, &playerPosY)) {
//				//Ÿ�� ����� �ν� �������� �����ϴ°�
//				if (true) {
//					//�� ����
//					if (enemy->m_X > playerPosX) {
//						enemy->m_Direction = false;
//					}
//					else {
//						enemy->m_Direction = true;
//					}
//
//					//Ÿ�� ����� ���� �����ȿ� �����ϴ°�
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
//					//����
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
//					//���
//				}
//			}
//			else {
//				//���
//			}
//		}//if Ÿ���� ������ �ƴ���
//	}//for
//	
//}
//
//
///*****************************
//	SelectPlayer
//	�÷��̾ ã��
//******************************/
//bool SelectPlayer(int *PlayerPosX, int *PlayerPosY) {
//	CList<BaseObject *>::iterator iter;		//���ͷ�����
//	BaseObject *player;						//����Ʈ ������Ʈ
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