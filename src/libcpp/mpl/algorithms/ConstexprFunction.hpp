/*
 * Copyright (C) 2014-2021 DBIS Group - TU Ilmenau, All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PipeFabric is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with PipeFabric. If not,
 * see <http://www.gnu.org/licenses/>.
 */

/*
 * ConstexprFunction.hpp
 *
 *  Created on: Jul 10, 2014
 *      Author: Felix Beier <felix.beier@tu-ilmenau.de>
 */

#ifndef LIBCPP_MPL_ALGORITHMS_CONSTEXPRFUNCTION_HPP_
#define LIBCPP_MPL_ALGORITHMS_CONSTEXPRFUNCTION_HPP_

#include "libcpp/mpl/Forward.hpp"

// limit for recursive constexpr function invocations
// since an exponential recursion algorithm is used there will be 2^limit
// invocations at max
#define DEFAULT_MAX_CONSTEXPR_FUNCTION_DEPTH 100


namespace ns_mpl {

/// a type indicating a recursion depth
typedef unsigned int RecursionDepth;


/**
 * @brief Traits class for constant expression function objects.
 *
 * This traits class defines the interface all constant expression function
 * objects must adhere to in order to be invokable by @c constexprFunction().
 *
 * An instance of a @c ConstexprFunction is a "state" of a (recursive) function
 * invocation, storing all parameters and (intermediate) results computed so far.
 * The required traits that must be satisfied for the @c ConstexprFunction
 * parameter type are:
 *    - <b>nested types:</b>
 *      - @c ResultType the type of the @c ConstexprFunction's result
 *    - <b>nested functions:</b>
 *      - @c isDone():
 *           a boolean constexpr function returning
 *           @c true if the computation for the function is done and hence
 *                   recursion can be stopped
 *           @c false otherwise
 *      - @c getResult():
 *           a function returning the result of type @c ResultType accumulated
 *           in the object so far
 *      - @c eval():
 *           the invocation of the reduction function (recursively) leading to
 *           the base case which returns @c true via @c isDone()
 *      - All nested functions require an instance of @c ConstexprFunction as
 *        argument, indicating the progress of the recursive calculation.
 *
 * This default implementation extracts a nested @c ResultType from the
 * @c ConstexprFunction type and forwards all method invocations to the
 * @c ConstexprFunction instance provided as argument. If another behavior
 * is required for another type, this traits template can be overridden with
 * a providing a new specialization for that type.
 *
 * @see constexprFunction().
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 *
 * @tparam ConstexprFunction
 *     the actual constexpr function object type whose traits are defined here
 */
template<
	typename ConstexprFunction
>
struct ConstexprFunctionTraits {

	/// the function's result type
	typedef typename ConstexprFunction::ResultType ResultType;

	/**
	 * @brief Check if the calculation is done for the @c ConstexprFunction instance.
	 *
	 * This method will be used to determine the end of the recursive calculation.
	 * It just forwards the call to the @arg function instance.
	 *
	 * @param[in] function
	 *     the constexpr function object instance
	 * @return
	 *     @c true in case the calculation is done
	 *     @c false otherwise
	 */
	static constexpr bool isDone( const ConstexprFunction& function ) {
		return function.isDone();
	}

	/**
	 * @brief Obtain the constexpr function result computed so far.
	 *
	 * This method will be used to extract the function result, usually after the
	 * end of the recursive calculation.
	 * It just forwards the call to the @arg function instance.
	 *
	 * @param[in] function
	 *     the constexpr function object instance
	 * @return
	 *     the result accumulated in @c function so far
	 */
	static constexpr ResultType getResult( const ConstexprFunction& function ) {
		return function.getResult();
	}

