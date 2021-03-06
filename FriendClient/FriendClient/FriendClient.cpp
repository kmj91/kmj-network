﻿// FriendClient.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <conio.h>			//getch

#include "Protocol.h"
#include "CSerializationBuffer.h"
#include "CRingBuffer.h"

#pragma comment(lib, "ws2_32")

#define df_SERVERIP			L"127.0.0.1"

bool Connect();								// 서버에 접속
void PrintMenu();							// 메뉴
void Select(int iSelect);					// 선택한 메뉴
bool RecvProc();							// 리시브 처리
bool RecvPacketProc(BYTE byType, CSerializationBuffer *pPacket);		// 리시브 패킷 처리
void Request_AccountAdd();					// 회원가입 요청
void Response_AccountAdd(CSerializationBuffer *pPacket);				// 회원가입 응답
void Request_Login();						// 로그인 요청
void Response_Login(CSerializationBuffer *pPacket);						// 로그인 응답
void Request_AccountList();					// 회원 리스트 요청
void Response_AccountList(CSerializationBuffer *pPacket);				// 회원 리스트 응답
void Request_FriendList();					// 친구 리스트 요청
void Response_FriendList(CSerializationBuffer *pPacket);				// 친구 리스트 응답
void Request_FriendReplyList();				// 친구요청 받은거 리스트 요청
void Response_FriendReplyList(CSerializationBuffer *pPacket);			// 친구요청 받은거 리스트 응답
void Request_FriendRequestList();			// 친구요청 보낸거 리스트 요청
void Response_FriendRequestList(CSerializationBuffer *pPacket);			// 친구요청 보낸거 리스트 응답
void Request_FriendRequest();				// 친구요청 요청
void Response_FriendRequest(CSerializationBuffer *pPacket);				// 친구요청 응답
void Request_FriendCancel();				// 친구요청 취소 요청
void Response_FriendCancel(CSerializationBuffer *pPacket);				// 친구요청 취소 응답
void Request_FriendAgree();					// 친구요청 수락 요청
void Response_FriendAgree(CSerializationBuffer *pPacket);				// 친구요청 수락 응답
void Request_FriendDeny();					// 친구요청 거부 요청
void Response_FriendDeny(CSerializationBuffer *pPacket);				// 친구요청 거부 응답
void Request_FriendRemove();				// 친구관계 끊기 요청
void Response_FriendRemove(CSerializationBuffer *pPacket);				// 친구관계 끊기 응답
void CreateHeader(CSerializationBuffer &SerialBuff, WORD wMsgType, WORD	wPayloadSize);				// 해더 만들기
void CreateReqAccountAddPacket(CSerializationBuffer &SerialBuff, WCHAR * szNickname, int iSize);	// 회원가입 패킷 만들기
void CreateReqLoginPacket(CSerializationBuffer &SerialBuff, UINT64 uiAccountNo);					// 로그인 패킷 만들기
void CreateReqAccountListPacket(CSerializationBuffer &SerialBuff);									// 회원 리스트 패킷 만들기
void CreateReqFriendListPacket(CSerializationBuffer &SerialBuff, WCHAR * szNickname, int iSize);	// 친구 리스트 패킷 만들기
void CreateReqFriendRequestListPacket(CSerializationBuffer &SerialBuff, WCHAR * szNickname, int iSize);	// 친구요청 보낸거 리스트 패킷 만들기
void CreateReqFriendReplyListPacket(CSerializationBuffer &SerialBuff, WCHAR * szNickname, int iSize);	// 친구요청 받은거 리스트 패킷 만들기
void CreateReqFriendRemovePacket(CSerializationBuffer &SerialBuff, WCHAR * szNickname, int iSize);		// 친구관계 끊기 패킷 만들기
void CreateReqFriendRequestPacket(CSerializationBuffer &SerialBuff, WCHAR * szNickname, int iSize);		// 친구요청 패킷 만들기
void CreateReqFriendCancelPacket(CSerializationBuffer &SerialBuff, WCHAR * szNickname, int iSize);		// 친구요청 취소 패킷 만들기
void CreateReqFriendDenyPacket(CSerializationBuffer &SerialBuff, WCHAR * szNickname, int iSize);			// 친구요청 거부 만들기
void CreateReqFriendAgreePacket(CSerializationBuffer &SerialBuff, WCHAR * szNickname, int iSize);		// 친구요청 수락 패킷 만들기


SOCKET g_sock;
bool g_close = false;
CRingBuffer g_readBuff;

