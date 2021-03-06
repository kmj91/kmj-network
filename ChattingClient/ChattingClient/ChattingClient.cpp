// ChattingClient.cpp: 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include "ChattingClient.h"
#include <winsock2.h>
#include <WS2tcpip.h>				//InetPton

#include "Protocol.h"
#include "CRingBuffer.h"
#include "CSerializationBuffer.h"

#pragma comment(lib, "ws2_32")		//InetPton

#define df_SERVERIP					L"127.0.0.1"
#define df_NICKNAME					L"손님"
#define UM_SOCKET					(WM_USER+1)
#define df_ROOMNAME_MAX_LEN			15
#define df_ROOM_USERLISTMAX_SIZE	5				// 대화방 수용 인원 수


struct st_RoomUser {
	st_RoomUser() {
		flag = false;
	}

	bool flag;
	UINT uiUserNo;						// 유저 번호
	WCHAR wcNickName[dfNICK_MAX_LEN];	// 닉네임
};


// 전역 변수:

WCHAR g_szLogBuff[1024];						// 로그 저장시 필요한 임시 버퍼
HWND g_hWndConnect;								// 다이얼로그 핸들
HWND g_hWndLobby;								// 다이얼로그 핸들
HWND g_hWndChat;								// 다이얼로그 핸들
HWND g_hWnd;									// 다이얼로그 핸들
SOCKET g_sock;									// 소켓
bool g_connectFlag = false;						// 컨넥션 성공했는지 여부
bool g_sendFlag = false;						// 샌드 가능한지 여부
bool g_roomFlag = false;						// 대화방 입장 여부
CRingBuffer g_readBuffer;						// 리시브 버퍼
CRingBuffer g_writeBuffer;						// 샌드 버퍼
WCHAR g_wcNickname[dfNICK_MAX_LEN];				// 닉네임
UINT g_uiUserNum;								// 유저 번호
st_RoomUser *g_stRoomUser;						// 대화방 유저 정보

// 이 코드 모듈에 들어 있는 함수의 정방향 선언입니다.
INT_PTR CALLBACK    ConnectProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    LobbyProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    ChatProc(HWND, UINT, WPARAM, LPARAM);

bool Connect(WCHAR * wcServerIP);														//서버에 접속
bool ProcessSocketMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);		//유저 메시지 처리
void Request_Login();																	//로그인 요청
void Request_RoomList();																//대화방 리스트 요청
void Request_RoomCreate(WCHAR * wcRoomName, WORD wSize);								//대화방 생성 요청
void Request_RoomEnter(UINT uiRoomNo);													//대화방 입장 요청
void Request_Chat(WCHAR * wcChat, WORD wSize);											//채팅 송신 요청
void Request_Leave();																	//대화방 퇴장 요청
bool CheckSum(st_PACKET_HEADER header);													//체크섬 검사
BYTE GetCheckSum(char * chpBuffer, WORD wMsgType, WORD wPayloadSize);					//체크섬 값 취득
void RecvProc();																		//리시브 처리
void SendProc();																		//샌드 처리
bool RecvPacketProc(BYTE byType, CSerializationBuffer *pPacket);						//리시브 패킷 처리
void PacketSCLogin(CSerializationBuffer *pPacket);										//패킷) 로그인
void PacketSCRoomList(CSerializationBuffer *pPacket);									//패킷) 대화방 리스트
void PacketSCRoomCreate(CSerializationBuffer *pPacket);									//패킷) 대화방 생성
void PacketSCRoomEnter(CSerializationBuffer *pPacket);									//패킷) 대화방 입장
void PacketSCChat(CSerializationBuffer *pPacket);										//패킷) 채팅 송신
void PacketSCLeave(CSerializationBuffer *pPacket);										//패킷) 대화방 퇴장
void PacketSCRoomDelete(CSerializationBuffer *pPacket);									//패킷) 대화방 삭제
void PacketSCUserEnter(CSerializationBuffer *pPacket);									//패킷) 다른 유저 대화방 입장

