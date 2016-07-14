/*
 * StreamElementTraits.hpp
 *
 *  Created on: Feb 13, 2015
 *      Author: fbeier
 */

#ifndef STREAMELEMENTTRAITS_HPP_
#define STREAMELEMENTTRAITS_HPP_

// #include "PipeFabricTypes.hpp"
#include "PFabricTypes.hpp"

#include <type_traits>
#include <cassert>
#include <boost/mpl/not.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/mpl/assert.hpp>

#include "libcpp/types/types.hpp"

namespace pfabric {


namespace impl {

template<
	typename StreamElementType,
	class Enable = void
>
class StreamElementImplTraits;

/**
 * @brief StreamElementImplTraits implementation for pointer element types.
 *
 * This specialization is selected if a pointer type is passed as argument.
 *
 * @tparam StreamElementType
 *    a pointer stream element type whose traits are defined here
 */
template<
	typename StreamElementType
>
class StreamElementImplTraits<
	StreamElementType,
	typename boost::enable_if<
		typename ns_types::PointerTraits< StreamElementType >::isPointer
	>::type
>
{
private:

	/// the pointer interface implemented by the stream element
	typedef ns_types::PointerTraits< StreamElementType > PointerInterface;

	/// the actual type the stream element points to
	typedef typename PointerInterface::ElementType PointedElementType;

	/// the actual stream element type without any modifiers
	typedef typename std::remove_cv<
		typename std::remove_reference< StreamElementType >::type
	>::type StreamElement;

public:

	/**
	 * @brief Create a new stream element from a list of attributes.
	 *
	 * This method constructs a new element of the underlying implementation and returns
	 * the new element instance.
	 *
	 * @tparam Attributes
	 *    the attribute types which are used to construct the new element
	 * @param[in] attributes
	 *    the list of attributes which shall form the new element instance
	 * @return the new element instance
	 */
	template< typename... Attributes >
	static StreamElement create( Attributes&&... attributes ) {
		return ns_types::allocatePointer< StreamElement >( std::forward< Attributes >( attributes )... );
	}

	/**
	 * @brief Meta function returning the type of a specific stream element attribute.
	 *
	 * @tparam ID
	 *    the ID of the requested attribute, must be in [0 ... NUM_ATTRIBUTES)
	 */
	template< AttributeIdx ID >
	struct getAttributeType {
		BOOST_STATIC_ASSERT_MSG( ID < PointedElementType::NUM_ATTRIBUTES, "illegal attribute ID" );
		typedef typename PointedElementType::template getAttributeType< ID >::type type;
	};

	/**
	 * @brief Get the number of attributes the element comprises.
	 *
	 * @return the number of attributes of the element
	 */
	static constexpr TupleSize getNumAttributes() {
		return PointedElementType::NUM_ATTRIBUTES;
	}

	/**
	 * @brief Get a reference to the underlying stream element.
	 *
	 * For pointer types, the pointed element is returned.
	 *
	 * @param[in] element
	 *    a reference to the stream element
	 * @return a reference to the stream element
	 */
	static PointedElementType& getElementRef( StreamElement& element ) {
		assert( !ns_types::isNullPointer( element ) );
		return *element;
	}

	/**
	 * @brief Get a const reference to the underlying stream element.
	 *
	 * For pointer types, the pointed element is returned.
	 *
	 * @param[in] element
	 *    a reference to the stream element
	 * @return a reference to the stream element
	 */
	static const PointedElementType& getElementRef( const StreamElement& element ) {
		assert( !ns_types::isNullPointer( element ) );
		return *element;
	}
};

/**
 * @brief StreamElementTraits implementation for non-pointer element types.
 *
 * This specialization is selected if not a pointer type is passed as argument.
 *
 * @tparam StreamElementType
 *    a non-pointer stream element type whose traits are defined here
 */
template<
	typename StreamElementType
>
class StreamElementImplTraits<
	StreamElementType,
	typename boost::enable_if<
		boost::mpl::not_<
			typename ns_types::PointerTraits< StreamElementType >::isPointer
		>
	>::type
>
{
	/// the actual stream element type without any modifiers
	typedef typename std::remove_cv<
		typename std::remove_reference< StreamElementType >::type
	>::type StreamElement;

public:

	/**
	 * @brief Create a new stream element from a list of attributes.
	 *
	 * TODO Felix:
	 *      This is not always so easily possible. If, e.g., views are used, the corresponding
	 *      elements cannot be created that easily. That's why the joins have a separate TupleFactor
	 *      parameter responsible for that. I suggest to pull the entire creation logic
	 *      out of the element traits using a factory pattern and only keep attribute access
	 *      and modification logic inside this class. The latter can be uniformly used
	 *      over all implementations, even on views.
	 *
	 * This method constructs a new element of the underlying implementation and returns
	 * the new element instance.
	 *
	 * @tparam Attributes
	 *    the attribute types which are used to construct the new element
	 * @param[in] attributes
	 *    the list of attributes which shall form the new element instance
	 * @return the new element instance
	 */
	template< typename... Attributes >
	static StreamElement create( Attributes&&... attributes ) {
		return StreamElement( std::forward< Attributes >( attributes )... );
	}

	/**
	 * @brief Meta function returning the type of a specific stream element attribute.
	 *
	 * @tparam ID
	 *    the ID of the requested attribute, must be in [0 ... NUM_ATTRIBUTES)
	 */
	template< AttributeIdx ID >
	struct getAttributeType {
		BOOST_STATIC_ASSERT_MSG( ID < StreamElement::NUM_ATTRIBUTES, "illegal attribute ID" );
		typedef typename StreamElement::template getAttributeType< ID >::type type;
	};

	/**
	 * @brief Get the number of attributes the element comprises.
	 *
	 * @return the number of attributes of the element
	 */
	static constexpr TupleSize getNumAttributes() {
		return StreamElement::NUM_ATTRIBUTES;
	}

	/**
	 * @brief Get a reference to the underlying stream element.
	 *
	 * For non-pointer types, the element reference is returned.
	 *
	 * @param[in] element
	 *    a reference to the stream element
	 * @return a reference to the stream element
	 */
	static StreamElement& getElementRef( StreamElement& element ) {
		return element;
	}

	/**
	 * @brief Get a reference to the underlying stream element.
	 *
	 * For non-pointer types, the element reference is returned.
	 *
	 * @param[in] element
	 *    a reference to the stream element
	 * @return a reference to the stream element
	 */
	static const StreamElement& getElementRef( const StreamElement& element ) {
		return element;
	}
};


} /* end namespace impl */


/**
 * @brief Traits defining the interface of data stream elements that are handled by PipeFabric.
 *
 * This class defines the interface for all data elements that flow through an operator
 * graph representing a PipeFabric query. Each operator uses this interface to access
 * the properties of the elements like timestamps and the actual data.
 * Stream elements are considered to be tuples consisting of a fixed number of attributed.
 *
 * TODO Should we model simple elements and complex elements?
 *
 * @tparam StreamElementType
 *    the stream element implementation for which the traits are defined
 *
 * Specialize the class for all StreamElement implementations that do not implement the
 * default interface.
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
template<
	typename StreamElementType
>
class StreamElementTraits {
private:

	/// the actual interface for the StreamElement whose traits are defined here
	typedef impl::StreamElementImplTraits< StreamElementType > ElementTraits;

public:

	//////   public constants   //////

	/// the number of attributes in the element
	static const TupleSize NUM_ATTRIBUTES = ElementTraits::getNumAttributes();


	//////   public types   //////

	/// the element type for which the traits are defined
	typedef StreamElementType StreamElement;

	/**
	 * @brief Meta function returning the type of a specific stream element attribute.
	 *
	 * @tparam ID
	 *    the ID of the requested attribute, must be in [0 ... NUM_ATTRIBUTES)
	 */
	template< AttributeIdx ID >
	struct getAttributeType {
		BOOST_STATIC_ASSERT_MSG( ID < NUM_ATTRIBUTES, "illegal attribute ID" );
		typedef typename ElementTraits::template getAttributeType< ID >::type type;
	};


