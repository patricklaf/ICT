// Stack
// © 2019 Patrick Lafarguette
//
// See https://github.com/zacsketches/Arduino_Vector
// and https://en.wikipedia.org/wiki/Bubble_sort

#ifndef STACK_H
#define STACK_H

#include <Arduino.h>

#ifndef UNUSED
#define UNUSED(X) (void)X
#endif

template<typename T>
void* operator new(size_t size, T* item) {
	UNUSED(size);
	return item;
}

template<typename T> struct Allocator {

	Allocator() {};

	T* allocate(unsigned int count) {
		return reinterpret_cast<T*>(new char[count * sizeof(T)]);
	}

	void deallocate(T* item) {
		delete[] reinterpret_cast<char*>(item);
	}

	void create(T* pointer, const T& item) {
		new (pointer) T(item);
	}
	void destroy(T* item) {
		item->~T();
	}
};

template<class T, class A = Allocator<T> >
class Stack {
private:
	unsigned int _count;
	unsigned int _capacity;

	A _allocator;
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
			_allocator.destroy(&_items[index]);
		}
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
	void push(const T& item);
	void pop();
	void clear();
	void sort(bool (*Compare)(T& a, T& b));
	void dump();

private:
	void swap(const unsigned int a, const unsigned int b);
};

template<class T, class A> Stack<T, A>& Stack<T, A>::operator=(const Stack& stack) {
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

	T* items = _allocator.allocate(stack.count());
	for (unsigned int index = 0; index < stack.count(); ++index) {
		_allocator.create(&items[index], stack[index]);
	}
	for (unsigned int index = 0; index < _count; ++index) {
		_allocator.destroy(&_items[index]);
	}
	_capacity = _count = stack.count();
	_items = items;
	return *this;
}

template<class T, class A> void Stack<T, A>::reserve(unsigned int capacity) {
	if (capacity <= _capacity) {
		return;
	}
	T* pointer = _allocator.allocate(capacity);
	for (unsigned int index = 0; index < _count; ++index) {
		_allocator.create(&pointer[index], _items[index]);
	}
	for (unsigned int index = 0; index < _count; ++index) {
		_allocator.destroy(&_items[index]);
	}
	_allocator.deallocate(_items);
	_items = pointer;
	_capacity = capacity;
}

template<class T, class A>
void Stack<T, A>::push(const T& item) {
	if (_capacity == 0) {
		reserve(4);
	} else if (_count == _capacity) {
		reserve(2 * _capacity);
	}
	_allocator.create(&_items[_count], item);
	++_count;
}

template<class T, class A>
void Stack<T, A>::pop() {
	if (_count > 0) {
		_allocator.destroy(&_items[--_count]);
	}
}

template<class T, class A>
void Stack<T, A>::clear() {
	for (unsigned int index = 0; index < _count; ++index) {
		_allocator.destroy(&_items[index]);
	}
	_count = 0;
}

template<class T, class A>
void Stack<T, A>::sort(bool (*Compare)(T& a, T& b)) {
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

template<class T, class A>
void Stack<T, A>::swap(const unsigned int a, const unsigned int b) {
	T swap = _items[a];
	_items[a] = _items[b];
	_items[b] = swap;
}

template<class T, class A>
void Stack<T, A>::dump() {
	for (unsigned int index = 0; index < _count; ++index) {
		Serial.print(index);
		Serial.print(" : ");
		Serial.println(*_items[index]);
	}
	Serial.println();
	Serial.flush();
}

#endif /* STACK_H */