void CreateHeader(CSerializationBuffer &SerialBuffer, BYTE byCheckSum, WORD wMsgType, WORD	wPayloadSize);	//헤더 만들기
void CreateReqLoginPacket(CSerializationBuffer &SerialBuffer);							//로그인 요청 패킷 만들기
void CreateReqRoomListPacket(CSerializationBuffer &SerialBuffer);						//대화방 리스트 요청 패킷 만들기
void CreateReqRoomCreatePacket(CSerializationBuffer &SerialBuffer, WCHAR * wcRoomName, WORD wSize);	//대화방 생성 요청 패킷 만들기
void CreateReqRoomEnterPacket(CSerializationBuffer &SerialBuffer, UINT uiRoomNo);		//대화방 입장 요청 패킷 만들기
void CreateReqChatPacket(CSerializationBuffer &SerialBuffer, WCHAR * wcChat, WORD wSize);			//채팅 송신 요청 패킷 만들기
void CreateReqLeavePacket(CSerializationBuffer &SerialBuffer);							//대화방 퇴장 요청 패킷 만들기


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    // TODO: 여기에 코드를 입력합니다.

    MSG msg;
	BOOL bRet;

	// 서버 IP, 포트번호 입력 받기
	g_hWndConnect = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_CONNECT), NULL, ConnectProc);
	ShowWindow(g_hWndConnect, nCmdShow);
	g_hWnd = g_hWndConnect;

	// 로비
	g_hWndLobby = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_LOBBY), NULL, LobbyProc);

	// 대화방
	g_hWndChat = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_CHAT), NULL, ChatProc);

    // 기본 메시지 루프입니다.
	while (bRet = GetMessage(&msg, NULL, 0, 0) != 0) {
		if (bRet == -1) {
			break;
		}
		
		if (!IsWindow(g_hWnd) || !IsDialogMessage(g_hWnd, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (g_sendFlag) {
			if (g_writeBuffer.GetUseSize() != 0) {
				// 샌드 처리
				SendProc();
			}
		}
	}


	//while (bRet = GetMessage(&msg, NULL, 0, 0) != 0) {
	//	if (bRet == -1) {
	//		break;
	//	}
	//	bRet = FALSE;

	//	if (IsDialogMessage(g_hWndLobby, &msg)) {
	//		bRet |= IsDialogMessage(g_hWndLobby, &msg);
	//	}

	//	if (IsDialogMessage(g_hWndConnect, &msg)) {
	//		bRet |= IsDialogMessage(g_hWndConnect, &msg);
	//	}
	//	
	//	if(!bRet) {
	//		TranslateMessage(&msg);
	//		DispatchMessage(&msg);
	//	}

	//	if (g_sendFalg) {
	//		if (g_writeBuffer.GetUseSize() != 0) {
	//			// 샌드 처리
	//			SendProc();
	//		}
	//	}
	//}

    return (int) msg.wParam;
}


// 접속 처리
INT_PTR CALLBACK ConnectProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int iLen;							// 문자열 크기		
	WCHAR wcServerIP[32];				// 서버 아이피
	WCHAR wcNickname[32];				// 닉네임

	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		// 기본 서버 IP 값
		SetDlgItemTextW(hDlg, IDC_EDIT_SERVER_IP, df_SERVERIP);
		// 기본 닉네임 값
		SetDlgItemTextW(hDlg, IDC_EDIT_NICKNAME, df_NICKNAME);
		// 닉네임 입력창에 포커스
		SetFocus(GetDlgItem(hDlg, IDC_EDIT_NICKNAME));
		//// 다이얼로그 핸들 전역변수에 저장
		//g_hDlg = hDlg;
		return (INT_PTR)FALSE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			// 서버 IP 입력값
			GetDlgItemText(hDlg, IDC_EDIT_SERVER_IP, wcServerIP, 32);
			iLen = wcsnlen_s(wcServerIP, 32);
			if (iLen > 16) {
				MessageBox(hDlg, L"자리 수 입력제한", L"Error", MB_OK | MB_ICONERROR);
				SetWindowText(GetDlgItem(hDlg, IDC_EDIT_SERVER_IP), wcServerIP);
				return FALSE;
			}
			else if (iLen == 0) {
				MessageBox(hDlg, L"IP를 입력해주세요", L"Error", MB_OK | MB_ICONERROR);
				return FALSE;
			}

			// 닉네임 입력값
			GetDlgItemText(hDlg, IDC_EDIT_NICKNAME, wcNickname, 32);
			GetDlgItemText(hDlg, IDC_EDIT_NICKNAME, g_wcNickname, dfNICK_MAX_LEN);
			iLen = wcsnlen_s(wcNickname, 32);
			if (iLen > dfNICK_MAX_LEN) {
				MessageBox(hDlg, L"닉네임은 15자 이하로 입력해주세요", L"Error", MB_OK | MB_ICONERROR);
				SetWindowText(GetDlgItem(hDlg, IDC_EDIT_SERVER_IP), wcServerIP);
				return FALSE;
			}
			else if (iLen == 0) {
				MessageBox(hDlg, L"닉네임을 입력해주세요", L"Error", MB_OK | MB_ICONERROR);
				return FALSE;
			}

			// 서버에 접속
			if (!Connect(wcServerIP)) {
				MessageBox(hDlg, L"접속 실패", L"Error", MB_OK | MB_ICONERROR);
				return FALSE;
			}

			return (INT_PTR)TRUE;
		case IDCANCEL:
			PostQuitMessage(0);
			EndDialog(hDlg, TRUE);
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

// 로비 처리
INT_PTR CALLBACK LobbyProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	int iLen;							// 문자열 크기
	int iSelIndex;						// 선택한 리스트 인덱스
	int iData;							// 리스트 데이터값
	WCHAR wcRoomName[32];				// 방 이름
	HWND hListBox;						// 리스트 핸들

	switch (message) {
	case WM_INITDIALOG:
		return TRUE;
	case UM_SOCKET:
		ProcessSocketMessage(hDlg, message, wParam, lParam);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			// 대화방 이름 값
			GetDlgItemText(hDlg, IDC_EDIT_ROOM, wcRoomName, 32);
			iLen = wcsnlen_s(wcRoomName, 32);
			if (iLen > df_ROOMNAME_MAX_LEN) {
				MessageBox(hDlg, L"15자리 이하로 입력해 주세요", L"Error", MB_OK | MB_ICONERROR);
				SetWindowText(GetDlgItem(hDlg, IDC_EDIT_ROOM), wcRoomName);
				return FALSE;
			}
			else if (iLen == 0) {
				MessageBox(hDlg, L"방 이름을 입력해 주세요", L"Error", MB_OK | MB_ICONERROR);
				return FALSE;
			}

			Request_RoomCreate(wcRoomName, iLen * 2);

			return TRUE;
		case IDC_LIST_ROOM:

			if (g_roomFlag) {
				return TRUE;
			}

			iSelIndex = SendMessage((HWND)lParam, LB_GETCURSEL, 0, 0);
			
			if (iSelIndex != -1) {
				//대화방 입장
				g_roomFlag = true;

				hListBox = GetDlgItem(g_hWndLobby, IDC_LIST_ROOM);
				iData = SendMessage(hListBox, LB_GETITEMDATA, iSelIndex, 0);
				Request_RoomEnter(iData);
			}

			return TRUE;
		}
		break;
	case WM_CLOSE:
		PostQuitMessage(0);
		EndDialog(hDlg, TRUE);
		return TRUE;
	}

	return FALSE;
}

// 대화방 처리
INT_PTR CALLBACK ChatProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	int iLen;							// 문자열 크기
	WCHAR wcChat[32];					// 대화 내용
	WCHAR wcChat2[48];					// 나 : 대화 내용
	HWND hListBox;						// 리스트 핸들
	int iSize;							// 리스트 목록 수

	switch (message) {
	case WM_INITDIALOG:
		return TRUE;
	case UM_SOCKET:
		ProcessSocketMessage(hDlg, message, wParam, lParam);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_BUTTON_OK:
			// 대화방 이름 값
			GetDlgItemText(hDlg, IDC_EDIT_CHAT, wcChat, 32);
			iLen = wcsnlen_s(wcChat, 32);
			if (iLen == 0) {
				return FALSE;
			}

			// 나 : 채팅
			wsprintf(wcChat2, L"나 : %s", wcChat);
			// 리스트 박스 핸들 얻기
			hListBox = GetDlgItem(g_hWndChat, IDC_LIST_CHAT);
			// 리스트 박스에 문자열 추가
			SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)wcChat2);
			// 채팅 텍스트 에디터 비우기
			SetWindowText(GetDlgItem(g_hWndChat, IDC_EDIT_CHAT), L"");


			Request_Chat(wcChat, iLen * 2);

			return TRUE;
		}
		break;
	case WM_CLOSE:
		// 대화방 나가기
		Request_Leave();
		g_hWnd = g_hWndLobby;
		g_roomFlag = false;

		//대화방 유저 리스트 핸들 얻기
		hListBox = GetDlgItem(g_hWndChat, IDC_LIST_USER);
		//대화방 유저 리스트 모든 항목 삭제
		SendMessage(hListBox, LB_RESETCONTENT, 0, 0);

		//대화방 채팅 핸들 얻기
		hListBox = GetDlgItem(g_hWndChat, IDC_LIST_CHAT);
		//대화방 채팅 모든 항목 삭제
		SendMessage(hListBox, LB_RESETCONTENT, 0, 0);

		EndDialog(hDlg, TRUE);
		return TRUE;
	}

	return FALSE;
}

//서버에 접속
bool Connect(WCHAR * wcServerIP) {
	int iretval;

	//윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return false;

	// socket()
	g_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (g_sock == INVALID_SOCKET) {
		iretval = GetLastError();
		return false;
	}

	// WSAAsyncSelect()
	iretval = WSAAsyncSelect(g_sock, g_hWndLobby, UM_SOCKET,
		FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE);
	if (iretval == SOCKET_ERROR) {
		iretval = GetLastError();
		closesocket(g_sock);
		WSACleanup();
		return false;
	}

	// connect()
	SOCKADDR_IN serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	InetPton(AF_INET, wcServerIP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(dfNETWORK_PORT);
	connect(g_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));


	return true;
}

