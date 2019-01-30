//Examples of operators: TopologyTest.cpp
//.toStream for parallelism
//https://github.com/walmartlabs/linearroad for data generation
//.queue für operatoren, die viel rechnen

/*
  First version of implementation, no attempt to be efficient, focus is a working implementation.
  I tried to stay as close to the notations and names used in the Linear Road paper as possible.

  Implementation TODO's:
  - If correct, smarter/more efficient implementation of data structures + parallelism (substreams)
*/

#include "pfabric.hpp"
#include <vector>
#include <boost/optional.hpp>

using namespace pfabric;
using namespace std;
using boost::optional;


/* ----------------------------------------------------------------- */
/* New data types. */
/* ----------------------------------------------------------------- */

//types of input data for better readablity
using ReportType = int;   //position reports (type = 0), others aren't needed
using Time = int;   //(0...10799), time at whicht the position report was emitted, there are 10800 seconds in a 3 hours simulation period
using VID = int;    //(0...MAXINT), vehicle identifier
using Spd = int;    //(0...100), speed/velocity in MPH
using XWay = int;   //(0...L-1), expressway
using Lane = int;   //(0...4), Lane of expressway (0 = ENTRY, 1-3 = TRAVEL, 4 = EXIT)
using Dir = int;    //(0...1) Direction (0 = Eastbound, 1 = Westbound)
using Seg = int;    //(0...99), mile-long segment
using Pos = int;    //(0...527999), horizontal position (in feet)
//all other input data isn't needed (only used for historical queries)

//types needed for outgoing tuples
using Emit = int;   //(0...10799), specifying the time the toll notification is emitted
using Toll = int;   //calculated toll

//other helpful data types
using Minute = int;


/* ----------------------------------------------------------------- */
/* Global variables. */
/* ----------------------------------------------------------------- */

//size of the sliding window (in seconds)
auto slidingWindowSize = 600;
auto globalTimeSeconds = 0;
auto globalTimeMinute = 1;

//only for testing purposes
auto activePosReports = 0;


/* ----------------------------------------------------------------- */
/* Used tuples. */
/* ----------------------------------------------------------------- */

//input tuple
typedef TuplePtr<ReportType, Time, VID, Spd, XWay, Lane, Dir, Seg, Pos, int, int, int, int, int, int> lrTuples;


/* ----------------------------------------------------------------- */
/* Used structs. */
/* ----------------------------------------------------------------- */

//all necessary information of a position report
struct p {
  Time  t;
  VID   v;
  Spd   spd;
  XWay  x;
  Lane  l;
  Dir   d;
  Seg   s;
  Pos   pos;
};

//all necessary information of an accident alert
struct accAlert {
  ReportType  repType;
  Time        t;
  Emit        t_emit;
  Seg         s;
};

//all necessary information of a toll notification
struct tollNote {
  ReportType  repType;
  VID         v;
  Time        t;
  Emit        t_emit;
  Spd         spd;
  Toll        toll;
};

//used as a key in the map posReports
struct posID {
  Time  t;
  VID   v;

  //overwrite existing operators for == and < to use this struct as a key in a map
  bool const operator==(const posID &o) const {
    return t == o.t && v == o.v;
  }
  bool const operator<(const posID &o) const {
    return v < o.v || (v == o.v && t < o.t);
  }
};

//used as data struct in the accidents set
struct accidentID {
  XWay    x;
  Seg     s;
  VID     v;
  Minute  m;
  Dir     d;

  //overwrite existing operators for == and < to use this struct as a key in a map
  bool const operator==(const accidentID &o) const {
    return s == o.s && x == o.x && v == o.v && m == o.m && d == o.d;
  }
  bool const operator<(const accidentID &o) const {
    return x < o.x || (x == o.x && s < o.s) || (x == o.x && s == o.s && v < o.v) || (x == o.x && s == o.s && v == o.v && m < o.m) || (x == o.x && s == o.s && v == o.v && m == o.m && d < o.d);
  }
};

