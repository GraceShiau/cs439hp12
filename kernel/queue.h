#ifndef _QUEUE_H_
#define _QUEUE_H_

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
			value(value) {}
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
    void addTail(T v) {
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
    bool isEmpty() {
        return first == 0;
    }
    T removeHead() {
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
    
    int GetSize() const
    {
    	return this->size;
    }

};

#endif