//유저 메시지 처리
bool ProcessSocketMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	int iretval;

	// 오류 발생 여부 확인
	if (WSAGETSELECTERROR(lParam)) {
		iretval = WSAGETSELECTERROR(lParam);
		SendMessage(g_hWndLobby, UM_SOCKET, g_sock, FD_CLOSE);
		if (iretval == WSAECONNREFUSED) {
			if (!g_connectFlag) {
				MessageBox(g_hWndConnect, L"접속 실패", L"Error", MB_OK | MB_ICONERROR);
				return TRUE;
			}
		}
		return TRUE;
	}

	// 메시지 처리
	switch (WSAGETSELECTEVENT(lParam)) {
	case FD_CONNECT:
		// 접속 성공
		if (g_connectFlag)
			return TRUE;

		g_connectFlag = true;
		//로그인 요청
		Request_Login();												
		return TRUE;
	case FD_READ:
		if (!g_connectFlag) {
			return TRUE;
		}
		RecvProc();
		return TRUE;
	case FD_WRITE:
		g_sendFlag = true;
		return TRUE;
	case FD_CLOSE:
		closesocket(g_sock);
		g_connectFlag = false;
		g_sendFlag = false;
		return FALSE;
	default:
		
		return FALSE;
	}

	return TRUE;
}

// 로그인 요청
void Request_Login() {
	int iTotalSize;							// 전체 버퍼 사이즈
	WORD wPayloadSize;						// 페이로드 사이즈
	char *chpPayload;						// 버퍼 포인터
	BYTE byCheckSum;						// 체크섬 값
	CSerializationBuffer serialBuffer;		// 직렬 버퍼

	// 패킷 만들기
	CreateReqLoginPacket(serialBuffer);
	// 페이로드 사이즈
	wPayloadSize = serialBuffer.GetUseSize();
	// 페이로드 시작부분
	chpPayload = serialBuffer.GetReadBufferPtr();		
	// 체크섬 값 취득
	byCheckSum = GetCheckSum(chpPayload, df_REQ_LOGIN, wPayloadSize);
	// 헤더 만들기
	CreateHeader(serialBuffer, byCheckSum, df_REQ_LOGIN, wPayloadSize);

	// 헤더 + 페이로드 사이즈
	iTotalSize = serialBuffer.GetUseSize();
	if (g_writeBuffer.GetFreeSize() < iTotalSize) {
		//쓰기 버퍼에 자리없음
		return;
	}

	// 페이로드 앞 헤더 시작 부분
	chpPayload = serialBuffer.GetReadBufferPtr();
	// 쓰기 버퍼에 넣기
	g_writeBuffer.Enqueue(chpPayload, serialBuffer.GetUseSize());
}

//대화방 리스트 요청
void Request_RoomList() {
	//------------------------------------------------------------
	// 3 Req 대화방 리스트
	//
	//	None
	//------------------------------------------------------------

	int iTotalSize;							// 전체 버퍼 사이즈
	WORD wPayloadSize;						// 페이로드 사이즈
	char *chpPayload;						// 버퍼 포인터
	BYTE byCheckSum;						// 체크섬 값
	CSerializationBuffer serialBuffer;		// 직렬 버퍼

	// 패킷 만들기
	CreateReqRoomListPacket(serialBuffer);
	// 페이로드 사이즈
	wPayloadSize = serialBuffer.GetUseSize();
	// 페이로드 시작부분
	chpPayload = serialBuffer.GetReadBufferPtr();
	// 체크섬 값 취득
	byCheckSum = GetCheckSum(chpPayload, df_REQ_ROOM_LIST, wPayloadSize);
	// 헤더 만들기
	CreateHeader(serialBuffer, byCheckSum, df_REQ_ROOM_LIST, wPayloadSize);

	// 헤더 + 페이로드 사이즈
	iTotalSize = serialBuffer.GetUseSize();
	if (g_writeBuffer.GetFreeSize() < iTotalSize) {
		//쓰기 버퍼에 자리없음
		return;
	}

	// 페이로드 앞 헤더 시작 부분
	chpPayload = serialBuffer.GetReadBufferPtr();
	// 쓰기 버퍼에 넣기
	g_writeBuffer.Enqueue(chpPayload, serialBuffer.GetUseSize());
}

//대화방 생성 요청
void Request_RoomCreate(WCHAR * wcRoomName, WORD wSize) {
	//------------------------------------------------------------
	// 5 Req 대화방 생성
	//
	// 2Byte : 방제목 Size			유니코드 문자 바이트 길이 (널 제외)
	// Size  : 방제목 (유니코드)
	//------------------------------------------------------------

	int iTotalSize;							// 전체 버퍼 사이즈
	WORD wPayloadSize;						// 페이로드 사이즈
	char *chpPayload;						// 버퍼 포인터
	BYTE byCheckSum;						// 체크섬 값
	CSerializationBuffer serialBuffer;		// 직렬 버퍼

	// 패킷 만들기
	CreateReqRoomCreatePacket(serialBuffer, wcRoomName, wSize);
	// 페이로드 사이즈
	wPayloadSize = serialBuffer.GetUseSize();
	// 페이로드 시작부분
	chpPayload = serialBuffer.GetReadBufferPtr();
	// 체크섬 값 취득
	byCheckSum = GetCheckSum(chpPayload, df_REQ_ROOM_CREATE, wPayloadSize);
	// 헤더 만들기
	CreateHeader(serialBuffer, byCheckSum, df_REQ_ROOM_CREATE, wPayloadSize);

	// 헤더 + 페이로드 사이즈
	iTotalSize = serialBuffer.GetUseSize();
	if (g_writeBuffer.GetFreeSize() < iTotalSize) {
		//쓰기 버퍼에 자리없음
		return;
	}

	// 페이로드 앞 헤더 시작 부분
	chpPayload = serialBuffer.GetReadBufferPtr();
	// 쓰기 버퍼에 넣기
	g_writeBuffer.Enqueue(chpPayload, serialBuffer.GetUseSize());
}

//대화방 입장 요청
void Request_RoomEnter(UINT uiRoomNo) {
	//------------------------------------------------------------
	// 7 Req 대화방 입장
	//
	//	4Byte : 방 No
	//------------------------------------------------------------

	int iTotalSize;							// 전체 버퍼 사이즈
	WORD wPayloadSize;						// 페이로드 사이즈
	char *chpPayload;						// 버퍼 포인터
	BYTE byCheckSum;						// 체크섬 값
	CSerializationBuffer serialBuffer;		// 직렬 버퍼

											// 패킷 만들기
	CreateReqRoomEnterPacket(serialBuffer, uiRoomNo);
	// 페이로드 사이즈
	wPayloadSize = serialBuffer.GetUseSize();
	// 페이로드 시작부분
	chpPayload = serialBuffer.GetReadBufferPtr();
	// 체크섬 값 취득
	byCheckSum = GetCheckSum(chpPayload, df_REQ_ROOM_ENTER, wPayloadSize);
	// 헤더 만들기
	CreateHeader(serialBuffer, byCheckSum, df_REQ_ROOM_ENTER, wPayloadSize);

	// 헤더 + 페이로드 사이즈
	iTotalSize = serialBuffer.GetUseSize();
	if (g_writeBuffer.GetFreeSize() < iTotalSize) {
		//쓰기 버퍼에 자리없음
		return;
	}

	// 페이로드 앞 헤더 시작 부분
	chpPayload = serialBuffer.GetReadBufferPtr();
	// 쓰기 버퍼에 넣기
	g_writeBuffer.Enqueue(chpPayload, serialBuffer.GetUseSize());
}

