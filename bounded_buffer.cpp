#include "bounded_buffer.h"

template <class T> void BoundedBuffer<T>::push(T elem)
{
	empty.P();
	elems.push(elem);
	full.V();
}

template <class T> T BoundedBuffer<T>::pop()
{
	full.P();
	T temp = elems.front();
	elems.pop();
	empty.V();

	return temp;
}
