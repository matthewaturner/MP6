#include <queue>
#include "semaphore.h"

template <class T> class BoundedBuffer {
public:
	BoundedBuffer(int n)
		: full{0}, empty{n}, elems{} {}
		
	void push(T elem);
	T pop();
private:
	Semaphore full, empty;
	std::queue<T> elems;
};