//used as a key in the map spdOfSegments
struct segID {
  Minute  m;
  XWay    x;
  Seg     s;
  Dir     d;

  //overwrite existing operators for == and < to use this struct as a key in a map
  bool const operator==(const segID &o) const {
    return m == o.m && x == o.x && s == o.s && d == o.d;
  }
  bool const operator<(const segID &o) const {
    return m < o.m || (m == o.m && x < o.x) || (m == o.m && x == o.x && s < o.s) || (m == o.m && x == o.x && s == o.s && d < o.d);
  }
};

//used as data struct to calculate average speed of an vehicle
struct avgSpd {
  Spd   sum;
  int   n;
};


/* ----------------------------------------------------------------- */
/* Used data structures. */
/* ----------------------------------------------------------------- */

//map containing all position reports
map<posID, p> posReports;
//current segment of each vehicle
map<VID,Seg> segs;
//holds all current accidents
set<accidentID> accidents;
//used to calculate average speed on each segment
//also used to get number of cars at one segment
map<segID,map<VID,avgSpd>> spdOfSegments;


/* ----------------------------------------------------------------- */
/* Functions not defined in the paper. */
/* ----------------------------------------------------------------- */

/* Helper function to print an element of p. */
void printPosReport(p posReport) {
  cout << "Time: " << posReport.t << ", ";
  cout << "VID: " << posReport.v << ", ";
  cout << "Spd: " << posReport.spd << ", ";
  cout << "XWay: " << posReport.x << ", ";
  cout << "Lane: " << posReport.l << ", ";
  cout << "Dir: " << posReport.d << ", ";
  cout << "Seg: " << posReport.s << ", ";
  cout << "Pos: " << posReport.pos << endl;
}

/* Helper function to print an element of posID. */
void printPosReportKey(posID key) {
  cout << "Time: " << key.t << ", ";
  cout << "VID: " << key.v << endl;
}

/* Helper function to print an accident alert. */
void printAccidentAlert(accAlert alert) {
  cout << "ReportType: " << alert.repType << ", ";
  cout << "Time: " << alert.t << ", ";
  cout << "Emit: " << alert.t_emit << ", ";
  cout << "Seg: " << alert.s << endl;
}

/* Helper function to print a toll notification. */
void printTollNotification(tollNote note) {
  cout << "ReportType: " << note.repType << ", ";
  cout << "VID: " << note.v << ", ";
  cout << "Time: " << note.t << ", ";
  cout << "Emit: " << note.t_emit << ", ";
  cout << "Spd: " << note.spd << ", ";
  cout << "Toll: " << note.toll << endl;
}

/* Helper function to determine whether a vehicle just entered a new segment. */
bool changedSegment(VID v, Seg s) {
  Seg oldSeg;

  //try to get the current segment of the vehicle
  try {
    //throws an out-of-range exception if element does not exist
    oldSeg = segs.at(v);
  } catch (const out_of_range& oor) {
    segs.insert(pair<VID,Seg>(v, s));
    return false;
  }

  //check if the vehicle changed its segment
  if (oldSeg != s) {
    segs.erase(v);
    segs.insert(pair<VID,Seg>(v, s));
    //vehicle changed its segment
    return true;
  }

  //vehicle didn't change its segment
  return false;
}

/* Helper function to erase tuple from the posReport map. */
void eraseFromPosReports(Time t, VID v) {

  //only for testing purposes
  activePosReports--;

  //get the key of the pos report
  posID key = {t, v};
  //erase from posReports
  posReports.erase(key);
}

/* Helper function to erase tuple from segs map. */
void eraseFromSegs(VID v) {
  //erase from segs
  segs.erase(v);
}

