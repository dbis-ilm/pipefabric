#include "UniqueNameGenerator.hpp"
#include "fmt/format.h"

using namespace pfabric;

UniqueNameGenerator* UniqueNameGenerator::theInstance = nullptr;
std::mutex UniqueNameGenerator::mCreateMtx;

UniqueNameGenerator::UniqueNameGenerator() : mCounter(0) {
}

UniqueNameGenerator* UniqueNameGenerator::instance() {
  std::lock_guard<std::mutex> lock(mCreateMtx);
  if (theInstance == nullptr) 
    theInstance = new UniqueNameGenerator();
  return theInstance;
}

std::string UniqueNameGenerator::uniqueName(const std::string& prefix) {
  std::lock_guard<std::mutex> lock(mNextMtx);
  mCounter++;
  return fmt::format("{0}_{1}", prefix, mCounter);
}
