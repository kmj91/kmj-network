#pragma once

template <typename T>
class CList
{
public:
	struct Node
	{
		T _data;
		Node *_Prev;
		Node *_Next;
	};

	class iterator
	{
	private:
	public:
		Node *_node;
	public:
		iterator(Node *node = nullptr)
		{
			//인자로 들어온 Node 포인터를 저장
			this->_node = node;
		}

		//전위 증가
		iterator& operator ++()
		{
			//현재 노드를 다음 노드로 이동
			_node = _node->_Next;
			return (*this);
		}

		//후위 증가
		iterator operator ++(int)
		{
			//현재 노드를 다음 노드로 이동
			const iterator temp = *this;
			++*this;
			return (temp);
		}

		//전위 감소
		iterator& operator--()
		{
			_node = _node->_Prev;
			return (*this);
		}

		//후위 감소
		iterator operator--(int)
		{
			const iterator temp = *this;
			--*this;
			return (temp);
		}

		//같은감?
		bool operator==(const iterator& _Right) const
		{
			return (_node == _Right._node);
		}

		//안같은감?
		bool operator!=(const iterator& _Right) const
		{
			return (!(*this == _Right));
		}

		//현제 노드의 데이터 뽑음
		T& operator *() const
		{
			return (_node->_data);
		}

		//되려나?
		Node* operator->() const {
			return _node;
		}
	};

public:
	CList();
	~CList();

	iterator begin()
	{
		//첫번째 노드를 가리키는 이터레이터 리턴
		return iterator(_head->_Next);
	}
	iterator end()
	{
		//Tail 노드를 가리키는(데이터가 없는 진짜 끝 노드) 이터레이터를 리턴
		//	또는 끝으로 인지할 수 있는 이터레이터를 리턴
		return iterator(_tail);
	}

	void initNode(Node *ptr);
	void push_front(T data);
	void push_back(T data);
	void clear();
	void delete_Node(T data);
	int size();
	bool is_empty();

	iterator erase(iterator itor) {
		//	- 이터레이터의 그 노드를 지움.
		//	- 그리고 지운 노드의 다음 노드를 카리키는 이터레이터 리턴
		//		* 지운 노드의 다음 노드가 아니라 이전 노드
		Node *node = _head->_Next;
		Node *temp = nullptr;
		while (node != _tail) {
			if (node == itor._node) {
				temp = node->_Next;
				temp->_Prev = node->_Prev;
				temp = node->_Prev;
				temp->_Next = node->_Next;
				delete node;
				--_size;
				return temp;
			}
			node = node->_Next;
		}
		return nullptr;
	}
private:
	int _size = 0;
	Node *_head;
	Node *_tail;
};


//생성자
template <typename T>
CList<T>::CList() {
	_head = new Node;
	initNode(_head);
	_tail = new Node;
	initNode(_tail);
	_head->_Next = _tail;
	_tail->_Prev = _head;
}

//파괴자
template <typename T>
CList<T>::~CList() {
	clear();
	delete _head;
	delete _tail;
}

//노드 초기화
template <typename T>
void CList<T>::initNode(Node *ptr) {
	ptr->_Prev = NULL;
	ptr->_Next = NULL;
}

//헤드 뒤 삽입
template <typename T>
void CList<T>::push_front(T data) {
	Node *node = new Node;
	if (_head->_Next != _tail) {
		node->_data = data;
		node->_Prev = _head;
		node->_Next = _head->_Next;
	}
	else {
		node->_data = data;
		node->_Prev = _head;
		node->_Next = _tail;
		_tail->_Prev = node;
	}
	_head->_Next = node;
	++_size;
}

//꼬리 앞 삽입
template <typename T>
void CList<T>::push_back(T data) {
	Node *node = new Node;
	Node *temp = _tail->_Prev;
	node->_data = data;
	node->_Prev = _tail->_Prev;
	node->_Next = _tail;
	
	temp->_Next = node;
	_tail->_Prev = node;
	++_size;
}

//전부 삭제
template <typename T>
void CList<T>::clear() {
	Node *node = _head->_Next;
	Node *temp = _head;
	while (node != _tail) {
		temp->_Next = node->_Next;
		delete node;
		node = temp->_Next;
		--_size;
	}
}

//해당 노드 삭제
template <typename T>
void CList<T>::delete_Node(T data) {
	Node *node = _head->_Next;
	Node *temp;
	while (node != _tail) {
		if (node->_data == data) {
			temp = node->_Prev;
			temp->_Next = node->_Next;
			temp = node->_Next;
			temp->_Prev = node->_Prev;
			delete node;
			--_size;
			return;
		}
		node = node->_Next;
	}
}

template <typename T>
int CList<T>::size() {
	return _size;
}

template <typename T>
bool CList<T>::is_empty() {
	if (_size == 0) {
		return true;
	}
	else {
		return false;
	}
}