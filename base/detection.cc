#include <fstream>
#include <iostream>
#include "detection.h"

using namespace Eigen;
using namespace std;

namespace structured_indoor_modeling {

istream& operator >>(istream& istr, Detection& detection) {
    string header,temp;
  istr >> header
       >> detection.panorama
       >> detection.us[0] >> detection.us[1]
       >> detection.vs[0] >> detection.vs[1];
  int num_names;
  istr >> num_names;
  detection.names.resize(num_names);
  for (int i = 0; i < num_names; ++i)
    istr >> detection.names[i];
  istr >> detection.score;

  if (header == "DETECTION_WITH_ICON" || header == "DETECTION_WITH_ICON_AND_POLYGON") {
    istr >> detection.room >> detection.object;
    for (int a = 0; a < 3; ++a) {
      for (int i = 0; i < 2; ++i) {
        istr >> detection.ranges[a][i];
      }
    }
  }
  if(header == "DETECTION_WITH_ICON_AND_POLYGON"){
      istr >> temp;
      int v_num, e_num;
      istr >> v_num >> e_num;
      detection.vlist.resize(v_num);
      detection.elist.resize(e_num);
      for(int vid=0; vid<v_num; ++vid)
	  istr >> detection.vlist[vid][0] >> detection.vlist[vid][1];
      for(int eid=0; eid<e_num; ++eid)
	  istr >> detection.elist[eid][0] >> detection.elist[eid][1] >> detection.elist[eid][2];
  }
  return istr;
}

ostream& operator <<(ostream& ostr, const Detection& detection) {
  ostr << "DETECTION_WITH_ICON_AND_POLYGON" << endl
       << detection.panorama << endl
       << detection.us[0] << ' ' << detection.us[1] << endl
       << detection.vs[0] << ' ' << detection.vs[1] << endl
       << (int)detection.names.size();
  for (int i = 0; i < (int)detection.names.size(); ++i)
    ostr << ' ' << detection.names[i];
  ostr << endl
       << detection.score << endl
       << detection.room << ' ' << detection.object << endl;
  for (int a = 0; a < 3; ++a)
    ostr << detection.ranges[a][0] << ' ' << detection.ranges[a][1] << endl;
  ostr << "POLYGON" <<endl;
  ostr << detection.vlist.size()<<' '<<detection.elist.size()<<endl;
  for(const auto&v: detection.vlist)
      ostr << v[0]<<' '<<v[1]<<endl;
  for(const auto&e: detection.elist)
      ostr << e[0]<<' '<<e[1]<<' '<<e[2]<<endl;
  return ostr;
}

istream& operator >>(istream& istr, vector<Detection>& detections) {
  string header;
  int num_detections;
  istr >> header >> num_detections;
  detections.clear();
  detections.resize(num_detections);
  for (int d = 0; d < num_detections; ++d)
    istr >> detections[d];
  
  return istr;
}
  
ostream& operator <<(ostream& ostr, const vector<Detection>& detections) {
  ostr << "DETECTIONS" << endl
       << (int)detections.size() << endl;
  for (const auto& detection : detections)
    ostr << detection << endl;

  return ostr;
}

}  // namespace structured_indoor_modeling
  
