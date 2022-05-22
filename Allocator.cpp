#include "Allocator.h"

// Some kind of template test by template instantiation:
extern template class SeparateHeapAllocator<char>;
extern template class SeparateHeapAllocator<int>;
extern template class SeparateHeapAllocator<void*>;
