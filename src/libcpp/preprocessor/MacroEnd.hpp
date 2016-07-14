/*
 * MacroEnd.hpp
 *
 *  Created on: Apr 30, 2015
 *      Author: fbeier
 */

#ifndef LIBCPP_PREPROCESSOR_MACROEND_HPP_
#define LIBCPP_PREPROCESSOR_MACROEND_HPP_


/**
 * @brief The implementation of a macro that requires a semicolon at the end of the line.
 *
 * This macro declares a non-existing external function and concatenates the
 * function name with the line in the source file in case the macro is used
 * several times within the same file to avoid possible compiler warnings.
 */
#define MACRO_END_( LINE ) \
	extern void anExternalFunctionDeclarationRequiringASemicolonAtTheEndOfTheLine ## LINE(void)

/**
 * @brief A macro which can be used at the end of another macro outside of function scope
 *        in order to require a semicolon after the macro which is ended by this one.
 *
 * @see http://stackoverflow.com/questions/18786848/macro-that-swallows-semicolon-outside-of-function
 */
#define MACRO_END \
	MACRO_END_(__LINE__)


#endif /* LIBCPP_PREPROCESSOR_MACROEND_HPP_ */
