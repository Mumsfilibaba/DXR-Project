#pragma once

// Macro for deleting objects safley
#define SAFEDELETE(Object)  if ((Object)) { delete (Object); (Object) = nullptr; }

// Zero memory
#define ZERO_MEMORY(Object, SizeInBytes) memset(Object, 0, SizeInBytes)