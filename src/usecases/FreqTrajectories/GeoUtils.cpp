#include <limits>
#include <cstdlib>
#include <cmath>

/* from https://gist.github.com/ed-flanagan/e6dc6b8d3383ef5a354a */

static const double earth_radius_km = 6371.0;

double deg2rad(double deg)
{
  return (deg * M_PI / 180.0);
}

double haversine_distance(double latitude1, double longitude1, double latitude2,
                          double longitude2)
{
  double lat1 = deg2rad(latitude1);
  double lon1 = deg2rad(longitude1);
  double lat2 = deg2rad(latitude2);
  double lon2 = deg2rad(longitude2);
  
  double d_lat = fabs(lat1 - lat2);
  double d_lon = fabs(lon1 - lon2);
  
  double a = pow(sin(d_lat / 2), 2) + cos(lat1) * cos(lat2) * pow(sin(d_lon / 2), 2);
  
  //double d_sigma = 2 * atan2(sqrt(a), sqrt(1 - a));
  double d_sigma = 2 * asin(sqrt(a));
  
  return earth_radius_km * d_sigma;
}

