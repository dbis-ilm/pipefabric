/*
 * FunctionTraits.hpp
 *
 *  Created on: Mar 15, 2015
 *      Author: fbeier
 */

#ifndef LIBCPP_TYPES_TRAITS_FUNCTIONTRAITS_HPP_
#define LIBCPP_TYPES_TRAITS_FUNCTIONTRAITS_HPP_

#include <tuple>

namespace ns_types {

/**
 * @brief static properties of function(-like) types.
 *
 * @see https://functionalcpp.wordpress.com/2013/08/05/function-traits/
 *
 * @tparam Function
 *    the function type whose traits are defined
 */
template< typename Function >
class FunctionTraits;


/**
 * @brief Specialization for a free function.
 *
 * @tparam ReturnType
 *    the return type of the function
 * @tparam Arguments
 *    a list with all arguments for the function
 */
template<
	typename _ReturnType,
	typename... Arguments
>
class FunctionTraits< _ReturnType( Arguments... ) > {
public:

	//////   public constants   //////

	/// the function arity, i.e., its number of arguments
	static constexpr std::size_t ARITY = sizeof...( Arguments );


	//////   public types   //////

	/// the return type of the function
	using ReturnType = _ReturnType;

	/**
	 * @brief Meta function extracting the type of a specific argument.
	 *
	 * @tparam ParameterIdx
	 *    the index of the requested argument (must be less than ARITY)
	 */
	template< std::size_t ParameterIdx >
	class ArgumentType {
		static_assert( ParameterIdx < ARITY, "error: invalid argument index" );
		typedef std::tuple< Arguments... > ArgumentTuple;
		using type = typename std::tuple_element< ParameterIdx, ArgumentTuple >::type;
	};
};

/**
 * @brief Specialization for a free function pointer.
 *
 * @tparam ReturnType
 *    the return type of the function
 * @tparam Arguments
 *    a list with all arguments for the function
 */
template<
	typename ReturnType,
	typename... Arguments
>
class FunctionTraits< ReturnType(*)(Arguments...) > :
	public FunctionTraits< ReturnType(Arguments...) >
{};


/**
 * @brief Specialization for a member function pointer.
 *
 * @tparam Class
 *    the class containing the member function
 * @tparam ReturnType
 *    the return type of the function
 * @tparam Arguments
 *    a list with all arguments for the function
 */
template<
	typename Class,
	typename ReturnType,
	typename... Arguments
>
class FunctionTraits< ReturnType(Class::*)( Arguments... ) > :
	public FunctionTraits< ReturnType( Class&, Arguments... ) >
{};

/**
 * @brief Specialization for a const member function pointer.
 *
 * @tparam Class
 *    the class containing the member function
 * @tparam ReturnType
 *    the return type of the function
 * @tparam Arguments
 *    a list with all arguments for the function
 */
template<
	typename Class,
	typename ReturnType,
	typename... Arguments
>
class FunctionTraits< ReturnType(Class::*)( Arguments... ) const > :
	public FunctionTraits< ReturnType( Class&, Arguments... ) >
{};

/**
 * @brief Specialization for a member object pointer.
 *
 * @tparam Class
 *    the class containing the member function
 * @tparam ReturnType
 *    the return type of the function
 */
template<
	class Class,
	class ReturnType
>
class FunctionTraits< ReturnType( Class::* ) > :
	public FunctionTraits< ReturnType( Class& ) >
{};


/**
 * @brief Default implementation for functors.
 *
 * @tparam Functor
 *    a functor type
 */
template<
	typename Functor
>
class FunctionTraits {
private:

	/// type of the functor invocation
	using CallType = FunctionTraits< decltype( &Functor::type::operator()) >;

public:

	//////   public constants   //////

	/// the function arity, i.e., its number of arguments
	static constexpr std::size_t ARITY = CallType::ARITY - 1;

	using ReturnType = typename CallType::ReturnType;


	/**
	 * @brief Meta function extracting the type of a specific argument.
	 *
	 * @tparam ParameterIdx
	 *    the index of the requested argument (must be less than ARITY)
	 */
	template< std::size_t ParameterIdx >
	class ArgumentType {
		static_assert( ParameterIdx < ARITY, "error: invalid argument index" );
		using type = typename CallType::template argument< ParameterIdx + 1 >::type;
	};
};


/**
 * @brief Specialization for a lvalue reference to a function type
 *
 * @tparam Function
 *    the function type
 */
template<
	typename Function
>
class FunctionTraits< Function& > :
	public FunctionTraits< Function >
{};

/**
 * @brief Specialization for a rvalue reference to a function type
 *
 * @tparam Function
 *    the function type
 */
template<
	typename Function
>
class FunctionTraits< Function&& > :
	public FunctionTraits< Function >
{};

} /* end namespace ns_types */


#endif /* LIBCPP_TYPES_TRAITS_FUNCTIONTRAITS_HPP_ */
