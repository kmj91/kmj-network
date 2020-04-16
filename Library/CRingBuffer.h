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
	CRingBuffer();
	CRingBuffer(const int Size);
	~CRingBuffer();

	int Enqueue(const char *chpData, int iSize);
	int Dequeue(char *chpData, int iSize);
	int Peek(char *chpData, int iSize);

	int GetBufferSize();		// ���� ũ��
	int	GetUseSize();			// ������� ũ��
	int GetFreeSize();			// ���ۿ� ���� ũ��
	
	int GetNotBrokenGetSize();
	int GetNotBrokenPutSize();

	void MoveFront(int iSize);
	int MoveRear(int iSize);

	void ClearBuffer();

	char *GetBufferPtr();		// ���� ������
	char *GetReadBufferPtr();	// front ������
	char *GetWriteBufferPtr();	// rear ������

	bool isEmpty();		// �����Ͱ� �ִ��� ������ true / false

	void LockExclusive();
	void LockShared();
	void ReleaseLockExclusive();
	void ReleaseLockShared();

protected:
	char * _queue;
	int _rear;
	int _front;
	int _bufferSize;	// ���� ũ��

	SRWLOCK _rock;
};



CRingBuffer::CRingBuffer() {
	_queue = new char[DEFAULT_SIZE];
	_rear = 0;
	_front = 0;
	_bufferSize = DEFAULT_SIZE;

	InitializeSRWLock(&_rock);
}

CRingBuffer::CRingBuffer(const int Size) {
	_queue = new char[Size];
	_rear = 0;
	_front = 0;
	_bufferSize = Size;

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
		chpData = chpData - iSize;				//������ �ּ� ����ġ
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
		chpData = chpData - (iSize - cnt);		//������ �ּ� ����ġ
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
	front �̵�
************************/
void CRingBuffer::MoveFront(int iSize) {
	int iUseSize = GetUseSize();

	if (iSize > iUseSize)
		iSize = iUseSize;

	_front = (_front + iSize) % _bufferSize;
}

/***********************
	MoveRear
	rear �̵�
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
	�ʱ�ȭ
************************/
void CRingBuffer::ClearBuffer() {
	_rear = 0;
	_front = 0;
}

/***********************
	GetBufferPtr
	���� ������
************************/
char *CRingBuffer::GetBufferPtr() {
	return &_queue[0];
}

/***********************
	GetReadBufferPtr
	front ������ ����
************************/
char *CRingBuffer::GetReadBufferPtr() {
	return &_queue[_front];
}

/***********************
	GetWriteBufferPtr
	rear ������ ����
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

// ���� ��
void CRingBuffer::LockExclusive() {
	AcquireSRWLockExclusive(&_rock);
}

// �б� ��
void CRingBuffer::LockShared() {
	AcquireSRWLockShared(&_rock);
}

// ����
void CRingBuffer::ReleaseLockExclusive() {
	ReleaseSRWLockExclusive(&_rock);
}

// ����
void CRingBuffer::ReleaseLockShared() {
	ReleaseSRWLockShared(&_rock);
}