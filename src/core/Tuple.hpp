/*
 * Copyright (C) 2014-2018 DBIS Group - TU Ilmenau, All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PipeFabric is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PipeFabric. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef Tuple_hpp_
#define Tuple_hpp_

#include "PFabricTypes.hpp"
#include "TuplePrinter.hpp"
#include "parser/TupleParser.hpp"
#include "serialize.hpp"
#include "ElementSerializable.hpp"

#include <vector>
#include <iostream>
#include <ostream>
#include <utility>
#include <bitset>
#include <stdexcept>

#include <boost/variant.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/atomic.hpp>
// #include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/mpl/assert.hpp>


namespace pfabric {


/**
 * @brief A Tuple instance represents an element of a data stream.
 *
 * A data stream consists of a sequence of elements, which are represented
 * by instances of the class Tuple. Each tuple contains a field of data values,
 * a reference counter. Furthermore, it maintains a internal bit vector for
 * indicating null values.
 *
 * Note, that tuples should be always created on the heap and handled by smart pointers
 * (intrusive pointers). For this purpose, a helper function makeTuplePtr is provided,
 * which creates a Tuple object from the list of given values and returns a smart pointer
 * to this object:
 *
 * @code
 * auto tup = makeTuplePtr(42, 10.0);
 * @endcode
 *
 * Components of a tuple can be accessed via the typesafe getAttribute template accessor or a
 * static getAttribute template function:
 *
 * @code
 * int i = tup->getAttribute<0>();
 * double d = getAttribute<1>(*tp);
 * @endcode
 *
 * In the same way, the components of a tuple can be updated:
 *
 * @code
 * tup->setAttribute<0>(10);
 * @endcode
 *
 *  Note, that the component index (0, 1, ...) has to specified at compile time. If it has to
 *  be given at runtime, you can use dynamic_get which returns a boost::variant object:
 *
 *    int idx = 1;
 *    boost::variant& val = dynamic_get(idx, *tup);
 */
