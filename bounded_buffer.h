#include <list>
#include "semaphore.h"

/*---------------------------------------------------------------------------*/
/* Definition                                                                */
/*---------------------------------------------------------------------------*/

template <class T> class BoundedBuffer {
public:
	BoundedBuffer(int n)
		: full{0}, empty{n}, elems{} {}
		
	void push(T elem);
	T pop();
	int size() { return elems.size(); }
private:
	Semaphore full, empty;
	std::list<T> elems;
};

/*---------------------------------------------------------------------------*/
/* Functions                                                                 */ 
/*---------------------------------------------------------------------------*/

template <class T> void BoundedBuffer<T>::push(T elem)
{
	empty.P();
	elems.push_back(elem);
	full.V();
}

template <class T> T BoundedBuffer<T>::pop()
{
	full.P();
	T temp = elems.front();
	elems.pop_front();
	empty.V();

	return temp;
}
