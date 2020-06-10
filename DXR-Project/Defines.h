#pragma once

// Macro for deleting objects safley
#define SAFEDELETE(Object) if ((Object)) { delete (Object); (Object) = nullptr; }