//채팅 송신 요청
void Request_Chat(WCHAR * wcChat, WORD wSize) {
	//------------------------------------------------------------
	// 9 Req 채팅송신
	//
	// 2Byte : 메시지 Size
	// Size  : 대화내용(유니코드)
	//------------------------------------------------------------

	int iTotalSize;							// 전체 버퍼 사이즈
	WORD wPayloadSize;						// 페이로드 사이즈
	char *chpPayload;						// 버퍼 포인터
	BYTE byCheckSum;						// 체크섬 값
	CSerializationBuffer serialBuffer;		// 직렬 버퍼

	// 패킷 만들기
	CreateReqChatPacket(serialBuffer, wcChat, wSize);
	// 페이로드 사이즈
	wPayloadSize = serialBuffer.GetUseSize();
	// 페이로드 시작부분
	chpPayload = serialBuffer.GetReadBufferPtr();
	// 체크섬 값 취득
	byCheckSum = GetCheckSum(chpPayload, df_REQ_CHAT, wPayloadSize);
	// 헤더 만들기
	CreateHeader(serialBuffer, byCheckSum, df_REQ_CHAT, wPayloadSize);

	// 헤더 + 페이로드 사이즈
	iTotalSize = serialBuffer.GetUseSize();
	if (g_writeBuffer.GetFreeSize() < iTotalSize) {
		//쓰기 버퍼에 자리없음
		return;
	}

	// 페이로드 앞 헤더 시작 부분
	chpPayload = serialBuffer.GetReadBufferPtr();
	// 쓰기 버퍼에 넣기
	g_writeBuffer.Enqueue(chpPayload, serialBuffer.GetUseSize());
}

//대화방 퇴장 요청
void Request_Leave() {
	//------------------------------------------------------------
	// 11 Req 방퇴장 
	//
	// None
	//------------------------------------------------------------

	int iTotalSize;							// 전체 버퍼 사이즈
	WORD wPayloadSize;						// 페이로드 사이즈
	char *chpPayload;						// 버퍼 포인터
	BYTE byCheckSum;						// 체크섬 값
	CSerializationBuffer serialBuffer;		// 직렬 버퍼

	// 패킷 만들기
	CreateReqLeavePacket(serialBuffer);
	// 페이로드 사이즈
	wPayloadSize = serialBuffer.GetUseSize();
	// 페이로드 시작부분
	chpPayload = serialBuffer.GetReadBufferPtr();
	// 체크섬 값 취득
	byCheckSum = GetCheckSum(chpPayload, df_REQ_ROOM_LEAVE, wPayloadSize);
	// 헤더 만들기
	CreateHeader(serialBuffer, byCheckSum, df_REQ_ROOM_LEAVE, wPayloadSize);

	// 헤더 + 페이로드 사이즈
	iTotalSize = serialBuffer.GetUseSize();
	if (g_writeBuffer.GetFreeSize() < iTotalSize) {
		//쓰기 버퍼에 자리없음
		return;
	}

	// 페이로드 앞 헤더 시작 부분
	chpPayload = serialBuffer.GetReadBufferPtr();
	// 쓰기 버퍼에 넣기
	g_writeBuffer.Enqueue(chpPayload, serialBuffer.GetUseSize());
}

// 체크섬 검사
bool CheckSum(st_PACKET_HEADER header) {
	char *chpTemp = g_readBuffer.GetReadBufferPtr();
	BYTE retval = 0;

	//readBuffer->Peek(chpTemp, wPacketSize);

	for (int cnt = 0; cnt < header.wPayloadSize; cnt++) {
		retval = retval + *chpTemp;
		chpTemp++;
	}

	chpTemp = (char *)&header.wMsgType;

	for (int cnt = 0; cnt < sizeof(header.wMsgType); cnt++) {
		retval = retval + *chpTemp;
		chpTemp++;
	}

	retval = retval % 256;

	if (header.byCheckSum != retval) {
		return true;
	}
	return false;
}

// 체크섬 값 취득
BYTE GetCheckSum(char * chpBuffer, WORD wMsgType, WORD wPayloadSize) {
	char *chpTemp;
	BYTE retval = 0;

	chpTemp = chpBuffer;

	for (int cnt = 0; cnt < wPayloadSize; cnt++) {
		retval = retval + *chpTemp;
		chpTemp++;
	}

	chpTemp = (char *)&wMsgType;

	for (int cnt = 0; cnt < sizeof(wMsgType); cnt++) {
		retval = retval + *chpTemp;
		chpTemp++;
	}

	retval = retval % 256;

	return retval;
}