	//////   public interface   //////

	////// instance creation

	/**
	 * @brief Create a new stream element from a list of attributes.
	 *
	 * This method constructs a new element of the underlying implementation and returns
	 * the new element instance.
	 *
	 * @tparam Attributes
	 *    the attribute types which are used to construct the new element
	 * @param[in] attributes
	 *    the list of attributes which shall form the new element instance
	 * @return the new element instance
	 */
	template< typename... Attributes >
	static StreamElement create( Attributes&&... attributes ) {
		return ElementTraits::template create( std::forward< Attributes >( attributes )... );
	}


	////// attribute access

	/**
	 * @brief Get the number of attributes the element comprises.
	 *
	 * @return the number of attributes of the element
	 */
	static constexpr TupleSize getNumAttributes() {
		return NUM_ATTRIBUTES;
	}

	/**
	 * @brief Get a specific attribute value from the stream element.
	 *
	 * @tparam ID
	 *    the ID of the requested attribute, must be in [0 ... NUM_ATTRIBUTES)
	 * @param[in] element
	 *    the element whose attribute shall be returned
	 * @return a reference to the element's attribute with the requested @c ID
	 */
	template< AttributeIdx ID >
	static typename getAttributeType< ID >::type& getAttribute( StreamElement& element ) {
		BOOST_STATIC_ASSERT_MSG( ID < NUM_ATTRIBUTES, "illegal attribute ID" );
		return ElementTraits::getElementRef( element ).template getAttribute< ID >();
	}

