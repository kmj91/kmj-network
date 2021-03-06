// ChattingServer.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include <WinSock2.h>
#include <WS2tcpip.h>

#include <map>
//#include <algorithm>

#include "Protocol.h"
#include "CRingBuffer.h"
#include "CSerializationBuffer.h"

#pragma comment(lib, "ws2_32")

#ifndef	MAIN_DEFINE

#define dfFDSET_SIZE				64
#define dfLOGINMAX_SIZE				100			// 로그인 유저 인원 수
#define dfUSERMAX_SIZE				100			// 유저 수용 인원 수
#define dfROOMMAX_SIZE				10			// 대화방 생성 개수
#define dfROOM_USERLISTMAX_SIZE		5			// 대화방 수용 인원 수
#define dfUSERSTATE_ZERO			0
#define dfUSERSTATE_LOGIN			1
#define dfUSERSTATE_CLOSED			2
#define dfUSERSTATE_DELETE			4

#define dfROOMSTATE_ZERO			0
#define dfROOMSTATE_USE				1

#define dfUSERLIST_INIT				0

#endif

using namespace std;


struct st_SESSION_ROOM {
	BYTE state;					//상태
	UINT roomNumber;			//대화방 번호
	WORD roomNameSize;			//대화방 이름 크기
	WCHAR *roomName;			//대화방 이름
	BYTE totalUser;				//대화방 참가인원 수
	UINT * userList;			//참가 인원 키 리스트
};

struct st_SESSION_USER {
	SOCKET sock;									// 소켓
	UINT userNumber;								// 유저 번호
	WCHAR * nickName;								// 닉네임
	SOCKADDR_IN addr;								// 주소
	CRingBuffer * readBuffer;						// 리시브 버퍼
	CRingBuffer * writeBuffer;						// 샌드 버퍼
	st_SESSION_ROOM * sessionRoom;					// 입장한 대화방 주소
	BYTE state;										// 상태
};

//struct NICKNAME_FINDER
//{
//	NICKNAME_FINDER(WCHAR *Nickname) : wchNickname(Nickname)
//	{
//	}
//	bool operator() (const pair<UINT, st_SESSION_USER *>& iter) const
//	{
//		if (!wcscmp(iter.second->nickName, wchNickname))
//			return true;
//
//		return false;
//	}
//	WCHAR *wchNickname;
//};


bool Network_init();				//네트워크 초기화
void Session_init();				//세션 초기화
bool NetworkService();
void AcceptProc();					//억셉트 처리
void CreateUser();					//유저 생성
void LoginProc();					//로그인 처리
void NetworkProc();					//네트워크 처리
bool CheckSum(st_PACKET_HEADER header, CRingBuffer *readBuffer);					//체크섬 검사
BYTE GetCheckSum(char * chpBuffer, WORD wMsgType, int iPayloadSize);				//체크섬 값 취득
void ReceiveProc(st_SESSION_USER * SessionUser, UINT uiKey);						//리시브 처리
void SandProc(st_SESSION_USER * SessionUser);										//샌드 처리
void Request_Login(st_SESSION_USER * SessionUser, WORD wPayloadSize);				//로그인 요청
void Response_Login(st_SESSION_USER * SessionUser, BYTE Result);					//로그인 응답
void Request_RoomList(st_SESSION_USER * SessionUser);								//대화방 리스트 요청 - 필요없음 세션에 유저 번호가 있기 때문에 식별 가능
void Response_RoomList(st_SESSION_USER * SessionUser);								//대화방 리스트 응답
void Request_RoomCreate(st_SESSION_USER * SessionUser, WORD wPayloadSize);			//대화방 생성 요청
void Response_RoomCreate(st_SESSION_USER * SessionUser, st_SESSION_ROOM * SessionRoom, BYTE Result);	//대화방 생성 응답
void Request_RoomEnter(st_SESSION_USER * SessionUser, WORD wPayloadSize, UINT uiKey);					//대화방 입장 요청
void Response_RoomEnter(st_SESSION_USER * SessionUser, st_SESSION_ROOM * SessionRoom, UINT uiKey, BYTE Result);	//대화방 입장 응답
void Response_UserEnter(st_SESSION_USER * SessionUser, UINT * UserList, UINT uiKey);				//대화방 유저 입장 응답
void Request_Chat(st_SESSION_USER * SessionUser, WORD wPayloadSize, UINT uiKey);					//채팅 송신 요청
void Response_Chat(st_SESSION_USER * SessionUser, WORD wMessageSize, WCHAR * wchpMessage, UINT uiKey);	//채팅 송신 응답
void Request_RoomLeave(st_SESSION_USER * SessionUser);												//대화방 퇴장 요청
void Response_RoomLeave(st_SESSION_USER * SessionUser, st_SESSION_ROOM * SessionRoom);				//대화방 퇴장 응답
void Response_RoomDelete(UINT RoomNumber);															//대화방 삭제
bool CheckNickname(WCHAR *wchNickname);																//닉네임 중복 체크
void ReadPayload(CSerializationBuffer *SerialBuffer, CRingBuffer *readBuffer, WORD wPayloadSize);	//페이로드 읽기
bool CheckRoomName(WCHAR *wchpRoomName);															//대화방 중복 체크
void GetRoomList(CSerializationBuffer * SerialBuffer);												//대화방 리스트 획득
void EraseUserList(UINT *userList, UINT uiKey);														//대화방 유저 삭제
bool SetUserList(UINT *userList, UINT uiKey);														//대화방 참가인원 유저 추가
void GetUserList(CSerializationBuffer * SerialBuffer, st_SESSION_ROOM * SessionRoom, bool Option = false);	//대화방 참가인원 취득
void GetUserList_UserInfo(CSerializationBuffer * SerialBuffer, UINT uiKey, bool Option);			//유저 정보 취득
bool FindEmptyRoom(UINT *iNumber);																	//빈 대화방 번호 찾기
bool FindRoom(UINT uiRoomNumber);																	//대화방 찾기
void CreateHeader(st_PACKET_HEADER * Header, BYTE byCode, BYTE byCheckSum, WORD wMsgType, WORD wPayloadSize);	//헤더 생성
void CreateSessionRoom(st_SESSION_ROOM * SessionRoom, UINT uiArrayindex, WORD wRoomNameSize, WCHAR *wchpRoomName);	//대화방 세션 생성
void Unicast(CRingBuffer * WriteBuffer, st_PACKET_HEADER Header, char * chpPayload, WORD wPayloadSize);		//유니캐스트
void Broadcast(st_PACKET_HEADER Header, char * chpPayload, WORD wPayloadSize, UINT uiKey = 0);				//브로드캐스트
void RoomBroadcast(UINT * UserList, st_PACKET_HEADER Header, char * chpPayload, WORD wPayloadSize, UINT uiKey = 0);	//대화방 브로드캐스트
bool CheckSessionClosed(st_SESSION_USER * SessionUser);												//세션 삭제 검사
void RemoveSession(st_SESSION_USER * SessionUser);													//세션 삭제

