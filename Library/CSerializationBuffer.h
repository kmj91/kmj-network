#pragma once

#include <memory.h>

class CSerializationBuffer {
public:
	enum SerializationBuffer
	{
		DEFAULT_SIZE = 1124
	};

public:
	explicit CSerializationBuffer() : _rear(0), _front(0), _bufferSize(DEFAULT_SIZE) {
		_queue = new char[DEFAULT_SIZE];
	}

	explicit CSerializationBuffer(const int iSize) : _rear(0), _front(0), _bufferSize(iSize) {
		_queue = new char[iSize];
	}

	CSerializationBuffer(const CSerializationBuffer& rhs) : _rear(rhs._rear), _front(rhs._front), _bufferSize(rhs._bufferSize) {
		_queue = new char[_bufferSize];

		memcpy_s(_queue, _bufferSize, rhs._queue, _bufferSize);
	}

	CSerializationBuffer& operator=(const CSerializationBuffer& rhs) {
		_bufferSize = rhs._bufferSize;
		_rear = rhs._rear;
		_front = rhs._front;

		// 자기 대입에 대한 안전 처리
		char* pOriginal = _queue;
		_queue = new char[_bufferSize];
		memcpy_s(_queue, _bufferSize, rhs._queue, _bufferSize);
		delete[] pOriginal;

		return *this;
	}

	virtual ~CSerializationBuffer() {
		delete[] _queue;
	}


	//버퍼 초기화
	void ClearBuffer() {
		_front = 0;
		_rear = 0;
		//_freeSize = _bufferSize;
		//_useSize = 0;
	}



	//char 삽입
	CSerializationBuffer& operator<<(const char chData)
	{
		int typeSize = sizeof(char);
		if (_rear >= _bufferSize) {
			//에러 나거나 버퍼 사이즈 새로 할당

			return (*this);
		}
		_queue[_rear] = chData;
		_rear = _rear + typeSize;
		return (*this);
	}

	//BYTE 삽입
	CSerializationBuffer& operator<<(const unsigned char byData)
	{
		int typeSize = sizeof(unsigned char);
		if (_rear >= _bufferSize) {
			//에러 나거나 버퍼 사이즈 새로 할당

			return (*this);
		}
		_queue[_rear] = byData;
		_rear = _rear + typeSize;
		return (*this);
	}

	//short 삽입
	CSerializationBuffer& operator<<(const short shData)
	{
		char *chpRear = &_queue[_rear];
		int typeSize = sizeof(short);

		if (_rear + typeSize >= _bufferSize) {
			//에러 나거나 버퍼 사이즈 새로 할당

			return (*this);
		}
		memcpy_s(chpRear, typeSize, &shData, typeSize);
		_rear = _rear + typeSize;
		return (*this);
	}

	//WORD 삽입
	CSerializationBuffer& operator<<(const unsigned short wData)
	{
		char *chpRear = &_queue[_rear];
		int typeSize = sizeof(unsigned short);

		if (_rear + typeSize >= _bufferSize) {
			//에러 나거나 버퍼 사이즈 새로 할당

			return (*this);
		}
		memcpy_s(chpRear, typeSize, &wData, typeSize);
		_rear = _rear + typeSize;
		return (*this);
	}

	//int 삽입
	CSerializationBuffer& operator<<(const int iData)
	{
		char *chpRear = &_queue[_rear];
		int typeSize = sizeof(int);

		if (_rear + typeSize >= _bufferSize) {
			//에러 나거나 버퍼 사이즈 새로 할당

			return (*this);
		}
		memcpy_s(chpRear, typeSize, &iData, typeSize);
		_rear = _rear + typeSize;
		return (*this);
	}

	//UINT 삽입
	CSerializationBuffer& operator<<(const unsigned int uiData)
	{
		char *chpRear = &_queue[_rear];
		int typeSize = sizeof(unsigned int);

		if (_rear + typeSize >= _bufferSize) {
			//에러 나거나 버퍼 사이즈 새로 할당

			return (*this);
		}
		memcpy_s(chpRear, typeSize, &uiData, typeSize);
		_rear = _rear + typeSize;
		return (*this);
	}

	//long 삽입
	CSerializationBuffer& operator<<(const long lData)
	{
		char *chpRear = &_queue[_rear];
		int typeSize = sizeof(long);

		if (_rear + typeSize >= _bufferSize) {
			//에러 나거나 버퍼 사이즈 새로 할당

			return (*this);
		}
		memcpy_s(chpRear, typeSize, &lData, typeSize);
		_rear = _rear + typeSize;
		return (*this);
	}

