#ifndef PrefixSpan_hpp_
#define PrefixSpan_hpp_

#include <list>
#include <string>

#include "Pattern.hpp"


class PrefixSpan {
public:
  typedef std::list<Pattern> PatternList;

  PrefixSpan(std::size_t minSupp) : minSupport(minSupp) {}

  PatternList mineFreqPatterns(const PatternList& dataSet);

  void mineFreqPatterns(const PatternList& dataSet, std::size_t pfxLen, PatternList& result);

  PatternList findFreqPrefixes(const PatternList& dataSet, std::size_t pfxLen);

  PatternList projectedPatternDB(const PatternList& dataSet, const Pattern& pfx);

  PatternList growPattern(const PatternList& dataSet, const Pattern& pattern, std::size_t pfxLen);

  static PatternList suppressSubPatterns(const PatternList& dataSet);

private:
  std::size_t minSupport;
};

#endif