typedef pair <UINT, st_SESSION_USER *> SessionUser_Pair;

SOCKET g_listen_sock;
UINT g_uiUserNumber = 0;
//UINT g_uiMapKey = 0;			//사용하지 않음
st_SESSION_ROOM *g_arraySessionRoom;
map<UINT, st_SESSION_USER *> g_mapLogin;			//로그인 대기중 유저
map<UINT, st_SESSION_USER *> g_mapSessionUser;		//로그인 성공한 유저
//CSerializationBuffer g_buffer;


int main()
{
	if (!Network_init()) {
		return 0;
	}
	Session_init();

	//로직
	while (1) {
		if (!NetworkService())
			break;
	}

	/*
	// key는 1, value는 35를 추가.
	map1.insert( map< int, int >::value_type(1, 35)); 
	// 또는 STL의 pair를 사용하기도 한다.
	typedef pair < int, int > Int_Pair;
	map1.insert(Int_Pair(2, 45)); 

	// 첫 번째 위치에 key 1, value 35를 추가
	map1.insert( map1.begin(), map< int, int >::value_type(1, 35) ); 
	// 또는
	map1.insert( map1.begin(),  Int_Pair(2, 45) ); 
	*/
    return 0;
}


//네트워크 초기화
bool Network_init() {
	int retval;

	//윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return false;

	//리슨 소켓
	g_listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (g_listen_sock == INVALID_SOCKET) {
		retval = GetLastError();
		printf("[%d] listen socket error\n", retval);
		return false;
	}

	//넌블로킹 전환
	u_long on = 1;
	retval = ioctlsocket(g_listen_sock, FIONBIO, &on);
	if (retval == SOCKET_ERROR) {
		retval = GetLastError();
		printf("[%d] ioctlsocket error\n", retval);
		return false;
	}

	//bind()
	SOCKADDR_IN serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(dfNETWORK_PORT);
	retval = bind(g_listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) {
		retval = GetLastError();
		printf("[%d] bind error\n", retval);
		return false;
	}

	//listen()
	retval = listen(g_listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) {
		retval = GetLastError();
		printf("[%d] listen error\n", retval);
		return false;
	}

	printf("Server Open\n");
	return true;
}

//세션 초기화
void Session_init() {
	int icnt;
	g_arraySessionRoom = new st_SESSION_ROOM[dfROOMMAX_SIZE];

	for (icnt = 0; icnt < dfROOMMAX_SIZE; icnt++) {
		g_arraySessionRoom[icnt].state = dfROOMSTATE_ZERO;
	}
}

//네트워크
bool NetworkService() {
	
	AcceptProc();
	LoginProc();
	NetworkProc();

	return true;
}


//억셉트 처리
void AcceptProc() {
	FD_SET rset;
	timeval time;
	time.tv_sec = 0;
	time.tv_usec = 0;
	int iretcnt = 0;
	int err;
	static int icnt = 0;

	while (1) {
		//초기화
		FD_ZERO(&rset);
		//FD 셋
		FD_SET(g_listen_sock, &rset);
		//select
		iretcnt = select(0, &rset, NULL, NULL, &time);
		if (iretcnt == SOCKET_ERROR) {
			err = GetLastError();
			printf("[%d] select error\n", err);
			return;
		}
		if (FD_ISSET(g_listen_sock, &rset)) {
			//신규 유저 생성
			CreateUser();
			++icnt;
			if (icnt >= dfLOGINMAX_SIZE) {
				break;
			}
		}
		else {
			break;
		}
	}
}


//신규 유저 생성
void CreateUser() {
	int addrlen;
	st_SESSION_USER *newSessionUser = new st_SESSION_USER;
	SOCKET clientSocket;		//신규 소켓
	int err;
	WCHAR szClientIP[16];

	addrlen = sizeof(newSessionUser->addr);
	//accept()
	clientSocket = accept(g_listen_sock, (SOCKADDR *)&newSessionUser->addr, &addrlen);
	if (clientSocket == INVALID_SOCKET) {
		if (clientSocket == WSAEWOULDBLOCK) {
			//연결 요청 받지 못함
			delete newSessionUser;
			return;
		}
		err = GetLastError();
		printf("[%d] accept error\n", err);
		delete newSessionUser;
		return;
	}

	//세션 셋
	newSessionUser->sock = clientSocket;
	newSessionUser->userNumber = ++g_uiUserNumber;
	newSessionUser->nickName = new WCHAR[dfNICK_MAX_LEN];
	newSessionUser->readBuffer = new CRingBuffer;
	newSessionUser->writeBuffer = new CRingBuffer;
	newSessionUser->sessionRoom = NULL;
	newSessionUser->state = dfUSERSTATE_LOGIN;

	g_mapLogin.insert(SessionUser_Pair(g_uiUserNumber, newSessionUser));
	
	printf("Aceept %S::%d  [UserNumber:%d]\n",
		InetNtop(AF_INET, &newSessionUser->addr.sin_addr, szClientIP, 16),
		ntohs(newSessionUser->addr.sin_port),
		newSessionUser->userNumber);

	return;
}