	//DWORD 삽입
	CSerializationBuffer& operator<<(const unsigned long dwData)
	{
		char *chpRear = &_queue[_rear];
		int typeSize = sizeof(unsigned long);

		if (_rear + typeSize >= _bufferSize) {
			//에러 나거나 버퍼 사이즈 새로 할당
			
			return (*this);
		}
		memcpy_s(chpRear, typeSize, &dwData, typeSize);
		_rear = _rear + typeSize;
		return (*this);
	}

	//INT64 삽입
	CSerializationBuffer& operator<<(const __int64 i64Data)
	{
		char *chpRear = &_queue[_rear];
		int typeSize = sizeof(i64Data);

		if (_rear + typeSize >= _bufferSize) {
			//에러 나거나 버퍼 사이즈 새로 할당

			return (*this);
		}
		memcpy_s(chpRear, typeSize, &i64Data, typeSize);
		_rear = _rear + typeSize;
		return (*this);
	}

	//UINT64 삽입
	CSerializationBuffer& operator<<(const unsigned __int64 ui64Data)
	{
		char *chpRear = &_queue[_rear];
		int typeSize = sizeof(ui64Data);

		if (_rear + typeSize >= _bufferSize) {
			//에러 나거나 버퍼 사이즈 새로 할당

			return (*this);
		}
		memcpy_s(chpRear, typeSize, &ui64Data, typeSize);
		_rear = _rear + typeSize;
		return (*this);
	}

	//WCHAR 삽입
	CSerializationBuffer& operator<<(wchar_t * const wchData)
	{
		int len = _msize(wchData);
		char *chpRear = &_queue[_rear];

		if (_front + len >= _bufferSize) {
			//에러 나거나 버퍼 사이즈 새로 할당

			return (*this);
		}
		memcpy_s(chpRear, len, wchData, len);
		_rear = _rear + len;
		return (*this);
	}

	

	//char 출력
	CSerializationBuffer& operator>>(char * const chData)
	{
		int typeSize = sizeof(char);
		if (_front + typeSize > _rear) {
			//에러 나거나 버퍼 사이즈 새로 할당
			
			return (*this);
		}
		*chData = _queue[_front];
		_front = _front + typeSize;
		return (*this);
	}

	//BYTE 출력
	CSerializationBuffer& operator>>(unsigned char * const byData)
	{
		int typeSize = sizeof(unsigned char);
		if (_front + typeSize > _rear) {
			//에러 나거나 버퍼 사이즈 새로 할당
			
			return (*this);
		}
		*byData = _queue[_front];
		_front = _front + typeSize;
		return (*this);
	}

	//short 출력
	CSerializationBuffer& operator>>(short * const shData)
	{
		char *chpFront = &_queue[_front];
		int typeSize = sizeof(short);
		if (_front + typeSize > _rear) {
			//에러 나거나 버퍼 사이즈 새로 할당
			
			return (*this);
		}
		memcpy_s(shData, typeSize, chpFront, typeSize);
		_front = _front + typeSize;
		return (*this);
	}

	//WORD 출력
	CSerializationBuffer& operator>>(unsigned short * const wData)
	{
		char *chpFront = &_queue[_front];
		int typeSize = sizeof(unsigned short);
		if (_front + typeSize > _rear) {
			//에러 나거나 버퍼 사이즈 새로 할당
			
			return (*this);
		}
		memcpy_s(wData, typeSize, chpFront, typeSize);
		_front = _front + typeSize;
		return (*this);
	}

	//int 출력
	CSerializationBuffer& operator>>(int * const iData)
	{
		char *chpFront = &_queue[_front];
		int typeSize = sizeof(int);
		if (_front + typeSize > _rear) {
			//에러 나거나 버퍼 사이즈 새로 할당
			
			return (*this);
		}
		memcpy_s(iData, typeSize, chpFront, typeSize);
		_front = _front + typeSize;
		return (*this);
	}

	//UINT 출력
	CSerializationBuffer& operator>>(unsigned int * const uiData)
	{
		char *chpFront = &_queue[_front];
		int typeSize = sizeof(unsigned int);
		if (_front + typeSize > _rear) {
			//에러 나거나 버퍼 사이즈 새로 할당

			return (*this);
		}
		memcpy_s(uiData, typeSize, chpFront, typeSize);
		_front = _front + typeSize;
		return (*this);
	}

	//long 출력
	CSerializationBuffer& operator>>(long * const lData)
	{
		char *chpFront = &_queue[_front];
		int typeSize = sizeof(long);
		if (_front + typeSize > _rear) {
			//에러 나거나 버퍼 사이즈 새로 할당
			
			return (*this);
		}
		memcpy_s(lData, typeSize, chpFront, typeSize);
		_front = _front + typeSize;
		return (*this);
	}