bool g_LoginFlag = false;
bool g_AccountListFlag = false;
UINT64 g_AccountNo;
WCHAR g_Nickname[dfNICK_MAX_LEN];

int main()
{
	int iSelect;

	if (!Connect())
		return 0;

	while (!g_close) {
		PrintMenu();

		wprintf(L":");
		scanf_s("%d", &iSelect);
		Select(iSelect);
		//리시브 처리
		if (!RecvProc())
			break;
	}

	closesocket(g_sock);

    return 0;
}

// 서버에 접속
bool Connect() {
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

	// connect()
	SOCKADDR_IN serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	InetPton(AF_INET, df_SERVERIP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(dfNETWORK_PORT);
	iretval = connect(g_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (iretval == SOCKET_ERROR) {
		iretval = GetLastError();
		printf("connect error [%d]", iretval);
		return false;
	}


	return true;
}

// 메뉴
void PrintMenu() {
	if (g_AccountListFlag) {
		g_AccountListFlag = false;
	}
	else {
		system("cls");
	}
	
	printf("--- 친구관리 메인메뉴 ---\n");
	
	if (g_LoginFlag) {
		printf("\n# 내 AccountNo : %d\n", g_AccountNo);
		printf("# 내 닉네임 : %ls\n\n", g_Nickname);
	}
	else {
		printf("# 로그인이 안되었습니다.\n\n");
	}
	printf("1. 회원추가\n");
	printf("2. 로그인\n");
	printf("3. 회원목록\n");
	printf("4. 친구목록\n");
	printf("5. 받은 친구요청 목록\n");
	printf("6. 보낸 친구요청 목록\n");
	printf("7. 친구요청 보내기\n");
	printf("8. 친구요청 취소\n");
	printf("9. 친구요청 수락\n");
	printf("10. 친구요청 거부\n");
	printf("11. 친구끊기\n");
}

// 선택한 메뉴
void Select(int iSelect) {
	switch (iSelect) {
	case 1:
		// 회원추가
		Request_AccountAdd();
		break;
	case 2:
		// 로그인
		Request_Login();
		break;
	case 3:
		// 회원 목록
		Request_AccountList();
		break;
	case 4:
		// 친구 목록
		Request_FriendList();
		break;
	case 5:
		// 받은 친구요청 목록
		Request_FriendReplyList();
		break;
	case 6:
		// 보낸 친구요청 목록
		Request_FriendRequestList();
		break;
	case 7:
		// 친구요청 보내기
		Request_FriendRequest();
		break;
	case 8:
		// 친구요청 취소
		Request_FriendCancel();
		break;
	case 9:
		// 친구요청 수락
		Request_FriendAgree();
		break;
	case 10:
		// 친구요청 거부
		Request_FriendDeny();
		break;
	case 11:
		// 친구끊기
		Request_FriendRemove();
		break;
	default:
		break;
	}
}

// 리시브 처리
bool RecvProc() {
	int iretval;
	int iHeaderSize = sizeof(st_PACKET_HEADER);
	int iReadSize;						// 읽어들일 수 있는 버퍼 사이즈
	char *chpRear;						// 버퍼 포인터
	st_PACKET_HEADER header;
	CSerializationBuffer serialBuff;

	while (1) {
		// 버퍼 포인터
		chpRear = g_readBuff.GetWriteBufferPtr();
		iReadSize = g_readBuff.GetNotBrokenPutSize();

		//데이터 받기
		iretval = recv(g_sock, chpRear, iReadSize, 0);
		if (iretval == SOCKET_ERROR) {
			iretval = GetLastError();
			printf("[%d] recvn error\n", iretval);
			return false;
		}
		else if (iretval == 0) {
			printf("서버와 연결이 끊겼습니다.\n");
			return false;
		}

		//받은만큼 이동
		g_readBuff.MoveRear(iretval);

		//완성된 패킷 전부 처리
		while (1) {

			//헤더 읽기
			iretval = g_readBuff.Peek((char *)&header, iHeaderSize);
			if (iretval < iHeaderSize) {
				break;
			}

			//패킷 코드 확인
			if (header.byCode != dfPACKET_CODE) {
				return false;
			}

			//패킷 완성됬나
			iretval = g_readBuff.GetUseSize();
			if (iretval < header.wPayloadSize + iHeaderSize) {
				break;
			}
			// 해더 크기만큼 버퍼 프론트 이동
			g_readBuff.MoveFront(iHeaderSize);

			//패킷
			g_readBuff.Dequeue(serialBuff.GetWriteBufferPtr(), header.wPayloadSize);
			serialBuff.MoveRear(header.wPayloadSize);

			//패킷 처리
			if (!RecvPacketProc(header.wMsgType, &serialBuff)) {
				printf("RecvProc(), packet type error type : %d\n", header.wMsgType);
				return false;
			}
			return true;
		}//while(1)
	}//while(1)
}

// 리시브 패킷 처리
bool RecvPacketProc(BYTE byType, CSerializationBuffer *pPacket) {
	//패킷 타입
	switch (byType) {
	case df_RES_ACCOUNT_ADD:
		//회원가입
		Response_AccountAdd(pPacket);
		break;
	case df_RES_LOGIN:
		//로그인
		Response_Login(pPacket);
		break;
	case df_RES_ACCOUNT_LIST:
		//회원 리스트
		Response_AccountList(pPacket);
		break;
	case df_RES_FRIEND_LIST:
		//친구 리스트
		Response_FriendList(pPacket);
		break;
	case df_RES_FRIEND_REQUEST_LIST:
		//친구요청 보낸거 리스트
		Response_FriendRequestList(pPacket);
		break;
	case df_RES_FRIEND_REPLY_LIST:
		//친구요청 받은거 리스트
		Response_FriendReplyList(pPacket);
		break;
	case df_RES_FRIEND_REMOVE:
		//친구관계 끊기
		Response_FriendRemove(pPacket);
		break;
	case df_RES_FRIEND_REQUEST:
		//친구요청 요청
		Response_FriendRequest(pPacket);
		break;
	case df_RES_FRIEND_CANCEL:
		//친구요청 취소 요청
		Response_FriendCancel(pPacket);
		break;
	case df_RES_FRIEND_DENY:
		//친구요청 거부 요청
		Response_FriendDeny(pPacket);
		break;
	case df_RES_FRIEND_AGREE:
		//친구요청 수락 요청
		Response_FriendAgree(pPacket);
		break;
	default:
		return false;
	}
	return true;
}

// 회원가입 요청
void Request_AccountAdd() {
	int iretval;
	int iWriteSize;					// 해더 + 페이로드
	WORD wPayloadSize;				// 페이로드 사이즈
	char *chpFront;
	char szNickname[dfNICK_MAX_LEN];
	WCHAR szBuff[dfNICK_MAX_LEN];
	CSerializationBuffer serialBuff;
	
	printf("신규 닉네임:");
	scanf_s("%s", szNickname, dfNICK_MAX_LEN);
	
	int iRe = MultiByteToWideChar(CP_ACP, MB_COMPOSITE, szNickname, strlen(szNickname), szBuff, dfNICK_MAX_LEN);
	if (iRe < dfNICK_MAX_LEN)
		szBuff[iRe] = L'\0';

	// 패킷 만들기
	CreateReqAccountAddPacket(serialBuff, szBuff, sizeof(szNickname));
	// 페이로드 사이즈
	wPayloadSize = serialBuff.GetUseSize();
	// 헤더 만들기
	CreateHeader(serialBuff, df_REQ_ACCOUNT_ADD, wPayloadSize);
	// 샌드 사이즈
	iWriteSize = serialBuff.GetUseSize();
	// 버퍼 포인터
	chpFront = serialBuff.GetReadBufferPtr();

	iretval = send(g_sock, chpFront, iWriteSize, 0);
	if (iretval == SOCKET_ERROR) {
		iretval = GetLastError();
		printf("[%d] send error\n", iretval);
		g_close = true;
		return;
	}
}

// 회원가입 응답
void Response_AccountAdd(CSerializationBuffer *pPacket) {
	//------------------------------------------------------------
	// 회원가입 결과
	//
	// {
	//		UINT64		AccountNo
	// }
	//------------------------------------------------------------

	UINT64 uiNo;

	*pPacket >> &uiNo;

	printf("\n신규회원 추가 완료 AccountNo:%d\n\n", uiNo);
	printf("press any key...");
	_getch();
}

// 로그인 요청
void Request_Login() {
	int	AccountNo;				// 회원 번호
	int iretval;
	int iWriteSize;					// 해더 + 페이로드
	WORD wPayloadSize;				// 페이로드 사이즈
	char *chpFront;
	char szNickname[dfNICK_MAX_LEN];
	CSerializationBuffer serialBuff;

	printf("로그인 AccountNo:");
	scanf_s("%d", &AccountNo);


	// 패킷 만들기
	CreateReqLoginPacket(serialBuff, AccountNo);
	// 페이로드 사이즈
	wPayloadSize = serialBuff.GetUseSize();
	// 헤더 만들기
	CreateHeader(serialBuff, df_REQ_LOGIN, wPayloadSize);
	// 샌드 사이즈
	iWriteSize = serialBuff.GetUseSize();
	// 버퍼 포인터
	chpFront = serialBuff.GetReadBufferPtr();

	iretval = send(g_sock, chpFront, iWriteSize, 0);
	if (iretval == SOCKET_ERROR) {
		iretval = GetLastError();
		printf("[%d] send error\n", iretval);
		g_close = true;
		return;
	}
}

// 로그인 응답
void Response_Login(CSerializationBuffer *pPacket) {
	//------------------------------------------------------------
	// 회원로그인 결과
	//
	// {
	//		UINT64					AccountNo		// 0 이면 실패
	//		WCHAR[dfNICK_MAX_LEN]	NickName
	// }
	//------------------------------------------------------------

	*pPacket >> &g_AccountNo;

	if (g_AccountNo == 0) {
		printf("\n로그인 실패!\n\n");
		printf("press any key...");
		_getch();
		return;
	}

	pPacket->ReadString(g_Nickname, sizeof(g_Nickname));

	g_LoginFlag = true;
	printf("\n로그인 완료 닉네임:%S  AccountNo:%d\n\n", g_Nickname, g_AccountNo);
	printf("press any key...");
	_getch();
}

// 회원 리스트 요청
void Request_AccountList() {
	//------------------------------------------------------------
	// 회원리스트 요청
	//
	// {
	//		없음.
	// }
	//------------------------------------------------------------

	int	AccountNo;					// 회원 번호
	int iretval;
	int iWriteSize;					// 해더 + 페이로드
	WORD wPayloadSize;				// 페이로드 사이즈
	char *chpFront;
	CSerializationBuffer serialBuff;

	// 패킷 만들기
	CreateReqAccountListPacket(serialBuff);
	// 페이로드 사이즈
	wPayloadSize = serialBuff.GetUseSize();
	// 헤더 만들기
	CreateHeader(serialBuff, df_REQ_ACCOUNT_LIST, wPayloadSize);
	// 샌드 사이즈
	iWriteSize = serialBuff.GetUseSize();
	// 버퍼 포인터
	chpFront = serialBuff.GetReadBufferPtr();

	iretval = send(g_sock, chpFront, iWriteSize, 0);
	if (iretval == SOCKET_ERROR) {
		iretval = GetLastError();
		printf("[%d] send error\n", iretval);
		g_close = true;
		return;
	}
}

// 회원 리스트 응답
void Response_AccountList(CSerializationBuffer *pPacket) {
	//------------------------------------------------------------
	// 회원리스트 결과
	//
	// {
	//		UINT	Count		// 회원 수
	//		{
	//			UINT64					AccountNo
	//			WCHAR[dfNICK_MAX_LEN]	NickName
	//		}
	// }
	//------------------------------------------------------------

	int iCnt;
	UINT Count;
	UINT64 AccountNo;
	WCHAR Nickname[dfNICK_MAX_LEN];

	g_AccountListFlag = true;

	system("cls");
	printf("/// 회원정보 ///////////////\n");

	*pPacket >> &Count;
	printf("총 회원수 : %d\n", Count);
	
	for (iCnt = 0; iCnt < Count; iCnt++) {
		*pPacket >> &AccountNo;
		pPacket->ReadString(Nickname, sizeof(Nickname));
		wprintf(L"%d. %ls\n", AccountNo, Nickname);
	}
	printf("\n");
}

// 친구 리스트 요청
void Request_FriendList() {

}

// 친구 리스트 응답
void Response_FriendList(CSerializationBuffer *pPacket) {

}

// 친구요청 받은거 리스트 요청
void Request_FriendReplyList() {

}

//친구요청 받은거 리스트 응답
void Response_FriendReplyList(CSerializationBuffer *pPacket) {

}

// 친구요청 보낸거 리스트 요청
void Request_FriendRequestList() {

}

// 친구요청 보낸거 리스트 응답
void Response_FriendRequestList(CSerializationBuffer *pPacket) {

}

// 친구요청 요청
void Request_FriendRequest() {

}

// 친구요청 응답
void Response_FriendRequest(CSerializationBuffer *pPacket) {

}

// 친구요청 취소 요청
void Request_FriendCancel() {

}

// 친구요청 취소 응답
void Response_FriendCancel(CSerializationBuffer *pPacket) {

}

// 친구요청 수락 요청			
void Request_FriendAgree() {

}

// 친구요청 수락 응답
void Response_FriendAgree(CSerializationBuffer *pPacket) {

}

// 친구요청 거부 요청
void Request_FriendDeny() {

}

// 친구요청 거부 응답
void Response_FriendDeny(CSerializationBuffer *pPacket) {

}

// 친구관계 끊기 요청
void Request_FriendRemove() {

}

// 친구관계 끊기 응답
void Response_FriendRemove(CSerializationBuffer *pPacket) {

}

// 해더 만들기
void CreateHeader(CSerializationBuffer &SerialBuff, WORD wMsgType, WORD	wPayloadSize) {
	//------------------------------------------------------
	//  패킷헤더
	//
	//	| PacketCode | MsgType | PayloadSize | * Payload * |
	//		1Byte       2Byte      2Byte        Size Byte     
	//
	//------------------------------------------------------
	
	int headerSize;							// 해더 사이즈
	char *chpPayload;						// 버퍼 포인터
	st_PACKET_HEADER header;				// 헤더
	headerSize = sizeof(st_PACKET_HEADER);
	SerialBuff.MoveFront(-headerSize);
	chpPayload = SerialBuff.GetReadBufferPtr();

	header.byCode = dfPACKET_CODE;
	header.wMsgType = wMsgType;
	header.wPayloadSize = wPayloadSize;

	memcpy_s(chpPayload, headerSize, &header, headerSize);
}

// 회원가입 요청 패킷 만들기
void CreateReqAccountAddPacket(CSerializationBuffer &SerialBuff, WCHAR * szNickname, int iSize) {
	//------------------------------------------------------------
	// 회원가입 요청
	//
	// {
	//		WCHAR[dfNICK_MAX_LEN]	닉네임
	// }
	//------------------------------------------------------------

	int headerSize = sizeof(st_PACKET_HEADER);

	SerialBuff.MoveFront(headerSize);
	SerialBuff.MoveRear(headerSize);

	SerialBuff.WriteString(szNickname, iSize);
}

// 로그인 패킷 만들기
void CreateReqLoginPacket(CSerializationBuffer &SerialBuff, UINT64 uiAccountNo) {
	//------------------------------------------------------------
	// 회원로그인
	//
	// {
	//		UINT64		AccountNo
	// }
	//------------------------------------------------------------

	int headerSize = sizeof(st_PACKET_HEADER);

	SerialBuff.MoveFront(headerSize);
	SerialBuff.MoveRear(headerSize);

	SerialBuff << uiAccountNo;
}

// 회원 리스트 패킷 만들기
void CreateReqAccountListPacket(CSerializationBuffer &SerialBuff) {
	//------------------------------------------------------------
	// 회원리스트 요청
	//
	// {
	//		없음.
	// }
	//------------------------------------------------------------

	int headerSize = sizeof(st_PACKET_HEADER);

	SerialBuff.MoveFront(headerSize);
	SerialBuff.MoveRear(headerSize);
}

// 친구 리스트 패킷 만들기
void CreateReqFriendListPacket(CSerializationBuffer &SerialBuff, WCHAR * szNickname, int iSize) {

}

// 친구요청 보낸거 리스트 패킷 만들기
void CreateReqFriendRequestListPacket(CSerializationBuffer &SerialBuff, WCHAR * szNickname, int iSize) {

}

// 친구요청 받은거 리스트 패킷 만들기
void CreateReqFriendReplyListPacket(CSerializationBuffer &SerialBuff, WCHAR * szNickname, int iSize) {

}

// 친구관계 끊기 패킷 만들기
void CreateReqFriendRemovePacket(CSerializationBuffer &SerialBuff, WCHAR * szNickname, int iSize) {

}

// 친구요청 패킷 만들기
void CreateReqFriendRequestPacket(CSerializationBuffer &SerialBuff, WCHAR * szNickname, int iSize) {

}

// 친구요청 취소 패킷 만들기
void CreateReqFriendCancelPacket(CSerializationBuffer &SerialBuff, WCHAR * szNickname, int iSize) {

}

// 친구요청 거부 만들기
void CreateReqFriendDenyPacket(CSerializationBuffer &SerialBuff, WCHAR * szNickname, int iSize) {

}

// 친구요청 수락 패킷 만들기
void CreateReqFriendAgreePacket(CSerializationBuffer &SerialBuff, WCHAR * szNickname, int iSize) {

}