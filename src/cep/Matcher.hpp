/*
 * Copyright (c) 2014 The PipeFabric team,
 *                    All Rights Reserved.
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

#ifndef  Matcher_hpp_
#define  Matcher_hpp_
#include "CEPEngine.hpp"
#include "engine/FirstMatchEngine.hpp"
#include "engine/NextMatchEngine.hpp"

#include "qop/UnaryTransform.hpp"
#include "MatchProducer.hpp"
#include "util/Partition.hpp"

/**
 * @brief The matcher operator for detecting complex events.
 *
 * This operator implements the complex event processing of tuple streams. This class aims to run a particular engine according to the selected strategy
 * to process the incoming tuples incrementally. Then, it produces the matches (the results) according to selected output strategy.
 * The matcher operator is implemented by Nondeterministic Finite Automata (NFA) to evaluate event patterns over tuple streams in a way similar to regular expressions
 */
namespace pfabric {
template<typename InputStreamElement, typename OutputStreamElement,
		typename EventDependency>
class Matcher: public UnaryTransform<InputStreamElement, OutputStreamElement> // use default unary transform
{

	typedef UnaryTransform<InputStreamElement, OutputStreamElement> TransformBase;
	typedef typename TransformBase::InputDataChannel InputDataChannel;
	typedef typename TransformBase::InputPunctuationChannel InputPunctuationChannel;
	typedef typename TransformBase::InputDataElementTraits InputDataElementTraits;

public:
	/**
	 * The available selection strategy, the tuples matcher engine would be run accordingly
	 * see the engine folder
	 */
	enum SelectionStrategy {
		NextMatches, AllMatches, ContiyuityMatches, FirstMatch, RecentMatch
	};
	/**
	 * The available output strategy which the output would be generated accordingly,
	 * because the output of this operator is a complex event or a combination of tuples
	 * one by one means generate the tuples one after another, in this case the resulting tuples
	 * have fixed schema, whereas combined strategy combines all tuples (complex event) in one big tuple
	 * which has variable schema
	 *
	 */
	enum OutputStrategy {
		OneByOne, Combined
	};

private:
	/**
	 * The current working engine
	 */
	CEPEngine<InputStreamElement, OutputStreamElement, EventDependency>* engine;
	/**
	 * The strategy to process the incoming events
	 */
	Matcher::SelectionStrategy strategy;
	/**
	 * The output strategy to specify the form of output (one by one or combined)
	 */
	Matcher::OutputStrategy outStrategy;
	/**
	 * An object to create the matched events
	 */
	typename MatchProducer<InputStreamElement, OutputStreamElement,
			EventDependency>::MatchProducerPtr matcher;
	/**
	 * Create CEP engine according to an assigned strategy
	 */
	void createEngine();

	/**
	 * @brief Bind the callback for the data channel.
	 */
	BIND_INPUT_CHANNEL_DEFAULT( InputDataChannel, Matcher, processDataElement );

	/**
	 * @brief Bind the callback for the punctuation channel.
	 */
	BIND_INPUT_CHANNEL_DEFAULT( InputPunctuationChannel, Matcher, processPunctuation );
public:

	/**
	 * A constructor to specify the selected strategy to run the matcher accordingly and
	 * to specify the way of generating the resulting tuples.
	 *
	 * @param selectStr the selected strategy (default Matcher::FirstMatch)
	 * @param outStr output strategy (default Matcher::OneBYOne)
	 */
	Matcher(Matcher::SelectionStrategy selectStr = Matcher::FirstMatch,
			Matcher::OutputStrategy outStr = Matcher::OneByOne) {
		setSelectionStrategy(selectStr);
		setOutputStrategy(outStr);
		createEngine();
		matcher = typename MatchProducer<InputStreamElement, OutputStreamElement, EventDependency>::MatchProducerPtr(new MatchProducer<InputStreamElement, OutputStreamElement, EventDependency>());
		assert(matcher);
	}
	/**
	 * A destructor to release the resources and clean-up
	 */
	virtual ~Matcher() {delete engine;}
	/**
	 * Get the current running engine according to the selected strategy
	 * @return the current running engine
	 */
	CEPEngine<InputStreamElement, OutputStreamElement, EventDependency>* getEngine() const {return this->engine;}
	/**
	 * get the selection strategy whether NextMatches, AllMatches, ContiyuityMatches, FirstMatch or RecentMatch
	 * @return the selection strategy
	 */
	Matcher::SelectionStrategy getSelectionStrategy() const;
	/**
	 * get the selection strategy whether NextMatches, AllMatches, ContiyuityMatches, FirstMatch or RecentMatch as string
	 * @return the selection strategy as string
	 */
	std::string getSelectionStrategyString() const {return strategy;}
	/**
	 * set the selection strategy
	 * @param the selection strategy
	 *
	 */
	void setSelectionStrategy(Matcher::SelectionStrategy strategy) {
		if (strategy >= Matcher::NextMatches && strategy <= Matcher::RecentMatch)
		this->strategy = strategy;
		else
		this->strategy = Matcher::FirstMatch;
	}

