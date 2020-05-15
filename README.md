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
