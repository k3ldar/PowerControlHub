#pragma once

#include <Arduino.h>

template<typename T>
struct QItem
{
	T data;
	QItem* next;
	
	QItem(T d) : data(d), next(NULL) {}
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
    bool isFull();
    bool isEmpty();
    void enqueue(T item);
    T dequeue();
    T average();
};

// Implementation must be in header for templates
template<typename T>
Queue<T>::Queue(uint8_t capacity, T defaultValue)
{
	_front = NULL;
	_rear = NULL;
    _capacity = capacity;
	_size = 0;
	_defaultValue = defaultValue;
}

template<typename T>
bool Queue<T>::isFull()
{
    return (_size == _capacity);
}

template<typename T>
bool Queue<T>::isEmpty()
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
	
	if (_rear == NULL)
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
	
	if (_front == NULL)
		_rear = NULL;
	
	T value = temp->data;
	delete(temp);
	
    return value;
}

template<typename T>
T Queue<T>::average()
{
    if (isEmpty() || _front == NULL)
	{
        return _defaultValue;
	}
        
    T sum = T(); // Zero-initialized
	QItem<T>* temp = _front;
	
	while (temp != NULL)
	{
		sum += temp->data;
		temp = temp->next;
	}
	
    return (sum / _size);
}
