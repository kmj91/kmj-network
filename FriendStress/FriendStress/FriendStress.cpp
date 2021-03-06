// FriendStress.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include <WinSock2.h>
#include <WS2tcpip.h>

#include "Protocol.h"
#include "CSerializationBuffer.h"
#include "CRingBuffer.h"

#pragma comment(lib, "ws2_32")
#pragma comment( lib, "Winmm.lib")			// timeBeginPeriod

#define df_SERVERIP		L"127.0.0.1"
#define df_CONNECT_CNT	1500

struct st_SESSION {
	st_SESSION() {
		flag = false;
		sendFlag = false;
		readBuffer = new CRingBuffer;
		writeBuffer = new CRingBuffer;
	}
	bool flag;										// 사용 여부
	bool sendFlag;
	SOCKET sock;									// 소켓
	UINT64 AccountNo;								// 유저 번호
	CRingBuffer * readBuffer;						// 리시브 버퍼
	CRingBuffer * writeBuffer;						// 샌드 버퍼
	DWORD timeSend;
	DWORD timeRecv;
};

bool Connect(int iCnt);
void Network();
void RecvProc(st_SESSION *pSession);
void SendProc(st_SESSION *pSession);
void Response_StressEcho(st_SESSION *pSession, CSerializationBuffer *pSerialBuff);			// 에코 받기
void CreateHeader(CSerializationBuffer *SerialBuff, WORD wMsgType, WORD	wPayloadSize);		// 해더 만들기
void CreateReqEchoPacket(CSerializationBuffer *SerialBuff);									// 에코 패킷 만들기
void Print();

int g_iTryCnt = 0;				// 컨넥트 시도 수
int g_iFailCnt = 0;				// 컨넥트 실패 수
int g_iSuccessCnt = 0;			// 성공 수
int g_iErrorCnt = 0;			// 에러 카운트
int g_iRecvCnt;					// 리시브 횟수
DWORD g_dwSecMaxTime = 0;		// 초당 최대
DWORD g_dwMaxTime = 0;			// 최대
DWORD g_dwMinTime = 0;			// 최소
DWORD g_dwAverageTime = 0;		// 평균
DWORD g_dwOldTime;

st_SESSION * g_Session;			// 세션
WCHAR g_text[82] = L"1234567890 abcdefghijklmnopqrstuvwxyz 1234567890 abcdefghijklmnopqrstuvwxyz 12345";

int main()
{
	timeBeginPeriod(1);

	int iCnt;
	g_Session = new st_SESSION[df_CONNECT_CNT];
	g_dwOldTime = timeGetTime();

	for (iCnt = 0;iCnt < df_CONNECT_CNT; iCnt++) {
		if (!Connect(iCnt)) {
			++g_iFailCnt;
		}
	}

	while (1) {
		Network();
		Print();
	}

	timeEndPeriod(1);
    return 0;
}

