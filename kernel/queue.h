#ifndef _QUEUE_H_
#define _QUEUE_H_
#include "libk.h"
#include "machine.h"

class String
{
public:
	int length;
	char* c_string;

	String(const char* c_str) :
		length(K::strlen(c_str)),
		c_string(new char[length + 1])
	{
		memcpy(this->c_string, c_str, this->length + 1);
	}
	bool operator<(const String& rhs) const
	{
		for(int a = 0; a < this->length && a < rhs.length; ++a)
		{
			if(rhs.c_string[a] < this->c_string[a])
			{
				return false;
			}
			else if(this->c_string[a] < rhs.c_string[a])
			{
				return true;
			}
		}

		return this->length <= rhs.length;
	}

	bool operator>(const String& rhs) const
	{
		for(int a = 0; a < this->length && a < rhs.length; ++a)
		{
			if(rhs.c_string[a] > this->c_string[a])
			{
				return false;
			}
			else if(this->c_string[a] > rhs.c_string[a])
			{
				return true;
			}
		}

		return this->length >= rhs.length;
	}

	bool operator==(const String& rhs) const
	{
		for(int a = 0; a < this->length && a < rhs.length; ++a)
		{
			if(rhs.c_string[a] != this->c_string[a])
			{
				return false;
			}
		}

		return this->length == rhs.length;
	}

	char& operator[](int ind)
	{
		return this->c_string[ind];
	}
};

template<typename KEY_TYPE, typename VALUE_TYPE> class Map
{
public:
	class MapNode {
	public:
		KEY_TYPE key;
		VALUE_TYPE value;
		MapNode* leftChild;
		MapNode* rightChild;

		MapNode(KEY_TYPE key) :
			key(key),
			value(value),
			leftChild(nullptr),
			rightChild(nullptr)
		{

		}
	};

	MapNode* root;

	Map() :
		root(nullptr)
	{

	}

	//always creates a key if does not exist.  use find() to check first.
	VALUE_TYPE& operator[](const KEY_TYPE& key)
	{
		if(this->root == nullptr)
		{
			MapNode* newNode = new MapNode(key);
			this->root = newNode;
			return newNode->value;
		}

		MapNode* foundNode = this->Find(this->root, key);

		if(foundNode != nullptr)
		{
			return foundNode->value;
		}

		foundNode = new MapNode(key);
		if(this->root == nullptr)
		{
			this->root = foundNode;
		}
		else
		{
			this->addNode(this->root, foundNode);
		}

		return foundNode->value;
	}

	//to check whether a key exists, call this function directly
	MapNode* Find(MapNode * node, const KEY_TYPE& target)
	{
		if(node == nullptr)
		{
			return nullptr;
		}

		if(target < node->key)
		{
			return Find(node->leftChild, target);
		}
		else if(target > node->key)
		{
			return Find(node->rightChild, target);
		}
		else
		{
			return node;
		}

		return nullptr;
	}

private:
	//root is set elsewhere.
	void addNode(MapNode* node, MapNode* newChild)
	{
		if(newChild->key < node->key)
		{
			if(node->leftChild == nullptr)
			{
				node->leftChild = newChild;
			}
			else
			{
				this->addNode(node->leftChild, newChild);
			}
		}
		else
		{
			if(node->rightChild == nullptr)
			{
				node->rightChild = newChild;
			}
			else
			{
				this->addNode(node->rightChild, newChild);
			}
		}
	}
};

template<typename T> class List
{
public:
	class ListNode
	{
	public:
		ListNode* next;
		ListNode* previous;
		T value;

		ListNode(const T& val) :
			next(nullptr),
			previous(nullptr),
			value(val) {}
	};

	ListNode* first;

	List() :
		first(nullptr)
	{

	}

	inline void Push(const T& v)
	{
		ListNode* newNode = new ListNode(v);

		if(this->first == nullptr)
		{
			this->first = newNode;
		}
		else
		{
			this->first->previous = newNode;
			newNode->next = this->first;
			this->first = newNode;
		}
	}

	inline ListNode* GetHead() {return this->first;}
};

template<typename T> class Queue {
public:
    virtual void addTail(T v) = 0;
    virtual T removeHead()= 0;
    virtual bool isEmpty() = 0;
};

template<typename T> class SimpleQueue : public Queue<T> {
private:
    class Node {
    public:
        Node* next;
        Node* prev;
        T value;
    };

    Node *first;
    Node *last;
    int size;
public:
    virtual ~SimpleQueue(){}
    SimpleQueue() : first(0), last(0), size(0) {}
    inline void addTail(T v) {
        Node *n = new Node();
        n->value = v;
        n->prev = last;
        n->next = 0;
        if (last != 0) {
            last->next = n;
        }
        last = n;
        if (first == 0) {
            first = n;
        }
        ++size;
    }
    inline bool isEmpty() {
        return first == 0;
    }

    inline T peekHead()
    {
    	if(first != nullptr)
    	{
    		return first->value;
    	}

    	return nullptr;
    }

    inline T removeHead() {
        Node *p = first;
        first = p->next;
        if (first == 0) {
            last = 0;
        } else {
            first->prev = 0;
        }
        T v = p->value;
        delete p;
        --size;
        return v;
    }
    
    inline int GetSize() const
    {
    	return this->size;
    }

};

#endif