/* Helper function to add a tuple to the pos reports map. */
void addToPosReports(p posReport) {

  //only for testing purposes
  activePosReports++;

  //get the key of the pos report from tuple
  posID key = {posReport.t, posReport.v};
  //add to posReports
  posReports.insert(pair<posID,p>(key,posReport));
}

//TODO: should be done during outdated notification
/* Deletes all speed entries older than 5 minutes. */
void deleteSpeedEntries(Minute m) {

  //use cbegin and cend because we are erasing elements while iterating over it
  for (auto it = spdOfSegments.cbegin(); it != spdOfSegments.cend();) {
    auto key = (*it).first;
    //check if older than 5 minutes
    if (key.m <= m) {
      it = spdOfSegments.erase(it);
    } else {
      ++it;
    }
  }
}


/* ----------------------------------------------------------------- */
/* (Modified) functions defined in the paper. */
/* ----------------------------------------------------------------- */

/* Minute number of t. */
Minute M(Time t) {
  //+1 because first minute of simulation is 1
  return (t/60) + 1;
}

/* Denotes the i'th position report emitted by v prior to t. */
p Last(int i, VID v, Time t) {

  //get the bounds for the iterator
  Time timeLowerBound = t - 30 * i;
  Time timeUpperBound = (t - 30 * (i - 1)) - 1;

  posID keyLowerBound = {timeLowerBound, v};
  posID keyUpperBound = {timeUpperBound, v};

  //get a fitting posReport
  for (std::map<posID,p>::iterator it = posReports.lower_bound(keyLowerBound); it != posReports.upper_bound(keyUpperBound); ++it) {
    //key
    posID key = it -> first;
    //element
    p posReport = it -> second;

    return posReport;
  }

  //automatically assigns the corresponding NULL values to all fields in p
  //if no fitting posReport was found
  struct p nullStruct = {0};

  return nullStruct;
}

/* Holds if the four most recent position reports from v as of time t are from the same location. */
bool Stop(VID v, Time t, XWay x, Lane l, Pos pos, Dir d) {

  for (int i = 1; i <= 4; i++) {
    p last = Last(i, v, t);
    if ((last.x != x) || (last.l != l) || (last.pos != pos) || (last.d != d)) {
      return false;
    }
  }
  return true;
}

/* Returns the segment if there was an accident in the segment that is exactly i segments downstream of s, in expressway x and in the travel lanes for direction d during minute m. */
Seg DetectAccident(p posReport) {

  //get accident key
  accidentID accID = {posReport.x, posReport.s, posReport.v, M(posReport.t), posReport.d};

  //only insert into accidents, if lane equals 'TRAVEL'
  if (posReport.l != 0 && posReport.l != 4) {
    //adjust accidents set
    if (Stop(posReport.v, posReport.t, posReport.x, posReport.l, posReport.pos, posReport.d)) {
      //create entry
      accidents.emplace(accID);
    } else {
      //make sure to delete from accidents if inside
      accidents.erase(accID);
    }
  } else {
    //make sure to delete from accidents if inside
    accidents.erase(accID);
  }

  //if more than one entry, an accident occured
  int numEntriesFound = 0;
  //check if accident occured
  for (set<accidentID>::iterator it = accidents.begin(); it != accidents.end(); ++it) {
    accidentID accID = *it;

    //check if entry fits XWay and segment
    //therefore, we need the direction, to either look upstream or downstream

    if (posReport.d == 0) {
      if (accID.x == posReport.x && accID.s >= posReport.s && accID.s <= posReport.s + 4 && accID.m == M(posReport.t - 1) && accID.d == posReport.d) {
        numEntriesFound++;
      }
    } else {
      if (accID.x == posReport.x && accID.s >= posReport.s - 4 && accID.s <= posReport.s && accID.m == M(posReport.t - 1) && accID.d == posReport.d) {
        numEntriesFound++;
      }
    }

    //two vehicles stopped at the same segment, return segment
    if (numEntriesFound >= 2) {
      return accID.s;
    }
  }

  //no accident occured
  return -1;
}

