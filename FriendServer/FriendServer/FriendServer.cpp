﻿// FriendServer.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <map>
#include <conio.h>

#include "Protocol.h"
#include "CRingBuffer.h"
#include "CSerializationBuffer.h"

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#pragma comment(lib, "ws2_32")

using namespace std;
using namespace rapidjson;

#ifndef	MAIN_DEFINE

#define df_FDSET_SIZE				60
#define df_LOGINMAX_SIZE			5000
#define df_ACCOUNT_ZERO				0
#define df_ACCOUNT_LOGIN			1

#define df_USERSTATE_ZERO			0
#define df_USERSTATE_LOGIN			1
#define df_USERSTATE_CLOSED			2
#define df_USERSTATE_DELETE			4

#define df_FILENAME					L"data\\DB_json.txt"

#endif

struct st_SESSION_USER {
	SOCKET sock;									// 소켓
	SOCKADDR_IN addr;								// 주소
	UINT64 AccountNo;								// 유저 번호
	WCHAR * NickName;								// 닉네임
	CRingBuffer * readBuffer;						// 리시브 버퍼
	CRingBuffer * writeBuffer;						// 샌드 버퍼
	BYTE state;										// 상태
};

struct st_Account {
	BYTE state;
	UINT64 AccountNo;
	WCHAR * NickName;
};

struct st_Friend {
	UINT64 FromAccountNo;
	UINT64 ToAccountNo;
};


bool Network_init();				//네트워크 초기화
bool NetworkService();
void AcceptProc();					//억셉트 처리
void CreateUser();					//유저 생성
void LoginProc();					//로그인 처리
void NetworkProc();					//네트워크 처리
void ReceiveProc(st_SESSION_USER * SessionUser, UINT64 uiKey);								//리시브 처리
void SandProc(st_SESSION_USER * SessionUser);												//샌드 처리

void Request_AccountAdd(st_SESSION_USER * SessionUser, WORD wPayloadSize);					//회원가입 요청
void Response_AccountAdd(st_SESSION_USER * SessionUser, UINT64 accountNo);					//회원가입 응답
void Request_Login(st_SESSION_USER * SessionUser, WORD wPayloadSize);						//로그인 요청
void Response_Login(st_SESSION_USER * SessionUser, st_Account * Account, UINT64 Result);	//로그인 응답
void Response_AccountList(st_SESSION_USER * SessionUser);									//회원 리스트 응답
void Response_FriendList(st_SESSION_USER * SessionUser);									//친구 리스트 응답
void Response_FriendRequestList(st_SESSION_USER * SessionUser);								//친구요청 보낸거 리스트 응답
void Response_FriendReplyList(st_SESSION_USER * SessionUser);								//친구요청 받은거 리스트 응답
void Request_FriendRemove(st_SESSION_USER * SessionUser, WORD wPayloadSize);				//친구관계 끊기 요청
void Response_FriendRemove(st_SESSION_USER * SessionUser, UINT64 FriendAccountNo, BYTE Result);		//친구관계 끊기 응답
void Request_FriendRequest(st_SESSION_USER * SessionUser, WORD wPayloadSize);						//친구요청 요청
void Response_FriendRequest(st_SESSION_USER * SessionUser, UINT64 FriendAccountNo, BYTE Result);	//친구요청 응답
void Request_FriendCancel(st_SESSION_USER * SessionUser, WORD wPayloadSize);						//친구요청 취소 요청
void Response_FriendCancel(st_SESSION_USER * SessionUser, UINT64 FriendAccountNo, BYTE Result);		//친구요청 취소 응답
void Request_FriendDeny(st_SESSION_USER * SessionUser, WORD wPayloadSize);							//친구요청 거부 요청
void Response_FriendDeny(st_SESSION_USER * SessionUser, UINT64 FriendAccountNo, BYTE Result);		//친구요청 거부 응답
void Request_FriendAgree(st_SESSION_USER * SessionUser, WORD wPayloadSize);							//친구요청 수락 요청
void Response_FriendAgree(st_SESSION_USER * SessionUser, UINT64 FriendAccountNo, BYTE Result);		//친구요청 수락 응답

void CreateHeader(st_PACKET_HEADER * Header, BYTE byCode, WORD wMsgType, WORD wPayloadSize);			//헤더 생성
void CreateAccount(st_Account * newAccount);															//Account 생성
void CreateAccount(UINT64 AcountNo, WCHAR * Nickname);													//Account 생성
void CreateFriend(st_Friend * Friend, UINT64 FromAccountNo, UINT64 ToAccountNo);						//친구 추가
void CreateFriend(UINT64 FromAccountNo, UINT64 ToAccountNo);											//친구 추가
void CreateFriendRequest(UINT64 FromAccountNo, UINT64 ToAccountNo);										//친구요청 추가

bool LoginService(st_Account ** Account, UINT64 AccountNo, UINT64 LoginAccountNo);						//로그인 처리
void FriendRequestService(UINT64 AccountNo, UINT64 FriendAccountNo, BYTE * Result);						//친구요청 처리
void FriendAgreeService(UINT64 AccountNo, UINT64 FriendAccountNo, BYTE * Result);						//친구요청 수락 처리

bool CheckFriend(UINT64 AccountNo, UINT64 FriendAccountNo);												//친구관계인지 확인
bool CheckFriendRequest(UINT64 AccountNo, UINT64 FriendAccountNo);										//친구 요청했는지 확인
bool FindAccount(UINT64 AccountNo);																		//번호로 찾기
bool GetAccount(st_Account ** Account, UINT64 AccountNo);												//번호로 값 취득
void GetFriend(UINT64 FriendKey, st_Account ** Account, bool Option);									//친구 찾기
void GetFriendRequest(UINT64 FriendKey, st_Account ** Account, bool Option);							//친구요청 찾기
void GetAccountList(CSerializationBuffer * SerialBuffer);												//회원 리스트 획득
void GetFriendList(CSerializationBuffer * SerialBuffer, UINT64 AccountNo);								//친구 리스트 획득
void GetFriendRequestList(CSerializationBuffer * SerialBuffer, UINT64 AccountNo);						//친구요청 보낸거 리스트 획득
void GetFriendReplyList(CSerializationBuffer * SerialBuffer, UINT64 AccountNo);							//친구요청 받은거 리스트 획득
bool RemoveFriend(UINT64 AccountNo, UINT64 FriendAccountNo);											//친구관계 끊기
void RemoveFromFriend(UINT64 AccountNo, UINT64 FriendKey);												//친구관계 끊기
void RemoveToFriend(UINT64 AccountNo, UINT64 FriendKey);												//친구관계 끊기
bool FriendRequestCancel(UINT64 AccountNo, UINT64 FriendAccountNo);										//친구요청 취소
void RemoveFromFriendRequest(UINT64 AccountNo, UINT64 FriendKey);										//친구요청 취소
void RemoveToFriendRequest(UINT64 AccountNo, UINT64 FriendKey);											//친구요청 취소

void ReadPayload(CSerializationBuffer *SerialBuffer, CRingBuffer *readBuffer, WORD wPayloadSize);		//페이로드 읽기
void Unicast(CRingBuffer * WriteBuffer, st_PACKET_HEADER Header, char * chpPayload, WORD wPayloadSize);	//유니캐스트
void Broadcast(st_PACKET_HEADER Header, char * chpPayload, WORD wPayloadSize, UINT uiKey = 0);			//브로드캐스트
bool CheckSessionClosed(st_SESSION_USER * SessionUser);													//세션 삭제 검사
void RemoveSession(st_SESSION_USER * SessionUser);														//세션 삭제
void Request_StressEcho(st_SESSION_USER * SessionUser, WORD wPayloadSize);								//스트레스 테스트용 에코 요청
void Response_StressEcho(st_SESSION_USER * SessionUser, WORD EchoSize, WCHAR * String);					//스트레스 테스트용 에코 응답
void PrintCount();