	/**
	 * @brief Inoke the constexpr function.
	 *
	 * This method represents a (recursive) invocation of the @arg function object,
	 * i.e., the next computation step. It returns a new (reduced) function instance
	 * which either represents a base case (@c isDone == @c true) or another
	 * general case (@c isDone == @c false) which can be further reduced by
	 * another invocation.
	 *
	 * @param[in] function
	 *     the constexpr function object instance
	 * @return
	 *     the reduced function object instance for the next invocation
	 */
	static constexpr ConstexprFunction eval( const ConstexprFunction& function ) {
		return function();
	}
};


namespace impl {

/**
 * @brief Recursively invoke a constexpr functor.
 *
 * This method recursively invokes a @c constexpr functor in order to calculate
 * its result. A binary recursion tree is formed in order to limit the recursion
 * depth that is actually instantiated. The C++ standard puts a limit on this
 * which is at least @c 512. The <b>trampolin technique</b> is used here.
 * See the link below to get further information about this.
 *
 * The actual recursion depth of each tree branch that is reached is limited by
 * the @c MaxRecursionDepth parameter. The number of @c constexpr evaluation steps
 * might be additionally limited by the compiler in order to avoid infinite
 * recursions. See @c constexprFunction() for further comments on this.
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 * @see http://fendrich.se/blog/2012/11/22/compile-time-loops-in-c-plus-plus-11-with-trampolines-and-exponential-recursion/
 *
 * @tparam MaxRecursionDepth
 *     the maximum depth of the recursion tree for invoking the functor
 * @tparam ConstexprFunction
 *     a type of a constexpr function object that shall be executed,
 *     must satisfy the @c ConstexprFunctionTraits
 * @param[in] function
 *     the @c constexpr functor that shall be calculated
 * @param[in] currentRecursionDepth
 *     the current depth in the recursion tree
 * @return a new @c constexpr functor which was recursively reduced
 */
template<
	RecursionDepth MaxRecursionDepth,
	typename ConstexprFunction
	// TODO is it more efficient to pass current depth here too?
>
constexpr ConstexprFunction apply( const ConstexprFunction function,
		const RecursionDepth currentRecursionDepth ) {
	typedef ConstexprFunctionTraits< ConstexprFunction > Function;
	return Function::isDone( function ) ?
		// if the calculation is done, return the function object itself
		// it contains the final result
		function :
		( currentRecursionDepth == MaxRecursionDepth ?
			// if we reached the maximum recursion depth for this branch
			// just evaluate the function and return the result
			Function::eval( function ) :
			// if we did not reached maximum recursion depth, spawn two recursion
			// branches with next depth, evaluating the functor
			apply< MaxRecursionDepth >(
				apply< MaxRecursionDepth >(
					Function::eval( function ), currentRecursionDepth+1
				),
				currentRecursionDepth+1
			)
		)
	;
}

} /* end namespace impl */


/**
 * @brief Calculate the result of a constexpr functor.
 *
 * This method can be used to recursively compute the result of a @c constexpr
 * function object of type @c ConstexprFunction. First, A @c ConstexprFunction
 * instance will be created forwarding @arg parms to the functor's constructor.
 * Second, the result is calculated with recursively invoking the functor on
 * its previous result in @c impl::apply until the computation is done.
 * The result accumulated in the final call will be returned. Recursion is
 * needed here, since loops are not allowed in @c constexpr expressions.
 *
 * The @c ConstexprFunction type passed as argument must satisfy the
 * @c ConstexprFunctionTraits. The maximum depth of the recursion tree is limited
 * by the @c MaxRecursionDepth. Use the other @c constexprFunction() for using
 * a default value.
 *
 * @note Some compilers (as clang) may limit the absolute number of steps executed
 *       (which increase exponentially with the recursion depth) in a @c constexpr
 *       evaluation. In that case, compilation may fail before the maximum recursion
 *       depth is reached. @see http://stackoverflow.com/questions/24591466/constexpr-depth-limit-with-clang-fconstexpr-depth-doesnt-seem-to-work
 *
 * TODO Maybe we can verify that the function is actually marked as done before
 *      returning its result. We could throw an exception in this case, aborting
 *      compilation or in the runtime-evaluation case giving us the opportunity
 *      to react on this error. This behavior might be configured with an additional
 *      template flag or via the @c ConstexprFunctionTraits.
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 * @see http://fendrich.se/blog/2012/11/22/compile-time-loops-in-c-plus-plus-11-with-trampolines-and-exponential-recursion/
 *
 * @tparam MaxRecursionDepth
 *     the maximum depth of the recursion tree for invoking the functor
 * @tparam ConstexprFunction
 *     a type of a constexpr function object that shall be executed,
 *     must satisfy the @c ConstexprFunctionTraits
 * @tparam Parameters
 *     the types of the initial parameters for constructing the
 *     @c ConstexprFunction instance
 * @param[in] parms
 *     the initial parameters for the function invocation
 * @return
 *     the result which computed by recursively applying the @c ConstexprFunction
 *     until an @c ConstexprFunction instance indicates that calculation is done
 */
template<
	RecursionDepth MaxRecursionDepth,
	typename ConstexprFunction,
	typename... Parameters
>
inline constexpr typename ConstexprFunctionTraits< ConstexprFunction >::ResultType
constexprFunction( Parameters... parms ) {
	typedef ConstexprFunctionTraits< ConstexprFunction > Function;
	using ns_mpl::forward; // std::forward fix for constexpr

	// TODO verify done and throw otherwise, i.e., recursion limit exceeded?
	return Function::getResult(
		impl::apply<MaxRecursionDepth>(
				ConstexprFunction( forward< Parameters >(parms)... ), 0
		)
	);
}


/**
 * @brief Calculate the result of a constexpr functor.
 *
 * This is an overload using a default value for the maximum depth of the
 * recursion tree. This cannot be a default template parameter since we have
 * a variadic parameter list for forwarding parameters to the functor's constructor.
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 *
 * @tparam ConstexprFunction
 *     a type of a constexpr function object that shall be executed,
 *     must satisfy the @c ConstexprFunctionTraits
 * @tparam Parameters
 *     the types of the initial parameters for constructing the
 *     @c ConstexprFunction instance
 * @param[in] parms
 *     the initial parameters for the function invocation
 * @return
 *     the result which computed by recursively applying the @c ConstexprFunction
 *     until an @c ConstexprFunction instance indicates that calculation is done
 */
template<
	typename ConstexprFunction,
	typename... Parameters
>
inline constexpr typename ConstexprFunctionTraits< ConstexprFunction >::ResultType
constexprFunction( Parameters... parms ) {
	using ns_mpl::forward; // std::forward fix for constexpr
	return constexprFunction<
		DEFAULT_MAX_CONSTEXPR_FUNCTION_DEPTH,
		ConstexprFunction
	>( forward< Parameters >(parms)... );
}

} /* end namespace ns_mpl */


#endif /* LIBCPP_MPL_ALGORITHMS_CONSTEXPRFUNCTION_HPP_ */
