/*
 * SmartFuseBox
 * Copyright (C) 2025 Simon Carter (s1cart3r@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
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