//로그인 처리
void LoginProc() {
	map<UINT, st_SESSION_USER *>::iterator iter;
	map<UINT, st_SESSION_USER *>::iterator iterTemp;
	map<UINT, st_SESSION_USER *>::iterator iter_end;
	FD_SET rset;
	FD_SET wset;
	timeval time;
	time.tv_sec = 0;
	time.tv_usec = 0;
	int iFDcnt;
	int icnt;
	int iretcnt = 0;
	int err;
	st_SESSION_USER * sessionUser;
	st_SESSION_USER * sessionArray[dfFDSET_SIZE];
	SOCKET sock;
	WCHAR szClientIP[16];
	bool iterFlag = true;

	if (g_mapLogin.size() == 0) {
		return;
	}

	iter = g_mapLogin.begin();
	iter_end = g_mapLogin.end();
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
			if (iFDcnt >= dfFDSET_SIZE) {
				break;
			}
		}

		//select
		iretcnt = select(0, &rset, &wset, NULL, &time);
		if (iretcnt == SOCKET_ERROR) {
			err = GetLastError();
			printf("[%d] select error\n", err);
			return;
		}
		//클라 소켓
		icnt = 0;
		while (icnt < iFDcnt) {
			sessionUser = sessionArray[icnt];
			++icnt;
			if (FD_ISSET(sessionUser->sock, &rset)) {
				ReceiveProc(sessionUser, NULL);
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
				//_LOG(df_LOG_LEVEL_NOTICE, L"Disconnect %s::%d  [ID:%d]",
				//	InetNtop(AF_INET, &sessionUser->addr.sin_addr, szClientIP, 16),
				//	ntohs(sessionUser->addr.sin_port),
				//	sessionUser->dwSessionID);
				printf("Disconnect %S::%d  [UserNumber:%d]\n",
					InetNtop(AF_INET, &sessionUser->addr.sin_addr, szClientIP, 16),
					ntohs(sessionUser->addr.sin_port),
					sessionUser->userNumber);

				//세션 삭제
				iterTemp = g_mapLogin.find(sessionUser->userNumber);
				if (icnt == iFDcnt) {

					iter = g_mapLogin.erase(iterTemp);
				}
				else {
					g_mapLogin.erase(iterTemp);
				}

				RemoveSession(sessionUser);
			}
			//로그인 성공 체크
			else if (sessionUser->state == dfUSERSTATE_ZERO) {
				//세션 삭제
				iterTemp = g_mapLogin.find(sessionUser->userNumber);
				if (icnt == iFDcnt) {

					iter = g_mapLogin.erase(iterTemp);
				}
				else {
					g_mapLogin.erase(iterTemp);
				}
				//2018.05.03
				//사용 하지않음
				//++g_uiMapKey;
				//g_mapSessionUser.insert(SessionUser_Pair(g_uiMapKey, sessionUser));
				g_mapSessionUser.insert(SessionUser_Pair(sessionUser->userNumber, sessionUser));
				sessionUser->state = dfUSERSTATE_LOGIN;
				iterFlag = false;
			}

			//루프 정지
			if (iretcnt <= 0) {
				break;
			}
		}//while (iretcnt > 0)
	}//while (iterTemp != iter_end)
}

/*
void LoginProc() {
	map<UINT, st_SESSION_USER *>::iterator iter;
	map<UINT, st_SESSION_USER *>::iterator iterTemp;
	map<UINT, st_SESSION_USER *>::iterator iter_begin;
	map<UINT, st_SESSION_USER *>::iterator iter_end;
	FD_SET rset;
	FD_SET wset;
	timeval time;
	time.tv_sec = 0;
	time.tv_usec = 0;
	int iFDcnt;
	int iretcnt = 0;
	int err;
	st_SESSION_USER * sessionUser;
	SOCKET sock;
	WCHAR szClientIP[16];
	bool iterFlag = true;

	if (g_mapLogin.size() == 0) {
		return;
	}

	iter_begin = g_mapLogin.begin();
	iter_end = g_mapLogin.end();
	while (iter_begin != iter_end)
	{
		//초기화
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		//FD 셋
		//클라 소켓
		iter = iter_begin;
		iFDcnt = 0;
		while (iter != iter_end) {
			sock = iter->second->sock;
			FD_SET(sock, &rset);
			FD_SET(sock, &wset);
			++iter;
			++iFDcnt;
			if (iFDcnt >= dfFDSET_SIZE) {
				break;
			}
		}
		iterTemp = iter_begin;
		iter_begin = iter;
		iter = iterTemp;

		//select
		iretcnt = select(0, &rset, &wset, NULL, &time);
		if (iretcnt == SOCKET_ERROR) {
			err = GetLastError();
			printf("[%d] select error\n", err);
			return;
		}
		//클라 소켓
		while (iretcnt > 0) {
			sessionUser = iter->second;

			if (FD_ISSET(sessionUser->sock, &rset)) {
				ReceiveProc(sessionUser, iter->first);
				--iretcnt;
			}

			if (FD_ISSET(sessionUser->sock, &wset)) {
				if (sessionUser->writeBuffer->GetUseSize() != 0) {
					SandProc(sessionUser);
					//로그인 성공 체크
					if (sessionUser->state == dfUSERSTATE_ZERO) {
						iter = g_mapLogin.erase(iter);
						++g_uiMapKey;
						g_mapSessionUser.insert(SessionUser_Pair(g_uiMapKey, sessionUser));
						sessionUser->state = dfUSERSTATE_LOGIN;
						iterFlag = false;
					}
				}
				else {
					//세션 삭제 체크
					if ((sessionUser->state & dfUSERSTATE_CLOSED) == dfUSERSTATE_CLOSED) {
						iter = g_mapLogin.erase(iter);
						closesocket(sessionUser->sock);
						printf("Disconnect %S::%d  [UserNumber:%d]\n",
							InetNtop(AF_INET, &sessionUser->addr.sin_addr, szClientIP, 16),
							ntohs(sessionUser->addr.sin_port),
							sessionUser->userNumber);
						delete sessionUser->readBuffer;
						delete sessionUser->writeBuffer;
						delete[] sessionUser->nickName;
						delete sessionUser;
						iterFlag = false;
					}
				}
				--iretcnt;
			}

			if (iterFlag) {
				++iter;
			}
			else {
				iterFlag = true;
			}
		}//while (iretcnt > 0)
	}//while (iterTemp != iter_end)
}
*/

