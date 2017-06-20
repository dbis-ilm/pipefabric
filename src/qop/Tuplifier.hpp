/*
 * Copyright (c) 2014-17 The PipeFabric team,
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

#ifndef Tuplifier_hpp_
#define Tuplifier_hpp_

#include <list>
#include <string>
#include <thread>
#include <vector>
#include <mutex>

#include <boost/unordered/unordered_map.hpp>

#include "qop/OperatorMacros.hpp"
#include "qop/TriggerNotifier.hpp"
#include "qop/UnaryTransform.hpp"

namespace pfabric {
  struct TuplifierParams {
 /**
   * An enumeration to specify the tuplifying mode
   */
  enum TuplifyMode {
    ORDERED,     //< we assume triples arrive ordered on the subject
    WINDOW,      //< we maintain a time window and publish all tuples (including
                 //incomplete tuples) if they are outdated
    PUNCTUATED,  //< at a punctuation we publish all tuples received so far
    COMPLETED    //< as soon as a tuple is complete, we publish it
  };
};

/**
 * @brief this class provides an operator to transform a set of primtive tuples
 * (triples)
 *  into a complete tuple by grouping a set of triples based on a common subject
 * using
 *  a specific schema (a list of RDF predicates). In real applications, we pay
 * more attention to
 *  meaningful higher-level information to be extracted from LSD rather than
 * primitive ones.
 *  Bases on TuplifyMode, the tuple can be produced as soon as a triple with a
 * different subject is
 *  received (ordered mode).
 *  If the triple stream is not ordered according to the subject, a window-based
 * strategy can be
 *  used (window mode).
 *  Here, the incoming triples are kept in a window either for a given time
 * period or a number of triples.
 *  As soon as the first triple of the group  with the same subject is outdated,
 * the complete tuple
 *  is produced (completed mode).
 */
