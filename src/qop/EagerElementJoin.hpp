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

#ifndef EagerElementJoin_hpp_
#define EagerElementJoin_hpp_

#include "core/TupleFactoryTraits.hpp"

#include "libcpp/mpl/sequences/GenerateIndexes.hpp"


namespace pfabric {

/**
 * @brief An eager join implementation for concatenating two stream elements.
 *
 * This class implements a join of all attributes of two stream element implementations.
 * In the join result, all attributes from the @c LeftStreamElement implementation precede
 * those of the @c RightStreamElement.
 *
 * Result elements are created via the @c StreamElementFactory class with forwarding all
 * attributes from the @c LeftStreamElement and the @c RightStreamElement in that order
 * to the factory method. Since this usually results in copies of the element attributes,
 * this might be an expensive operation. If copying attributes is expensive, consider
 * creating stream elements via pointers or references and use a @c LazyElementJoin instead.
 *
 * @tparam LeftStreamElement
 *    the type of the left stream element having the first attributes
 * @tparam RightStreamElement
 *    the type of the right stream element having the last attributes
 * @tparam StreamElementFactory
 *    a factory class for creating result element instances from attributes
 */
template<
	typename LeftStreamElement,
	typename RightStreamElement,
	typename StreamElementFactory
>
class EagerElementJoin {
private:

	/// stream element interface for the left element
	typedef StreamElementTraits< LeftStreamElement > LeftElement;

	/// stream element interface for the right element
	typedef StreamElementTraits< RightStreamElement > RightElement;

	/// factory class for generating result tuples
	typedef TupleFactoryTraits< StreamElementFactory > TupleFactory;


	/**
	 * @brief The eager join implementation.
	 *
	 * This method creates a new joined element using the @c StreamElementFactory,
	 * passing all attributes from the left and right side as constructor arguments.
	 *
	 * An index for each attribute for the @c leftElement and the @c rightElement is
	 * used to extract each of the source attributes via parameter pack expansion.
	 * The necessary @c leftAttributeIndexes and @c rightAttributeIndexes are injected
	 * by the public join method.
	 *
	 * @param[in] leftElement
	 *    the left element of the join having the first attributes
	 * @param[in] leftAttributeIndexes
	 *    a structure having a compile-time constant for each attribute index of the @c leftElement
	 * @param[in] rightElement
	 *    the right element of the join having the last attributes
	 * @param[in] rightAttributeIndexes
	 *    a structure having a compile-time constant for each attribute index of the @c rightElement
	 * @return a new joined stream element instance comprising all attributes
	 */
	template<
		int... LeftAttributeIndexes,
		int... RightAttributeIndexes
	>
	static auto joinImpl( const LeftStreamElement& leftElement,
		const ns_mpl::IndexTuple< LeftAttributeIndexes... >& leftAttributeIndexes,
		const RightStreamElement& rightElement,
		const ns_mpl::IndexTuple< RightAttributeIndexes... >& rightAttributeIndexes )
		-> decltype(
			( TupleFactory::create(
				getAttribute< LeftAttributeIndexes >( leftElement )...,
				getAttribute< RightAttributeIndexes >( rightElement )...
			  )
			)
		)
	{
		// produce a new joint stream element with all attributes using the factory
		auto joinedElement = StreamElementFactory::create(
			// get each attribute from the left side using parameter pack expansion
			getAttribute< LeftAttributeIndexes >( leftElement )...,
			// get each attribute from the right side using parameter pack expansion
			getAttribute< RightAttributeIndexes >( rightElement )...
		);

		// copy null attributes for the left handside element
		for( AttributeIdx attributeIdx = 0; attributeIdx < LeftElement::NUM_ATTRIBUTES; attributeIdx++ ) {
			setNull( joinedElement, attributeIdx, isNull( leftElement, attributeIdx ) );
		}

		// copy null attributes for the right handside element
		for( AttributeIdx attributeIdx = 0; attributeIdx < RightElement::NUM_ATTRIBUTES; attributeIdx++ ) {
			setNull( joinedElement, attributeIdx + LeftElement::NUM_ATTRIBUTES, isNull( rightElement, attributeIdx ) );
		}

		return joinedElement;
	}


public:


	/**
	 * @brief Create a new stream element as join between two two elements.
	 *
	 * This element join delivers a new stream element having all attributes from the
	 * @c LeftElement followed by those from the @c RightElement. The joined element is
	 * created as new instance using the @c StreamElementFactory, passing all attributes
	 * from the left and right side as constructor arguments.
	 *
	 * @note Since a new stream element instance is created as join result, it is safe
	 *       to destroy the @c leftElement and @c rightElement after the join.
	 *
	 * @param[in] leftElement
	 *    the left element of the join having the first attributes
	 * @param[in] rightElement
	 *    the right element of the join having the last attributes
	 * @return a new joined stream element instance comprising all attributes
	 */
	static auto joinElements( const LeftStreamElement& leftElement, const RightStreamElement& rightElement )
		-> decltype (
			joinImpl(
				leftElement, typename ns_mpl::generateIndexes< LeftElement::NUM_ATTRIBUTES >::type(),
				rightElement, typename ns_mpl::generateIndexes< RightElement::NUM_ATTRIBUTES >::type()
			)
		)
	{
		return joinImpl(
			// generate a list of attribute indexes for each attribute in the left element
			leftElement, typename ns_mpl::generateIndexes< LeftElement::NUM_ATTRIBUTES >::type(),
			// generate a list of attribute indexes for each attribute in the right element
			rightElement, typename ns_mpl::generateIndexes< RightElement::NUM_ATTRIBUTES >::type()
		);
	}


	/// the join result element type
	typedef typename ns_types::FunctionTraits<
		decltype( &EagerElementJoin::joinElements )
	>::ReturnType ResultElement;
};

} /* end namespace pfabric */


#endif /* EagerElementJoin_hpp_ */
