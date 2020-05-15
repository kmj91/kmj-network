#pragma once
#include <windows.h>
#include <string.h>

class CRingBuffer {
private:
	enum RingBuffer
	{
		DEFAULT_SIZE = 10240
	};

public:
	explicit CRingBuffer();
	explicit CRingBuffer(const int Size);
	CRingBuffer(const CRingBuffer& rhs);
	CRingBuffer& operator=(const CRingBuffer& rhs);

	~CRingBuffer();

	int Enqueue(const char *chpData, int iSize);
	int Dequeue(char *chpData, int iSize);
	int Peek(char *chpData, int iSize);

	int GetBufferSize();		// 버퍼 크기
	int	GetUseSize();			// 사용중인 크기
	int GetFreeSize();			// 버퍼에 남은 크기
	
	int GetNotBrokenGetSize();
	int GetNotBrokenPutSize();

	void MoveFront(int iSize);
	int MoveRear(int iSize);

	void ClearBuffer();

	char *GetBufferPtr();		// 버퍼 포인터
	char *GetReadBufferPtr();	// front 포인터
	char *GetWriteBufferPtr();	// rear 포인터

	bool isEmpty();		// 데이터가 있는지 없는지 true / false

	void LockExclusive();
	void LockShared();
	void ReleaseLockExclusive();
	void ReleaseLockShared();

protected:
	char * _queue;
	int _rear;
	int _front;
	int _bufferSize;	// 버퍼 크기

	SRWLOCK _rock;
};



CRingBuffer::CRingBuffer() : _rear(0), _front(0), _bufferSize(DEFAULT_SIZE) {
	_queue = new char[DEFAULT_SIZE];

	InitializeSRWLock(&_rock);
}

CRingBuffer::CRingBuffer(const int Size) : _rear(0), _front(0), _bufferSize(Size) {
	_queue = new char[Size];

	InitializeSRWLock(&_rock);
}

CRingBuffer::CRingBuffer(const CRingBuffer& rhs) : _rear(rhs._rear), _front(rhs._front), _bufferSize(rhs._bufferSize) {
	_queue = new char[_bufferSize];

	memcpy_s(_queue, _bufferSize, rhs._queue, _bufferSize);

	InitializeSRWLock(&_rock);
}

CRingBuffer& CRingBuffer::operator=(const CRingBuffer& rhs) {
	_rear = rhs._rear;
	_front = rhs._front;
	_bufferSize = rhs._bufferSize;
	_queue = new char[_bufferSize];

	memcpy_s(_queue, _bufferSize, rhs._queue, _bufferSize);

	InitializeSRWLock(&_rock);
}

CRingBuffer::~CRingBuffer() {
	delete[]_queue;
}

int CRingBuffer::Enqueue(const char *chpData, int iSize) {
	char *chpRear = &_queue[_rear];
	int cnt = 0;
	int iFreeSize = GetFreeSize();

	if (iSize > iFreeSize)
		iSize = iFreeSize;

	if ((_rear + iSize) / _bufferSize) {
		cnt = _bufferSize - _rear;
		memcpy_s(chpRear, cnt, chpData, cnt);
		chpData = chpData + cnt;
		chpRear = &_queue[0];
		cnt = iSize - cnt;
		memcpy_s(chpRear, cnt, chpData, cnt);
		chpData = chpData - iSize;				//포인터 주소 원위치
	}
	else {
		memcpy_s(chpRear, iSize, chpData, iSize);
	}

	_rear = (_rear + iSize) % _bufferSize;

	return iSize;
}

int CRingBuffer::Dequeue(char *chpData, int iSize) {
	char *chpFront = &_queue[_front];
	int cnt = 0;
	int iUseSize = GetUseSize();

	if (iSize > iUseSize)
		iSize = iUseSize;

	if ((_front + iSize) / _bufferSize) {
		cnt = _bufferSize - _front;
		memcpy_s(chpData, cnt, chpFront, cnt);
		chpData = chpData + cnt;
		chpFront = &_queue[0];
		cnt = iSize - cnt;
		memcpy_s(chpData, cnt, chpFront, cnt);
		chpData = chpData - (iSize - cnt);		//포인터 주소 원위치
	}
	else {
		memcpy_s(chpData, iSize, chpFront, iSize);
	}

	_front = (_front + iSize) % _bufferSize;

	return iSize;
}