//네트워크 처리
void NetworkProc() {
	map<UINT, st_SESSION_USER *>::iterator iter;
	map<UINT, st_SESSION_USER *>::iterator iterTemp;
	map<UINT, st_SESSION_USER *>::iterator iter_end;
	FD_SET rset;
	FD_SET wset;
	timeval time;
	time.tv_sec = 0;
	time.tv_usec = 0;
	int iFDcnt;
	int icnt;
	int iretcnt = 0;
	int err;
	st_SESSION_USER * sessionUser;
	st_SESSION_USER * sessionArray[dfFDSET_SIZE];
	SOCKET sock;
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
			if (iFDcnt >= dfFDSET_SIZE) {
				break;
			}
		}

		//select
		iretcnt = select(0, &rset, &wset, NULL, &time);
		if (iretcnt == SOCKET_ERROR) {
			err = GetLastError();
			printf("[%d] select error\n", err);
			return;
		}

		//클라 소켓
		icnt = 0;
		while (icnt < iFDcnt) {
			sessionUser = sessionArray[icnt];
			++icnt;
			if (FD_ISSET(sessionUser->sock, &rset)) {
				ReceiveProc(sessionUser, sessionUser->userNumber);
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
				//_LOG(df_LOG_LEVEL_NOTICE, L"Disconnect %s::%d  [ID:%d]",
				//	InetNtop(AF_INET, &sessionUser->addr.sin_addr, szClientIP, 16),
				//	ntohs(sessionUser->addr.sin_port),
				//	sessionUser->dwSessionID);
				printf("Disconnect %S::%d  [UserNumber:%d]\n",
					InetNtop(AF_INET, &sessionUser->addr.sin_addr, szClientIP, 16),
					ntohs(sessionUser->addr.sin_port),
					sessionUser->userNumber);

				//세션 삭제
				iterTemp = g_mapSessionUser.find(sessionUser->userNumber);
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
		}//while (iretcnt > 0)
	}//while (iterTemp != iter_end)
}

//체크섬
bool CheckSum(st_PACKET_HEADER header, CRingBuffer *readBuffer) {
	int wPacketSize = header.wPayloadSize;
	char *chpPayload = new char[wPacketSize];
	char *chpTemp = chpPayload;
	BYTE retval = 0;

	readBuffer->Peek(chpPayload, wPacketSize);

	for (int cnt = 0; cnt < wPacketSize; cnt++) {
		retval = retval + *chpPayload;
		chpPayload++;
	}
	retval = retval + header.wMsgType;
	retval = retval % 256;

	delete[] chpTemp;

	if (header.byCheckSum != retval) {
		return true;
	}
	return false;
}

