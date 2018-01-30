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

#ifndef OperatorMacros_hpp_
#define OperatorMacros_hpp_

#include <boost/preprocessor/cat.hpp>

#include "core/StreamElementTraits.hpp"
#include "pubsub/channels/ConnectChannels.hpp"

/**
 * Typedefs for an operator class which does not transform tuples (i.e. changing
 * the tuple type) and is directly derived from DataSource.
 */
#define PFABRIC_SOURCE_TYPEDEFS(StreamElement) \
typedef DataSource<StreamElement> SourceBase; \
typedef typename SourceBase::OutputDataChannel OutputDataChannel; \
typedef typename SourceBase::OutputPunctuationChannel OutputPunctuationChannel; \
typedef typename SourceBase::OutputDataElementTraits OutputDataElementTraits;

/**
 * Typedefs for an operator class which does not transform tuples (i.e. changing
 * the tuple type) and is directly derived from SynchronizedDataSink.
 */
#define PFABRIC_SYNC_SINK_TYPEDEFS(StreamElement) \
typedef SynchronizedDataSink< StreamElement > SinkBase; \
typedef typename SinkBase::InputDataChannel InputDataChannel; \
typedef typename SinkBase::InputPunctuationChannel InputPunctuationChannel; \
typedef typename SinkBase::InputDataElementTraits InputDataElementTraits;

/**
 * Typedefs for an operator class which does not transform tuples (i.e. changing
 * the tuple type) and is directly derived from dDataSink.
 */
#define PFABRIC_SINK_TYPEDEFS(StreamElement) \
typedef DataSink< StreamElement > SinkBase; \
typedef typename SinkBase::InputDataChannel InputDataChannel; \
typedef typename SinkBase::InputPunctuationChannel InputPunctuationChannel; \
typedef typename SinkBase::InputDataElementTraits InputDataElementTraits;

/**
 * Typedefs for an operator class which does not transform tuples (i.e. changing
 * the tuple type) and is  derived from a given BaseClass.
 */
#define PFABRIC_BASE_TYPEDEFS(BaseClass, StreamElement) \
typedef typename BaseClass ::InputDataChannel InputDataChannel; \
typedef typename BaseClass ::InputPunctuationChannel InputPunctuationChannel; \
typedef typename BaseClass ::InputDataElementTraits InputDataElementTraits;

/**
 * Typedefs for an operator class which transforms tuples (i.e. changing
 * the tuple type) and is derived from the @ UnaryTransform base class.
 */
#define PFABRIC_UNARY_TRANSFORM_TYPEDEFS(InputStreamElement, OutputStreamElement) \
typedef UnaryTransform< InputStreamElement, OutputStreamElement > TransformBase; \
typedef typename TransformBase::InputDataChannel InputDataChannel; \
typedef typename TransformBase::InputPunctuationChannel InputPunctuationChannel; \
typedef typename TransformBase::InputDataElementTraits InputDataElementTraits; \
typedef typename TransformBase::OutputDataChannel OutputDataChannel; \
typedef typename TransformBase::OutputPunctuationChannel OutputPunctuationChannel; \
typedef typename TransformBase::OutputDataElementTraits OutputDataElementTraits;

/**
 * Typedefs for an operator class which transforms tuples (i.e. changing
 * the tuple type) and is derived from the @ UnaryTransform base class.
 */
#define PFABRIC_SYNC_UNARY_TRANSFORM_TYPEDEFS(InputStreamElement, OutputStreamElement) \
typedef UnaryTransform< InputStreamElement, OutputStreamElement, true > TransformBase; \
typedef typename TransformBase::InputDataChannel InputDataChannel; \
typedef typename TransformBase::InputPunctuationChannel InputPunctuationChannel; \
typedef typename TransformBase::InputDataElementTraits InputDataElementTraits; \
typedef typename TransformBase::OutputDataChannel OutputDataChannel; \
typedef typename TransformBase::OutputPunctuationChannel OutputPunctuationChannel; \
typedef typename TransformBase::OutputDataElementTraits OutputDataElementTraits;

/**
 * Typedefs for an operator class which combines and transforms two input tuples
 * into a single tuple of a new tuple type and is derived from
 * the @ BinaryTransform base class.
 */
#define PFABRIC_BINARY_TRANSFORM_TYPEDEFS(LeftInputStreamElement, RightInputStreamElement, OutputStreamElement) \
typedef BinaryTransform< LeftInputStreamElement, RightInputStreamElement, OutputStreamElement> TransformBase; \
typedef typename TransformBase::LeftInputDataChannel LeftInputChannel; \
typedef typename TransformBase::RightInputDataChannel RightInputChannel; \
typedef typename TransformBase::InputPunctuationChannel InputPunctuationChannel; \
typedef typename TransformBase::OutputDataChannel OutputDataChannel; \
typedef typename TransformBase::OutputPunctuationChannel OutputPunctuationChannel;

/**
 * Connects two operators with a publisher-subscriber link. Both the data and
 * the punctuation channels are connected.
 */
#define CREATE_LINK(Publisher, Subscriber) \
connectChannels(Publisher->getOutputDataChannel(), Subscriber->getInputDataChannel()); \
connectChannels(Publisher->getOutputPunctuationChannel(), Subscriber->getInputPunctuationChannel());

/**
 * Connects two operators with a publisher-subscriber link. Only the data is used.
 */
#define CREATE_DATA_LINK(Publisher, Subscriber) \
connectChannels(Publisher->getOutputDataChannel(), Subscriber->getInputDataChannel());

#endif /* OperatorMacros_hpp_ */