int CRingBuffer::Peek(char *chpData, int iSize) {
	char *chpFront = &_queue[_front];
	int cnt = 0;
	int iUseSize = GetUseSize();

	if (iSize > iUseSize)
		iSize = iUseSize;

	if ((_front + iSize) / _bufferSize) {
		cnt = _bufferSize - _front;
		memcpy_s(chpData, cnt, chpFront, cnt);
		chpData = chpData + cnt;
		chpFront = &_queue[0];
		cnt = iSize - cnt;
		memcpy_s(chpData, cnt, chpFront, cnt);
		chpFront = chpFront + cnt;
	}
	else {
		memcpy_s(chpData, iSize, chpFront, iSize);
		chpFront = chpFront + iSize;
	}
	return iSize;
}

int CRingBuffer::GetBufferSize() {
	return _bufferSize;
}

int	CRingBuffer::GetUseSize() {
	if (_front < _rear) {
		return _rear - _front;
	}
	else if (_front > _rear) {
		return (_bufferSize - _front) + _rear;
	}
	else {
		return 0;
	}
}

int CRingBuffer::GetFreeSize() {
	if (_front < _rear) {
		return (_bufferSize - _rear) + _front - 1;
	}
	else if (_front > _rear) {
		return _front - _rear - 1;
	}
	else {
		return _bufferSize - 1;
	}
}

int CRingBuffer::GetNotBrokenPutSize() {
	if (_front < _rear) {
		return _bufferSize - _rear;
	}
	else if (_front > _rear) {
		return _front - _rear - 1;
	}
	else {
		if (_rear == 0) {
			return _bufferSize - 1;
		}
		else {
			return _bufferSize - _rear;
		}
	}
}

int CRingBuffer::GetNotBrokenGetSize() {
	if (_front < _rear) {
		return _rear - _front;
	}
	else if (_front > _rear) {
		return _bufferSize - _front;
	}
	else {
		return 0;
	}
}

/***********************
	MoveFront
	front 이동
************************/
void CRingBuffer::MoveFront(int iSize) {
	int iUseSize = GetUseSize();

	if (iSize > iUseSize)
		iSize = iUseSize;

	_front = (_front + iSize) % _bufferSize;
}

/***********************
	MoveRear
	rear 이동
************************/
int CRingBuffer::MoveRear(int iSize) {
	int iFreeSize = GetFreeSize();

	if (iSize > iFreeSize)
		iSize = iFreeSize;

	_rear = (_rear + iSize) % _bufferSize;

	return iSize;
}

/***********************
	ClearBuffer
	초기화
************************/
void CRingBuffer::ClearBuffer() {
	_rear = 0;
	_front = 0;
}

/***********************
	GetBufferPtr
	버퍼 포인터
************************/
char *CRingBuffer::GetBufferPtr() {
	return &_queue[0];
}

/***********************
	GetReadBufferPtr
	front 포인터 리턴
************************/
char *CRingBuffer::GetReadBufferPtr() {
	return &_queue[_front];
}

/***********************
	GetWriteBufferPtr
	rear 포인터 리턴
************************/
char *CRingBuffer::GetWriteBufferPtr() {
	return &_queue[_rear];
}

bool CRingBuffer::isEmpty() {
	if ((_rear + 1) % _bufferSize == _front) {
		return false;
	}
	else {
		return true;
	}
}

// 쓰기 락
void CRingBuffer::LockExclusive() {
	AcquireSRWLockExclusive(&_rock);
}

// 읽기 락
void CRingBuffer::LockShared() {
	AcquireSRWLockShared(&_rock);
}

// 해제
void CRingBuffer::ReleaseLockExclusive() {
	ReleaseSRWLockExclusive(&_rock);
}

// 해제
void CRingBuffer::ReleaseLockShared() {
	ReleaseSRWLockShared(&_rock);
}