/* Add pos report to map spdOfSegments, used to calculate avg spd. */
void addToSpeedEntries(p posReport) {

  segID key = {M(posReport.t), posReport.x, posReport.s, posReport.d};

  //try to get the current entry for this segment in this minute
  try {

    //throws an out-of-range exception if element does not exist
    auto segMap = &spdOfSegments.at(key);

    //try to get the current entry for this vehicle
    try {
      //throws an out-of-range exception if element does not exist
      auto speedEntry = &((*segMap).at(posReport.v));
      //change existing entry
      (*speedEntry).sum += posReport.spd;
      (*speedEntry).n++;

    } catch (const out_of_range& oor) {
      //no entry for this vehicle existed, create new one
      avgSpd avgSpdEntry = {posReport.spd, 1};
      (*segMap).insert(pair<VID, avgSpd>(posReport.v, avgSpdEntry));
    }

  } catch (const out_of_range& oor) {
    //no entry for this segment in this minute existed
    //create new speed entry
    avgSpd avgSpdEntry = {posReport.spd, 1};
    //create new segment map value and insert speed entry
    map<VID,avgSpd> segMap;
    segMap.insert(pair<VID, avgSpd>(posReport.v, avgSpdEntry));
    //insert new seg map
    spdOfSegments.insert(pair<segID,map<VID,avgSpd>>(key, segMap));
  }
}

/* Specifies the average speed of all vehicles that emitted a position report from segment s of expressway x in direction d during minute m. */
float Avgs(Minute m, XWay x, Seg s, Dir d) {
  segID key = {m, x, s, d};
  float sumSpeeds = 0.0;
  float numSpeeds = 0.0;

  //try to get the current entry for this segment in this minute
  try {
    //throws an out-of-range exception if element does not exist
    auto segMap = &spdOfSegments.at(key);
    //iterate over map to calculate average speed of all vehicles
    for (std::map<VID,avgSpd>::iterator it=(*segMap).begin(); it!=(*segMap).end(); ++it) {
      //spd entry
      avgSpd spdEntry = it -> second;
      //get avg speed and increase counter
      sumSpeeds += static_cast<float>(spdEntry.sum) / static_cast<float>(spdEntry.n);
      numSpeeds += 1.0;
    }
  } catch (const out_of_range& oor) {
    //no entries exist for this minute, so the average speed is zero
    return 0.0;
  }

  return sumSpeeds / numSpeeds;
}

/* Latest average velocity. Computes the average speed on some expressway x, segment s and direction d by averaging vehicle speeds over the 5 minutes
that precede minute m = M(t). */
Spd Lav(Minute m, XWay x, Seg s, Dir d) {
  //store the overall number of Avgs and their sum
  int numAvgs = 0;
  float sumAvgs = 0.0;

  //get number of Avgs's and their sum by ranging over all vehicles concerned
  for (int i = 1; i <= 5; i++) {
    if (m - i > 0) {
      numAvgs++;
      sumAvgs += Avgs(m - i, x, s, d);
    }
  }

  //make sure we don't divide by zero
  if (numAvgs == 0) {
    return 0;
  }

  //calculate the average speed and return it
  return (static_cast<int>(sumAvgs / static_cast<float>(numAvgs)));
}

/* Returns the set of all vehicles that emit position reports from segment s on expressway x while traveling in direction d during minute m. */
int Cars(Minute m, XWay x, Seg s, Dir d) {
  segID key = {m, x, s, d};
  //try to get the current entry for this segment in this minute
  try {
    //throws an out-of-range exception if element does not exist
    auto segMap = &spdOfSegments.at(key);
    //get the size of segMap and return it
    return (*segMap).size();
  } catch (const out_of_range& oor) {
    //no entries exist for this minute, so the number of vehicles is zero
    return 0;
  }
}


