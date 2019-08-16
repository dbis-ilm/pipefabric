/* These functions will be put into the main file step by step, whenever needed.
They are kinda outdated, meaning some adaptions must be done in order to use them in the main file,
so they are more like some skeleton functions. */

/* Denotes the i'th position report emitted by v prior to t. */
//alternatively optional<p>
p Last(int i, VID v, Time t) {

  //get the needed position report (if exists) by ranging over all position reports until the fitting one is found
  for (p posReport : P) {
    if ((posReport.v == v) && (30 * (i - 1) <= t - posReport.t) && (t - posReport.t < 30 * i)) {
      return posReport;
    }
  }

  //automatically assigns the corresponding NULL values to all fields in p
  //instead of optional<p>, only p as return type
  struct p nullStruct = {0};
  //return optional<p>{};

  return nullStruct;
}

/* Returns the set of all vehicles that emit position reports from segment s on expressway x while traveling in direction d during minute m. */
vector<int> cars(Minute m, XWay x, Seg s, Dir d) {

  //vector containing all vid's
  vector<int> vids;

  //get all relevant vids by ranging over all position reports
  for (p posReport : P) {
    if ((M(posReport.t) == m) && (posReport.x == x) && (posReport.s == s) && (posReport.d == d)) {
      vids.push_back(posReport.v);
    }
  }

  //make sure vids only contains distinct values
  sort(vids.begin(), vids.end());
  auto last = unique(vids.begin(), vids.end());
  vids.erase(last, vids.end());

  return vids;
}

/* Calculates the average speed of vehicle v according to all of the position reports it emits during minute m. */
float Avgsv(VID v, Minute m, XWay x, Seg s, Dir d) {

  //store the overall number of speeds and their sum
  int numSpeeds = 0;
  int sumSpeeds = 0;

  //get number of speeds and their sum by ranging over all position reports
  for (p posReport : P) {
    if ((posReport.v == v) && (M(posReport.t) == m) && (posReport.x == x) && (posReport.s == s) && (posReport.d == d)) {
      numSpeeds++;
      sumSpeeds += posReport.spd;
    }
  }

  //make sure we don't divide by zero
  if (numSpeeds == 0) {
    return 0.0;
  }

  //calculate the average value of speeds and return it
  return (static_cast<float>(sumSpeeds) / static_cast<float>(numSpeeds));
}

/* Specifies the average speed of all vehicles that emitted a position report from segment s of expressway x in direction d during minute m. */
float Avgs(Minute m, XWay x, Seg s, Dir d) {

  //get all vids emitting fitting position reports
  auto vids = cars(m, x, s, d);

  //store the overall number of Avgsvs and their sum
  int numAvgsvs = 0;
  float sumAvgsvs = 0.0;

  //get number of Avgsvs and their sum by ranging over all vehicles concerned
  for (VID v : vids) {
    numAvgsvs++;
    sumAvgsvs += Avgsv(v, m, x, s, d);
  }

  //make sure we don't divide by zero
  if (numAvgsvs == 0) {
    return 0.0;
  }

  //calculate the average value of avgsvs and return it
  return (sumAvgsvs / static_cast<float>(numAvgsvs));
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

/* Segment that is i segments downstream of s. */
int Dn(Seg s, Dir d, int i) {

  if (d == 0) {
    //min(s + 1, 99)
    if (s + 1 < 99) {
      return s + 1;
    } else {
      return 99;
    }
  } else {
    //max(s - 1, 0)
    if (s - i > 0) {
      return s - i;
    } else {
      return 0;
    }
  }
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

/* Hold if there were two vehicles stopped as of time t at the same position p of expressway x in direction d. */
bool Acc(Time t, XWay x, Pos pos, Dir d) {

  //vector for each travel lane
  vector<int> vids [3];

  //push all VIDs of cars that stopped into vids
  for (p posReport : P) {
    if ((posReport.l > 0) && (posReport.l < 4) && Stop(posReport.v, t, x, posReport.l, pos, d)) {
      vids[posReport.l - 1].push_back(posReport.v);
    }
  }

  //check if there are two different vehicles that stopped in the same lane
  for (int i = 0; i < 3; i++) {

    sort(vids[i].begin(), vids[i].end());
    auto last = unique(vids[i].begin(), vids[i].end());
    vids[i].erase(last, vids[i].end());

    if (vids[i].size() > 1) {
      return true;
    }
  }

  return false;
}

/* Holds if there was an accident in the segment that is exactly i segments downstream of s, in expressway x and in the travel lanes for direction d during minute m. */
bool Acc_in_Seg(Minute m, XWay x, Seg s, Dir d) {
  //need good implementation here because t ∈ [m, m + 1) and p ∈ [s, s + 1) (would be a nested loop of 60 and 5280)
  return true;
}


/*
  Toll processing (only toll notification):

  - calculate a toll every time a vehicle reports a position in a new segment
  - notify the driver of this toll

  - params: number of vehicles in segment, avg speed in segment, proximity of accidents
  - trigger: position report q (Type = 0, Time t, VID v, Spd spd, XWay x, Seg s, Pos p, Lane l, Dir d), q.Seg != q.OldSeg, l != EXIT
  - output: (Type = 0, VID v, Time t, Emit t', Spd Lav(M(t), x, s, d), Toll Toll(M(t), x, s , d))
  - recipient: v
  - response: t' - t <= 5sec

*/