// 리시브 처리
void RecvProc() {
	int iretval;
	int iReadSize;
	char *chpRear;
	st_PACKET_HEADER header;
	CSerializationBuffer serialBuffer;

	chpRear = g_readBuffer.GetWriteBufferPtr();
	iReadSize = g_readBuffer.GetNotBrokenPutSize();

	//데이터 받기
	iretval = recv(g_sock, chpRear, iReadSize, 0);
	if (iretval == SOCKET_ERROR) {
		iretval = GetLastError();
		if (iretval == WSAEWOULDBLOCK) {
			return;
		}
		//_LOG(df_LOG_LEVEL_ERROR, L"RecvProc(), recv error[%d]", iretval);
		wsprintf(g_szLogBuff, L"RecvProc(), recv error[%d]\n", iretval);
		OutputDebugString(g_szLogBuff);
		SendMessage(g_hWndLobby, UM_SOCKET, g_sock, FD_CLOSE);
		return;
	}
	else if (iretval == 0) {
		//_LOG(df_LOG_LEVEL_ERROR, L"RecvProc(), recv error 0");
		wsprintf(g_szLogBuff, L"RecvProc(), recv error 0\n");
		OutputDebugString(g_szLogBuff);
		SendMessage(g_hWndLobby, UM_SOCKET, g_sock, FD_CLOSE);
		return;
	}

	//받은만큼 이동
	g_readBuffer.MoveRear(iretval);

	//완성된 패킷 전부 처리
	while (1) {
		//해더 읽기
		iretval = g_readBuffer.Peek((char *)&header, sizeof(st_PACKET_HEADER));
		if (iretval < sizeof(st_PACKET_HEADER)) {
			// 해더 완성 안됨
			return;
		}

		//패킷 코드 확인
		if (header.byCode != dfPACKET_CODE) {
			//_LOG(df_LOG_LEVEL_ERROR, L"RecvProc(), packet code error");
			wsprintf(g_szLogBuff, L"RecvProc(), packet code error\n");
			OutputDebugString(g_szLogBuff);
			//패킷코드 에러
			SendMessage(g_hWndLobby, UM_SOCKET, g_sock, FD_CLOSE);
			return;
		}

		//패킷 완성됬나
		iretval = g_readBuffer.GetUseSize();
		if (iretval < header.wPayloadSize + sizeof(st_PACKET_HEADER)) {
			// 패킷 완성 안됨
			return;
		}
		// 해더 크기만큼 버퍼 프론트 이동
		g_readBuffer.MoveFront(sizeof(st_PACKET_HEADER));

		//체크섬 검사
		if (CheckSum(header)) {
			wsprintf(g_szLogBuff, L"RecvProc(), packet checkSum error\n");
			OutputDebugString(g_szLogBuff);
			//체크섬 에러
			SendMessage(g_hWndLobby, UM_SOCKET, g_sock, FD_CLOSE);
			return;
		}

		//패킷
		g_readBuffer.Dequeue(serialBuffer.GetWriteBufferPtr(), header.wPayloadSize);
		serialBuffer.MoveRear(header.wPayloadSize);

		//패킷 처리
		if (!RecvPacketProc(header.wMsgType, &serialBuffer)) {
			//_LOG(df_LOG_LEVEL_ERROR, L"RecvProc(), packet type error type:%d", header.byType);
			wsprintf(g_szLogBuff, L"RecvProc(), packet type error type:%d\n", header.wMsgType);
			OutputDebugString(g_szLogBuff);
			//잘못된 패킷 타입 에러
			SendMessage(g_hWndLobby, UM_SOCKET, g_sock, FD_CLOSE);
			return;
		}

		//직렬버퍼 초기화
		serialBuffer.ClearBuffer();
	}//while(1)


	return;
}

