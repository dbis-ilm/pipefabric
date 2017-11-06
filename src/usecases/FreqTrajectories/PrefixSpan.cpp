#include <unordered_map>
#include <string>
#include <iostream>

#include "PrefixSpan.hpp"

using namespace std;

PrefixSpan::PatternList PrefixSpan::projectedPatternDB(const PatternList& dataSet, const Pattern& pfx) {
  PatternList result;
  for (auto& seq : dataSet) {
    auto pseq = seq.findSuffix(pfx);
    if (!pseq.isEmpty())
      result.push_back(pseq);
  }
  return result;
}

PrefixSpan::PatternList PrefixSpan::mineFreqPatterns(const PatternList& dataSet) {
  PatternList result;
  mineFreqPatterns(dataSet, 0, result);
  return result;
}

void PrefixSpan::mineFreqPatterns(const PatternList& dataSet, std::size_t pfxLen, PatternList& result) {
    for (auto& p : dataSet) {
      std::cout << p << std::endl;
    }

  auto prefixes = findFreqPrefixes(dataSet, pfxLen + 1);
  for (auto& prefix : prefixes) {
    auto pdb = projectedPatternDB(dataSet, prefix);
    for (auto& p : pdb) {
      std::cout << p << std::endl;
    }

    auto patterns = growPattern(pdb, prefix, pfxLen + 2);
    for (auto& pattern : patterns) {
      if (pattern.frequency() >= minSupport) {
        result.push_back(pattern);

        PatternList nextResult;

        mineFreqPatterns(pdb, pfxLen + 1, nextResult);
        result.insert(result.end(), nextResult.begin(), nextResult.end());
      }
    }
  }
}

PrefixSpan::PatternList PrefixSpan::findFreqPrefixes(const PatternList& dataSet, std::size_t pfxLen) {
  PatternList result;
  unordered_map<string, Pattern> prefixMap;

  for (auto& seq : dataSet) {
    auto subSeqs = seq.findSubPatterns(pfxLen);
    for (auto& sub : subSeqs) {
      auto key = sub.toString();
      auto iter = prefixMap.find(key);
      if (iter != prefixMap.end()) 
        iter->second.incrFrequency();
      else
        prefixMap.insert({ key, Pattern(sub) });
    }
  }
  for (auto& entry : prefixMap) {
    if (entry.second.frequency() >= minSupport) {
      result.push_back(entry.second);
    }
  }
  return result;
}

PrefixSpan::PatternList PrefixSpan::growPattern(const PatternList& dataSet, const Pattern& pattern, 
    std::size_t pfxLen) {
  PatternList result;
  unordered_map<int, size_t> items;
  for (auto& seq : dataSet) {
    if (seq.length() <= pfxLen - 1)
      continue;

   if (seq.isEqualN(pattern, pfxLen - 1) && seq.length() >= pfxLen) {
     auto key = seq[pfxLen - 1];
     auto iter = items.find(key);
     if (iter == items.end()) 
       items.insert({ key, 1 });
     else
       items[key] = iter->second + 1;
   }
  }
  for (auto iter = items.begin(); iter != items.end(); iter++) {
    result.push_back(Pattern(pattern.concat(iter->first), iter->second));
  }
  return result;
}

PrefixSpan::PatternList PrefixSpan::suppressSubPatterns(const PatternList& dataSet) {
  PatternList result;
  int p1 = 0, p2 = 0;
  for (auto it1 = dataSet.begin(); it1 != dataSet.end(); it1++, p1++) {
    bool suppress = false;
    p2 = 0;
    for (auto it2 = dataSet.begin(); it2 != dataSet.end(); it2++, p2++) {
      if (it1 == it2)
        continue;
      if ((*it1 == *it2 && p1 < p2) || it2->contains(*it1))
        suppress = true;
    }
    if (!suppress) 
      result.push_back(*it1);
  }

  return result;
}
