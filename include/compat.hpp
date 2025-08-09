#pragma once

// Disable problematic extended floating-point types for IntelliSense compatibility
#ifndef _GLIBCXX_USE_FLOAT128
#define _GLIBCXX_USE_FLOAT128 0
#endif

#ifndef __GLIBC_USE_FLOAT128
#define __GLIBC_USE_FLOAT128 0
#endif

// Prevent inclusion of problematic float128 definitions
#ifdef __STRICT_ANSI__
#undef __STRICT_ANSI__
#endif

// Force compatibility mode
#define __NO_MATH_INLINES
#define _GNU_SOURCE 0

// Standard includes that should work
#include <cmath>
#include <limits>
#include <algorithm>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <fstream>
#include <sstream>
