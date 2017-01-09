#ifndef UniqueNameGenerator_hpp_
#define UniqueNameGenerator_hpp_

#include <string>
#include <mutex>

namespace pfabric {

  class UniqueNameGenerator {
    protected:
      UniqueNameGenerator();

    public:
      static UniqueNameGenerator* instance();

      std::string uniqueName(const std::string& prefix = "");

    private:
      static UniqueNameGenerator *theInstance;
      static std::mutex mCreateMtx;

      unsigned int mCounter;
      std::mutex mNextMtx;
  };

}

#endif
