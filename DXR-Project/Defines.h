#pragma once

// Macro for dele´ting objects safley
#define SAFEDELETE(Object) if ((Object)) { delete (Object); (Object) = nullptr; } 