	/**
	 * @brief This method is invoked when a punctuation arrives.
	 *
	 * It simply forwards the punctuation to the subscribers.
	 *
	 * @param[in] punctuation
	 *    the incoming punctuation tuple
	 */
	void processPunctuation( const PunctuationPtr& punctuation ) {
		this->getOutputPunctuationChannel().publish(punctuation);
	}

	/**
	 * This method is invoked when a data stream element arrives.
	 *
	 * It applies the projection function and forwards the projected element to its subscribers.
	 *
	 * @param[in] data
	 *    the incoming stream element
	 * @param[in] outdated
	 *    flag indicating whether the tuple is new or invalidated now
	 */
	void processDataElement( const InputStreamElement& data, const bool outdated ) {
		engine->runEngine(data);
	}
	/**
	 * Print information about this operator
	 * @param os the output stream object
	 */
	void printInfo(std::ostream& os) const {
		//TODO
	}
	/**
	 * Set the output strategy either one by one or combined
	 * @param the output strategy
	 */
	void setOutputStrategy(Matcher::OutputStrategy str) {
		if (str >= Matcher::OneByOne && str <= Matcher::Combined)
		this->outStrategy = str;
		else
		this->outStrategy = Matcher::OneByOne;
	}
	/**
	 * Get the output strategy
	 * @return the output strategy
	 */
	Matcher::OutputStrategy getOutputStrategy() const {return outStrategy;}
	/**
	 *  Since our systems depends on NFA concept. This methods return a pointer to NFA builder or controller
	 *  @return return our main NFA
	 */
	const typename NFAController<InputStreamElement, OutputStreamElement, EventDependency>::NFAControllerPtr getNFAController() const {return engine->getNFA();}
	/**
	 * set the window constraint parameters by the CEP engine which implemented by within cluase
	 * @param period the time needed to detect the complex event within
	 * @param fromEvent
	 * @param tovent
	 */
	void setWindowConstraint(long period, int fromEvent = -1, int toEvent =
			-1) {
		assert(fromEvent >= toEvent);
		engine->setWindowConstraint(period, fromEvent, toEvent);
	}
	/**
	 * Get the window information from the matcher engine
	 */
	typename CEPEngine<InputStreamElement, OutputStreamElement, EventDependency>::WindowStruct* getWindow() const {return engine->getWindow();}

	/**
	 * Set the partition object itself
	 */
	void setEqulity(Partition<InputStreamElement>* par) {
		if (engine)
		engine->setEquality(par);
	}
	/**
	 * Publish the new matches or the complex event (combination of tuples or one by one) to a next operator
	 * Once the engine detects the complex event, it publishes the result to the next operator in the chain
	 * The matches are exist in structurePtr object which responsible for storing the matches
	 * @param matches the matches object
	 */
	void publishResulMatches(const typename NFAStructure<InputStreamElement, OutputStreamElement, EventDependency>::NFAStructurePtr& matches);
};
}

namespace pfabric {
template<class InputStreamElement, class OutputStreamElement,
	class EventDependency>
void Matcher<InputStreamElement, OutputStreamElement, EventDependency>::createEngine() {
if (strategy == Matcher::FirstMatch)
	engine = new FirstMatchEngine<InputStreamElement, OutputStreamElement,
			EventDependency>(this);
else if (strategy == Matcher::NextMatches) {
	engine = new NextMatchEngine<InputStreamElement, OutputStreamElement,
			EventDependency>(this);
}
/*else if (strategy == Matcher::RecentMatch) {
 engine = new RecentMatchEngine(this);
 } else if (strategy == Matcher::AllMatches) {
 engine = new AllMatchesEngine(this);
 } else if (strategy == Matcher::ContiyuityMatches) {
 engine = new ContiguityMatchesEngine(this);
 }*/
}

template<class InputStreamElement, class OutputStreamElement,
	class EventDependency>
void Matcher<InputStreamElement, OutputStreamElement, EventDependency>::publishResulMatches(
	const typename NFAStructure<InputStreamElement, OutputStreamElement,
			EventDependency>::NFAStructurePtr& matches) {

if (outStrategy == Matcher::Combined) {
}
//this->template getOutputChannel<0>().publish((matcher->produceTogether(matches), false));
else {
	typename MatchProducer<InputStreamElement, OutputStreamElement,
			EventDependency>::matchesList list = matches->getEvents();
	for (typename MatchProducer<InputStreamElement, OutputStreamElement,
			EventDependency>::matchConstIterator i = list.begin();
			i != list.end(); i++) {
		//std::cout << (*i)->convertInstanceToTuple() << std::endl;
		//this->getOutputDataChannel().publish( (*i)->convertInstanceToTuple(), false);
	}
}
}

}
#endif /*  Matcher_hpp_ */