// 샌드 처리
void SendProc() {
	int iretval;
	int iSendSize;
	char *chpFront;

	while (1) {
		chpFront = g_writeBuffer.GetReadBufferPtr();
		iSendSize = g_writeBuffer.GetNotBrokenGetSize();

		//데이터 보내기
		iretval = send(g_sock, chpFront, iSendSize, 0);
		if (iretval == SOCKET_ERROR) {
			iretval = GetLastError();
			if (iretval == WSAEWOULDBLOCK) {
				g_sendFlag = false;
				return;
			}
			//_LOG(df_LOG_LEVEL_ERROR, L"RecvProc(), send error[%d]", iretval);
			wsprintf(g_szLogBuff, L"SendProc(), send error[%d]\n", iretval);
			OutputDebugString(g_szLogBuff);
			SendMessage(g_hWndLobby, UM_SOCKET, g_sock, FD_CLOSE);
			return;
		}
		else if (iretval == 0) {
			//_LOG(df_LOG_LEVEL_ERROR, L"RecvProc(), send error 0");
			wsprintf(g_szLogBuff, L"SendProc(), send error 0\n");
			OutputDebugString(g_szLogBuff);
			SendMessage(g_hWndLobby, UM_SOCKET, g_sock, FD_CLOSE);
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

// 리시브 패킷 처리
bool RecvPacketProc(BYTE byType, CSerializationBuffer *pPacket) {
	// 패킷 처리
	switch (byType) {
	case df_RES_LOGIN:
		//로그인 결과
		PacketSCLogin(pPacket);
		break;
	case df_RES_ROOM_LIST:
		//대화방 리스트
		PacketSCRoomList(pPacket);
		break;
	case df_RES_ROOM_CREATE:
		//대화방 생성
		PacketSCRoomCreate(pPacket);
		break;
	case df_RES_ROOM_ENTER:
		//대화방 입장
		PacketSCRoomEnter(pPacket);
		break;
	case df_RES_CHAT:
		//채팅 수신
		PacketSCChat(pPacket);
		break;
	case df_RES_ROOM_LEAVE:
		//대화방 퇴장
		PacketSCLeave(pPacket);
		break;
	case df_RES_ROOM_DELETE:
		//대화방 삭제
		PacketSCRoomDelete(pPacket);
		break;
	case df_RES_USER_ENTER:
		//다른 유저 대화방 입장
		PacketSCUserEnter(pPacket);
		break;
	default:
		return false;
	}
	return true;
}

// 패킷) 로그인
void PacketSCLogin(CSerializationBuffer *pPacket) {
	//------------------------------------------------------------
	// 2 Res 로그인                              
	// 
	// 1Byte	: 결과 (1:OK / 2:중복닉네임 / 3:사용자초과 / 4:기타오류)
	// 4Byte	: 사용자 NO
	//------------------------------------------------------------

	BYTE byResult;
	WCHAR wcUserNum[8];

	//패킷 읽기
	*pPacket >> &byResult;

	switch (byResult) {
	case 1:
		*pPacket >> &g_uiUserNum;
		// Connect 다이얼로그 끄기
		EndDialog(g_hWndConnect, TRUE);
		// 로비 켜기
		ShowWindow(g_hWndLobby, SW_SHOWDEFAULT);
		g_hWnd = g_hWndLobby;

		SetDlgItemTextW(g_hWndLobby, IDC_STATIC_NICKNAME, g_wcNickname);
		wsprintfW(wcUserNum, L"%d", g_uiUserNum);
		SetDlgItemTextW(g_hWndLobby, IDC_STATIC_USERNO, wcUserNum);

		// 대화방 리스트 요청
		Request_RoomList();
		break;
	case 2:
		// 닉네임 중복
		wsprintf(g_szLogBuff, L"PacketSCLogin(), packet error [%d]\n", byResult);
		OutputDebugString(g_szLogBuff);
		SendMessage(g_hWndLobby, UM_SOCKET, g_sock, FD_CLOSE);
		MessageBox(g_hWndConnect, L"닉네임 중복", L"Error", MB_OK | MB_ICONERROR);
		break;
	case 3:
		// 사용자 초과
		wsprintf(g_szLogBuff, L"PacketSCLogin(), packet error [%d]\n", byResult);
		OutputDebugString(g_szLogBuff);
		SendMessage(g_hWndLobby, UM_SOCKET, g_sock, FD_CLOSE);
		MessageBox(g_hWndConnect, L"사용자 초과", L"Error", MB_OK | MB_ICONERROR);
		break;
	case 4:
		// 기타 오류
		wsprintf(g_szLogBuff, L"PacketSCLogin(), packet error [%d]\n", byResult);
		OutputDebugString(g_szLogBuff);
		SendMessage(g_hWndLobby, UM_SOCKET, g_sock, FD_CLOSE);
		MessageBox(g_hWndConnect, L"기타 오류", L"Error", MB_OK | MB_ICONERROR);
		break;
	default:
		// 잘못된 결과 값
		wsprintf(g_szLogBuff, L"PacketSCLogin(), packet error [%d]\n", byResult);
		OutputDebugString(g_szLogBuff);
		SendMessage(g_hWndLobby, UM_SOCKET, g_sock, FD_CLOSE);
		MessageBox(g_hWndConnect, L"잘못된 결과 값", L"Error", MB_OK | MB_ICONERROR);
		break;
	}
}

//패킷) 대화방 리스트
void PacketSCRoomList(CSerializationBuffer *pPacket) {
	//------------------------------------------------------------
	// 4 Res 대화방 리스트
	//
	//  2Byte	: 개수
	//  {
	//		4Byte : 방 No
	//		2Byte : 방이름 byte size
	//		Size  : 방이름 (유니코드)
	//
	//		// 21.01.25 15:14
	//		// 로비에서 아래 패킷은 왜 필요한지 모르겠음 보류
	//		1Byte : 참여인원		
	//		{
	//			WHCAR[15] : 닉네임
	//		}
	//	
	//	}
	//------------------------------------------------------------
	
	WORD wRoomCount;
	UINT uiRoomNo;
	WORD wRoomNameSize;
	char *wcpRoomName = NULL;	// 대화방 이름
	HWND hListBox;				// 대화방 리스트 핸들
	int iIndex;					// 리스트 인덱스

	//패킷 읽기
	*pPacket >> &wRoomCount;

	for (WORD wCnt = 0; wCnt < wRoomCount; ++wCnt) {
		*pPacket >> &uiRoomNo >> &wRoomNameSize;
		wcpRoomName = new char[wRoomNameSize + 2];

		pPacket->ReadString((WCHAR *)wcpRoomName, wRoomNameSize);
		// 2021.01.22 13:43
		// 문자열 복사 후 뒤에 null 문자 추가 해야 합니다
		ZeroMemory(&wcpRoomName[wRoomNameSize], 2);
		//리스트 박스 핸들 얻기
		hListBox = GetDlgItem(g_hWndLobby, IDC_LIST_ROOM);
		//리스트 박스에 문자열 추가
		iIndex = SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)wcpRoomName);
		SendMessage(hListBox, LB_SETITEMDATA, (WPARAM)iIndex, (LPARAM)uiRoomNo);

		delete[] wcpRoomName;
	}
}

//패킷) 대화방 생성
void PacketSCRoomCreate(CSerializationBuffer *pPacket) {
	//------------------------------------------------------------
	// 6 Res 대화방 생성 (수시로)
	//
	// 1Byte : 결과 (1:OK / 2:방이름 중복 / 3:개수초과 / 4:기타오류)
	//
	//
	// 4Byte : 방 No
	// 2Byte : 방제목 바이트 Size
	// Size  : 방제목 (유니코드)
	//------------------------------------------------------------

	BYTE byResult;
	UINT uiRoomNo;
	WORD wRoomNameSize;
	char *wcpRoomName = NULL;	// 대화방 이름
	HWND hListBox;				// 대화방 리스트 핸들
	int iIndex;					// 리스트 인덱스

	//패킷 읽기
	*pPacket >> &byResult;

	switch (byResult) {
	case 1:
		*pPacket >> &uiRoomNo >> &wRoomNameSize;
		wcpRoomName = new char[wRoomNameSize + 2];
		pPacket->ReadString((WCHAR *)wcpRoomName, wRoomNameSize);
		// 2021.01.22 13:43
		// 문자열 복사 후 뒤에 null 문자 추가 해야 합니다
		ZeroMemory(&wcpRoomName[wRoomNameSize], 2);
		//리스트 박스 핸들 얻기
		hListBox = GetDlgItem(g_hWndLobby, IDC_LIST_ROOM);
		//리스트 박스에 문자열 추가
		iIndex = SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)wcpRoomName);
		SendMessage(hListBox, LB_SETITEMDATA, (WPARAM)iIndex, (LPARAM)uiRoomNo);

		delete[] wcpRoomName;
		break;
	case 2:
		// 대화방 이름 중복
		wsprintf(g_szLogBuff, L"PacketSCRoomCreate(), packet error [%d]\n", byResult);
		OutputDebugString(g_szLogBuff);
		MessageBox(g_hWndConnect, L"대화방 이름 중복", L"Error", MB_OK | MB_ICONERROR);
		break;
	case 3:
		// 대화방 초과
		wsprintf(g_szLogBuff, L"PacketSCRoomCreate(), packet error [%d]\n", byResult);
		OutputDebugString(g_szLogBuff);
		MessageBox(g_hWndConnect, L"대화방 초과", L"Error", MB_OK | MB_ICONERROR);
		break;
	case 4:
		// 기타 오류
		wsprintf(g_szLogBuff, L"PacketSCRoomCreate(), packet error [%d]\n", byResult);
		OutputDebugString(g_szLogBuff);
		SendMessage(g_hWndLobby, UM_SOCKET, g_sock, FD_CLOSE);
		MessageBox(g_hWndConnect, L"기타 오류", L"Error", MB_OK | MB_ICONERROR);
		break;
	default:
		// 잘못된 결과 값
		wsprintf(g_szLogBuff, L"PacketSCRoomCreate(), packet error [%d]\n", byResult);
		OutputDebugString(g_szLogBuff);
		SendMessage(g_hWndLobby, UM_SOCKET, g_sock, FD_CLOSE);
		MessageBox(g_hWndConnect, L"잘못된 결과 값", L"Error", MB_OK | MB_ICONERROR);
		break;
	}
}

