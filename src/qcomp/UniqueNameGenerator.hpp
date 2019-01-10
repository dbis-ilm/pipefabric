/*
 * Copyright (C) 2014-2019 DBIS Group - TU Ilmenau, All Rights Reserved.
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
