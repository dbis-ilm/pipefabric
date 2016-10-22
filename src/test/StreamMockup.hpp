#ifndef StreamMockup_hpp_
#define StreamMockup_hpp_

#include "catch.hpp"

#include <boost/core/ignore_unused.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string.hpp>

#include <vector>

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
	public DataSink< OutputStreamElement >
{
public:
	typedef DataSource< InputStreamElement > SourceBase;
	typedef typename SourceBase::OutputDataChannel OutputDataChannel;
	typedef typename SourceBase::OutputPunctuationChannel OutputPunctuationChannel;
	typedef typename SourceBase::OutputDataElementTraits OutputDataElementTraits;

	typedef DataSink< OutputStreamElement > SinkBase;
	typedef typename SinkBase::InputDataChannel InputDataChannel;
	typedef typename SinkBase::InputPunctuationChannel InputPunctuationChannel;
	typedef typename SinkBase::InputDataElementTraits InputDataElementTraits;

	StreamMockup(const std::vector<InputStreamElement>& input,
		const std::vector<OutputStreamElement>& expected) : inputTuples(input),
		expectedTuples(expected),
		tuplesProcessed(0) {}

	StreamMockup(const std::string& inputStream, const std::string& expectedStream)
	: tuplesProcessed(0) {
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

	int numTuplesProcessed() const { return tuplesProcessed; }

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
		// std::cout << "StreamMockup::processDataElement: " << data << std::endl;
		REQUIRE(unsigned(tuplesProcessed) < expectedTuples.size());
		REQUIRE(data->data() == expectedTuples[tuplesProcessed]->data());
		tuplesProcessed++;
	}

	void processPunctuation( const PunctuationPtr& punctuation ) {
		boost::ignore_unused( punctuation );
	}

	std::vector<InputStreamElement> inputTuples;
	std::vector<OutputStreamElement> expectedTuples;
	int tuplesProcessed;
};

}

#endif
