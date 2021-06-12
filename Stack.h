// Stack
// © 2021 Patrick Lafarguette

#ifndef STACK_H_
#define STACK_H_

#include <Arduino.h>

template<class T>
class Stack {
private:
	unsigned int _count;
	unsigned int _capacity;

	T* _items;

	Stack(const Stack&);

public:
	Stack() :
			_count(0), _capacity(0), _items(0) {
	}

	Stack(const int capacity) :
			_count(0) {
		reserve(capacity);
	}

	Stack& operator=(const Stack&);

	~Stack() {
		for (unsigned int index = 0; index < _count; ++index) {
			free(_items[index]);
		}
		free(_items);
	}

	T& operator[](unsigned int index) {
		return _items[index];
	}

	const T& operator[](unsigned int index) const {
		return _items[index];
	}

	unsigned int count() const {
		return _count;
	}

	unsigned int capacity() const {
		return _capacity;
	}

	void reserve(unsigned int capacity);
	void push(const T item);
	void pop();
	void clear();
	void sort(bool (*Compare)(T& a, T& b));
	void dump();

private:
	void swap(const unsigned int a, const unsigned int b);
};

template<class T> Stack<T>& Stack<T>::operator=(const Stack& stack) {
	if (this == &stack) {
		return *this;
	}
	if (stack.count() <= _capacity) {
		for (unsigned int index = 0; index < stack._count(); ++index) {
			_items[index] = stack[index];
		}
		_count = stack._count();
		return *this;
	}

	T* items = malloc(stack.count() * sizeof(T));
	for (unsigned int index = 0; index < stack.count(); ++index) {
		items[index] = stack[index];
	}
	for (unsigned int index = 0; index < _count; ++index) {
		delete _items[index];
	}
	_capacity = _count = stack.count();
	_items = items;
	return *this;
}

template<class T> void Stack<T>::reserve(unsigned int capacity) {
	if (capacity <= _capacity) {
		return;
	}
	T* pointer = (T*)malloc(capacity * sizeof(T));
	for (unsigned int index = 0; index < _count; ++index) {
		pointer[index] = _items[index];
	}
	free(_items);
	_items = pointer;
	_capacity = capacity;
}

template<class T>
void Stack<T>::push(const T item) {
	if (_capacity == 0) {
		reserve(4);
	} else if (_count == _capacity) {
		reserve(2 * _capacity);
	}
	_items[_count] = item;
	++_count;
}

template<class T>
void Stack<T>::pop() {
	if (_count > 0) {
		delete(_items[--_count]);
	}
}

template<class T>
void Stack<T>::clear() {
	for (unsigned int index = 0; index < _count; ++index) {
		delete(_items[index]);
	}
	_count = 0;
}

template<class T>
void Stack<T>::sort(bool (*Compare)(T& a, T& b)) {
	if (_count) {
		unsigned int last = _count;
		do {
			unsigned int next = 0;
			for (unsigned int index = 1; index < last; ++index) {
				if (Compare(_items[index - 1], _items[index])) {
					// Swap
					swap(index - 1, index);
					next = index;
				}
			}
			last = next;
		} while (last > 1);
	}
}

template<class T>
void Stack<T>::swap(const unsigned int a, const unsigned int b) {
	T swap = _items[a];
	_items[a] = _items[b];
	_items[b] = swap;
}

template<class T>
void Stack<T>::dump() {
	for (unsigned int index = 0; index < _count; ++index) {
		Serial.print(index);
		Serial.print(" : ");
		Serial.println(*_items[index]);
	}
	Serial.println();
	Serial.flush();
}

#endif /* STACK_H_ */