	/**
	 * @brief Get a specific attribute value from the stream element.
	 *
	 * @tparam ID
	 *    the ID of the requested attribute, must be in [0 ... NUM_ATTRIBUTES)
	 * @param[in] element
	 *    the element whose attribute shall be returned
	 * @return a reference to the element's attribute with the requested @c ID
	 */
	template< AttributeIdx ID >
	static const typename getAttributeType< ID >::type& getAttribute( const StreamElement& element ) {
		BOOST_STATIC_ASSERT_MSG( ID < NUM_ATTRIBUTES, "illegal attribute ID" );
		return ElementTraits::getElementRef( element ).template getAttribute< ID >();
	}

	/**
	 * @brief Set a specific attribute value of the tuple.
	 *
	 * @tparam ID
	 *    the ID of the requested attribute, must be in [0 ... NUM_ATTRIBUTES)
	 * @tparam AttributeValue
	 *    the type of the new value for the attribute, must be convertible to the requested attribute
	 * @param[in] element
	 *    the element whose attribute shall be set
	 * @param[in] value
	 *    the new attribute value
	 * @return a reference to the tuple's attribute with the requested @c ID
	 */
	template<
		AttributeIdx ID,
		typename AttributeValue
	>
	static void setAttribute( StreamElement& element, AttributeValue&& value ) {
		BOOST_STATIC_ASSERT_MSG( ID < NUM_ATTRIBUTES, "illegal attribute ID" );
		ElementTraits::getElementRef( element ).template setAttribute< ID >( std::forward< AttributeValue >( value ) );
	}


	////// timestamp access

	/**
	 * @brief Get the timestamp associated with the element.
	 *
	 * @param[in] element
	 *    the element whose timestamp shall be returned
	 * @return the element's timestamp
	 */
	static const Timestamp& getTimestamp( const StreamElement& element ) {
		return ElementTraits::getElementRef( element ).getTimestamp();
	}

	////// nullability access

	/**
	 * @brief Check if a specific attribute of the element is set to @c NULL.
	 *
	 * @param[in] element
	 *    the element whose attribute @c NULL property shall be checked
	 * @param[in] index
	 *    the index of the attribute, must be in [0 ... NUM_ATTRIBUTES)
	 * @return @c true if the element's attribute at the index is @c NULL,
	 *         @c false otherwise
	 */
	static bool isNull( const StreamElement& element, const AttributeIdx& index ) {
		assert( index < NUM_ATTRIBUTES );
		return ElementTraits::getElementRef( element ).isNull( index );
	}

	/**
	 * @brief Set the @c NULL property of a specific element attribute.
	 *
	 * @param[in] element
	 *    the element whose attribute @c NULL property shall be set
	 * @param[in] index
	 *    the index of the attribute, must be in [0 ... NUM_ATTRIBUTES)
	 * @param[in] value
	 *    flag indicating if the attribute shall be set to @c NULL (if @c true, or not (if @c false),
	 *    (default @c true)
	 */
	static void setNull( StreamElement& element, const AttributeIdx& index, const bool value = true ) {
		assert( index < NUM_ATTRIBUTES );
		ElementTraits::getElementRef( element ).setNull( index, value );
	}

	/**
	 * @brief Set all element attributes to @c NULL
	 *
	 * @param[in] element
	 *    the element whose attributes shall be set to @c NULL
	 */
	static void setNull( StreamElement& element ) {
		ElementTraits::getElementRef( element ).setNull();
	}
};

/// provide a definition for storing the number of attributes in the traits class
template< typename StreamElementType >
const TupleSize StreamElementTraits< StreamElementType >::NUM_ATTRIBUTES;


////// global accessor functions

////// attribute access

/**
 * @brief Get a the type of a specific attribute of a stream element.
 *
 * This meta function returns the type of an attribute with a specific ID of a
 * stream element.
 *
 * @tparam ID
 *    the ID of the requested attribute, must be in [0 ... NUM_ATTRIBUTES)
 * @tparam StreamElementType
 *    the stream element implementation type
 */
template<
	AttributeIdx ID,
	typename StreamElementType
