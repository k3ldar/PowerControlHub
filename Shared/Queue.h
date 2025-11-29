#ifndef __Queue__
#define __Queue__

#include <Arduino.h>
#define INT_MIN 0

struct QItem
{
	int data;
	QItem* next;
	
	QItem(int d)
	{
		data = d;
		next = NULL;
	}
};

class Queue {
private:
    int _capacity, _size;
    QItem *_front, *_rear;
public:
    Queue(int capacity);
    bool isFull();
    bool isEmpty();
    void enqueue(int item);
    int dequeue();
    int average();
};

#endif