	//DWORD 출력
	CSerializationBuffer& operator>>(unsigned long  * const dwData)
	{
		char *chpFront = &_queue[_front];
		int typeSize = sizeof(unsigned long);
		if (_front + typeSize > _rear) {
			//에러 나거나 버퍼 사이즈 새로 할당
			
			return (*this);
		}
		memcpy_s(dwData, typeSize, chpFront, typeSize);
		_front = _front + typeSize;
		return (*this);
	}

	//INT64 출력
	CSerializationBuffer& operator>>(__int64 * const i64Data)
	{
		char *chpFront = &_queue[_front];
		int typeSize = sizeof(__int64);
		if (_front + typeSize > _rear) {
			//에러 나거나 버퍼 사이즈 새로 할당

			return (*this);
		}
		memcpy_s(i64Data, typeSize, chpFront, typeSize);
		_front = _front + typeSize;
		return (*this);
	}

	//UINT64 출력
	CSerializationBuffer& operator>>(unsigned __int64 * const ui64Data)
	{
		char *chpFront = &_queue[_front];
		int typeSize = sizeof(unsigned __int64);
		if (_front + typeSize > _rear) {
			//에러 나거나 버퍼 사이즈 새로 할당

			return (*this);
		}
		memcpy_s(ui64Data, typeSize, chpFront, typeSize);
		_front = _front + typeSize;
		return (*this);
	}

	//WCHAR 출력
	CSerializationBuffer& operator>>(wchar_t * const wchData)
	{
		int len = _msize(wchData);
		char *chpFront = &_queue[_front];

		if (_front + len > _rear) {
			//에러 나거나 버퍼 사이즈 새로 할당
			
			return (*this);
		}
		memcpy_s(wchData, len, chpFront, len);
		_front = _front + len;
		return (*this);
	}

	//WCHAR 문자열 삽입
	//void PutString(WCHAR * const wchData, int iSize) {
	//	if (_front + iSize >= _bufferSize) {
	//		//에러 나거나 버퍼 사이즈 새로 할당
	//		throw;
	//	}
	//	memcpy_s(&_queue[_rear], iSize, wchData, iSize);
	//	_rear = _rear + iSize;
	//}

	//WCHAR 문자열 삽입
	void WriteString(wchar_t * const wchData, int iSize) {
		if (_rear + iSize >= _bufferSize) {
			//에러 나거나 버퍼 사이즈 새로 할당
			throw;
		}
		memcpy_s(&_queue[_rear], iSize, wchData, iSize);
		_rear = _rear + iSize;
	}

	//WCHAR 문자열 출력
	void ReadString(wchar_t * wchData, int iSize) {
		char *chpFront = &_queue[_front];

		if (_front + iSize > _rear) {
			//에러 나거나 버퍼 사이즈 새로 할당
			throw;
		}
		memcpy_s(wchData, iSize, chpFront, iSize);
		_front = _front + iSize;
		
		//wchData = (WCHAR *)((char * )wchData + iSize);
		//*(WCHAR *)wchData = NULL;
		//wchData = (WCHAR *)((char *)wchData - iSize);
	}

	//char 문자열 삽입
	void WriteString(char * const chData, int iSize) {
		if (_rear + iSize >= _bufferSize) {
			//에러 나거나 버퍼 사이즈 새로 할당
			throw;
		}
		memcpy_s(&_queue[_rear], iSize, chData, iSize);
		_rear = _rear + iSize;
	}

	//char 문자열 출력
	void ReadString(char * chData, int iSize) {
		char *chpFront = &_queue[_front];

		if (_front + iSize > _rear) {
			//에러 나거나 버퍼 사이즈 새로 할당
			throw;
		}
		memcpy_s(chData, iSize, chpFront, iSize);
		_front = _front + iSize;
	}


	char *GetWriteBufferPtr() {
		return &_queue[_rear];
	}

	char *GetReadBufferPtr() {
		return &_queue[_front];
	}

	void MoveFront(int iSize) {
		int temp;

		temp = _front + iSize;

		// 버퍼 범위 벗어나는 것 예외처리
		if (temp >= _bufferSize)
			temp = _bufferSize - 1;

		if (temp < 0)
			temp = 0;

		_front = temp;

		return;
	}

	void MoveRear(int iSize) {
		int temp;

		temp = _rear + iSize;

		// 버퍼 범위 벗어나는 것 예외처리
		if (temp >= _bufferSize)
			temp = _bufferSize - 1;

		if (temp < 0)
			temp = 0;

		_rear = temp;

		return;
	}

	// 사용중인 버퍼 사이즈
	int GetUseSize() {
		return _rear - _front;
	}

	// 버퍼 사이즈
	int GetBufferSize() {
		return _bufferSize;
	}


protected:
	char * _queue;
	int _rear;
	int _front;
	int _bufferSize;	// 버퍼 크기
	//int _freeSize;		// 사용 가능 크기
	//int _useSize;
};