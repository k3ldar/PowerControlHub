#include "Queue.h"

Queue::Queue(uint8_t capacity)
{
	_front = NULL;
	_rear = NULL;
    _capacity = capacity;
	_size = 0;
}
 
// Queue is full when size
// becomes equal to the capacity
bool Queue::isFull()
{
    return (_size == _capacity);
}
 
// Queue is empty when size is 0
bool Queue::isEmpty()
{
    return (_size == 0);
}

void Queue::enqueue(QueueDataType item)
{
    if (isFull())
        return;
	
   _size++;
	QItem* temp = new QItem(item);
	
	if (_rear == NULL)
	{
		_front = _rear = temp;
		return;
	}

    _rear->next = temp;
	_rear = temp;
}
 
QueueDataType Queue::dequeue()
{
    if (isEmpty())
        return QUEUE_DEFAULT_VALUE;
        
    QItem* temp = _front;
	_front = _front->next;
    _size--;
	
	if (_front == NULL)
		_rear = NULL;
	
	QueueDataType value = temp->data;
	delete(temp);
	
    return value;
}
 
QueueDataType Queue::average()
{
    if (isEmpty() || _front == NULL)
	{
        return QUEUE_DEFAULT_VALUE;
	}
        
    QueueDataType sum = 0;
	QItem* temp = _front;
	
	while (temp != NULL)
	{
		sum += temp->data;
		temp = temp->next;
	}
	
    return (sum / _size);
}
