/*
 * Copyright (C) 2014-2019 DBIS Group - TU Ilmenau, All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PipeFabric is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with PipeFabric. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <future>
#include "pfabric.hpp"

using namespace pfabric;
using namespace ns_types;
using namespace std::chrono_literals;

using T1 = TuplePtr<int, int>;
using RunningAvg = Aggregator1<T1, AggrAvg<double, double>, 1>;
using Sum = Aggregator1<T1, AggrSum<int>, 1>;

void runTimeExperiment1();
void runTimeExperiment2();
void runTimeExperiment3();
void runTimeExperiment4();
void runTimeExperiment5();
void runTimeExperiment6();
void runTupleExperiment1();
void runTupleExperiment2();
void runTupleExperiment3();
void runTupleExperiment4();
void runTupleExperiment5();

auto func = [](auto tp) {
  return Timestamp((long)(get<0>(tp)* 1000 * 1000));
};

/* ---------------------------------------------------------------------------------------------- */
int main(int argc, char **argv) {
  //TODO: Some experiments are not working correctly,
  //      needs fixing in aggregation or window implementation
  enum class WinType { NA, Time, Tuple};
  auto winType = WinType::NA;
  auto expNum = 0u;

  /// Input parsing
  if (argc == 1) {
    /// Do nothing
  } else if (argc != 3) {
    std::cout << "wrong number of arguments\n"
              << "usage: " << argv[0]
              << " [windowType(time/tuple) experimentNumber(1-6)]\n";
    return -1;
  } else {
    winType = strcmp(argv[1], "time") == 0 ?  WinType::Time :
              strcmp(argv[1], "tuple") == 0 ? WinType::Tuple : WinType::NA;
    if (winType == WinType::NA) {
      std::cout << "Invalid window type (allowed: time or tuple)\n";
      return -1;
    }
    expNum = atoi(argv[2]);
  }

  switch(winType) {
    case WinType::Time : {
      switch(expNum) {
        case 1: runTimeExperiment1(); break;
        case 2: runTimeExperiment2(); break;
        case 3: runTimeExperiment3(); break;
        case 4: runTimeExperiment4(); break;
        case 5: runTimeExperiment5(); break;
        case 6: runTimeExperiment6(); break;
        default: std::cout << "Invalid experiment number" << std::endl;
      }
    } break;
    case WinType::Tuple : {
      switch(expNum) {
        case 1: runTupleExperiment1(); break;
        case 2: runTupleExperiment2(); break;
        case 3: runTupleExperiment3(); break;
        case 4: runTupleExperiment4(); break;
        case 5: runTupleExperiment5(); break;
        default: std::cout << "Invalid experiment number" << std::endl;
      }
    } break;
    default : {
      std::cout << "Running all now ...\n" << std::endl;
      runTimeExperiment1();
      runTimeExperiment2();
      runTimeExperiment4();
      runTimeExperiment5();
      runTimeExperiment6();
      runTupleExperiment2();
      runTupleExperiment3();
      runTupleExperiment5();
    }
  }
}

/* -------------------------------------------------------------------------- */

void runTimeExperiment1() {
  const std::array<T1, 10> input = {
    makeTuplePtr(10,10), makeTuplePtr(11,20), makeTuplePtr(12,30),
    makeTuplePtr(13,40), makeTuplePtr(14,50), makeTuplePtr(15,60),
    makeTuplePtr(16,70), makeTuplePtr(17,80), makeTuplePtr(18,90),
    makeTuplePtr(19,100)
  };
  StreamGenerator<T1>::Generator gen([&input] (size_t n) {
    return input[n];
  });

  Topology t;
  auto s = t.streamFromGenerator<T1>(gen, input.size())
    .assignTimestamps(func)
    .tumblingWindow(WindowParams::RangeWindow, 3)
    .aggregate<RunningAvg>(/*TriggerByTimestamp, 3*/)
    .print(std::cout);

  std::cout << "\nRunning Time-based Window Experiment 1" << std::endl;
  t.start(true);
  t.wait();
}

/* ---------------------------------------------------------------------------------------------- */

void runTimeExperiment2() {
  const std::array<T1, 3> input = {
    makeTuplePtr(30,10), makeTuplePtr(31,20), makeTuplePtr(36,30)
  };
  StreamGenerator<T1>::Generator gen([&input] (size_t n) {
    return input[n];
  });

  Topology t;
  auto s = t.streamFromGenerator<T1>(gen, input.size())
    .assignTimestamps(func)
    .slidingWindow(WindowParams::RangeWindow, 5)
    .print(std::cout);

  std::cout << "\nRunning Time-based Window Experiment 2" << std::endl;
  t.start(true);
  t.wait();
}

/* ---------------------------------------------------------------------------------------------- */

void runTimeExperiment3() {
  std::cout << "The Time-based Window Experiment 3 was designed for Coral8 and is not relevant here\n" << std::endl;
}

/* ---------------------------------------------------------------------------------------------- */

void runTimeExperiment4() {
  const std::array<T1, 6> input = {
    makeTuplePtr(3,10), makeTuplePtr(5,20), makeTuplePtr(5,30),
    makeTuplePtr(5,40), makeTuplePtr(5,50), makeTuplePtr(7,60)
  };
  StreamGenerator<T1>::Generator gen([&input] (size_t n) {
    return input[n];
  });

  Topology t;
  auto s = t.streamFromGenerator<T1>(gen, input.size())
    .assignTimestamps(func)
    .slidingWindow(WindowParams::RangeWindow, 4)
    .aggregate<Sum>()
    .print(std::cout);

  std::cout << "\nRunning Time-based Window Experiment 4" << std::endl;
  t.start(true);
  t.wait();
}