bool Connect(int iCnt) {
	int iretval;

	++g_iTryCnt;

	//윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return false;

	// socket()
	g_Session[iCnt].sock = socket(AF_INET, SOCK_STREAM, 0);
	if (g_Session[iCnt].sock == INVALID_SOCKET) {
		iretval = GetLastError();
		return false;
	}

	// connect()
	SOCKADDR_IN serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	InetPton(AF_INET, df_SERVERIP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(dfNETWORK_PORT);
	iretval = connect(g_Session[iCnt].sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (iretval == SOCKET_ERROR) {
		iretval = GetLastError();
		//printf("connect error [%d]", iretval);
		return false;
	}

	g_Session[iCnt].flag = true;
	++g_iSuccessCnt;

	return true;
}

void Network() {
	FD_SET rset;
	FD_SET wset;
	timeval time;
	time.tv_sec = 0;
	time.tv_usec = 0;
	int iFDcnt;				// 64개 카운트 용
	int iCnt = 0;
	int iretcnt = 0;		// select 결과 값
	int iBegin;				// 배열 처음
	int iEnd;				// 배열 끝

	while (iCnt < df_CONNECT_CNT)
	{
		// 배열 시작 부분 백업
		iBegin = iCnt;

		// 초기화
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		iFDcnt = 0;
		while (1) {
			if (g_Session[iCnt].flag) {
				// FD 셋
				FD_SET(g_Session[iCnt].sock, &rset);
				FD_SET(g_Session[iCnt].sock, &wset);
			}
			
			++iCnt;
			++iFDcnt;

			if (iFDcnt >= df_CONNECT_CNT || iFDcnt >= 64) {
				// 배열 끝 부분 백업
				iEnd = iCnt;
				break;
			}
		}

		// select
		iretcnt = select(0, &rset, &wset, NULL, &time);
		if (iretcnt == SOCKET_ERROR) {
			iretcnt = GetLastError();
			//printf("[%d] select error\n", err);
			return;
		}

		// 로직
		while (iBegin < iEnd) {
			if (!g_Session[iBegin].flag) {
				++iBegin;
				continue;
			}
			

			if (FD_ISSET(g_Session[iBegin].sock, &rset)) {
				RecvProc(&g_Session[iBegin]);
				++g_iRecvCnt;
				--iretcnt;
			}

			if (FD_ISSET(g_Session[iBegin].sock, &wset)) {
				if (!g_Session[iBegin].sendFlag) {
					SendProc(&g_Session[iBegin]);
					--iretcnt;
				}
				
			}

			++iBegin;

			//루프 정지
			if (iretcnt <= 0) {
				break;
			}
		}//while (iretcnt > 0)
	}//while (iterTemp != iter_end)
}

void RecvProc(st_SESSION *pSession) {
	int iretval;
	int iReadSize;			// 읽기 버퍼 사이즈
	int wPacketSize;		// 페이로드 사이즈
	int iHeaderSize;		// 해더 크기
	DWORD dwTime;			// recv 시간
	DWORD dwLatency;		// 레이턴시
	DWORD dwMin;			// 최소값
	char *chpRear;			// 버퍼 포인터
	st_PACKET_HEADER header;
	CRingBuffer * readBuff = pSession->readBuffer;
	CSerializationBuffer serialBuff;

	iHeaderSize = sizeof(header);

	chpRear = readBuff->GetWriteBufferPtr();
	iReadSize = readBuff->GetNotBrokenPutSize();

	//데이터 받기
	iretval = recv(pSession->sock, chpRear, iReadSize, 0);
	if (iretval == SOCKET_ERROR) {
		iretval = GetLastError();
		//if (iretval == WSAEWOULDBLOCK) {
		//	return;
		//}

		wprintf(L"[%d] recv error\n", iretval);
		return;
	}
	else if (iretval == 0) {
		wprintf(L"서버와 연결이 끊겼습니다\n");
		return;
	}

	// 레이턴시
	dwTime = timeGetTime();
	dwLatency = dwTime - pSession->timeSend;
	g_dwSecMaxTime = __max(dwLatency, g_dwSecMaxTime);
	g_dwMaxTime = __max(dwLatency, g_dwMaxTime);
	if (g_dwMinTime == 0) {
		g_dwMinTime = dwLatency;
	}
	else {
		g_dwMinTime = __min(dwLatency, g_dwMinTime);
	}
	g_dwAverageTime += dwLatency;
	g_dwAverageTime = g_dwAverageTime / 2;

	//받은만큼 이동
	readBuff->MoveRear(iretval);

	//완성된 패킷 전부 처리
	while (1) {

		//헤더 읽기
		iretval = readBuff->Peek((char *)&header, iHeaderSize);
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
		iretval = readBuff->GetUseSize();
		if (iretval < wPacketSize + iHeaderSize) {
			return;
		}

		readBuff->MoveFront(iHeaderSize);

		//패킷
		readBuff->Dequeue(serialBuff.GetWriteBufferPtr(), header.wPayloadSize);
		serialBuff.MoveRear(header.wPayloadSize);

		//패킷 타입
		switch (header.wMsgType) {
		case df_RES_STRESS_ECHO:
			//스트레스 테스트용 에코
			Response_StressEcho(pSession, &serialBuff);
			break;
		default:
			++g_iErrorCnt;
			return;
		}
		readBuff->ClearBuffer();
		pSession->writeBuffer->ClearBuffer();

	}//while(1)
}

void SendProc(st_SESSION *pSession) {
	int iRetval;
	int iNotBrokenSize;
	int iWriteSize;					// 해더 + 페이로드
	WORD wPayloadSize;				// 페이로드 사이즈
	char *chpFront;
	CRingBuffer *writeBuff = pSession->writeBuffer;
	CSerializationBuffer serialBuff;

	// 에코 패킷 만들기
	CreateReqEchoPacket(&serialBuff);
	// 페이로드 사이즈
	wPayloadSize = serialBuff.GetUseSize();
	// 헤더 만들기
	CreateHeader(&serialBuff, df_REQ_STRESS_ECHO, wPayloadSize);

	chpFront = serialBuff.GetReadBufferPtr();
	iWriteSize = serialBuff.GetUseSize();

	// 링버퍼에 넣기
	writeBuff->Enqueue(chpFront, iWriteSize);

	chpFront = writeBuff->GetReadBufferPtr();
	iNotBrokenSize = writeBuff->GetNotBrokenGetSize();

	// 시간
	pSession->timeSend = timeGetTime();

	iRetval = send(pSession->sock, chpFront, iNotBrokenSize, 0);
	if (iRetval == SOCKET_ERROR) {
		iRetval = GetLastError();
		//if (iretval == WSAEWOULDBLOCK) {
		//	return;
		//}

		wprintf(L"[%d] send error\n", iRetval);

		return;
	}
	else if (iRetval == 0) {
		wprintf(L"서버와 연결이 끊겼습니다\n");
		return;
	}

	pSession->sendFlag = true;
}

// 에코 받기
void Response_StressEcho(st_SESSION *pSession, CSerializationBuffer *pSerialBuff) {
	//------------------------------------------------------------
	// 스트레스 테스트용 에코응답
	//
	// {
	//			WORD		Size
	//			Size		문자열 (WCHAR 유니코드)
	// }
	//------------------------------------------------------------

	WORD wSize;
	WORD wHikakuSize;
	char *chpString;
	char *chpHikakuString;
	st_PACKET_HEADER header;

	// 패킷 문자열 사이즈
	*pSerialBuff >> &wSize;
	chpString = new char[wSize];
	// 패킷 문자열 얻기
	pSerialBuff->ReadString((WCHAR *)chpString, wSize);

	pSerialBuff->ClearBuffer();

	pSession->writeBuffer->Dequeue((char *)&header, sizeof(header));

	int RingSize = pSession->writeBuffer->GetUseSize();
	char *chpTemp = pSerialBuff->GetWriteBufferPtr();

	// 보낸 패킷
	pSession->writeBuffer->Dequeue(chpTemp, RingSize);
	pSerialBuff->MoveRear(RingSize);
	// 문자열 사이즈
	*pSerialBuff >> &wHikakuSize;
	chpHikakuString = new char[wHikakuSize];
	// 문자열 얻기
	pSerialBuff->ReadString((WCHAR *)chpHikakuString, wHikakuSize);

	if (memcmp(chpString, chpHikakuString, wHikakuSize)) {
		++g_iErrorCnt;
	}

	pSession->sendFlag = false;
}

// 해더 만들기
void CreateHeader(CSerializationBuffer *SerialBuff, WORD wMsgType, WORD	wPayloadSize) {
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
	SerialBuff->MoveFront(-headerSize);
	chpPayload = SerialBuff->GetReadBufferPtr();

	header.byCode = dfPACKET_CODE;
	header.wMsgType = wMsgType;
	header.wPayloadSize = wPayloadSize;

	memcpy_s(chpPayload, headerSize, &header, headerSize);
}

// 에코 패킷 만들기
void CreateReqEchoPacket(CSerializationBuffer *SerialBuff) {
	//------------------------------------------------------------
	// 스트레스 테스트용 에코
	//
	// {
	//			WORD		Size
	//			Size		문자열 (WCHAR 유니코드)
	// }
	//------------------------------------------------------------

	WORD wRandPut;
	int headerSize = sizeof(st_PACKET_HEADER);

	SerialBuff->MoveFront(headerSize);
	SerialBuff->MoveRear(headerSize);

	wRandPut = ((rand() % 81) + 1) * 2;
	
	*SerialBuff << wRandPut;
	SerialBuff->WriteString(g_text, wRandPut);
}


void Print() {
	int count;
	DWORD dwTime;

	dwTime = timeGetTime();
	if (dwTime - g_dwOldTime >= 1000) {
		wprintf(L"////////////////////////////////////////////\n");
		wprintf(L"connectTry:%d\n", g_iTryCnt);
		wprintf(L"connectFail:%d\n", g_iFailCnt);
		wprintf(L"connectSuccess:%d\n", g_iSuccessCnt);
		wprintf(L"Recv Count:%d / Error Count:%d\n", g_iRecvCnt, g_iErrorCnt);
		wprintf(L"Echo avr Latency %d ms / maximum %d ms / minimum %d ms / sec max %d ms\n\n",
			g_dwAverageTime, g_dwMaxTime, g_dwMinTime, g_dwSecMaxTime);

		g_dwOldTime = dwTime;
		g_dwSecMaxTime = 0;
		g_iRecvCnt = 0;
	}
}