>
struct getAttributeType {
	typedef typename StreamElementTraits< StreamElementType >::template getAttributeType< ID >::type type;
};

/**
 * @brief Get the number of attributes the element comprises.
 *
 * This global accessor function can be used to reduce boilerplate code that needs to be
 * written if the number of attributes of a stream @c element shall accessed.
 * The element is accessed through the common @c StreamElementTraits interface.
 *
 * @tparam StreamElementType
 *    the StreamElement implementation
 * @param[in] element
 *    the element whose number of attribtutes shall be returned
 * @return the number of attributes of the element
 */
template<
	typename StreamElementType
>
static TupleSize getNumAttributes( const StreamElementType& element ) {
	boost::ignore_unused( element );
	return StreamElementTraits< StreamElementType >::getNumAttributes();
}

/**
 * @brief Get a specific attribute value from the stream element.
 *
 * This global accessor function can be used to reduce boilerplate code that needs to be
 * written if a specific attribute of a stream @c element shall be accessed.
 * The element is accessed through the common @c StreamElementTraits interface.
 *
 * @tparam ID
 *    the ID of the requested attribute, must be in [0 ... NUM_ATTRIBUTES)
 * @tparam StreamElementType
 *    the StreamElement implementation
 * @param[in] element
 *    the element whose attribute shall be returned
 * @return a reference to the element's attribute with the requested @c ID
 */
template<
	AttributeIdx ID,
	typename StreamElementType
>
auto getAttribute( const StreamElementType& element ) ->
	decltype( (StreamElementTraits< StreamElementType >::template getAttribute< ID >( element )) )
{
	return StreamElementTraits< StreamElementType >::template getAttribute< ID >( element );
}

/**
 * @brief Get a specific attribute value from the stream element.
 *
 * This global accessor function can be used to reduce boilerplate code that needs to be
 * written if a specific attribute of a stream @c element shall be accessed.
 * The element is accessed through the common @c StreamElementTraits interface.
 *
 * @tparam ID
 *    the ID of the requested attribute, must be in [0 ... NUM_ATTRIBUTES)
 * @tparam StreamElementType
 *    the StreamElement implementation
 * @param[in] element
 *    the element whose attribute shall be returned
 * @return a reference to the element's attribute with the requested @c ID
 */
template<
	AttributeIdx ID,
	typename StreamElementType
>
auto getAttribute( StreamElementType& element ) ->
	decltype( (StreamElementTraits< StreamElementType >::template getAttribute< ID >( element )) )
{
	return StreamElementTraits< StreamElementType >::template getAttribute< ID >( element );
}

/**
 * @brief Set a specific attribute value of the stream element to a new value.
 *
 * This global accessor function can be used to reduce boilerplate code that needs to be
 * written if a specific attribute of a stream @c element shall be accessed.
 * The element is accessed through the common @c StreamElementTraits interface.
 *
 * @tparam ID
 *    the ID of the requested attribute, must be in [0 ... NUM_ATTRIBUTES)
 * @tparam StreamElementType
 *    the StreamElement implementation
 * @param[in] element
 *    the element whose attribute shall be returned
 * @return a reference to the element's attribute with the requested @c ID
 */
template<
	AttributeIdx ID,
	typename StreamElementType,
	typename AttributeValue
>
void setAttribute( StreamElementType& element, AttributeValue&& value ) {
	StreamElementTraits< StreamElementType >::template setAttribute< ID >( element, std::forward< AttributeValue >( value ) );
}


/////// nullability access

/**
 * @brief Check if a specific attribute of the element is set to @c NULL.
 *
 * This global accessor function can be used to reduce boilerplate code that needs to be
 * written if the null property of a stream @c element attribute shall tested.
 * The element is accessed through the common @c StreamElementTraits interface.
 *
 * @tparam StreamElementType
 *    the StreamElement implementation
 * @param[in] element
 *    the element whose attribute @c NULL property shall be checked
 * @param[in] index
 *    the index of the attribute, must be in [0 ... NUM_ATTRIBUTES)
 * @return @c true if the element's attribute at the index is @c NULL,
 *         @c false otherwise
 */
template<
	typename StreamElementType
>
static bool isNull( const StreamElementType& element, const AttributeIdx& index ) {
	return StreamElementTraits< StreamElementType >::isNull( element, index );
}

/**
 * @brief Set the @c NULL property of a specific element attribute.
 *
 * This global accessor function can be used to reduce boilerplate code that needs to be
 * written if the null property of a stream @c element attribute shall set.
 * The element is accessed through the common @c StreamElementTraits interface.
 *
 * @tparam StreamElementType
 *    the StreamElement implementation
 * @param[in] element
 *    the element whose attribute @c NULL property shall be set
 * @param[in] index
 *    the index of the attribute, must be in [0 ... NUM_ATTRIBUTES)
 * @param[in] value
 *    flag indicating if the attribute shall be set to @c NULL (if @c true, or not (if @c false),
 *    (default @c true)
 */
