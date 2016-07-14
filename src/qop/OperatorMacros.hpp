/*
 * Copyright (c) 2014-16 The PipeFabric team,
 *                       All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License (GPL) as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.
 * If not you can find the GPL at http://www.gnu.org/copyleft/gpl.html
 */

#ifndef OperatorMacros_hpp_
#define OperatorMacros_hpp_

#include "core/StreamElementTraits.hpp"
#include "pubsub/channels/ConnectChannels.hpp"

// TODO put helper macros here

#define PFABRIC_SOURCE_TYPEDEFS(StreamElement) \
typedef DataSource<StreamElement> SourceBase; \
typedef typename SourceBase::OutputDataChannel OutputDataChannel; \
typedef typename SourceBase::OutputPunctuationChannel OutputPunctuationChannel; \
typedef typename SourceBase::OutputDataElementTraits OutputDataElementTraits;


#define PFABRIC_SYNC_SINK_TYPEDEFS(StreamElement) \
typedef SynchronizedDataSink< StreamElement > SinkBase; \
typedef typename SinkBase::InputDataChannel InputDataChannel; \
typedef typename SinkBase::InputPunctuationChannel InputPunctuationChannel; \
typedef typename SinkBase::InputDataElementTraits InputDataElementTraits;

#define PFABRIC_SINK_TYPEDEFS(StreamElement) \
typedef DataSink< StreamElement > SinkBase; \
typedef typename SinkBase::InputDataChannel InputDataChannel; \
typedef typename SinkBase::InputPunctuationChannel InputPunctuationChannel; \
typedef typename SinkBase::InputDataElementTraits InputDataElementTraits;

#define PFABRIC_UNARY_TRANSFORM_TYPEDEFS(InputStreamElement, OutputStreamElement) \
typedef UnaryTransform< InputStreamElement, OutputStreamElement > TransformBase; \
typedef typename TransformBase::InputDataChannel InputDataChannel; \
typedef typename TransformBase::InputPunctuationChannel InputPunctuationChannel; \
typedef typename TransformBase::InputDataElementTraits InputDataElementTraits; \
typedef typename TransformBase::OutputDataChannel OutputDataChannel; \
typedef typename TransformBase::OutputPunctuationChannel OutputPunctuationChannel; \
typedef typename TransformBase::OutputDataElementTraits OutputDataElementTraits;

#define PFABRIC_BINARY_TRANSFORM_TYPEDEFS(LeftInputStreamElement, RightInputStreamElement, OutputStreamElement) \
typedef BinaryTransform< LeftInputStreamElement, RightInputStreamElement, OutputStreamElement> TransformBase; \
typedef typename TransformBase::LeftInputDataChannel LeftInputChannel; \
typedef typename TransformBase::RightInputDataChannel RightInputChannel; \
typedef typename TransformBase::InputPunctuationChannel InputPunctuationChannel; \
typedef typename TransformBase::OutputDataChannel OutputDataChannel; \
typedef typename TransformBase::OutputPunctuationChannel OutputPunctuationChannel;

#define CREATE_LINK(Publisher, Subscriber) \
connectChannels(Publisher->getOutputDataChannel(), Subscriber->getInputDataChannel()); \
connectChannels(Publisher->getOutputPunctuationChannel(), Subscriber->getInputPunctuationChannel());

#define CREATE_DATA_LINK(Publisher, Subscriber) \
connectChannels(Publisher->getOutputDataChannel(), Subscriber->getInputDataChannel());

#endif /* OperatorMacros_hpp_ */