//패킷) 대화방 입장
void PacketSCRoomEnter(CSerializationBuffer *pPacket) {
	//------------------------------------------------------------
	// 8 Res 대화방 입장
	//
	// 1Byte : 결과 (1:OK / 2:방No 오류 / 3:인원초과 / 4:기타오류)
	//
	// OK 의 경우에만 다음 전송
	//	{
	//		4Byte : 방 No
	//		2Byte : 방제목 Size
	//		Size  : 방제목 (유니코드)
	//
	//		1Byte : 참가인원
	//		{
	//			WCHAR[15] : 닉네임(유니코드)
	//			4Byte     : 사용자No
	//		}
	//	}
	//------------------------------------------------------------

	int iCnt;
	int iIndex;							// 리스트 인덱스
	UINT uiRoomNo;						// 대화방 번호
	WORD wRoomNameSize;					// 대화방 제목 사이즈
	char *wcpRoomName = NULL;			// 대화방 이름
	BYTE byResult;						// 결과
	BYTE byCount;						// 참가 인원 수
	HWND hListBox;						// 대화방 리스트 핸들
	
	g_stRoomUser = new st_RoomUser[df_ROOM_USERLISTMAX_SIZE];

	//패킷 읽기
	*pPacket >> &byResult;

	switch (byResult) {
	case 1:
		*pPacket >> &uiRoomNo >> &wRoomNameSize;
		wcpRoomName = new char[wRoomNameSize + 2];
		pPacket->ReadString((WCHAR *)wcpRoomName, wRoomNameSize);
		// 2021.01.22 13:43
		// 문자열 복사 후 뒤에 null 문자 추가 해야 합니다
		ZeroMemory(&wcpRoomName[wRoomNameSize], 2);
		*pPacket >> &byCount;
		
		// 대화방 켜기
		ShowWindow(g_hWndChat, SW_SHOWDEFAULT);
		g_hWnd = g_hWndChat;
		// 대화방 이름
		SetDlgItemTextW(g_hWndChat, IDC_STATIC_ROOMNAME, (WCHAR *)wcpRoomName);
		delete[] wcpRoomName;
		//리스트 박스 핸들 얻기
		hListBox = GetDlgItem(g_hWndChat, IDC_LIST_USER);

		for (iCnt = 0; iCnt < byCount; iCnt++) {
			g_stRoomUser[iCnt].flag = true;
			pPacket->ReadString(g_stRoomUser[iCnt].wcNickName, sizeof(g_stRoomUser[iCnt].wcNickName));
			*pPacket >> &g_stRoomUser[iCnt].uiUserNo;
			//리스트 박스 목록 수
			int iSize = SendMessage(hListBox, LB_GETCOUNT, 0, 0);
			//리스트 박스에 문자열 추가
			iIndex = SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)g_stRoomUser[iCnt].wcNickName);
			SendMessage(hListBox, LB_SETITEMDATA, (WPARAM)iIndex, (LPARAM)g_stRoomUser[iCnt].uiUserNo);
		}

		break;
	case 2:
		// 대화방 오류
		wsprintf(g_szLogBuff, L"PacketSCRoomEnter(), packet error [%d]\n", byResult);
		OutputDebugString(g_szLogBuff);
		SendMessage(g_hWndLobby, UM_SOCKET, g_sock, FD_CLOSE);
		MessageBox(g_hWndConnect, L"대화방 오류", L"Error", MB_OK | MB_ICONERROR);
		break;
	case 3:
		// 인원 초과
		wsprintf(g_szLogBuff, L"PacketSCRoomEnter(), packet error [%d]\n", byResult);
		OutputDebugString(g_szLogBuff);
		SendMessage(g_hWndLobby, UM_SOCKET, g_sock, FD_CLOSE);
		MessageBox(g_hWndConnect, L"인원 초과", L"Error", MB_OK | MB_ICONERROR);
		break;
	case 4:
		// 기타 오류
		wsprintf(g_szLogBuff, L"PacketSCRoomEnter(), packet error [%d]\n", byResult);
		OutputDebugString(g_szLogBuff);
		SendMessage(g_hWndLobby, UM_SOCKET, g_sock, FD_CLOSE);
		MessageBox(g_hWndConnect, L"기타 오류", L"Error", MB_OK | MB_ICONERROR);
		break;
	default:
		// 잘못된 결과 값
		wsprintf(g_szLogBuff, L"PacketSCRoomCreate(), packet error [%d]\n", byResult);
		OutputDebugString(g_szLogBuff);
		SendMessage(g_hWndLobby, UM_SOCKET, g_sock, FD_CLOSE);
		MessageBox(g_hWndConnect, L"잘못된 결과 값", L"Error", MB_OK | MB_ICONERROR);
		break;
	}
}

//패킷) 채팅 송신
void PacketSCChat(CSerializationBuffer *pPacket) {
	//------------------------------------------------------------
	// 10 Res 채팅수신 (아무때나 올 수 있음)  (나에겐 오지 않음)
	//
	// 4Byte : 송신자 No
	//
	// 2Byte : 메시지 Size
	// Size  : 대화내용(유니코드)
	//------------------------------------------------------------

	UINT uiUserNo;						// 유저 번호
	int iCnt;							// 리스트 인덱스
	WORD wSize;							// 메시지 사이즈
	WCHAR wcNickName[dfNICK_MAX_LEN];	// 닉네임
	WCHAR wcChat[32];					// 대화 내용
	WCHAR wcChat2[48];					// 유저 : 대화 내용
	HWND hListBox;						// 대화방 리스트 핸들

	//패킷 읽기
	*pPacket >> &uiUserNo >> &wSize;
	pPacket->ReadString(wcChat, wSize);
	// 2021.01.22 16:25
	// 문자열 복사 후 뒤에 null 문자 추가 해야 합니다
	// 메시지 사이즈가 글자 수가 아닌 BYTE 단위의 사이즈 이기 때문에 나누기 2 해줘야 합니다
	ZeroMemory(&wcChat[wSize / 2], 2);

	//리스트 박스 핸들 얻기
	hListBox = GetDlgItem(g_hWndChat, IDC_LIST_CHAT);

	for (iCnt = 0; iCnt < df_ROOM_USERLISTMAX_SIZE; iCnt++) {
		if (g_stRoomUser[iCnt].flag == true && g_stRoomUser[iCnt].uiUserNo == uiUserNo) {
			// 유저 : 대화 내용
			wsprintf(wcChat2, L"%s : %s", g_stRoomUser[iCnt].wcNickName, wcChat);
			//리스트 박스에 문자열 추가
			SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)wcChat2);
			return;
		}
	}
}

//패킷) 대화방 퇴장
void PacketSCLeave(CSerializationBuffer *pPacket) {
	//------------------------------------------------------------
	// 12 Res 방퇴장 (수시)
	//
	// 4Byte : 사용자 No
	//------------------------------------------------------------

	UINT uiUserNo;						// 유저 번호
	int iCnt;
	int iSize;

	//패킷 읽기
	*pPacket >> &uiUserNo;

	//리스트 박스 핸들 얻기
	HWND hListBox = GetDlgItem(g_hWndChat, IDC_LIST_USER);

	//리스트 박스 목록 수
	iSize = SendMessage(hListBox, LB_GETCOUNT, 0, 0);
	
	// 목록에서 사용자 삭제
	for (iCnt = 0; iCnt < iSize; iCnt++) {
		if (SendMessage(hListBox, LB_GETITEMDATA, (WPARAM)iCnt, 0) == uiUserNo) {
			SendMessage(hListBox, LB_DELETESTRING, (WPARAM)iCnt, 0);
			break;
		}
	}

	// 대화방에서 사용자 플래그 OFF
	for (iCnt = 0; iCnt < df_ROOM_USERLISTMAX_SIZE; iCnt++) {
		if (g_stRoomUser[iCnt].uiUserNo == uiUserNo) {
			g_stRoomUser[iCnt].flag = false;
			break;
		}
	}
}

//패킷) 대화방 삭제
void PacketSCRoomDelete(CSerializationBuffer *pPacket) {
	//------------------------------------------------------------
	// 13 Res 방삭제 (수시)
	//
	// 4Byte : 방 No
	//------------------------------------------------------------

	UINT uiRoomNo;						// 유저 번호
	int iCnt;
	int iSize;

	//패킷 읽기
	*pPacket >> &uiRoomNo;

	//리스트 박스 핸들 얻기
	HWND hListBox = GetDlgItem(g_hWndLobby, IDC_LIST_ROOM);

	//리스트 박스 목록 수
	iSize = SendMessage(hListBox, LB_GETCOUNT, 0, 0);

	// 목록에서 대화방 삭제
	for (iCnt = 0; iCnt < iSize; iCnt++) {
		if (SendMessage(hListBox, LB_GETITEMDATA, (WPARAM)iCnt, 0) == uiRoomNo) {
			SendMessage(hListBox, LB_DELETESTRING, (WPARAM)iCnt, 0);
			break;
		}
	}
}

