# ChattingServer, ChattingClient
* 기초적인 네트워크 소켓 프로그래밍
### ChattingServer
* 클라이언트로부터 요청받은 목록을 패킷으로 보냅니다. [[프로토콜 참조]](https://github.com/kmj91/kmj-network/blob/master/ChattingServer/ChattingServer/Protocol.h)
* 체크섬 검사 및 패킷의 체크섬 값 취득
* 대화방 관리 및 세션 삭제
1. 로그인 요청 받음
2. 로그인 요청 응답
3. ~~대화방 리스트 요청 받음~~ - 세션에 유저 식별 번호가 있어서 필요없음
4. 대화방 리스트 요청 응답
5. 대화방 생성 요청 받음
6. 대화방 생성 응답
7. 대화방 입장 요청 받음
8. 대화방 입장 응답
9. 대화방 유저 입장 응답 - 대화방에 있던 유저에게 전파
10. 채팅 송신 요청 받음
11. 채팅 송신 응답
12. 대화방 삭제
13. 유니 캐스트, 브로드 캐스트

### ChattingClient
1. 로그인 요청
2. 대화방 리스트 요청
3. 대화방 생성 요청
4. 대화방 입장 요청
5. 채팅 송신 요청
6. 대화방 퇴장 요청
* 요청에 대응되는 패킷 생성
* 체크섬 검사 및 패킷의 체크섬 값 취득

# FriendServer, FriendClient, FriendStress
* 관계맺기
### FriendServer
회원 목록과 친구 목록, 친구 요청 목록을 json 형태로 저장하고 있습니다.

클라이언트로부터 요청받은 목록을 패킷으로 보냅니다. [[프로토콜 참조]](https://github.com/kmj91/kmj-network/blob/master/FriendServer/FriendServer/Protocol.h)
### FriendClient
1. 회원추가
2. 로그인
3. 회원목록
4. ~~친구목록~~
5. ~~받은 친구요청 목록~~
6. ~~보낸 친구요청 목록~~
7. ~~친구요청 보내기~~
8. ~~친구요청 취소~~
9. ~~친구요청 수락~~
10. ~~친구요청 거부~~
11. ~~친구끊기~~
### FriendStress
스트레스 테스트용 에코 서버


# PunisherServer, Punisher
* tcp mmo 서버 클라
### PunisherServer
Select 모델의 단일 스레드 서버
### Punisher
WSAAsyncSelect 모델 클라