void ReadJson();
void WriteJson();
bool ReadFile(FILE **fp, WCHAR *txt);
bool WriteFile(FILE **fp, WCHAR *txt);
void UTF8toUTF16(const char *szText, WCHAR *szBuff, int iBuffLen);


typedef pair <UINT64, st_SESSION_USER *> SessionUser_Pair;
typedef pair <UINT64, st_Account *> Account_Pair;
typedef pair <UINT64, st_Friend *> Friend_Pair;
typedef pair <UINT64, UINT64> Search_Pair;


SOCKET g_listen_sock;											//리슨 소켓
map<UINT64, st_SESSION_USER *> g_mapLogin;						//억셉트
map<UINT64, st_SESSION_USER *> g_mapSessionUser;				//로그인
map<UINT64, st_Account *> g_mapAccount;							//회원 리스트
map<UINT64, st_Friend *> g_mapFriend;							//친구 리스트
map<UINT64, UINT64> g_mapSearchFromFriend;						//검색용 친구 리스트
map<UINT64, UINT64> g_mapSearchToFriend;						//검색용 친구 리스트
map<UINT64, st_Friend *> g_mapFriendRequest;					//친구 요청 리스트
map<UINT64, UINT64> g_mapSearchFromFriendRequest;				//검색용 친구 요청 리스트
map<UINT64, UINT64> g_mapSearchToFriendRequest;					//검색용 친구 요청 리스트

UINT64 g_ui64LoginKey = 0;
UINT64 g_ui64MapKey = 0;
UINT64 g_ui64AccountNo = 0;
UINT64 g_ui64FriendRequestKey = 0;
UINT64 g_ui64FriendKey = 0;

UINT64 g_ui64Time;
UINT64 g_ui64OldTime = 0;

bool g_printFlag = false;