//체크섬 값 취득
BYTE GetCheckSum(char * chpBuffer, WORD wMsgType, int iPayloadSize) {
	char *chpTemp;
	BYTE retval = 0;

	chpTemp = chpBuffer;

	for (int cnt = 0; cnt < iPayloadSize; cnt++) {
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

//리시브 처리
void ReceiveProc(st_SESSION_USER * SessionUser, UINT uiKey) {
	int iretval;
	char *chpRear;
	int iReadSize;
	int wPacketSize;
	st_PACKET_HEADER header;
	int iHeaderSize = sizeof(header);
	CRingBuffer * readBuffer = SessionUser->readBuffer;



	chpRear = readBuffer->GetWriteBufferPtr();
	iReadSize = readBuffer->GetNotBrokenPutSize();

	//데이터 받기
	iretval = recv(SessionUser->sock, chpRear, iReadSize, 0);
	if (iretval == SOCKET_ERROR) {
		if (iretval == WSAEWOULDBLOCK) {
			return;
		}
		
		iretval = GetLastError();
		printf("[%d] recv error\n", iretval);
		SessionUser->state = SessionUser->state | dfUSERSTATE_CLOSED;
		return;
	}
	else if (iretval == 0) {
		SessionUser->state = SessionUser->state | dfUSERSTATE_CLOSED;
		return;
	}

	//받은만큼 이동
	readBuffer->MoveRear(iretval);

	//완성된 패킷 전부 처리
	while (1) {

		//헤더 읽기
		iretval = readBuffer->Peek((char *)&header, iHeaderSize);
		if (iretval < iHeaderSize) {
			return;
		}

		//패킷 코드 확인
		if (header.byCode != dfPACKET_CODE) {
			return;
		}
		
		//패킷 사이즈
		wPacketSize = header.wPayloadSize;

		//패킷 완성됬나
		iretval = readBuffer->GetUseSize();
		if (iretval < wPacketSize + iHeaderSize) {
			return;
		}

		readBuffer->MoveFront(iHeaderSize);

		//체크섬 검사
		if (CheckSum(header, readBuffer)) {
			return;
		}


		//패킷 타입
		switch (header.wMsgType) {
		case df_REQ_LOGIN:
			printf("PacketRecv [UserNumber:%d] [Type:%d]\n",
				SessionUser->userNumber, header.wMsgType);
			//로그인 요청
			Request_Login(SessionUser, wPacketSize);
			break;
		case df_REQ_ROOM_LIST:
			printf("PacketRecv [UserNumber:%d] [Type:%d]\n",
				SessionUser->userNumber, header.wMsgType);
			//대화방 리스트 요청
			//Request_RoomList(SessionUser);
			//대화방 리스트 응답
			Response_RoomList(SessionUser);
			break;
		case df_REQ_ROOM_CREATE:
			printf("PacketRecv [UserNumber:%d] [Type:%d]\n",
				SessionUser->userNumber, header.wMsgType);
			//대화방 생성 요청
			Request_RoomCreate(SessionUser, wPacketSize);
			break;
		case df_REQ_ROOM_ENTER:
			printf("PacketRecv [UserNumber:%d] [Type:%d]\n",
				SessionUser->userNumber, header.wMsgType);
			//대화방 입장 요청
			Request_RoomEnter(SessionUser, wPacketSize, uiKey);
			break;
		case df_REQ_CHAT:
			printf("PacketRecv [UserNumber:%d] [Type:%d]\n",
				SessionUser->userNumber, header.wMsgType);
			//채팅 송신 요청
			Request_Chat(SessionUser, wPacketSize, uiKey);
			break;
		case df_REQ_ROOM_LEAVE:
			printf("PacketRecv [UserNumber:%d] [Type:%d]\n",
				SessionUser->userNumber, header.wMsgType);
			//대화방 퇴장 요청
			Request_RoomLeave(SessionUser);
			break;
		default:
			return;
		}
	}//while(1)
}

//샌드 처리
void SandProc(st_SESSION_USER * SessionUser) {
	int iretval;
	char *chpRear;
	int iWriteSize;
	int iNotBrokenSize;
	CRingBuffer * writeBuffer = SessionUser->writeBuffer;

	chpRear = writeBuffer->GetReadBufferPtr();
	iWriteSize = writeBuffer->GetUseSize();
	iNotBrokenSize = writeBuffer->GetNotBrokenPutSize();

	//링버퍼 끝 체크
	if (iWriteSize > iNotBrokenSize) {
		iWriteSize = iNotBrokenSize;
	}

	iretval = send(SessionUser->sock, chpRear, iWriteSize, 0);
	if (iretval == SOCKET_ERROR) {
		if (iretval == WSAEWOULDBLOCK) {
			return;
		}

		iretval = GetLastError();
		printf("[%d] send error\n", iretval);

		return;
	}

	writeBuffer->MoveFront(iretval);
}

//로그인 요청
void Request_Login(st_SESSION_USER * SessionUser, WORD wPayloadSize) {
	CRingBuffer * readBuffer = SessionUser->readBuffer;
	CSerializationBuffer serialBuffer;
	BYTE result = df_RESULT_LOGIN_OK;

	//페이로드 읽기
	ReadPayload(&serialBuffer, readBuffer, wPayloadSize);
	serialBuffer >> SessionUser->nickName;

	//닉네임 중복 검사
	if (CheckNickname(SessionUser->nickName)) {
		result = df_RESULT_LOGIN_DNICK;
		SessionUser->state = dfUSERSTATE_CLOSED;
	}
	//사용자 초과 검사
	else if (g_mapSessionUser.size() >= dfUSERMAX_SIZE) {
		result = df_RESULT_LOGIN_MAX;
		SessionUser->state = dfUSERSTATE_CLOSED;
	}
	//로그인 응답
	Response_Login(SessionUser, result);
}

//로그인 응답
void Response_Login(st_SESSION_USER * SessionUser, BYTE Result) {
	st_PACKET_HEADER header;
	WORD wPayloadSize;
	char *chpPayload;
	CRingBuffer * writeBuffer = SessionUser->writeBuffer;
	CSerializationBuffer serialBuffer;
	BYTE byCheckSum;

	chpPayload = serialBuffer.GetReadBufferPtr();

	if (Result != df_RESULT_LOGIN_OK) {
		//페이로드 생성
		serialBuffer << Result;
		//페이로드 사이즈
		wPayloadSize = serialBuffer.GetUseSize();
		//체크섬 값 취득
		byCheckSum = GetCheckSum(chpPayload, df_RES_LOGIN, wPayloadSize);
	}
	else {
		//페이로드 생성
		serialBuffer << Result << SessionUser->userNumber;
		//페이로드 사이즈
		wPayloadSize = serialBuffer.GetUseSize();
		//체크섬 값 취득
		byCheckSum = GetCheckSum(chpPayload, df_RES_LOGIN, wPayloadSize);
		//넌 로그인 성공임
		SessionUser->state = dfUSERSTATE_ZERO;
	}

	//헤더 만들기
	CreateHeader(&header, dfPACKET_CODE, byCheckSum, df_RES_LOGIN, wPayloadSize);
	//유니캐스트
	Unicast(writeBuffer, header, chpPayload, wPayloadSize);
}

//대화방 리스트 요청
void Request_RoomList(st_SESSION_USER * SessionUser) {

}

//대화방 리스트 응답
void Response_RoomList(st_SESSION_USER * SessionUser) {
	st_PACKET_HEADER header;
	WORD wPayloadSize;
	char *chpPayload;
	CRingBuffer * writeBuffer = SessionUser->writeBuffer;
	CSerializationBuffer serialBuffer;
	BYTE byCheckSum;

	chpPayload = serialBuffer.GetReadBufferPtr();

	//대화방 리스트 획득
	GetRoomList(&serialBuffer);
	//페이로드 사이즈
	wPayloadSize = serialBuffer.GetUseSize();
	//체크섬 값 취득
	byCheckSum = GetCheckSum(chpPayload, df_RES_ROOM_LIST, wPayloadSize);
	//헤더 만들기
	CreateHeader(&header, dfPACKET_CODE, byCheckSum, df_RES_ROOM_LIST, wPayloadSize);
	//유니캐스트
	Unicast(writeBuffer, header, chpPayload, wPayloadSize);
}

//대화방 생성 요청
void Request_RoomCreate(st_SESSION_USER * SessionUser, WORD wPayloadSize) {
	CRingBuffer * readBuffer = SessionUser->readBuffer;
	UINT uiArrayindex;
	CSerializationBuffer serialBuffer;
	BYTE result = df_RESULT_ROOM_CREATE_OK;
	st_SESSION_ROOM * sessionRoom = NULL;
	WORD wRoomNameSize;
	char *wchpRoomName;

	//페이로드 읽기
	ReadPayload(&serialBuffer, readBuffer, wPayloadSize);
	
	//대화방 빈 배열 찾기
	if (FindEmptyRoom(&uiArrayindex)) {
		serialBuffer >> &wRoomNameSize;
		wchpRoomName = new char[wRoomNameSize + 2];
		serialBuffer.ReadString((WCHAR *)wchpRoomName, wRoomNameSize);

		//대화방 이름 중복 체크
		if (CheckRoomName((WCHAR *)wchpRoomName)) {
			//중복
			result = df_RESULT_ROOM_CREATE_DNICK;
			delete[] wchpRoomName;
		}
		else {
			sessionRoom = &g_arraySessionRoom[uiArrayindex];
			//세션 생성
			CreateSessionRoom(sessionRoom, uiArrayindex, wRoomNameSize, (WCHAR *)wchpRoomName);
		}
	}
	else {
		//못찾음
		result = df_RESULT_ROOM_CREATE_MAX;
	}
	//대화방 생성 응답
	Response_RoomCreate(SessionUser, sessionRoom, result);
}

//대화방 생성 응답
void Response_RoomCreate(st_SESSION_USER * SessionUser, st_SESSION_ROOM * SessionRoom, BYTE Result) {
	st_PACKET_HEADER header;
	WORD wPayloadSize;
	char *chpPayload;
	CRingBuffer * writeBuffer;
	CSerializationBuffer serialBuffer;
	BYTE byCheckSum;

	chpPayload = serialBuffer.GetReadBufferPtr();

	if (Result != df_RESULT_ROOM_CREATE_OK) {
		//페이로드 생성
		serialBuffer << Result;
		//페이로드 사이즈
		wPayloadSize = serialBuffer.GetUseSize();
		//체크섬 값 취득
		byCheckSum = GetCheckSum(chpPayload, df_RES_ROOM_CREATE, wPayloadSize);
		//헤더 만들기
		CreateHeader(&header, dfPACKET_CODE, byCheckSum, df_RES_ROOM_CREATE, wPayloadSize);
		//유니캐스트
		writeBuffer = SessionUser->writeBuffer;
		Unicast(writeBuffer, header, chpPayload, wPayloadSize);
	}
	else {
		//페이로드 생성
		serialBuffer << Result << SessionRoom->roomNumber << SessionRoom->roomNameSize;
		serialBuffer.WriteString(SessionRoom->roomName, SessionRoom->roomNameSize);
		//페이로드 사이즈
		wPayloadSize = serialBuffer.GetUseSize();
		//체크섬 값 취득
		byCheckSum = GetCheckSum(chpPayload, df_RES_ROOM_CREATE, wPayloadSize);
		//헤더 만들기
		CreateHeader(&header, dfPACKET_CODE, byCheckSum, df_RES_ROOM_CREATE, wPayloadSize);
		//브로드캐스트
		Broadcast(header, chpPayload, wPayloadSize);
	}
}

//대화방 입장 요청
void Request_RoomEnter(st_SESSION_USER * SessionUser, WORD wPayloadSize, UINT uiKey) {
	CRingBuffer * readBuffer = SessionUser->readBuffer;
	CSerializationBuffer serialBuffer;
	UINT uiRoomNumber;
	BYTE result = df_RESULT_ROOM_ENTER_OK;
	st_SESSION_ROOM * sessionRoom = NULL;

	//페이로드 읽기
	ReadPayload(&serialBuffer, readBuffer, wPayloadSize);
	serialBuffer >> &uiRoomNumber;
	
	//방 찾기
	if (FindRoom(uiRoomNumber)) {
		sessionRoom = &g_arraySessionRoom[uiRoomNumber];
		//대화방 참가인원 유저 추가
		if (!SetUserList(sessionRoom->userList, uiKey)) {
			result = df_RESULT_ROOM_ENTER_MAX;
		}
		else {
			//입장한 대화방 주소 저장
			SessionUser->sessionRoom = sessionRoom;
			//참가인원 수 증가
			++sessionRoom->totalUser;
		}
	}
	else {
		result = df_RESULT_ROOM_ENTER_NOT;
	}
	//대화방 입장 응답
	Response_RoomEnter(SessionUser, sessionRoom, uiKey, result);
}

//대화방 입장 응답
void Response_RoomEnter(st_SESSION_USER * SessionUser, st_SESSION_ROOM * SessionRoom, UINT uiKey, BYTE Result) {
	st_PACKET_HEADER header;
	WORD wPayloadSize;
	char *chpPayload;
	CRingBuffer * writeBuffer = SessionUser->writeBuffer;
	CSerializationBuffer serialBuffer;
	BYTE byCheckSum;

	chpPayload = serialBuffer.GetReadBufferPtr();

	if (Result != df_RESULT_ROOM_ENTER_OK) {
		//페이로드 생성
		serialBuffer << Result;
		//페이로드 사이즈
		wPayloadSize = serialBuffer.GetUseSize();
		//체크섬 값 취득
		byCheckSum = GetCheckSum(chpPayload, df_RES_ROOM_ENTER, wPayloadSize);
		//헤더 만들기
		CreateHeader(&header, dfPACKET_CODE, byCheckSum, df_RES_ROOM_ENTER, wPayloadSize);
	}
	else {
		//페이로드 생성
		serialBuffer << Result << SessionRoom->roomNumber << SessionRoom->roomNameSize;
		serialBuffer.WriteString(SessionRoom->roomName, SessionRoom->roomNameSize);
		//대화방 참가인원 취득
		GetUserList(&serialBuffer, SessionRoom);
		//페이로드 사이즈
		wPayloadSize = serialBuffer.GetUseSize();
		//체크섬 값 취득
		byCheckSum = GetCheckSum(chpPayload, df_RES_ROOM_ENTER, wPayloadSize);
		//헤더 만들기
		CreateHeader(&header, dfPACKET_CODE, byCheckSum, df_RES_ROOM_ENTER, wPayloadSize);
		//유저 입장 응답
		Response_UserEnter(SessionUser, SessionRoom->userList, uiKey);
	}
	//유니캐스트
	Unicast(writeBuffer, header, chpPayload, wPayloadSize);
}

//유저 입장 응답
void Response_UserEnter(st_SESSION_USER * SessionUser, UINT * UserList, UINT uiKey) {
	CSerializationBuffer serialBuffer;
	st_PACKET_HEADER header;
	BYTE byCheckSum;
	WORD wPayloadSize;
	char *chpPayload;

	chpPayload = serialBuffer.GetReadBufferPtr();
	//페이로드 생성
	serialBuffer << SessionUser->nickName << SessionUser->userNumber;
	//페이로드 사이즈
	wPayloadSize = serialBuffer.GetUseSize();
	//체크섬 값 취득
	byCheckSum = GetCheckSum(chpPayload, df_RES_USER_ENTER, wPayloadSize);
	//헤더 만들기
	CreateHeader(&header, dfPACKET_CODE, byCheckSum, df_RES_USER_ENTER, wPayloadSize);
	//대화방 브로드캐스트
	RoomBroadcast(UserList, header, chpPayload, wPayloadSize, uiKey);
}

//채팅 송신 요청
void Request_Chat(st_SESSION_USER * SessionUser, WORD wPayloadSize, UINT uiKey) {
	CRingBuffer * readBuffer = SessionUser->readBuffer;
	CSerializationBuffer serialBuffer;
	WORD wMessageSize;
	WCHAR * wchpMessage;

	//페이로드 읽기
	ReadPayload(&serialBuffer, readBuffer, wPayloadSize);
	serialBuffer >> &wMessageSize;
	wchpMessage = new WCHAR[wMessageSize / 2];
	serialBuffer >> wchpMessage;

	//채팅 송신 응답
	Response_Chat(SessionUser, wMessageSize, wchpMessage, uiKey);

	delete[] wchpMessage;
}

//채팅 송신 응답
void Response_Chat(st_SESSION_USER * SessionUser, WORD wMessageSize, WCHAR * wchpMessage, UINT uiKey) {
	CSerializationBuffer serialBuffer;
	st_PACKET_HEADER header;
	BYTE byCheckSum;
	WORD wPayloadSize;
	char *chpPayload;

	chpPayload = serialBuffer.GetReadBufferPtr();
	//페이로드 생성
	serialBuffer << SessionUser->userNumber << wMessageSize << wchpMessage;
	//페이로드 사이즈
	wPayloadSize = serialBuffer.GetUseSize();
	//체크섬 값 취득
	byCheckSum = GetCheckSum(chpPayload, df_RES_CHAT, wPayloadSize);
	//헤더 만들기
	CreateHeader(&header, dfPACKET_CODE, byCheckSum, df_RES_CHAT, wPayloadSize);
	//대화방 브로드캐스트
	RoomBroadcast(SessionUser->sessionRoom->userList, header, chpPayload, wPayloadSize, uiKey);
}

//대화방 퇴장 요청
void Request_RoomLeave(st_SESSION_USER * SessionUser) {
	st_SESSION_ROOM * sessionRoom = SessionUser->sessionRoom;

	//대화방 참가인원 수 감소
	--sessionRoom->totalUser;
	//대화방 퇴장 응답
	Response_RoomLeave(SessionUser, sessionRoom);
	//2021.01.22 17:57
	//대화방 유저 삭제
	EraseUserList(sessionRoom->userList, SessionUser->userNumber);
	//입장한 대화방 주소 초기화
	SessionUser->sessionRoom = NULL;
}

//대화방 퇴장 응답
void Response_RoomLeave(st_SESSION_USER * SessionUser, st_SESSION_ROOM * SessionRoom) {
	CSerializationBuffer serialBuffer;
	st_PACKET_HEADER header;
	BYTE byCheckSum;
	WORD wPayloadSize;
	char *chpPayload;

	chpPayload = serialBuffer.GetReadBufferPtr();
	//페이로드 생성
	serialBuffer << SessionUser->userNumber;
	//페이로드 사이즈
	wPayloadSize = serialBuffer.GetUseSize();
	//체크섬 값 취득
	byCheckSum = GetCheckSum(chpPayload, df_RES_ROOM_LEAVE, wPayloadSize);
	//헤더 만들기
	CreateHeader(&header, dfPACKET_CODE, byCheckSum, df_RES_ROOM_LEAVE, wPayloadSize);
	//대화방 브로드캐스트
	RoomBroadcast(SessionUser->sessionRoom->userList, header, chpPayload, wPayloadSize);

	//마지막 퇴장자
	if (SessionRoom->totalUser == 0) {
		//대화방 삭제
		Response_RoomDelete(SessionRoom->roomNumber);
	}
}

//대화방 삭제
void Response_RoomDelete(UINT RoomNumber) {
	CSerializationBuffer serialBuffer;
	st_PACKET_HEADER header;
	BYTE byCheckSum;
	WORD wPayloadSize;
	char *chpPayload;

	chpPayload = serialBuffer.GetReadBufferPtr();
	//페이로드 생성
	serialBuffer << RoomNumber;
	//페이로드 사이즈
	wPayloadSize = serialBuffer.GetUseSize();
	//체크섬 값 취득
	byCheckSum = GetCheckSum(chpPayload, df_RES_ROOM_DELETE, wPayloadSize);
	//헤더 만들기
	CreateHeader(&header, dfPACKET_CODE, byCheckSum, df_RES_ROOM_DELETE, wPayloadSize);
	//브로드캐스트
	Broadcast(header, chpPayload, wPayloadSize);

	//대화방 사용안함
	g_arraySessionRoom[RoomNumber].state = dfROOMSTATE_ZERO;
	delete[] g_arraySessionRoom[RoomNumber].userList;
}

//닉네임 중복 체크
bool CheckNickname(WCHAR *wchNickname) {
	map<UINT, st_SESSION_USER *>::iterator iter;
	map<UINT, st_SESSION_USER *>::iterator iter_end;

	iter = g_mapSessionUser.begin();
	iter_end = g_mapSessionUser.end();

	while (iter != iter_end) {
		if (!wcscmp(iter->second->nickName, wchNickname)) {
			//true
			return true;
		}
		//false
		++iter;
	}
	return false;
}

//페이로드 읽기
void ReadPayload(CSerializationBuffer *SerialBuffer, CRingBuffer *readBuffer, WORD wPayloadSize) {
	int iretval;
	char *chpRear;
	int iNotBrokenSize;
	int iReadSize;

	while (wPayloadSize != 0) {
		iReadSize = wPayloadSize;
		chpRear = SerialBuffer->GetWriteBufferPtr();
		iNotBrokenSize = readBuffer->GetNotBrokenGetSize();

		if (iReadSize > iNotBrokenSize) {
			iReadSize = iNotBrokenSize;
		}

		//페이로드 읽기
		iretval = readBuffer->Peek(chpRear, iReadSize);
		readBuffer->MoveFront(iretval);
		SerialBuffer->MoveRear(iretval);
		wPayloadSize = wPayloadSize - iretval;
	}
}

//대화방 중복 체크
bool CheckRoomName(WCHAR *wchpRoomName) {
	int icnt;

	for (icnt = 0; icnt < dfROOMMAX_SIZE; icnt++) {
		if (g_arraySessionRoom[icnt].state == dfROOMSTATE_ZERO) {
			continue;
		}
		//비교
		if (!wcscmp(g_arraySessionRoom[icnt].roomName, wchpRoomName)) {
			//true
			return true;
		}
	}
	return false;
}

//대화방 리스트 획득
void GetRoomList(CSerializationBuffer * SerialBuffer) {
	int icnt;
	WORD wRoomListSize = 0;
	st_SESSION_ROOM * sessionRoom;

	for (icnt = 0; icnt < dfROOMMAX_SIZE; icnt++) {
		if (g_arraySessionRoom[icnt].state != dfROOMSTATE_ZERO) {
			++wRoomListSize;
		}
	}

	//대화방 수
	*SerialBuffer << wRoomListSize;

	for (icnt = 0; icnt < dfROOMMAX_SIZE; icnt++) {
		if (g_arraySessionRoom[icnt].state != dfROOMSTATE_ZERO) {
			sessionRoom = &g_arraySessionRoom[icnt];
			//대화방 정보
			*SerialBuffer << sessionRoom->roomNumber << sessionRoom->roomNameSize;
			SerialBuffer->WriteString(sessionRoom->roomName, sessionRoom->roomNameSize);
			// 21.01.25 15:14
			// 로비에서 아래 패킷은 왜 필요한지 모르겠음 보류
			////대화방 참가인원 정보
			//GetUserList(SerialBuffer, sessionRoom, true);
		}
	}
}

//2021.01.22 17:57
//대화방 유저 삭제
void EraseUserList(UINT * userList, UINT uiKey)
{
	for (int icnt = 0; icnt < dfROOM_USERLISTMAX_SIZE; icnt++) {
		if (userList[icnt] == uiKey) {
			userList[icnt] = dfUSERLIST_INIT;
			return;
		}
	}
}

//대화방 참가인원 유저 추가
bool SetUserList(UINT *userList, UINT uiKey) {
	int icnt;

	for (icnt = 0; icnt < dfROOM_USERLISTMAX_SIZE; icnt++) {
		if (userList[icnt] == dfUSERLIST_INIT) {
			userList[icnt] = uiKey;
			return true;
		}
	}
	return false;
}

//대화방 참가인원 취득
void GetUserList(CSerializationBuffer * SerialBuffer, st_SESSION_ROOM * SessionRoom, bool Option) {
	int icnt;
	BYTE userListSize = 0;
	UINT * userList = SessionRoom->userList;

	//참가인원 수
	*SerialBuffer << SessionRoom->totalUser;

	for (icnt = 0; icnt < dfROOM_USERLISTMAX_SIZE; icnt++) {
		if (userList[icnt] != dfUSERLIST_INIT) {
			//유저 정보 취득
			GetUserList_UserInfo(SerialBuffer, userList[icnt], Option);
		}
	}
}

//유저 정보 취득
void GetUserList_UserInfo(CSerializationBuffer * SerialBuffer, UINT uiKey, bool Option) {
	map<UINT, st_SESSION_USER *>::iterator iter;
	map<UINT, st_SESSION_USER *>::iterator iter_end;
	st_SESSION_USER * sessionUser;

	iter = g_mapSessionUser.begin();
	iter_end = g_mapSessionUser.end();

	while (iter != iter_end) {
		if (iter->first == uiKey) {
			sessionUser = iter->second;
			*SerialBuffer << sessionUser->nickName;
			if (!Option) {
				*SerialBuffer << sessionUser->userNumber;
			}
			return;
		}
		++iter;
	}
}

//대화방 배열 빈 인덱스 찾기
bool FindEmptyRoom(UINT *getNumber) {
	UINT uicnt;

	for (uicnt = 0; uicnt < dfROOMMAX_SIZE; uicnt++) {
		if (g_arraySessionRoom[uicnt].state == dfROOMSTATE_ZERO) {
			*getNumber = uicnt;
			return true;
		}
	}
	return false;
}

//대화방 찾기
bool FindRoom(UINT uiRoomNumber) {
	UINT uicnt;

	for (uicnt = 0; uicnt < dfROOMMAX_SIZE; uicnt++) {
		if ((g_arraySessionRoom[uicnt].roomNumber & dfROOMSTATE_USE) == dfROOMSTATE_USE) {
			return true;
		}
	}
	return false;
}

//헤더 생성
void CreateHeader(st_PACKET_HEADER * Header, BYTE byCode, BYTE byCheckSum, WORD wMsgType, WORD wPayloadSize) {
	Header->byCode = byCode;
	Header->byCheckSum = byCheckSum;
	Header->wMsgType = wMsgType;
	Header->wPayloadSize = wPayloadSize;
}

//대화방 세션 생성
void CreateSessionRoom(st_SESSION_ROOM * SessionRoom, UINT uiArrayindex, WORD wRoomNameSize, WCHAR *wchpRoomName) {
	int icnt;
	UINT * arrayList;

	//대화방 생성 성공
	SessionRoom->state =dfROOMSTATE_USE;
	SessionRoom->roomNameSize = wRoomNameSize;
	SessionRoom->roomName = wchpRoomName;
	SessionRoom->roomNumber = uiArrayindex;
	SessionRoom->totalUser = 0;
	SessionRoom->userList = new UINT[dfROOM_USERLISTMAX_SIZE];

	arrayList = SessionRoom->userList;

	for (icnt = 0; icnt < dfROOM_USERLISTMAX_SIZE; icnt++) {
		arrayList[icnt] = dfUSERLIST_INIT;
	}
}

//유니캐스트
void Unicast(CRingBuffer * WriteBuffer, st_PACKET_HEADER Header, char * chpPayload, WORD wPayloadSize) {
	int iHeaderSize = sizeof(Header);
	WriteBuffer->Enqueue((char *)&Header, iHeaderSize);
	WriteBuffer->Enqueue(chpPayload, wPayloadSize);
}

//브로드캐스트
void Broadcast(st_PACKET_HEADER Header, char * chpPayload, WORD wPayloadSize, UINT uiKey) {
	map<UINT, st_SESSION_USER *>::iterator iter;
	map<UINT, st_SESSION_USER *>::iterator iter_end;

	iter = g_mapSessionUser.begin();
	iter_end = g_mapSessionUser.end();

	while (iter != iter_end) {
		if (iter->first == uiKey) {
			++iter;
			continue;
		}
		//유니캐스트
		Unicast(iter->second->writeBuffer, Header, chpPayload, wPayloadSize);
		++iter;
	}
}

//대화방 브로드캐스트
void RoomBroadcast(UINT * UserList, st_PACKET_HEADER Header, char * chpPayload, WORD wPayloadSize, UINT uiKey) {
	map<UINT, st_SESSION_USER *>::iterator iter;
	map<UINT, st_SESSION_USER *>::iterator iter_end;
	int icnt;
	UINT key;
	UINT iterKey;

	for (icnt = 0; icnt < dfROOM_USERLISTMAX_SIZE; icnt++) {
		key = UserList[icnt];
		if (UserList[icnt] == dfROOMSTATE_ZERO) {
			continue;
		}
		if (key == uiKey) {
			continue;
		}

		iter = g_mapSessionUser.begin();
		iter_end = g_mapSessionUser.end();

		while (iter != iter_end) {
			iterKey = iter->first;
			if (iterKey == key) {
				//유니캐스트
				Unicast(iter->second->writeBuffer, Header, chpPayload, wPayloadSize);
			}
			++iter;
		}
	}//for
}

//세션 삭제 검사
bool CheckSessionClosed(st_SESSION_USER * SessionUser) {
	if (SessionUser->state & dfUSERSTATE_CLOSED) {
		if (SessionUser->writeBuffer->GetUseSize() == 0) {
			//_LOG(df_LOG_LEVEL_DEBUG2, L"CheckSessionClosed(), session closed ID:%d", SessionUser->dwSessionID);
			return true;
		}
	}

	if (SessionUser->state & dfUSERSTATE_DELETE) {
		//_LOG(df_LOG_LEVEL_DEBUG2, L"CheckSessionClosed(), session delete ID:%d", SessionUser->dwSessionID);
		return true;
	}
	return false;
}

//세션 삭제
void RemoveSession(st_SESSION_USER * SessionUser) {
	closesocket(SessionUser->sock);
	delete SessionUser->readBuffer;
	delete SessionUser->writeBuffer;
	delete[] SessionUser->nickName;
	delete SessionUser;
}