/* ---------------------------------------------------------------------------------------------- */

void runTimeExperiment5() {
  const std::array<T1, 10> input = {
    makeTuplePtr(11,10), makeTuplePtr(12,20), makeTuplePtr(13,30),
    makeTuplePtr(14,40), makeTuplePtr(15,50), makeTuplePtr(16,60),
    makeTuplePtr(17,70), makeTuplePtr(18,80), makeTuplePtr(19,90),
    makeTuplePtr(20,100)
  };
  StreamGenerator<T1>::Generator gen([&input] (size_t n) {
    return input[n];
  });

  Topology t;
  auto s = t.streamFromGenerator<T1>(gen, input.size())
    .assignTimestamps(func)
    .tumblingWindow(WindowParams::RangeWindow, 3)
    .aggregate<Sum>(/*TriggerByTimestamp, 3*/)
    .print(std::cout);

  std::cout << "\nRunning Time-based Window Experiment 5" << std::endl;
  t.start(true);
  t.wait();
}

/* ---------------------------------------------------------------------------------------------- */

void runTimeExperiment6() {
  const std::array<T1, 10> input = {
    makeTuplePtr(10,10), makeTuplePtr(11,20), makeTuplePtr(12,30),
    makeTuplePtr(13,40), makeTuplePtr(14,50), makeTuplePtr(15,60),
    makeTuplePtr(16,70), makeTuplePtr(17,80), makeTuplePtr(18,90),
    makeTuplePtr(19,100)
  };
  StreamGenerator<T1>::Generator gen([&input] (size_t n) {
    return input[n];
  });

  Topology t;
  auto s = t.streamFromGenerator<T1>(gen, input.size())
    .assignTimestamps(func)
    .tumblingWindow(WindowParams::RangeWindow, 3)
    .aggregate<Sum>()
    .print(std::cout);

  std::cout << "\nRunning Time-based Window Experiment 6" << std::endl;
  t.start(true);
  t.wait();
}

/* ---------------------------------------------------------------------------------------------- */

void runTupleExperiment1() {
  std::cout << "Tuple-based Window Experiment 4 is not applicable as the sliding distance cannot be set\n" << std::endl;
}

/* ---------------------------------------------------------------------------------------------- */

void runTupleExperiment2() {
  const std::array<T1, 10> input = {
    makeTuplePtr(10,10), makeTuplePtr(11,20), makeTuplePtr(12,30),
    makeTuplePtr(13,40), makeTuplePtr(14,50), makeTuplePtr(15,60),
    makeTuplePtr(16,70), makeTuplePtr(17,80), makeTuplePtr(18,90),
    makeTuplePtr(19,100)
  };
  StreamGenerator<T1>::Generator gen([&input] (size_t n) {
    return input[n];
  });

  Topology t;
  auto s = t.streamFromGenerator<T1>(gen, input.size())
    .assignTimestamps(func)
    .tumblingWindow(WindowParams::RowWindow, 3)
    .aggregate<Sum>()
    .print(std::cout);

  std::cout << "\nRunning Tuple-based Window Experiment 2" << std::endl;
  t.start(true);
  t.wait();
}

/* ---------------------------------------------------------------------------------------------- */

void runTupleExperiment3() {
  const std::array<T1, 10> input = {
    makeTuplePtr(10,10), makeTuplePtr(10,20), makeTuplePtr(11,30),
    makeTuplePtr(12,40), makeTuplePtr(12,50), makeTuplePtr(12,60),
    makeTuplePtr(12,70), makeTuplePtr(13,80), makeTuplePtr(14,90),
    makeTuplePtr(15,100)
  };
  StreamGenerator<T1>::Generator gen([&input] (size_t n) {
    return input[n];
  });

  Topology t;
  auto s = t.streamFromGenerator<T1>(gen, input.size())
    .assignTimestamps(func)
    .tumblingWindow(WindowParams::RowWindow, 1)
    .aggregate<RunningAvg>()
    .print(std::cout);

  std::cout << "\nRunning Tuple-based Window Experiment 3" << std::endl;
  t.start(true);
  t.wait();
}

/* ---------------------------------------------------------------------------------------------- */

void runTupleExperiment4() {
  std::cout << "Tuple-based Window Experiment 4 is not applicable as the sliding distance cannot be set\n" << std::endl;
}

/* ---------------------------------------------------------------------------------------------- */

void runTupleExperiment5() {
  const std::array<T1, 10> input = {
    makeTuplePtr(10,10), makeTuplePtr(10,20), makeTuplePtr(11,30),
    makeTuplePtr(12,40), makeTuplePtr(12,50), makeTuplePtr(12,60),
    makeTuplePtr(12,70), makeTuplePtr(13,80), makeTuplePtr(14,90),
    makeTuplePtr(15,100)
  };
  StreamGenerator<T1>::Generator gen([&input] (size_t n) {
    return input[n];
  });

  Topology t;
  auto s = t.streamFromGenerator<T1>(gen, input.size())
    .assignTimestamps(func)
    .tumblingWindow(WindowParams::RowWindow, 2)
    .aggregate<Sum>()
    .print(std::cout);

  std::cout << "\nRunning Tuple-based Window Experiment 5" << std::endl;
  t.start(true);
  t.wait();
}
