// This file is part of the course TPV2@UCM - Samir Genaim

#pragma once

/*
 * The compiler defines a preprocessor variable when exceptions are disabled.
 *
 *  _CPPUNWIND for Visual Studio
 *  __EXCEPTIONS for g++
 *  _LIBCPP_NO_EXCEPTIONS is the one used in osx libraries of g++ (maybe redundant)
 */
#if defined __EXCEPTIONS | defined _LIBCPP_NO_EXCEPTIONS | defined _CPPUNWIND
#define _NO_EXCEPTIONS
#endif


/*
 * This function is used to throw a "const char*". It should be used
 * instead of throw, so you can easily disable exception by replacing
 * them with abort.
 *
 */
inline void __throw(const char *msg) {
#ifndef _NO_EXCEPTIONS
	throw msg;
#else
	abort();
#endif
}