int main()
{
	if (!Network_init()) {
		return 0;
	}

	ReadJson();

	//로직
	while (1) {
		NetworkService();
		PrintCount();
		if (_kbhit()) {
			switch (_getch()) {
			case 0x71:
				WriteJson();
				return 0;
			default:
				break;
			}
		}
		
	}

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
	static int icnt;
	icnt = 0;

	while (icnt < df_LOGINMAX_SIZE) {
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
	newSessionUser->AccountNo = df_ACCOUNT_ZERO;
	newSessionUser->NickName = new WCHAR[dfNICK_MAX_LEN];
	newSessionUser->readBuffer = new CRingBuffer;
	newSessionUser->writeBuffer = new CRingBuffer;
	newSessionUser->state = df_USERSTATE_LOGIN;

	++g_ui64LoginKey;
	g_mapLogin.insert(SessionUser_Pair(g_ui64LoginKey, newSessionUser));

	printf("Aceept %S::%d  [LoginKey:%d]\n",
		InetNtop(AF_INET, &newSessionUser->addr.sin_addr, szClientIP, 16),
		ntohs(newSessionUser->addr.sin_port),
		g_ui64LoginKey);

	return;
}

//로그인 처리
void LoginProc() {
	map<UINT64, st_SESSION_USER *>::iterator iter;
	map<UINT64, st_SESSION_USER *>::iterator iterTemp;
	map<UINT64, st_SESSION_USER *>::iterator iter_begin;
	map<UINT64, st_SESSION_USER *>::iterator iter_end;
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
			if (iFDcnt >= df_FDSET_SIZE) {
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
					if (sessionUser->state == df_USERSTATE_ZERO) {
						iter = g_mapLogin.erase(iter);
						++g_ui64MapKey;
						g_mapSessionUser.insert(SessionUser_Pair(g_ui64MapKey, sessionUser));
						sessionUser->state = df_USERSTATE_LOGIN;
						iterFlag = false;
					}
				}
				--iretcnt;
			}

			//세션 삭제 검사
			if (CheckSessionClosed(sessionUser)) {
				iter = g_mapLogin.erase(iter);
				//세션 삭제
				RemoveSession(sessionUser);
				iterFlag = false;
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

//네트워크 처리
void NetworkProc() {
	map<UINT64, st_SESSION_USER *>::iterator iter;
	map<UINT64, st_SESSION_USER *>::iterator iterTemp;
	map<UINT64, st_SESSION_USER *>::iterator iter_begin;
	map<UINT64, st_SESSION_USER *>::iterator iter_end;
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

	if (g_mapSessionUser.size() == 0) {
		return;
	}

	iter_begin = g_mapSessionUser.begin();
	iter_end = g_mapSessionUser.end();
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
			if (iFDcnt >= df_FDSET_SIZE) {
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
				}
				--iretcnt;
			}

			//세션 삭제 검사
			if (CheckSessionClosed(sessionUser)) {
				//2018.05.12
				if (sessionUser->AccountNo != df_ACCOUNT_ZERO) {
					map<UINT64, st_Account *>::iterator account_iter;
					account_iter = g_mapAccount.find(sessionUser->AccountNo);
					account_iter->second->state = df_ACCOUNT_ZERO;
				}
				iter = g_mapSessionUser.erase(iter);
				//세션 삭제
				RemoveSession(sessionUser);
				iterFlag = false;
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

//리시브 처리
void ReceiveProc(st_SESSION_USER * SessionUser, UINT64 uiKey) {
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
		iretval = GetLastError();
		if (iretval == WSAEWOULDBLOCK) {
			return;
		}
		
		printf("[%d] recv error\n", iretval);
		SessionUser->state = SessionUser->state | df_USERSTATE_CLOSED;
		return;
	}
	else if (iretval == 0) {
		SessionUser->state = SessionUser->state | df_USERSTATE_CLOSED;
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

		if (g_printFlag) {
			printf("PacketRecv [Key:%d] [Type:%d]\n",
				uiKey, header.wMsgType);
		}

		//패킷 타입
		switch (header.wMsgType) {
		case df_REQ_ACCOUNT_ADD:
			//회원가입 요청
			Request_AccountAdd(SessionUser, wPacketSize);
			break;
		case df_REQ_LOGIN:
			//로그인 요청
			Request_Login(SessionUser, wPacketSize);
			break;
		case df_REQ_ACCOUNT_LIST:
			//회원 리스트 응답
			Response_AccountList(SessionUser);
			break;
		case df_REQ_FRIEND_LIST:
			//친구 리스트 응답
			Response_FriendList(SessionUser);
			break;
		case df_REQ_FRIEND_REQUEST_LIST:
			//친구요청 보낸거 리스트 응답
			Response_FriendRequestList(SessionUser);
			break;
		case df_REQ_FRIEND_REPLY_LIST:
			//친구요청 받은거 리스트 응답
			Response_FriendReplyList(SessionUser);
			break;
		case df_REQ_FRIEND_REMOVE:
			//친구관계 끊기 요청
			Request_FriendRemove(SessionUser, wPacketSize);
			break;
		case df_REQ_FRIEND_REQUEST:
			//친구요청 요청
			Request_FriendRequest(SessionUser, wPacketSize);
			break;
		case df_REQ_FRIEND_CANCEL:
			//친구요청 취소 요청
			Request_FriendCancel(SessionUser, wPacketSize);
			break;
		case df_REQ_FRIEND_DENY:
			//친구요청 거부 요청
			Request_FriendDeny(SessionUser, wPacketSize);
			break;
		case df_REQ_FRIEND_AGREE:
			//친구요청 수락 요청
			Request_FriendAgree(SessionUser, wPacketSize);
			break;
		case df_REQ_STRESS_ECHO:
			//스트레스 테스트용 에코
			Request_StressEcho(SessionUser, wPacketSize);
			break;
		default:
			return;
		}
	}//while(1)
}

//샌드 처리
void SandProc(st_SESSION_USER * SessionUser) {
	int iretval;
	char *chpFront;
	int iWriteSize;
	int iNotBrokenSize;
	CRingBuffer * writeBuffer = SessionUser->writeBuffer;

	chpFront = writeBuffer->GetReadBufferPtr();
	iWriteSize = writeBuffer->GetUseSize();
	iNotBrokenSize = writeBuffer->GetNotBrokenGetSize();

	//링버퍼 끝 체크
	if (iWriteSize > iNotBrokenSize) {
		iWriteSize = iNotBrokenSize;
	}

	iretval = send(SessionUser->sock, chpFront, iWriteSize, 0);
	if (iretval == SOCKET_ERROR) {
		iretval = GetLastError();
		if (iretval == WSAEWOULDBLOCK) {
			return;
		}
		
		printf("[%d] send error\n", iretval);
		SessionUser->state = SessionUser->state | df_USERSTATE_DELETE;

		return;
	}

	writeBuffer->MoveFront(iretval);
}

//회원가입 요청
void Request_AccountAdd(st_SESSION_USER * SessionUser, WORD wPayloadSize) {
	CRingBuffer * readBuffer = SessionUser->readBuffer;
	CSerializationBuffer serialBuffer;
	st_Account * newAccount = new st_Account;
	UINT64 accountNo;

	//생성
	CreateAccount(newAccount);
	accountNo = newAccount->AccountNo;

	//페이로드 읽기
	ReadPayload(&serialBuffer, readBuffer, wPayloadSize);
	//serialBuffer >> newAccount->NickName;
	serialBuffer.ReadString(newAccount->NickName, dfNICK_MAX_LEN * 2);

	//print
	printf("Create Account [AccountNo:%d]\n", accountNo);

	//회원가입 응답
	Response_AccountAdd(SessionUser, accountNo);
}

//회원가입 응답
void Response_AccountAdd(st_SESSION_USER * SessionUser, UINT64 accountNo) {
	st_PACKET_HEADER header;
	WORD wPayloadSize;
	char *chpPayload;
	CRingBuffer * writeBuffer = SessionUser->writeBuffer;
	CSerializationBuffer serialBuffer;

	chpPayload = serialBuffer.GetReadBufferPtr();

	//페이로드 생성
	serialBuffer << accountNo;
	//페이로드 사이즈
	wPayloadSize = serialBuffer.GetUseSize();
	//헤더 만들기
	CreateHeader(&header, dfPACKET_CODE, df_RES_ACCOUNT_ADD, wPayloadSize);
	//유니캐스트
	Unicast(writeBuffer, header, chpPayload, wPayloadSize);
}

//로그인 요청
void Request_Login(st_SESSION_USER * SessionUser, WORD wPayloadSize) {
	CRingBuffer * readBuffer = SessionUser->readBuffer;
	CSerializationBuffer serialBuffer;
	UINT64 accountNo;								//acountNo
	st_Account * account = NULL;					//acount 구조체

	//페이로드 읽기
	ReadPayload(&serialBuffer, readBuffer, wPayloadSize);
	serialBuffer >> &accountNo;

	//로그인 처리
	if (LoginService(&account, SessionUser->AccountNo, accountNo)) {
		SessionUser->AccountNo = account->AccountNo;
		SessionUser->NickName = account->NickName;
		account->state = df_ACCOUNT_LOGIN;
		SessionUser->state = df_USERSTATE_ZERO;
	}
	else {
		//없음
		accountNo = df_ACCOUNT_ZERO;
	}

	//로그인 응답
	Response_Login(SessionUser, account, accountNo);
}

//로그인 응답
void Response_Login(st_SESSION_USER * SessionUser, st_Account * Account, UINT64 Result) {
	st_PACKET_HEADER header;
	WORD wPayloadSize;
	char *chpPayload;
	CRingBuffer * writeBuffer = SessionUser->writeBuffer;
	CSerializationBuffer serialBuffer;

	chpPayload = serialBuffer.GetReadBufferPtr();

	if (Result != df_ACCOUNT_ZERO) {
		//페이로드 생성
		serialBuffer << Account->AccountNo;// << Account->NickName;
		serialBuffer.WriteString(Account->NickName, dfNICK_MAX_LEN * 2);
		//페이로드 사이즈
		wPayloadSize = serialBuffer.GetUseSize();
	}
	else {
		//페이로드 생성
		serialBuffer << Result;
		//페이로드 사이즈
		wPayloadSize = serialBuffer.GetUseSize();
	}

	//헤더 만들기
	CreateHeader(&header, dfPACKET_CODE, df_RES_LOGIN, wPayloadSize);
	//유니캐스트
	Unicast(writeBuffer, header, chpPayload, wPayloadSize);
}

//회원 리스트 응답
void Response_AccountList(st_SESSION_USER * SessionUser) {
	st_PACKET_HEADER header;
	WORD wPayloadSize;
	char *chpPayload;
	CRingBuffer * writeBuffer = SessionUser->writeBuffer;
	CSerializationBuffer serialBuffer;

	chpPayload = serialBuffer.GetReadBufferPtr();

	//회원 리스트 획득
	GetAccountList(&serialBuffer);
	//페이로드 사이즈
	wPayloadSize = serialBuffer.GetUseSize();
	//헤더 만들기
	CreateHeader(&header, dfPACKET_CODE, df_RES_ACCOUNT_LIST, wPayloadSize);
	//유니캐스트
	Unicast(writeBuffer, header, chpPayload, wPayloadSize);
}

//친구 리스트 응답
void Response_FriendList(st_SESSION_USER * SessionUser) {
	st_PACKET_HEADER header;
	WORD wPayloadSize;
	char *chpPayload;
	CRingBuffer * writeBuffer = SessionUser->writeBuffer;
	CSerializationBuffer serialBuffer;

	chpPayload = serialBuffer.GetReadBufferPtr();

	//친구 리스트 획득
	GetFriendList(&serialBuffer, SessionUser->AccountNo);
	//페이로드 사이즈
	wPayloadSize = serialBuffer.GetUseSize();
	//헤더 만들기
	CreateHeader(&header, dfPACKET_CODE, df_RES_FRIEND_LIST, wPayloadSize);
	//유니캐스트
	Unicast(writeBuffer, header, chpPayload, wPayloadSize);
}

//친구요청 보낸거 리스트 응답
void Response_FriendRequestList(st_SESSION_USER * SessionUser) {
	st_PACKET_HEADER header;
	WORD wPayloadSize;
	char *chpPayload;
	CRingBuffer * writeBuffer = SessionUser->writeBuffer;
	CSerializationBuffer serialBuffer;

	chpPayload = serialBuffer.GetReadBufferPtr();

	//친구요청 보낸거 리스트 획득
	GetFriendRequestList(&serialBuffer, SessionUser->AccountNo);
	//페이로드 사이즈
	wPayloadSize = serialBuffer.GetUseSize();
	//헤더 만들기
	CreateHeader(&header, dfPACKET_CODE, df_RES_FRIEND_REQUEST_LIST, wPayloadSize);
	//유니캐스트
	Unicast(writeBuffer, header, chpPayload, wPayloadSize);
}

//친구요청 받은거 리스트 응답
void Response_FriendReplyList(st_SESSION_USER * SessionUser) {
	st_PACKET_HEADER header;
	WORD wPayloadSize;
	char *chpPayload;
	CRingBuffer * writeBuffer = SessionUser->writeBuffer;
	CSerializationBuffer serialBuffer;

	chpPayload = serialBuffer.GetReadBufferPtr();

	//친구요청 보낸거 리스트 획득
	GetFriendReplyList(&serialBuffer, SessionUser->AccountNo);
	//페이로드 사이즈
	wPayloadSize = serialBuffer.GetUseSize();
	//헤더 만들기
	CreateHeader(&header, dfPACKET_CODE, df_RES_FRIEND_REPLY_LIST, wPayloadSize);
	//유니캐스트
	Unicast(writeBuffer, header, chpPayload, wPayloadSize);
}

//친구관계 끊기 요청
void Request_FriendRemove(st_SESSION_USER * SessionUser, WORD wPayloadSize) {
	CRingBuffer * readBuffer = SessionUser->readBuffer;
	CSerializationBuffer serialBuffer;
	UINT64	friendAccountNo;
	BYTE result = df_RESULT_FRIEND_REMOVE_OK;

	//페이로드 읽기
	ReadPayload(&serialBuffer, readBuffer, wPayloadSize);
	serialBuffer >> &friendAccountNo;

	//친구관계 끊기
	if (!RemoveFriend(SessionUser->AccountNo, friendAccountNo)) {
		result = df_RESULT_FRIEND_REMOVE_NOTFRIEND;
	}
	//친구관계 끊기 응답
	Response_FriendRemove(SessionUser, friendAccountNo, result);
}

//친구관계 끊기 응답
void Response_FriendRemove(st_SESSION_USER * SessionUser, UINT64 FriendAccountNo, BYTE Result) {
	st_PACKET_HEADER header;
	WORD wPayloadSize;
	char *chpPayload;
	CRingBuffer * writeBuffer = SessionUser->writeBuffer;
	CSerializationBuffer serialBuffer;

	chpPayload = serialBuffer.GetReadBufferPtr();

	//페이로드 생성
	serialBuffer << FriendAccountNo << Result;
	//페이로드 사이즈
	wPayloadSize = serialBuffer.GetUseSize();
	//헤더 만들기
	CreateHeader(&header, dfPACKET_CODE, df_RES_FRIEND_REMOVE, wPayloadSize);
	//유니캐스트
	Unicast(writeBuffer, header, chpPayload, wPayloadSize);
}

//친구요청 요청
void Request_FriendRequest(st_SESSION_USER * SessionUser, WORD wPayloadSize) {
	CRingBuffer * readBuffer = SessionUser->readBuffer;
	CSerializationBuffer serialBuffer;
	UINT64	friendAccountNo;
	BYTE result = df_RESULT_FRIEND_REQUEST_OK;

	//페이로드 읽기
	ReadPayload(&serialBuffer, readBuffer, wPayloadSize);
	serialBuffer >> &friendAccountNo;

	//친구요청 처리
	FriendRequestService(SessionUser->AccountNo, friendAccountNo, &result);

	//친구요청 응답
	Response_FriendRequest(SessionUser, friendAccountNo, result);
}

//친구요청 응답
void Response_FriendRequest(st_SESSION_USER * SessionUser, UINT64 FriendAccountNo, BYTE Result) {
	st_PACKET_HEADER header;
	WORD wPayloadSize;
	char *chpPayload;
	CRingBuffer * writeBuffer = SessionUser->writeBuffer;
	CSerializationBuffer serialBuffer;

	chpPayload = serialBuffer.GetReadBufferPtr();

	//페이로드 생성
	serialBuffer << FriendAccountNo << Result;
	//페이로드 사이즈
	wPayloadSize = serialBuffer.GetUseSize();
	//헤더 만들기
	CreateHeader(&header, dfPACKET_CODE, df_RES_FRIEND_REQUEST, wPayloadSize);
	//유니캐스트
	Unicast(writeBuffer, header, chpPayload, wPayloadSize);
}

//친구요청 취소 요청
void Request_FriendCancel(st_SESSION_USER * SessionUser, WORD wPayloadSize) {
	CRingBuffer * readBuffer = SessionUser->readBuffer;
	CSerializationBuffer serialBuffer;
	UINT64	friendAccountNo;
	BYTE result = df_RESULT_FRIEND_CANCEL_OK;

	//페이로드 읽기
	ReadPayload(&serialBuffer, readBuffer, wPayloadSize);
	serialBuffer >> &friendAccountNo;

	//친구요청 취소
	if (!FriendRequestCancel(SessionUser->AccountNo, friendAccountNo)) {
		result = df_RESULT_FRIEND_CANCEL_NOTFRIEND;
	}

	//친구요청 취소 응답
	Response_FriendCancel(SessionUser, friendAccountNo, result);
}

//친구요청 취소 응답
void Response_FriendCancel(st_SESSION_USER * SessionUser, UINT64 FriendAccountNo, BYTE Result) {
	st_PACKET_HEADER header;
	WORD wPayloadSize;
	char *chpPayload;
	CRingBuffer * writeBuffer = SessionUser->writeBuffer;
	CSerializationBuffer serialBuffer;

	chpPayload = serialBuffer.GetReadBufferPtr();

	//페이로드 생성
	serialBuffer << FriendAccountNo << Result;
	//페이로드 사이즈
	wPayloadSize = serialBuffer.GetUseSize();
	//헤더 만들기
	CreateHeader(&header, dfPACKET_CODE, df_RES_FRIEND_CANCEL, wPayloadSize);
	//유니캐스트
	Unicast(writeBuffer, header, chpPayload, wPayloadSize);
}

//친구요청 거부 요청
void Request_FriendDeny(st_SESSION_USER * SessionUser, WORD wPayloadSize) {
	CRingBuffer * readBuffer = SessionUser->readBuffer;
	CSerializationBuffer serialBuffer;
	UINT64	friendAccountNo;
	BYTE result = df_RESULT_FRIEND_DENY_OK;

	//페이로드 읽기
	ReadPayload(&serialBuffer, readBuffer, wPayloadSize);
	serialBuffer >> &friendAccountNo;

	//친구요청 거부
	if (!FriendRequestCancel(friendAccountNo, SessionUser->AccountNo)) {
		result = df_RESULT_FRIEND_DENY_NOTFRIEND;
	}

	//친구요청 거부 응답
	Response_FriendDeny(SessionUser, friendAccountNo, result);
}

//친구요청 거부 응답
void Response_FriendDeny(st_SESSION_USER * SessionUser, UINT64 FriendAccountNo, BYTE Result) {
	st_PACKET_HEADER header;
	WORD wPayloadSize;
	char *chpPayload;
	CRingBuffer * writeBuffer = SessionUser->writeBuffer;
	CSerializationBuffer serialBuffer;

	chpPayload = serialBuffer.GetReadBufferPtr();

	//페이로드 생성
	serialBuffer << FriendAccountNo << Result;
	//페이로드 사이즈
	wPayloadSize = serialBuffer.GetUseSize();
	//헤더 만들기
	CreateHeader(&header, dfPACKET_CODE, df_RES_FRIEND_DENY, wPayloadSize);
	//유니캐스트
	Unicast(writeBuffer, header, chpPayload, wPayloadSize);
}

//친구요청 수락 요청
void Request_FriendAgree(st_SESSION_USER * SessionUser, WORD wPayloadSize) {
	CRingBuffer * readBuffer = SessionUser->readBuffer;
	CSerializationBuffer serialBuffer;
	UINT64	friendAccountNo;
	BYTE result = df_RESULT_FRIEND_AGREE_OK;

	//페이로드 읽기
	ReadPayload(&serialBuffer, readBuffer, wPayloadSize);
	serialBuffer >> &friendAccountNo;

	//친구요청 수락 처리
	FriendAgreeService(SessionUser->AccountNo, friendAccountNo, &result);

	//친구요청 수락 응답
	Response_FriendAgree(SessionUser, friendAccountNo, result);
}

//친구요청 수락 응답
void Response_FriendAgree(st_SESSION_USER * SessionUser, UINT64 FriendAccountNo, BYTE Result) {
	st_PACKET_HEADER header;
	WORD wPayloadSize;
	char *chpPayload;
	CRingBuffer * writeBuffer = SessionUser->writeBuffer;
	CSerializationBuffer serialBuffer;

	chpPayload = serialBuffer.GetReadBufferPtr();

	//페이로드 생성
	serialBuffer << FriendAccountNo << Result;
	//페이로드 사이즈
	wPayloadSize = serialBuffer.GetUseSize();
	//헤더 만들기
	CreateHeader(&header, dfPACKET_CODE, df_RES_FRIEND_AGREE, wPayloadSize);
	//유니캐스트
	Unicast(writeBuffer, header, chpPayload, wPayloadSize);
}










//헤더 생성
void CreateHeader(st_PACKET_HEADER * Header, BYTE byCode, WORD wMsgType, WORD wPayloadSize) {
	Header->byCode = byCode;
	Header->wMsgType = wMsgType;
	Header->wPayloadSize = wPayloadSize;
}

//Account 생성
void CreateAccount(st_Account * newAccount) {
	newAccount->state = df_ACCOUNT_ZERO;
	newAccount->AccountNo = ++g_ui64AccountNo;
	newAccount->NickName = new WCHAR[dfNICK_MAX_LEN];

	g_mapAccount.insert(Account_Pair(g_ui64AccountNo, newAccount));
}

//Account 생성
void CreateAccount(UINT64 AcountNo, WCHAR * Nickname) {
	st_Account * newAccount = new st_Account;
	int len;

	newAccount->state = df_ACCOUNT_ZERO;
	newAccount->AccountNo = AcountNo;
	newAccount->NickName = new WCHAR[dfNICK_MAX_LEN];
	len = _msize(Nickname);
	memcpy_s(newAccount->NickName, len, Nickname, len);

	g_mapAccount.insert(Account_Pair(AcountNo, newAccount));
	g_ui64AccountNo = AcountNo;
}

//친구 추가
void CreateFriend(st_Friend * Friend, UINT64 FromAccountNo, UINT64 ToAccountNo) {
	++g_ui64FriendKey;

	g_mapFriend.insert(Friend_Pair(g_ui64FriendKey, Friend));
	g_mapSearchFromFriend.insert(Search_Pair(FromAccountNo, g_ui64FriendKey));
	g_mapSearchToFriend.insert(Search_Pair(ToAccountNo, g_ui64FriendKey));
}

//친구 추가
void CreateFriend(UINT64 FromAccountNo, UINT64 ToAccountNo) {
	st_Friend * newFriend = new st_Friend;

	newFriend->FromAccountNo = FromAccountNo;
	newFriend->ToAccountNo = ToAccountNo;

	++g_ui64FriendKey;

	g_mapFriend.insert(Friend_Pair(g_ui64FriendKey, newFriend));
	g_mapSearchFromFriend.insert(Search_Pair(FromAccountNo, g_ui64FriendKey));
	g_mapSearchToFriend.insert(Search_Pair(ToAccountNo, g_ui64FriendKey));
}

//친구요청 추가
void CreateFriendRequest(UINT64 FromAccountNo, UINT64 ToAccountNo) {
	st_Friend * newFriendRequest = new st_Friend;

	newFriendRequest->FromAccountNo = FromAccountNo;
	newFriendRequest->ToAccountNo = ToAccountNo;

	++g_ui64FriendRequestKey;

	g_mapFriendRequest.insert(Friend_Pair(g_ui64FriendRequestKey, newFriendRequest));
	g_mapSearchFromFriendRequest.insert(Search_Pair(FromAccountNo, g_ui64FriendRequestKey));
	g_mapSearchToFriendRequest.insert(Search_Pair(ToAccountNo, g_ui64FriendRequestKey));
}

//로그인 처리
bool LoginService(st_Account ** Account, UINT64 AccountNo, UINT64 LoginAccountNo) {
	map<UINT64, st_Account *>::iterator iter;
	map<UINT64, st_Account *>::iterator iter_end;
	st_Account * beforeAccount = NULL;
	bool flag = false;
	
	//같은 번호인지
	if (AccountNo == LoginAccountNo) {
		return false;
	}

	iter = g_mapAccount.begin();
	iter_end = g_mapAccount.end();

	if (AccountNo != df_ACCOUNT_ZERO) {
		while (iter != iter_end) {
			//찾음
			if (iter->first == AccountNo) {
				flag = true;
				beforeAccount = iter->second;
				break;
			}
			++iter;
		}
	}

	iter = g_mapAccount.begin();

	while (iter != iter_end) {
		//찾음
		if (iter->first == LoginAccountNo) {
			//상태확인
			if (iter->second->state == df_ACCOUNT_LOGIN) {
				//누가 사용중임
				return false;
			}
			else {
				//사용 가능
				*Account = iter->second;
				if (flag) {
					beforeAccount->state = df_ACCOUNT_ZERO;
				}
				return true;
			}	
		}
		++iter;
	}
	//못찾음
	return false;
}

//친구요청 처리
void FriendRequestService(UINT64 AccountNo, UINT64 FriendAccountNo, BYTE * Result) {
	//로그인을 했는가?
	if (AccountNo == df_ACCOUNT_ZERO) {
		*Result = df_RESULT_FRIEND_REQUEST_NOTFOUND;
		return;
	}
	//혹시 자기자신 인가?
	else if (AccountNo == FriendAccountNo) {
		*Result = df_RESULT_FRIEND_REQUEST_NOTFOUND;
		return;
	}
	//친구가 존재하는 번호인가?
	else if (!FindAccount(FriendAccountNo)) {
		*Result = df_RESULT_FRIEND_REQUEST_NOTFOUND;
		return;
	}
	//이미 친구인가?
	else if (CheckFriend(AccountNo, FriendAccountNo)) {
		*Result = df_RESULT_FRIEND_REQUEST_ALREADY;
		return;
	}
	//이미 친구요청 했는가?
	else if (CheckFriendRequest(AccountNo, FriendAccountNo)) {
		*Result = df_RESULT_FRIEND_REQUEST_ALREADY;
		return;
	}

	//친구요청 추가
	CreateFriendRequest(AccountNo, FriendAccountNo);
}

//친구요청 수락 처리
void FriendAgreeService(UINT64 AccountNo, UINT64 FriendAccountNo, BYTE * Result) {
	map<UINT64, st_Friend *>::iterator iter;
	map<UINT64, st_Friend *>::iterator iter_end;
	st_Friend * temp;
	UINT64 friendKey;

	iter = g_mapFriendRequest.begin();
	iter_end = g_mapFriendRequest.end();

	while (iter != iter_end) {
		temp = iter->second;
		if (temp->FromAccountNo == FriendAccountNo) {
			if (temp->ToAccountNo == AccountNo) {
				friendKey = iter->first;
				RemoveFromFriendRequest(FriendAccountNo, friendKey);
				RemoveToFriendRequest(AccountNo, friendKey);
				g_mapFriendRequest.erase(iter);
				//친구 추가
				CreateFriend(temp, FriendAccountNo, AccountNo);
				return;
			}
		}
		++iter;
	}

	*Result = df_RESULT_FRIEND_AGREE_NOTFRIEND;
}

//친구관계인지 확인
bool CheckFriend(UINT64 AccountNo, UINT64 FriendAccountNo) {
	map<UINT64, st_Friend *>::iterator iter;
	map<UINT64, st_Friend *>::iterator iter_end;
	st_Friend * temp;

	iter = g_mapFriend.begin();
	iter_end = g_mapFriend.end();

	while (iter != iter_end) {
		temp = iter->second;
		if (temp->FromAccountNo == AccountNo) {
			if (temp->ToAccountNo == FriendAccountNo) {
				return true;
			}
		}
		else if (temp->ToAccountNo == AccountNo) {
			if (temp->FromAccountNo == FriendAccountNo) {
				return true;
			}
		}
		++iter;
	}
	return false;
}

//친구 요청했는지 확인
bool CheckFriendRequest(UINT64 AccountNo, UINT64 FriendAccountNo) {
	map<UINT64, st_Friend *>::iterator iter;
	map<UINT64, st_Friend *>::iterator iter_end;
	st_Friend * temp;

	//친구요청 확인
	iter = g_mapFriendRequest.begin();
	iter_end = g_mapFriendRequest.end();

	while (iter != iter_end) {
		temp = iter->second;
		if (temp->FromAccountNo == AccountNo) {
			if (temp->ToAccountNo == FriendAccountNo) {
				return true;
			}
		}
		else if (temp->ToAccountNo == AccountNo) {
			if (temp->FromAccountNo == FriendAccountNo) {
				return true;
			}
		}
		++iter;
	}
	return false;
}

//번호로 찾기
bool FindAccount(UINT64 AccountNo) {
	map<UINT64, st_Account *>::iterator iter;
	map<UINT64, st_Account *>::iterator iter_end;

	iter = g_mapAccount.begin();
	iter_end = g_mapAccount.end();

	while (iter != iter_end) {
		//찾음
		if (iter->first == AccountNo) {
			return true;
		}
		++iter;
	}
	//못찾음
	return false;
}

//번호로 값 취득
bool GetAccount(st_Account ** Account, UINT64 AccountNo) {
	map<UINT64, st_Account *>::iterator iter;
	map<UINT64, st_Account *>::iterator iter_end;

	iter = g_mapAccount.begin();
	iter_end = g_mapAccount.end();

	while (iter != iter_end) {
		//찾음
		if (iter->first == AccountNo) {
			*Account = iter->second;
			return true;
		}
		++iter;
	}
	//못찾음
	return false;
}

//친구 찾기
void GetFriend(UINT64 FriendKey, st_Account ** Account, bool Option) {
	map<UINT64, st_Friend *>::iterator iter;
	map<UINT64, st_Friend *>::iterator iter_end;
	UINT64 accountNo;

	iter = g_mapFriend.begin();
	iter_end = g_mapFriend.end();

	while (iter != iter_end) {
		if (iter->first == FriendKey) {
			if (Option) {
				accountNo = iter->second->ToAccountNo;
				GetAccount(Account, accountNo);
			}
			else {
				accountNo = iter->second->FromAccountNo;
				GetAccount(Account, accountNo);
			}
		}
		++iter;
	}
}

//친구요청 찾기
void GetFriendRequest(UINT64 FriendKey, st_Account ** Account, bool Option) {
	map<UINT64, st_Friend *>::iterator iter;
	map<UINT64, st_Friend *>::iterator iter_end;
	UINT64 accountNo;

	iter = g_mapFriendRequest.begin();
	iter_end = g_mapFriendRequest.end();

	while (iter != iter_end) {
		if (iter->first == FriendKey) {
			if (Option) {
				accountNo = iter->second->ToAccountNo;
				GetAccount(Account, accountNo);
				return;
			}
			else {
				accountNo = iter->second->FromAccountNo;
				GetAccount(Account, accountNo);
				return;
			}
		}
		++iter;
	}
}

//회원 리스트 획득
void GetAccountList(CSerializationBuffer * SerialBuffer) {
	map<UINT64, st_Account *>::iterator iter;
	map<UINT64, st_Account *>::iterator iter_end;
	st_Account * account;
	UINT count;

	iter = g_mapAccount.begin();
	iter_end = g_mapAccount.end();
	
	count = g_mapAccount.size();

	*SerialBuffer << count;

	while (iter != iter_end) {
		account = iter->second;
		*SerialBuffer << account->AccountNo;// << account->NickName;
		SerialBuffer->WriteString(account->NickName, dfNICK_MAX_LEN * 2);
		++iter;
	}
}

//친구 리스트 획득
void GetFriendList(CSerializationBuffer * SerialBuffer, UINT64 AccountNo) {
	map<UINT64, UINT64>::iterator iter;
	map<UINT64, UINT64>::iterator iter_end;
	UINT64 friendKey;
	st_Account * account = NULL;
	UINT friendCount = 0;
	int move = sizeof(UINT);
	char * chpBuffer;

	iter = g_mapSearchFromFriend.begin();
	iter_end = g_mapSearchFromFriend.end();

	chpBuffer = SerialBuffer->GetReadBufferPtr();

	SerialBuffer->MoveRear(move);
	SerialBuffer->MoveFront(move);

	while (iter != iter_end) {
		if (iter->first == AccountNo) {
			friendKey = iter->second;
			GetFriend(friendKey, &account, true);
			*SerialBuffer << account->AccountNo << account->NickName;
			++friendCount;
		}
		++iter;
	}

	iter = g_mapSearchToFriend.begin();
	iter_end = g_mapSearchToFriend.end();
	while (iter != iter_end) {
		if (iter->first == AccountNo) {
			friendKey = iter->second;
			GetFriend(friendKey, &account, false);
			*SerialBuffer << account->AccountNo << account->NickName;
			++friendCount;
		}
		++iter;
	}
	
	memcpy_s(chpBuffer, move, &friendCount, move);
	//프론트 원위치
	SerialBuffer->MoveFront(-move);
}

//친구요청 보낸거 리스트 획득
void GetFriendRequestList(CSerializationBuffer * SerialBuffer, UINT64 AccountNo) {
	map<UINT64, UINT64>::iterator iter;
	map<UINT64, UINT64>::iterator iter_end;
	st_Account * account = NULL;
	UINT friendCount = 0;
	int move = sizeof(UINT);
	char * chpBuffer;
	UINT64 friendKey;

	iter = g_mapSearchFromFriendRequest.begin();
	iter_end = g_mapSearchFromFriendRequest.end();

	chpBuffer = SerialBuffer->GetReadBufferPtr();

	SerialBuffer->MoveRear(move);
	SerialBuffer->MoveFront(move);

	while (iter != iter_end) {
		if (iter->first == AccountNo) {
			friendKey = iter->second;
			GetFriendRequest(friendKey, &account, true);
			*SerialBuffer << account->AccountNo << account->NickName;
			++friendCount;
		}
		++iter;
	}

	memcpy_s(chpBuffer, move, &friendCount, move);
	//프론트 원위치
	SerialBuffer->MoveFront(-move);
}

//친구요청 받은거 리스트 획득
void GetFriendReplyList(CSerializationBuffer * SerialBuffer, UINT64 AccountNo) {
	map<UINT64, UINT64>::iterator iter;
	map<UINT64, UINT64>::iterator iter_end;
	st_Account * account = NULL;
	UINT friendCount = 0;
	int move = sizeof(UINT);
	char * chpBuffer;
	UINT64 friendKey;

	iter = g_mapSearchToFriendRequest.begin();
	iter_end = g_mapSearchToFriendRequest.end();

	chpBuffer = SerialBuffer->GetReadBufferPtr();

	SerialBuffer->MoveRear(move);
	SerialBuffer->MoveFront(move);

	while (iter != iter_end) {
		if (iter->first == AccountNo) {
			friendKey = iter->second;
			GetFriendRequest(friendKey, &account, false);
			*SerialBuffer << account->AccountNo << account->NickName;
			++friendCount;
		}
		++iter;
	}

	memcpy_s(chpBuffer, move, &friendCount, move);
	//프론트 원위치
	SerialBuffer->MoveFront(-move);
}

//친구관계 끊기
bool RemoveFriend(UINT64 AccountNo, UINT64 FriendAccountNo) {
	map<UINT64, st_Friend *>::iterator iter;
	map<UINT64, st_Friend *>::iterator iter_end;
	st_Friend * temp;
	UINT64 friendKey;

	iter = g_mapFriend.begin();
	iter_end = g_mapFriend.end();

	while (iter != iter_end) {
		temp = iter->second;
		if (temp->FromAccountNo == AccountNo) {
			if (temp->ToAccountNo == FriendAccountNo) {
				friendKey = iter->first;
				RemoveFromFriend(AccountNo, friendKey);
				RemoveToFriend(FriendAccountNo, friendKey);
				g_mapFriend.erase(iter);
				delete temp;
				return true;
			}
		}
		else if (temp->ToAccountNo == AccountNo) {
			if (temp->FromAccountNo == FriendAccountNo) {
				friendKey = iter->first;
				RemoveFromFriend(FriendAccountNo, friendKey);
				RemoveToFriend(AccountNo, friendKey);
				g_mapFriend.erase(iter);
				delete temp;
				return true;
			}
		}
		++iter;
	}
	return false;
}

//친구관계 끊기
void RemoveFromFriend(UINT64 AccountNo, UINT64 FriendKey) {
	map<UINT64, UINT64>::iterator iter;
	map<UINT64, UINT64>::iterator iter_end;

	iter = g_mapSearchFromFriend.begin();
	iter_end = g_mapSearchFromFriend.end();

	while (iter != iter_end) {
		if (iter->first == AccountNo) {
			if (iter->second == FriendKey) {
				g_mapSearchFromFriend.erase(iter);
				return;
			}
		}
		++iter;
	}
}

//친구관계 끊기
void RemoveToFriend(UINT64 AccountNo, UINT64 FriendKey) {
	map<UINT64, UINT64>::iterator iter;
	map<UINT64, UINT64>::iterator iter_end;

	iter = g_mapSearchToFriend.begin();
	iter_end = g_mapSearchToFriend.end();

	while (iter != iter_end) {
		if (iter->first == AccountNo) {
			if (iter->second == FriendKey) {
				g_mapSearchToFriend.erase(iter);
				return;
			}
		}
		++iter;
	}
}

//친구요청 취소
bool FriendRequestCancel(UINT64 AccountNo, UINT64 FriendAccountNo) {
	map<UINT64, st_Friend *>::iterator iter;
	map<UINT64, st_Friend *>::iterator iter_end;
	st_Friend * temp;
	UINT64 friendKey;

	iter = g_mapFriendRequest.begin();
	iter_end = g_mapFriendRequest.end();

	while (iter != iter_end) {
		temp = iter->second;
		if (temp->FromAccountNo == AccountNo) {
			if (temp->ToAccountNo == FriendAccountNo) {
				friendKey = iter->first;
				RemoveFromFriendRequest(AccountNo, friendKey);
				RemoveToFriendRequest(FriendAccountNo, friendKey);
				g_mapFriendRequest.erase(iter);
				delete temp;
				return true;
			}
		}
		++iter;
	}
	return false;
}

//친구요청 취소
void RemoveFromFriendRequest(UINT64 AccountNo, UINT64 FriendKey) {
	map<UINT64, UINT64>::iterator iter;
	map<UINT64, UINT64>::iterator iter_end;

	iter = g_mapSearchFromFriendRequest.begin();
	iter_end = g_mapSearchFromFriendRequest.end();

	while (iter != iter_end) {
		if (iter->first == AccountNo) {
			if (iter->second == FriendKey) {
				g_mapSearchFromFriendRequest.erase(iter);
				return;
			}
		}
		++iter;
	}
}

//친구요청 취소
void RemoveToFriendRequest(UINT64 AccountNo, UINT64 FriendKey) {
	map<UINT64, UINT64>::iterator iter;
	map<UINT64, UINT64>::iterator iter_end;

	iter = g_mapSearchToFriendRequest.begin();
	iter_end = g_mapSearchToFriendRequest.end();

	while (iter != iter_end) {
		if (iter->first == AccountNo) {
			if (iter->second == FriendKey) {
				g_mapSearchToFriendRequest.erase(iter);
				return;
			}
		}
		++iter;
	}
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

//유니캐스트
void Unicast(CRingBuffer * WriteBuffer, st_PACKET_HEADER Header, char * chpPayload, WORD wPayloadSize) {
	int iHeaderSize = sizeof(Header);
	WriteBuffer->Enqueue((char *)&Header, iHeaderSize);
	WriteBuffer->Enqueue(chpPayload, wPayloadSize);
}

//브로드캐스트
void Broadcast(st_PACKET_HEADER Header, char * chpPayload, WORD wPayloadSize, UINT uiKey) {
	map<UINT64, st_SESSION_USER *>::iterator iter;
	map<UINT64, st_SESSION_USER *>::iterator iter_end;

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

//세션 삭제 검사
bool CheckSessionClosed(st_SESSION_USER * SessionUser) {
	if ((SessionUser->state & df_USERSTATE_CLOSED) == df_USERSTATE_CLOSED) {
		if (SessionUser->writeBuffer->GetUseSize() == 0) {
			return true;
		}
	}
	
	if ((SessionUser->state & df_USERSTATE_DELETE) == df_USERSTATE_DELETE) {
		return true;
	}
	return false;
}

void RemoveSession(st_SESSION_USER * SessionUser) {
	WCHAR szClientIP[16];

	closesocket(SessionUser->sock);
	printf("Disconnect %S::%d  [AccountNo:%d]\n",
		InetNtop(AF_INET, &SessionUser->addr.sin_addr, szClientIP, 16),
		ntohs(SessionUser->addr.sin_port),
		SessionUser->AccountNo);
	delete SessionUser->readBuffer;
	delete SessionUser->writeBuffer;
	//2018.05.14
	//이거 삭제하면 다음에 로그인할때 이 회원 닉네임 날라감
	//delete[] SessionUser->NickName;
	delete SessionUser;
}





//스트레스 테스트용 에코 요청
void Request_StressEcho(st_SESSION_USER * SessionUser, WORD wPayloadSize) {
	CRingBuffer * readBuffer = SessionUser->readBuffer;
	CSerializationBuffer serialBuffer;
	WORD echoSize;
	WORD size;
	WCHAR * string;

	//페이로드 읽기
	ReadPayload(&serialBuffer, readBuffer, wPayloadSize);
	serialBuffer >> &echoSize;

	string = (WCHAR *)new char[echoSize];
	serialBuffer >> string;

	//스트레스 테스트용 에코 응답
	Response_StressEcho(SessionUser, echoSize, string);
	
	delete[] string;
}

//스트레스 테스트용 에코 응답
void Response_StressEcho(st_SESSION_USER * SessionUser, WORD EchoSize, WCHAR * String) {
	st_PACKET_HEADER header;
	WORD wPayloadSize;
	char *chpPayload;
	CRingBuffer * writeBuffer = SessionUser->writeBuffer;
	CSerializationBuffer serialBuffer;

	chpPayload = serialBuffer.GetReadBufferPtr();

	//페이로드 생성
	serialBuffer << EchoSize << String;
	//페이로드 사이즈
	wPayloadSize = serialBuffer.GetUseSize();
	//헤더 만들기
	CreateHeader(&header, dfPACKET_CODE, df_RES_STRESS_ECHO, wPayloadSize);
	//유니캐스트
	Unicast(writeBuffer, header, chpPayload, wPayloadSize);
}

void PrintCount() {
	int count;
	UINT ui64TempTime;

	g_ui64Time = GetTickCount();
	ui64TempTime = g_ui64Time - g_ui64OldTime;
	if (ui64TempTime >= 1000) {
		count = g_mapLogin.size() + g_mapSessionUser.size();
		printf("Connect:%d\n", count);
		g_ui64OldTime = g_ui64Time;
	}
}

void ReadJson() {
	FILE *fp = NULL;
	WCHAR FileName[32];
	UINT64 fileSize;					// 읽어들일 JSON 사이즈
	int headerSize = sizeof(UINT64);

	wsprintfW(FileName, df_FILENAME);

	//파일 읽기
	if (!ReadFile(&fp, FileName)) {
		return;
	}
	//헤더 읽기
	fread_s(&fileSize, headerSize, headerSize, 1, fp);

	char *pJson;

	pJson = new char[fileSize + 1];
	fread_s(pJson, fileSize, fileSize, 1, fp);
	pJson[fileSize] = '\0';

	fclose(fp);


	Document Doc;
	Doc.Parse(pJson);

	UINT64 AccountNo;
	UINT64 FriendNo;
	UINT64 RequestNo;
	UINT64 FromAccountNo;
	UINT64 ToAccountNo;
	WCHAR * szNickname = new WCHAR[dfNICK_MAX_LEN];


	Value::ConstMemberIterator itr = Doc.FindMember("Account");
	if (itr != Doc.MemberEnd()){
		Value &AccountArray = Doc["Account"];
		for (SizeType i = 0; i < AccountArray.Size(); i++) {
			Value &AccountObject = AccountArray[i];
			itr = AccountObject.FindMember("AccountNo");
			if (itr == AccountObject.MemberEnd()) {
				break;
			}
			itr = AccountObject.FindMember("Nickname");
			if (itr == AccountObject.MemberEnd()) {
				break;
			}
			AccountNo = AccountObject["AccountNo"].GetUint64();
			UTF8toUTF16(AccountObject["Nickname"].GetString(), szNickname, dfNICK_MAX_LEN);

			CreateAccount(AccountNo, szNickname);
		}
	}

	itr = Doc.FindMember("Friend");
	if (itr != Doc.MemberEnd()) {
		Value &FriendArray = Doc["Friend"];
		for (SizeType i = 0; i < FriendArray.Size(); i++) {
			Value &FriendObject = FriendArray[i];
			itr = FriendObject.FindMember("FromAccountNo");
			if (itr == FriendObject.MemberEnd()) {
				break;
			}
			itr = FriendObject.FindMember("ToAccountNo");
			if (itr == FriendObject.MemberEnd()) {
				break;
			}
			FromAccountNo = FriendObject["FromAccountNo"].GetUint64();
			ToAccountNo = FriendObject["ToAccountNo"].GetUint64();

			CreateFriend(FromAccountNo, ToAccountNo);
		}
	}
	
	itr = Doc.FindMember("FriendRequest");
	if (itr != Doc.MemberEnd()) {
		Value &FriendRequestArray = Doc["FriendRequest"];
		for (SizeType i = 0; i < FriendRequestArray.Size(); i++) {
			Value &FriendRequestObject = FriendRequestArray[i];
			itr = FriendRequestObject.FindMember("FromAccountNo");
			if (itr == FriendRequestObject.MemberEnd()) {
				break;
			}
			itr = FriendRequestObject.FindMember("ToAccountNo");
			if (itr == FriendRequestObject.MemberEnd()) {
				break;
			}
			FromAccountNo = FriendRequestObject["FromAccountNo"].GetUint64();
			ToAccountNo = FriendRequestObject["ToAccountNo"].GetUint64();

			CreateFriendRequest(FromAccountNo, ToAccountNo);
		}
	}
	
}

void WriteJson() {
	map<UINT64, st_Account *>::iterator account_iter;
	map<UINT64, st_Account *>::iterator account_iter_end;
	map<UINT64, st_Friend *>::iterator friend_iter;
	map<UINT64, st_Friend *>::iterator friend_iter_end;
	map<UINT64, st_Friend *>::iterator request_iter;
	map<UINT64, st_Friend *>::iterator request_iter_end;
	st_Account * account;
	st_Friend * friend_;

	StringBuffer StringJSON;
	Writer<StringBuffer, UTF16<>> writer(StringJSON);

	account_iter = g_mapAccount.begin();
	account_iter_end = g_mapAccount.end();

	writer.StartObject();
	writer.String(L"Account");
	writer.StartArray();
	while (account_iter != account_iter_end) {
		account = account_iter->second;
		writer.StartObject();
		writer.String(L"AccountNo");
		writer.Uint64(account->AccountNo);
		writer.String(L"Nickname");
		writer.String(account->NickName);
		writer.EndObject();
		++account_iter;
	}
	writer.EndArray();

	friend_iter = g_mapFriend.begin();
	friend_iter_end = g_mapFriend.end();

	writer.String(L"Friend");
	writer.StartArray();
	while (friend_iter != friend_iter_end) {
		friend_ = friend_iter->second;
		writer.StartObject();
		writer.String(L"FromAccountNo");
		writer.Uint64(friend_->FromAccountNo);
		writer.String(L"ToAccountNo");
		writer.Uint64(friend_->ToAccountNo);
		writer.EndObject();
		++friend_iter;
	}
	writer.EndArray();

	request_iter = g_mapFriendRequest.begin();
	request_iter_end = g_mapFriendRequest.end();

	writer.String(L"FriendRequest");
	writer.StartArray();
	while (request_iter != request_iter_end) {
		friend_ = request_iter->second;
		writer.StartObject();
		writer.String(L"FromAccountNo");
		writer.Uint64(friend_->FromAccountNo);
		writer.String(L"ToAccountNo");
		writer.Uint64(friend_->ToAccountNo);
		writer.EndObject();
		++request_iter;
	}
	writer.EndArray();
	writer.EndObject();

	const char *pJson = StringJSON.GetString();
	int size = StringJSON.GetSize();

	WCHAR * wchBuffer = (WCHAR *)new char[size];

	FILE *fp = NULL;
	WCHAR FileName[32];
	UINT64 fileSize = size;

	wsprintfW(FileName, df_FILENAME);

	//파일 쓰기
	WriteFile(&fp, FileName);

	//헤더
	fwrite(&fileSize, sizeof(UINT64), 1, fp);
	//내용물
	fwrite(pJson, size, 1, fp);
	fclose(fp);
}

//파일 읽기
bool ReadFile(FILE **fp, WCHAR *txt) {
	int err;
	err = _wfopen_s(fp, txt, L"rt");
	if (err != 0) {
		return false;
	}
	return true;
}

//파일 쓰기
bool WriteFile(FILE **fp, WCHAR *txt) {
	int err;
	err = _wfopen_s(fp, txt, L"wt");
	if (err != 0) {
		return false;
	}
	return true;
}

//UTF-8 -> UTF-16
void UTF8toUTF16(const char *szText, WCHAR *szBuff, int iBuffLen) {
	int iRe = MultiByteToWideChar(CP_UTF8, 0, szText, strlen(szText), szBuff, iBuffLen);
	if (iRe < iBuffLen)
		szBuff[iRe] = L'\0';
	return;
}