template< typename... Types >
class Tuple :
	public ns_types::TupleType< Types... >, public ElementSerializable {
public:
	typedef ns_types::TupleType< Types... > Base;

	static const TupleSize NUM_ATTRIBUTES = ns_types::TupleSize< Base >::value;
	static_assert( sizeof...( Types ) == NUM_ATTRIBUTES, "unexpected tuple size" );

	/**
	 * @brief Meta function returning the type of a specific tuple attribute.
	 *
	 * @tparam ID
	 *    the ID of the requested attribute, must be in [0 ... NUM_ATTRIBUTES)
	 */
	template< AttributeIdx ID >
	struct getAttributeType {
		BOOST_STATIC_ASSERT_MSG( ID < NUM_ATTRIBUTES, "illegal attribute ID" );
		typedef typename ns_types::TupleElement< ID, Base >::type type;
	};


	Tuple(const Tuple<Types...>& tup) : Base(tup.data()), mRefCnt(0) {}

	Tuple(const ns_types::TupleType<Types...>& tup) : Base(tup), mRefCnt(0) {}

	/**
	 * @brief Constructs a new tuple instance.
	 *
	 * The constructor will forward the arguments to the base
	 *
	 * @tparam Args
	 *     the list of argument types to construct the tuple from
	 * @param[in] args
	 */
// TODO Can't we use a forwarding / conversion constructor here?
//      using the ...Args below leads to problems when constructing (actually parsing) from string vector below
//	template< typename... Args >

	Tuple( Types... args ) :
		// this does not actually forward anything (would require Args&&... args)
		// --> always resolves to a lvalue, never a rvalue ref for move construction
		Base( std::forward< Types >( args )... ), mRefCnt(0) {}

	/**
	 * @brief Parsing constructor for string tuples.
	 *
	 * This constructor will parse the tuple from a vector of strings.
	 *
	 * @param[in] sdata
	 *    a string tuple representing the values to be parsed
	 */
	Tuple( const StringTuple& sdata) :
		Base(), mRefCnt(0) {
		TupleParser::parseTuple< NUM_ATTRIBUTES >( *this, sdata );
	}

	/**
	 * @brief Parsing constructor for string reference tuples.
	 *
	 * This constructor will parse the tuple from a vector of strings.
	 *
	 * @param[in] sdata
	 *    a string tuple representing the values to be parsed
	 */
	Tuple(const StringRef* sdata) :
		Base(), mRefCnt(0) {
		TupleParser::parseTuple< NUM_ATTRIBUTES >( *this, sdata );
	}

	/**
	 * @brief Deserialization constructor.
	 *
	 * @param[in] byteStream
	 *    the byte stream containing the tuple in serialized format
	 */
	Tuple( StreamType& byteStream ) {
		deserializeFromStream( byteStream );
	}

/**
 * @brief Standard destructor.
 */
	~Tuple() {}

	/**
	 * @brief Get a specific attribute value from the tuple.
	 *
	 * @tparam ID
	 *    the ID of the requested attribute, must be in [0 ... NUM_ATTRIBUTES)
	 * @return a reference to the tuple's attribute with the requested @c ID
	 */
	template< AttributeIdx ID >
	typename getAttributeType< ID >::type& getAttribute() {
		BOOST_STATIC_ASSERT_MSG( ID < NUM_ATTRIBUTES, "illegal attribute ID" );
		return std::get< ID >( *this );
	}

	/**
	 * @brief Get a specific attribute value from the tuple.
	 *
	 * @tparam ID
	 *    the ID of the requested attribute, must be in [0 ... NUM_ATTRIBUTES)
	 * @return a reference to the tuple's attribute with the requested @c ID
	 */
	template< AttributeIdx ID >
	const typename getAttributeType< ID >::type& getAttribute() const {
		BOOST_STATIC_ASSERT_MSG( ID < NUM_ATTRIBUTES, "illegal attribute ID" );
		return std::get< ID >( *this );
	}

	/**
	 * @brief Set a specific attribute value of the tuple.
	 *
	 * @tparam ID
	 *    the ID of the requested attribute, must be in [0 ... NUM_ATTRIBUTES)
	 * @tparam AttributeValue
	 *    the type of the new value for the attribute, must be convertible to the requested attribute
	 * @param[in] value
	 *    the new attribute value
	 * @return a reference to the tuple's attribute with the requested @c ID
	 */
	template<
		AttributeIdx ID,
		typename AttributeValue
	>
	void setAttribute( AttributeValue&& value ) {
		BOOST_STATIC_ASSERT_MSG( ID < NUM_ATTRIBUTES, "illegal attribute ID" );
		std::get< ID >( *this ) = value;
	}

	/**
	 * @brief Set a specific attribute to @c NULL.
	 *
	 * Tuple supports the representation of null values in an internal bitset. Initially,
	 * all components are non-null values, but can be marked as null value using
	 * the setNull method.
	 *
	 * @param[in] index
	 *    the index of the attribute, must be in [0 ... NUM_ATTRIBUTES)
	 * @param[in] value
	 *    flag indicating if the attribute shall be set to @c NULL (if @c true, or not (if @c false),
	 *    (default true)
	 */
	void setNull( const AttributeIdx& index, const bool value = true ) {
		assert( index < NUM_ATTRIBUTES );
		mNulls.set(index, value);
	}

	/**
	 * @brief Checks whether field at position pos contains a null value.
	 *
	 * @param[in] pos
	 *  	the index of the field (starting at 0)
	 * @return true if the field value is a null value, false otherwise
	 */
	bool isNull( const AttributeIdx& index ) const {
		return mNulls[index];
	}

	/**
	 * @brief Sets all fields of this tuple to null.
	 */
	void setNull() {
		mNulls.set();
	}

	/**
	 * @brief  Returns the std::tuple<> representation corresponding to this object.
	 *
	 * @return
	 */
	Base& data() {
		return static_cast<Base&>(*this);
	}

	Base const& data() const {
		return static_cast<Base const&>(*this);
	}

	/**
	 * Helper functions for supporting intrusive pointers.
	 */
	friend void intrusive_ptr_add_ref(const Tuple *t) {
		t->mRefCnt.fetch_add(1, boost::memory_order_relaxed);
	}

	friend void intrusive_ptr_release(const Tuple *t) {
		if (t->mRefCnt.fetch_sub(1, boost::memory_order_release) == 1) {
  			boost::atomic_thread_fence(boost::memory_order_acquire);
			delete t;
		}
	}

	/**
	 * @brief Returns the size of the tuple.
	 *
	 * @return the number of fields of this tuple.
	 */
	TupleSize size() const { return NUM_ATTRIBUTES; }


	/**
	 * @brief Serializes a tuple to a buffer or stream.
	 *
	 * Instances of Tuple can be serialized/deserialized to/from a buffer. In this
	 * case, a vector of uint8_t should be used to store the tuple data.
	 *
	 * The following examples illustrate the usage of these methods:
	 *
	 * @code
	 *    typedef Tuple<int, std::string, double> MyTuple;
	 *    auto tp = makeTuplePtr(12, std::string("Hallo"), 42.0);
	 *
	 *    // serialize it into a buffer
	 *    StreamType res;
	 *    tp->serializeToStream(res);
	 *
	 *    // deserialize into a new tuple
	 *    auto tp2 = new MyTuple();
	 *    tp2->deserializeFromStream(res);
	 *
	 *    // compare the two tuples
	 *    BOOST_ASSERT(*tp == *tp2);
	 * @endcode
	 *
	 * @param res a predefined stream for storing the serialized data.
	 */
	void serializeToStream(StreamType& res) const override {
		serialize(TupleType::Normal, res);  // serialize the type of this tuple
		serialize(data(), res);
		serialize(mNulls, res);
	}

	/**
	 * @brief Deserializes a tuple from a stream.
	 *
	 * @param[in] res
	 *   a predefined stream which stores the binary representation of a tuple.
	 */
	void deserializeFromStream(StreamType& res) override {
		auto it = res.cbegin();
		/**
		 * don't store this information.
		 * This is only an indication whether this tuple is normal or punctuation
		 * in case of network communication
		 */
		deserialize<TupleType>(it, res.cend());
		data() = deserialize<Base>(it, res.cend());
		mNulls = deserialize<std::bitset<std::tuple_size<Base>::value>>(it, res.end());
	}

	/**
	 * @brief Returns the value of the reference counter.
	 *
	 * @return the number of references to this tuple
	 */
	short refCount() const { return mRefCnt.load(boost::memory_order_relaxed); }

private:

	mutable boost::atomic<short> mRefCnt;              //< the reference counter needed for intrusive_ptr
	std::bitset<std::tuple_size<Base>::value > mNulls; //< a bitset where a bit indicates that the corresponding field contains a null value
};

template< typename ...Types >
using TuplePtr = boost::intrusive_ptr<Tuple<Types...> >;

} // namespace pfabric


// -----------------------------------------------------------------------------------------------

// TODO hack for tuple factory function
#include "TuplePtrFactory.hpp"

// -----------------------------------------------------------------------------------------------

/**
 * \@brief Helper template for printing tuples to a ostream.
 *
 * Provides helper methods for printing a tuple to std::ostream via the << operator.
 *
 * @tparam
 * @param[in] os
 * @param[in] tp
 * @return
 */
template<typename ...Types>
std::ostream& operator<< (std::ostream& os, const pfabric::Tuple<Types ...>& tp) {
	pfabric::print(os, tp);
	return os;
}

/**
 * @brief Helper template for printing tuples to a ostream.
 *
 * Provides helper methods for printing a tuple pointer to std::ostream via the << operator.
 *
 * @tparam
 * @param[in] os
 * @param[in] tp
 * @return
 */
template<typename ...Types>
std::ostream& operator<< (std::ostream& os, const pfabric::TuplePtr<Types ...>& tp) {
	os << *tp;
	return os;
}

#endif