template<
	typename StreamElementType
>
static void setNull( StreamElementType& element, const AttributeIdx& index, const bool value = true ) {
	StreamElementTraits< StreamElementType >::setNull( element, index, value );
}

/**
 * @brief Set all element attributes to @c NULL
 *
 * This global accessor function can be used to reduce boilerplate code that needs to be
 * written if the null property of all stream @c element attributes shall set.
 * The element is accessed through the common @c StreamElementTraits interface.
 *
 * @tparam StreamElementType
 *    the StreamElement implementation
 * @param[in] element
 *    the element whose attributes shall be set to @c NULL
 */
template<
	typename StreamElementType
>
static void setNull( StreamElementType& element ) {
	StreamElementTraits< StreamElementType >::setNull( element );
}


namespace impl {

/**
 * @brief Functor for checking if two stream elements are equal.
 *
 * This functor is recursively instantiated for each attribute until all element attributes
 * are checked for equality.
 *
 * @tparam ID
 *    the id of the current attribute to be checked for equality
 */
template<
	AttributeIdx ID
>
struct ElementsEqual {

	/**
	 * @brief Check two elements for equality.
	 *
	 * @tparam LeftStreamElement
	 *    the type of the left handside stream element to be compared
	 * @tparam RightStreamElement
	 *    the type of the right handside stream element to be compared
	 * @param leftElement
	 *    the left handside stream element to be compared
	 * @param rightElement
	 *    the right handside stream element to be compared
	 * @return @c true if all attributes with an ID less than or equal to @c ID are
	 *         equal in both elements,
	 *         @c false otherwise
	 */
	template <
		typename LeftStreamElement,
		typename RightStreamElement
	>
	bool operator() (const LeftStreamElement& leftElement, const RightStreamElement& rightElement ) const {
		typedef StreamElementTraits< LeftStreamElement > LeftElement;
		typedef StreamElementTraits< RightStreamElement > RightElement;
		static_assert( LeftElement::NUM_ATTRIBUTES == RightElement::NUM_ATTRIBUTES,
			"trying to compare incompatible stream elements with different arity"
		);

		return
			// check the current attribute (-1 since 0-based)
			getAttribute< ID - 1 >( leftElement ) == getAttribute< ID - 1 >( rightElement ) &&
			// recursively check the following attribtues
			ElementsEqual< ID - 1 >()( leftElement, rightElement );
	}
};

/**
 * @brief Specialization for finalizing attribute the recursive equality check.
 */
template<>
struct ElementsEqual<0> {

	/**
	 * @brief Check two elements for equality.
	 *
	 * This operator finishes recursive attribute checks. It always returns @c true since
	 * all preceding attributes were equal when this functor is instantiated.
	 *
	 * @tparam LeftStreamElement
	 *    the type of the left handside stream element to be compared
	 * @tparam RightStreamElement
	 *    the type of the right handside stream element to be compared
	 * @param leftElement
	 *    the left handside stream element to be compared
	 * @param rightElement
	 *    the right handside stream element to be compared
	 * @return @c true
	 */
	template <
		typename LeftStreamElement,
		typename RightStreamElement
	>
	bool operator() ( const LeftStreamElement& leftElement, const RightStreamElement& rightElement ) const {
		boost::ignore_unused( leftElement );
		boost::ignore_unused( rightElement );
		return true;
	}
};

} /* end namespace impl */

/**
 * @brief Equality predicate for two stream elements.
 *
 * This equality predicate checks if all all element attributes are equal.
 *
 * @tparam LeftStreamElement
 *    first stream element type
 * @tparam RightStreamElement
 *    second stream element type
 * @param[in] leftElement
 *    the first element to be compared
 * @param[in] rightElement
 *    the second element to be compared
 * @return @c true if all attributes of both elements equal to each other,
 *         @c false otherwise
 */
template <
	typename LeftStreamElement,
	typename RightStreamElement
>
bool elementsEqual( const LeftStreamElement& leftElement, const RightStreamElement& rightElement ) {
	typedef StreamElementTraits< LeftStreamElement > LeftElement;
	return impl::ElementsEqual< LeftElement::NUM_ATTRIBUTES >()( leftElement, rightElement );
}

} /* end namespace pquery */


#endif /* STREAMELEMENTTRAITS_HPP_ */