template <class InputStreamElement, class OutputStreamElement>
class Tuplifier
    : public UnaryTransform<InputStreamElement, OutputStreamElement> {
  PFABRIC_UNARY_TRANSFORM_TYPEDEFS(InputStreamElement, OutputStreamElement)
 public:
 /**
   * A typedef for predicates list
   */
  typedef std::vector<std::string> PredicateList;
  typedef std::function<Timestamp(const InputStreamElement&)> TimestampExtractorFunc;

  /**
   * A default constructor where tuplifier instance receives the predicates
   * list, the tuplifying mode
   * and a window size in case of window mode to carry out a periodic
   * notification.
   * @param predList the predicate list
   * @param m the tuplifying mode
   * @param ws a window size for periodic notification (default = 0)
   */
  Tuplifier(const std::initializer_list<std::string>& predList, TuplifierParams::TuplifyMode m, unsigned int ws = 0)
      : mode(m),
        currentSubj(),
        notifier(
            ws > 0 && ws < UINT_MAX
                ? new TriggerNotifier(
                      boost::bind(&Tuplifier::notificationCallback, this), ws)
                : nullptr) {
    // assert(tupleSchema.size() == predList.size() + 1);
    int i = 0;
    for (auto li = predList.begin(); li != predList.end(); li++, i++) {
      predicates.insert(std::make_pair(*li, i + 1));
    }
  }

  Tuplifier(TimestampExtractorFunc func, 
      const std::initializer_list<std::string>& predList, TuplifierParams::TuplifyMode m, unsigned int ws = 0) : 
      Tuplifier(predList, m, ws) {
      mTimestampExtractor = func;
   }

  ~Tuplifier() {}

  /**
   * @brief Bind the callback for the data channel.
   */
  BIND_INPUT_CHANNEL_DEFAULT(InputDataChannel, Tuplifier, processDataElement);
  /**
   * @brief Bind the callback for the punctuation channel.
   */
  BIND_INPUT_CHANNEL_DEFAULT(InputPunctuationChannel, Tuplifier,
                             processPunctuation);

  /**
   * A callback function for the notifier. This function will be called when the
   * window
   * size is expired to produce all tuples including incomplete tuples.
   */
  void notificationCallback() {
    std::lock_guard<std::mutex> guard(bufMtx);
    produceOutdatedTuples();
  }

 private:
  /**
   * This method is invoked when a tuple arrives from the publisher. It applies
   * the projection
   * function and sends the new tuple to its subscribers.
   *
   * \param data the incoming tuple
   * \param c the channel at which we receive the tuple
   * \param outdated indicates whether the tuple is new or invalidated now
   * (outdated == true)
   */
  void processDataElement(const InputStreamElement& data,
                          const bool outdated = false) {
    if (mode == TuplifierParams::ORDERED) {
      const std::string& subj = get<0>(*data);
      if (currentSubj.empty() || subj == currentSubj) {
        // add triple to buffer
        addToBuffer(data);
        if (currentSubj.empty()) currentSubj = subj;
      } else {
        // we received a new tuple, let's publish the previous tuple
        produceTupleForSubject(currentSubj);
        // and start a new one
        currentSubj = subj;
        addToBuffer(data);
      }
    } else {
      // just add to tuple to the buffer
      addToBuffer(data);

      if (mode == TuplifierParams::COMPLETED) {
        // we try to publish all completed tuples
        produceCompleteTuples();
      }
    }
  }

  /**
   * This method is invoked when a punctuation arrives. It simply forwards the
   * punctuation
   * to the subscribers.
   *
   * \param data the incoming punctuation tuple
   * \param c the channel at which we receive the tuple
   * \param outdated indicates whether the tuple is new or invalidated now
   * (outdated == true)
   */
  void processPunctuation(const PunctuationPtr& pp) {
    if (mode == TuplifierParams::ORDERED) {
      produceTupleForSubject(currentSubj);
    } else {
      produceAllTuples();
    }
    this->getOutputPunctuationChannel().publish(pp);
  }
  /**
   * A container for triples of a particular tuple
   */
  typedef std::list<InputStreamElement> TripleList;
  struct BufferItem {
    TripleList tripleList;  //< the actual list of triples
    unsigned int matches;   //< the number of matches predicates
    Timestamp arrivalTime;  //< the arrival time of the first triple with the
                            //same subject

    BufferItem() : matches(0), arrivalTime(0) {}
    BufferItem(Timestamp t) : matches(0), arrivalTime(t) {}
  };
  /**
   * A buffer to store the items inside (BufferItem).
   */
  typedef boost::unordered_map<std::string, BufferItem> BufferMap;

  typedef boost::unordered_map<std::string, int> PredicateMap;
  /**
   * Produces the result tuple for the given subject.
   * @param subj the subject
   */
  void produceTupleForSubject(const std::string& subj) {
    typename BufferMap::iterator lit = tupleBuffer.find(subj);
    if (lit == tupleBuffer.end()) return;

    TripleList& tbuf = lit->second.tripleList;
    produceTuple(tbuf);
    // remove it from the map
    tupleBuffer.erase(lit);
  }

  /**
   * Inserts the given triple into the buffer according the subject component.
   * @param data the incoming triple where it should be stored in a particular
   * tuples
   */
  void addToBuffer(const InputStreamElement& data) {
    std::lock_guard<std::mutex> guard(bufMtx);

    PredicateMap::iterator pit = predicates.find(get<1>(*data));
    if (pit == predicates.end())
      // we don't need this predicate
      return;

    const std::string& subj = get<0>(*data);
    typename BufferMap::iterator it = tupleBuffer.find(subj);
    if (it != tupleBuffer.end()) {
      it->second.matches++;
      it->second.tripleList.push_back(data);
    } else {
      const Timestamp ts = this->mTimestampExtractor == nullptr ? 0 : this->mTimestampExtractor(data);
      
      BufferItem item(ts);
      item.tripleList.push_back(data);
      item.matches = 1;
      tupleBuffer.insert(std::make_pair(subj, item));
    }
  }

  /**
   * Produces the result tuple from the triples in the given list.
   * @tbuf  tbuf a list of triples
   */
  void produceTuple(TripleList& tbuf) {
    std::vector<std::string> data(predicates.size() + 1);
    data[0] = get<0>(*tbuf.front());
    // construct the tuple
    for (auto it = tbuf.begin(); it != tbuf.end(); it++) {
      const std::string& pred = get<1>(**it);
      const std::string& obj = get<2>(**it);
      PredicateMap::iterator pit = predicates.find(pred);
      assert(pit != predicates.end());
      int field = pit->second;
      data[field] = obj;
    }
    auto tn = OutputDataElementTraits::create(data);
    this->getOutputDataChannel().publish(tn, false);
  }

  /**
   * Scans the triple buffer and produces all tuples which are complete.
   */
  void produceCompleteTuples() {
    for (auto it = tupleBuffer.begin(); it != tupleBuffer.end();) {
      BufferItem& bitem = it->second;
      // for completeness we check the number of matches predicates
      if (bitem.matches == predicates.size()) {
        produceTuple(bitem.tripleList);
        it = tupleBuffer.erase(it);
      } else
        it++;
    }
  }
  /**
   * Scans the triple buffer and produces all tuples including incomplete ones.
   */
  void produceAllTuples() {
    for (auto it = tupleBuffer.begin(); it != tupleBuffer.end(); it++) {
      TripleList& tbuf = it->second.tripleList;
      produceTuple(tbuf);
    }
    tupleBuffer.clear();
  }

  /**
   * Scans the triple buffer and produces all tuples which have arrival time
   * equal to zero
   */
  void produceOutdatedTuples() {
    for (auto it = tupleBuffer.begin(); it != tupleBuffer.end();) {
      BufferItem& bitem = it->second;
      if (bitem.arrivalTime == 0) {
        produceTuple(bitem.tripleList);
        // remove it from the map
        it = tupleBuffer.erase(it);
      } else
        it++;
    }
  }

   TimestampExtractorFunc mTimestampExtractor; //< a function for extracting timestamps from a tuple
  BufferMap
      tupleBuffer;  //< a buffer for all received triples not yet published
  PredicateMap predicates;  //< a map containing all predicates and their
                            //position in the tuple
  TuplifierParams::TuplifyMode mode;         //< the mode for constructing tuples from triples
  std::string currentSubj;  //< the current subject in the triple stream (only
                            //useful for ordered)
  std::unique_ptr<TriggerNotifier> notifier;  //< the notifier object which
                                              //triggers the computation
                                              //periodically
  std::mutex bufMtx;                          //< a mutex for synchronization
};
}

#endif