//패킷) 다른 유저 대화방 입장
void PacketSCUserEnter(CSerializationBuffer *pPacket) {
	//------------------------------------------------------------
	// 14 Res 타 사용자 입장 (수시)
	//
	// WCHAR[15] : 닉네임(유니코드)
	// 4Byte : 사용자 No
	//------------------------------------------------------------

	
	int iIndex;							//리스트 인덱스
	int iCnt;
	HWND hListBox;						//대화방 리스트 핸들

	for (iCnt = 0; iCnt < df_ROOM_USERLISTMAX_SIZE; iCnt++) {
		if (g_stRoomUser[iCnt].flag == false) {
			g_stRoomUser[iCnt].flag = true;
			// 패킷 읽기
			pPacket->ReadString(g_stRoomUser[iCnt].wcNickName, sizeof(g_stRoomUser[iCnt].wcNickName));
			*pPacket >> &g_stRoomUser[iCnt].uiUserNo;

			// 리스트 박스 핸들 얻기
			hListBox = GetDlgItem(g_hWndChat, IDC_LIST_USER);
			// 리스트 박스에 문자열 추가
			iIndex = SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)g_stRoomUser[iCnt].wcNickName);
			SendMessage(hListBox, LB_SETITEMDATA, (WPARAM)iIndex, (LPARAM)g_stRoomUser[iCnt].uiUserNo);

			return;
		}
	}

	wsprintf(g_szLogBuff, L"PacketSCUserEnter(), error user list max\n");
	OutputDebugString(g_szLogBuff);
}


// 해더 만들기
void CreateHeader(CSerializationBuffer &SerialBuffer, BYTE	byCheckSum, WORD wMsgType, WORD	wPayloadSize) {
	int headerSize;							// 해더 사이즈
	char *chpPayload;						// 버퍼 포인터
	st_PACKET_HEADER header;				// 헤더
	headerSize = sizeof(st_PACKET_HEADER);
	SerialBuffer.MoveFront(-headerSize);
	chpPayload = SerialBuffer.GetReadBufferPtr();

	header.byCode = dfPACKET_CODE;
	header.byCheckSum = byCheckSum;
	header.wMsgType = wMsgType;
	header.wPayloadSize = wPayloadSize;

	memcpy_s(chpPayload, headerSize, &header, headerSize);
}

// 로그인 요청 패킷 만들기
void CreateReqLoginPacket(CSerializationBuffer &SerialBuffer) {
	//------------------------------------------------------------
	// 1 Req 로그인
	//
	//
	// WCHAR[15]	: 닉네임 (유니코드)
	//------------------------------------------------------------

	int headerSize = sizeof(st_PACKET_HEADER);
	// 2021.01.22 13:18
	// 큐의 Front 이동보다 Rear 이동이 먼저 되야함
	// Front 가 Rear 보다 넘어가는 현상을 예외처리 하면서 처리 순서를 바꿈
	SerialBuffer.MoveRear(headerSize);
	SerialBuffer.MoveFront(headerSize);

	SerialBuffer.WriteString(g_wcNickname, sizeof(g_wcNickname));
}

//대화방 리스트 요청 패킷 만들기
void CreateReqRoomListPacket(CSerializationBuffer &SerialBuffer) {
	//------------------------------------------------------------
	// 11 Req 방퇴장 
	//
	// None
	//------------------------------------------------------------

	int headerSize = sizeof(st_PACKET_HEADER);
	// 2021.01.22 13:18
	// 큐의 Front 이동보다 Rear 이동이 먼저 되야함
	// Front 가 Rear 보다 넘어가는 현상을 예외처리 하면서 처리 순서를 바꿈
	SerialBuffer.MoveRear(headerSize);
	SerialBuffer.MoveFront(headerSize);
}

//대화방 생성 요청 패킷 만들기
void CreateReqRoomCreatePacket(CSerializationBuffer &SerialBuffer, WCHAR * wcRoomName, WORD wSize) {
	//------------------------------------------------------------
	// 5 Req 대화방 생성
	//
	// 2Byte : 방제목 Size			유니코드 문자 바이트 길이 (널 제외)
	// Size  : 방제목 (유니코드)
	//------------------------------------------------------------

	int headerSize = sizeof(st_PACKET_HEADER);
	// 2021.01.22 13:18
	// 큐의 Front 이동보다 Rear 이동이 먼저 되야함
	// Front 가 Rear 보다 넘어가는 현상을 예외처리 하면서 처리 순서를 바꿈
	SerialBuffer.MoveRear(headerSize);
	SerialBuffer.MoveFront(headerSize);

	SerialBuffer << wSize;
	SerialBuffer.WriteString(wcRoomName, wSize);
}

//대화방 입장 요청 패킷 만들기
void CreateReqRoomEnterPacket(CSerializationBuffer &SerialBuffer, UINT uiRoomNo) {
	//------------------------------------------------------------
	// 7 Req 대화방 입장
	//
	//	4Byte : 방 No
	//------------------------------------------------------------

	int headerSize = sizeof(st_PACKET_HEADER);
	// 2021.01.22 13:18
	// 큐의 Front 이동보다 Rear 이동이 먼저 되야함
	// Front 가 Rear 보다 넘어가는 현상을 예외처리 하면서 처리 순서를 바꿈
	SerialBuffer.MoveRear(headerSize);
	SerialBuffer.MoveFront(headerSize);

	SerialBuffer << uiRoomNo;
}

//채팅 송신 요청 패킷 만들기
void CreateReqChatPacket(CSerializationBuffer &SerialBuffer, WCHAR * wcChat, WORD wSize) {
	//------------------------------------------------------------
	// 9 Req 채팅송신
	//
	// 2Byte : 메시지 Size
	// Size  : 대화내용(유니코드)
	//------------------------------------------------------------

	int headerSize = sizeof(st_PACKET_HEADER);
	// 2021.01.22 13:18
	// 큐의 Front 이동보다 Rear 이동이 먼저 되야함
	// Front 가 Rear 보다 넘어가는 현상을 예외처리 하면서 처리 순서를 바꿈
	SerialBuffer.MoveRear(headerSize);
	SerialBuffer.MoveFront(headerSize);

	SerialBuffer << wSize;
	SerialBuffer.WriteString(wcChat, wSize);
}

//대화방 퇴장 요청 패킷 만들기
void CreateReqLeavePacket(CSerializationBuffer &SerialBuffer) {
	//------------------------------------------------------------
	// 11 Req 방퇴장 
	//
	// None
	//------------------------------------------------------------

	int headerSize = sizeof(st_PACKET_HEADER);
	// 2021.01.22 13:18
	// 큐의 Front 이동보다 Rear 이동이 먼저 되야함
	// Front 가 Rear 보다 넘어가는 현상을 예외처리 하면서 처리 순서를 바꿈
	SerialBuffer.MoveRear(headerSize);
	SerialBuffer.MoveFront(headerSize);
}