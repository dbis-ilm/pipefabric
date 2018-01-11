/*
 * Copyright (c) 2014-18 The PipeFabric team,
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

#ifndef StreamMockup_hpp_
#define StreamMockup_hpp_

#include "catch.hpp"

#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string.hpp>

#include <vector>
#include <mutex>
#include <fstream>

#include "core/Tuple.hpp"
#include "qop/Map.hpp"
#include "qop/DataSource.hpp"
#include "qop/DataSink.hpp"
#include "qop/OperatorMacros.hpp"


namespace pfabric {

struct MockupHelper {
	template <typename StreamElement>
	static void readTuplesFromStream(std::ifstream& in, std::vector<StreamElement>& vec) {
		std::string line;
		std::vector<std::string> data(StreamElementTraits<StreamElement>::NUM_ATTRIBUTES);

		while (getline(in, line)) {
			int i = 0;
			for (boost::split_iterator<std::string::iterator> it =
				boost::make_split_iterator(line, boost::first_finder(",", boost::is_iequal()));
				it != boost::split_iterator<std::string::iterator>(); it++) {
				data[i++] = boost::copy_range<std::string>(*it);
			}
			auto tp = StreamElementTraits<StreamElement>::create(data);
			vec.push_back(tp);
		}
	}
};

template<
typename InputStreamElement,
typename OutputStreamElement>
class StreamMockup :
	public DataSource< InputStreamElement >,
	public SynchronizedDataSink< OutputStreamElement >
{
public:
	typedef DataSource< InputStreamElement > SourceBase;
	typedef typename SourceBase::OutputDataChannel OutputDataChannel;
	typedef typename SourceBase::OutputPunctuationChannel OutputPunctuationChannel;
	typedef typename SourceBase::OutputDataElementTraits OutputDataElementTraits;

	typedef SynchronizedDataSink< OutputStreamElement > SinkBase;
	typedef typename SinkBase::InputDataChannel InputDataChannel;
	typedef typename SinkBase::InputPunctuationChannel InputPunctuationChannel;
	typedef typename SinkBase::InputDataElementTraits InputDataElementTraits;

	typedef std::function<bool(const OutputStreamElement& lhs, const OutputStreamElement& rhs)> CompareFunc;

	/**
	 *
	 * @param input
	 * @param expected
	 * @param ordered
	 * @param cFunc
	 */
	StreamMockup(const std::vector<InputStreamElement>& input,
		const std::vector<OutputStreamElement>& expected, bool ordered = true, CompareFunc cFunc = nullptr) :
		inputTuples(input), expectedTuples(expected), tuplesProcessed(0), compareOrdered(ordered),
		compareFunc(cFunc) {
			if (!compareOrdered)
				BOOST_ASSERT_MSG(compareFunc != nullptr, "no comparison predicated given.");
		}

	StreamMockup(const std::string& inputStream, const std::string& expectedStream)
	: tuplesProcessed(0), compareOrdered(true) {
		auto inputFile = std::string(TEST_DATA_DIRECTORY) + inputStream;
		std::ifstream input(inputFile);
		REQUIRE(input.is_open());

		auto expectedFile = TEST_DATA_DIRECTORY + expectedStream;
		std::ifstream expected(expectedFile);
		REQUIRE(expected.is_open());

		MockupHelper::readTuplesFromStream<InputStreamElement>(input, inputTuples);
		MockupHelper::readTuplesFromStream<OutputStreamElement>(expected, expectedTuples);
	}

	void start() {
		const bool outdated = false;

 		for (auto tp : inputTuples) {
			this->getOutputDataChannel().publish( tp, outdated );
		}
	}

	int numTuplesProcessed() const {
		int res = 0;
		{
			std::lock_guard<std::mutex> guard(mMtx);
			res = tuplesProcessed;
		}
		return res;
	}

	void addExpected(const std::vector<OutputStreamElement>& expected) {
		std::lock_guard<std::mutex> guard(mMtx);
		expectedTuples.insert(expectedTuples.end(), expected.begin(), expected.end());
	}

	/**
	 * @brief Bind the callback for the data channel.
	 */
	BIND_INPUT_CHANNEL_DEFAULT( InputDataChannel, StreamMockup, processDataElement );

	/**
	 * @brief Bind the callback for the punctuation channel.
	 */
	BIND_INPUT_CHANNEL_DEFAULT( InputPunctuationChannel, StreamMockup, processPunctuation );

private:

	void processDataElement( const OutputStreamElement& data, const bool outdated ) {
		std::lock_guard<std::mutex> guard(mMtx);
		// std::cout << "StreamMockup::processDataElement: " << data << std::endl;
		REQUIRE(unsigned(tuplesProcessed) < expectedTuples.size());
		if (compareOrdered) {
			// If we can compare tuples in their order of arrival everything is easy:
			// simply compare the current tuple with the expected tuple at the
			// same position.
			REQUIRE(data->data() == expectedTuples[tuplesProcessed]->data());
			// TODO: handle Null values
			for (auto i = 0u; i < OutputDataElementTraits::NUM_ATTRIBUTES; i++) {
				REQUIRE(data->isNull(i) == expectedTuples[tuplesProcessed]->isNull(i));
			}
			tuplesProcessed++;
		}
		else {
			// Otherwise, more more is needed: first, we store the incoming tuple.
			processedTuples.push_back(data);

			if (++tuplesProcessed == expectedTuples.size()) {
				// if we got enough tuples ...
				REQUIRE(processedTuples.size() == expectedTuples.size());
				// we sort both the processed and the expected tuples using the given
				// comparison function
				std::sort(processedTuples.begin(), processedTuples.end(), compareFunc);
				std::sort(expectedTuples.begin(), expectedTuples.end(), compareFunc);

				// the difference of these two vectors should now be empty
				std::vector<OutputStreamElement> res;
				std::set_difference(processedTuples.begin(), processedTuples.end(),
					expectedTuples.begin(), expectedTuples.end(), std::back_inserter(res), compareFunc);
				REQUIRE(res.empty());
			}

		}
	}

	void processPunctuation( const PunctuationPtr& punctuation ) {
		boost::ignore_unused( punctuation );
	}

	std::vector<InputStreamElement> inputTuples;
	std::vector<OutputStreamElement> expectedTuples, processedTuples;
	unsigned int tuplesProcessed;
	bool compareOrdered;
	CompareFunc compareFunc;
	mutable std::mutex mMtx;
};

}

#endif