/* ----------------------------------------------------------------- */
/* Where programm execution starts. */
int main(int argc, char* argv[]) {
  
  if(argc < 2) {
    std::cout<<"Please provide the datafile name, it must be saved in /build/3rdparty/linroad/data/ folder."<<std::endl;
    std::cout<<"For an example, simply run the command \"./LinRoad datafile20seconds.dat\"."<<std::endl;
    return 0;
  }
  string fn_arg = argv[1];
  string fn = "./3rdparty/linroad/data/"+fn_arg;

  PFabricContext ctx;
  auto t = ctx.createTopology();

  //stream of tuples from data driver
  auto s = t -> newStreamFromLinRoad<lrTuples>(fn)

  //only take posReports
  .where([](auto tp, bool) {
    return get<0>(tp) == 0;
  })

  //timestamp is assigned to the second value (Time) of the tuple
  .assignTimestamps<1>()
  .print()

  //adds this tuple to the sliding window of length slidingWindowSize
  .slidingWindow(WindowParams::RangeWindow, slidingWindowSize)

  //get notified once the tuple is outdated
  .notify([](auto tp, bool outdated) {
    //check if tuple is outdated
    if(outdated) {
      eraseFromPosReports(get<1>(tp), get<2>(tp));
      eraseFromSegs(get<2>(tp));
    }
  })

  //only take position reports and add them to posReports, also adjust globalTime
  .where([](auto tp, bool outdated) {

    //get posReport from tuple
    p posReport = {get<1>(tp), get<2>(tp), get<3>(tp), get<4>(tp), get<5>(tp), get<6>(tp), get<7>(tp), get<8>(tp)};

    if(posReport.t > 12000) {
      cout << "Problem with time: " << posReport.t << endl;
    }
    //adjust global time if necessary
    if (posReport.t > globalTimeSeconds) {
      globalTimeSeconds = posReport.t;
      //cout<<"Current tuple: "<<get<0>(tp)<<","<<get<1>(tp)<<","<<get<2>(tp)<<","<<get<3>(tp)
      //    <<","<<get<4>(tp)<<","<<get<5>(tp)<<","<<get<6>(tp)<<","<<get<7>(tp)<<","<<get<8>(tp)<<std::endl;

      if (M(globalTimeSeconds) > globalTimeMinute) {
        globalTimeMinute = M(globalTimeSeconds);

        //delete all average speed entries older than M(t) - 1 from spdOfSegments
        deleteSpeedEntries(M(globalTimeMinute - 6));

        //only for testing purposes
        cout << "Entered minute " << globalTimeMinute << " of simulation." << endl;
        cout << "Number of active posReports: " << activePosReports << endl;
      }
    }

    //only process active reports
    if (!outdated) {
      addToPosReports(posReport);
      addToSpeedEntries(posReport);

      //check for accident and update set accidents
      int accSegment = DetectAccident(posReport);

      //check for toll notification and accident alert
      if ((posReport.l != 4) && changedSegment(posReport.v, posReport.s)) {

        //get lav and cars
        int lav = Lav(M(posReport.t), posReport.x, posReport.s, posReport.d);
        int cars = Cars(M(posReport.t) - 1, posReport.x, posReport.s, posReport.d);
        int toll = 0;

        //calculate toll
        if ((lav < 40) && (accSegment == -1) && (cars > 50)) {
          toll = 2 * (cars - 50)^2;
        }

        //print toll notification
        tollNote note = {0, posReport.v, posReport.t, globalTimeSeconds, lav, toll};

        //normally, all toll notes would have to be printed
        if (toll > 0) {
          printTollNotification(note);
        }

        if (accSegment != -1) {
          //generate accident alert
          accAlert alert = {1, posReport.t, globalTimeSeconds, accSegment};
          printAccidentAlert(alert);
        }
      }
    }

    return true;
  })
  ;

  t -> start();
  t -> wait();
}
