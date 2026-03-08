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
#pragma once

#include <Arduino.h>

template<typename T>
struct QItem
{
	T data;
	QItem* next;
	
	QItem(T d) : data(d), next(nullptr) {}
};

template<typename T>
class Queue {
private:
	uint8_t _capacity;
	uint8_t _size;
	QItem<T>* _front;
	QItem<T>* _rear;
    T _defaultValue;
    
public:
    Queue(uint8_t capacity, T defaultValue = T());
	bool isFull() const;
	bool isEmpty() const;
	void enqueue(T item);
	T dequeue();
	T average() const;
};

// Implementation must be in header for templates
template<typename T>
Queue<T>::Queue(uint8_t capacity, T defaultValue)
{
	_front = nullptr;
	_rear = nullptr;
    _capacity = capacity;
	_size = 0;
	_defaultValue = defaultValue;
}

template<typename T>
bool Queue<T>::isFull() const
{
    return (_size == _capacity);
}

template<typename T>
bool Queue<T>::isEmpty() const
{
    return (_size == 0);
}

template<typename T>
void Queue<T>::enqueue(T item)
{
    if (isFull())
        return;
	
   _size++;
	QItem<T>* temp = new QItem<T>(item);
	
	if (_rear == nullptr)
	{
		_front = _rear = temp;
		return;
	}

    _rear->next = temp;
	_rear = temp;
}

template<typename T>
T Queue<T>::dequeue()
{
    if (isEmpty())
        return _defaultValue;
        
    QItem<T>* temp = _front;
	_front = _front->next;
    _size--;
	
	if (_front == nullptr)
		_rear = nullptr;
	
	T value = temp->data;
	delete(temp);
	
    return value;
}

template<typename T>
T Queue<T>::average() const
{
    if (isEmpty() || _front == nullptr)
	{
        return _defaultValue;
	}
        
    T sum = T(); // Zero-initialized
	QItem<T>* temp = _front;
	
	while (temp != nullptr)
	{
		sum += temp->data;
		temp = temp->next;
	}
	
    return (sum / _size);
}
