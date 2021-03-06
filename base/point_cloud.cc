#include <Eigen/Dense>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <limits>
#include "file_io.h"
#include "point_cloud.h"

using namespace Eigen;
using namespace std;

namespace structured_indoor_modeling {

const int PointCloud::kDepthPositionOffset = 1;

PointCloud::PointCloud() {
  InitializeMembers();
}

void PointCloud::InitializeMembers() {
  center.resize(3);
  center[0] = 0;
  center[1] = 0;
  center[2] = 0;

  bounding_box.resize(6);
  bounding_box[0] = numeric_limits<double>::max();
  bounding_box[2] = numeric_limits<double>::max();
  bounding_box[4] = numeric_limits<double>::max();
  bounding_box[1] = -numeric_limits<double>::max();
  bounding_box[3] = -numeric_limits<double>::max();
  bounding_box[5] = -numeric_limits<double>::max();

  depth_width = 0;
  depth_height = 0;
  num_objects = 0;
}

bool PointCloud::Init(const FileIO& file_io, const int panorama) {
  return Init(file_io.GetLocalPly(panorama).c_str());
}

bool PointCloud::Init(const std::string& filename) {
  InitializeMembers();
  
  ifstream ifstr;
  ifstr.open(filename.c_str());
  if (!ifstr.is_open()) {
    ifstr.close();
    return false;
  }

  string stmp;
  for (int i = 0; i < 6; ++i)
    ifstr >> stmp;
  int num_points;
  ifstr >> num_points;
  for (int i = 0; i < 36; ++i)
    ifstr >> stmp;

  ifstr >> stmp;
  bool has_object_id = false;
  if(stmp == "property") {
    has_object_id = true;
    for(int i = 0; i < 3; ++i)
      ifstr >> stmp;
  }
    
  points.clear();
  points.resize(num_points);

  const int kInvalidObjectId = -1;

  // To handle different point format.
  for (auto& point : points) {
    ifstr >> point.depth_position[1] >> point.depth_position[0]
          >> point.position[0] >> point.position[1] >> point.position[2]
          >> point.color[0] >> point.color[1] >> point.color[2]
          >> point.normal[0] >> point.normal[1] >> point.normal[2]
          >> point.intensity;

    if (has_object_id){
      ifstr >> point.object_id;
    } else {
      point.object_id = kInvalidObjectId;
    }
    
    point.depth_position[0] -= kDepthPositionOffset;
    point.depth_position[1] -= kDepthPositionOffset;
  }

  ifstr.close();

  Update();

  return true;
}
  
void PointCloud::Write(const std::string& filename) {
  ofstream ofstr;
  ofstr.open(filename.c_str());
  if (!ofstr.is_open()) {
    cerr << "Failed in writing: " << filename << endl;
    exit (1);
  }

  ofstr << "ply" << endl
        << "format ascii 1.0" << endl
        << "element vertex " << (int)points.size() << endl
        << "property int height" << endl
        << "property int width" << endl
        << "property float x" << endl
        << "property float y" << endl
        << "property float z" << endl
        << "property uchar red" << endl
        << "property uchar green" << endl
        << "property uchar blue" << endl
        << "property float nx" << endl
        << "property float ny" << endl
        << "property float nz" << endl
        << "property uchar intensity" << endl
	<< "property uchar object_id" << endl
	<< "end_header" << endl;

  for (const auto& point : points) {
    ofstr << point.depth_position[1] + kDepthPositionOffset << ' '
          << point.depth_position[0] + kDepthPositionOffset << ' '
          << point.position[0] << ' '
          << point.position[1] << ' '
          << point.position[2] << ' '
          << (int)point.color[0] << ' '
          << (int)point.color[1] << ' '
          << (int)point.color[2] << ' '
          << point.normal[0] << ' '
          << point.normal[1] << ' '
          << point.normal[2] << ' '
          << point.intensity << ' '
	  << point.object_id << endl;
  }

  ofstr.close();
}

void PointCloud::WriteObject(const string& filename, const int objectid){
    vector<Point>object_points;
    for(const auto&pt: points){
	if(pt.object_id == objectid)
	    object_points.push_back(pt);
    }
    
    PointCloud objectcloud;
    objectcloud.AddPoints(object_points);
    objectcloud.Write(filename);
}

void PointCloud::Rotate(const Eigen::Matrix3d& rotation) {
  Matrix4d transformation;
  for (int y = 0; y < 3; ++y) {
    for (int x = 0; x < 3; ++x) {
      transformation(y, x) = rotation(y, x);
    }
    transformation(y, 3) = 0.0;
  }
  transformation(3, 0) = 0;
  transformation(3, 1) = 0;
  transformation(3, 2) = 0;
  transformation(3, 3) = 1;

  Transform(transformation);
}

void PointCloud::Translate(const Eigen::Vector3d& translation){
     for(auto &v: points){
	  v.position += translation;
     }
     Update();
}
  
void PointCloud::ToGlobal(const FileIO& file_io, const int panorama) {
  Matrix4d local_to_global;
  {
    ifstream ifstr;
    ifstr.open(file_io.GetLocalToGlobalTransformation(panorama).c_str());
    
    char ctmp;
    ifstr >> ctmp;
    for (int y = 0; y < 3; ++y) {
      for (int x = 0; x < 4; ++x) {
        ifstr >> local_to_global(y, x);
      }
    }
    ifstr.close();
    local_to_global(3, 0) = 0;
    local_to_global(3, 1) = 0;
    local_to_global(3, 2) = 0;
    local_to_global(3, 3) = 1;
  }
  Transform(local_to_global);
}

void PointCloud::AddPoints(const PointCloud& point_cloud, bool mergeid){
    AddPoints(point_cloud.points, mergeid);
}


void PointCloud::AddPoints(const vector<Point>& new_points, bool mergeid) {
     int orinum = points.size();
  points.insert(points.end(), new_points.begin(), new_points.end());
  if(!mergeid){
      for(int i = orinum; i< (int)points.size(); i++){
	  points[i].object_id += num_objects;
      }
  }
  Update();
}

void PointCloud::RemovePoints(const std::vector<int>& indexes) {
  vector<bool> keep(points.size(), true);
  for (const auto& index : indexes) {
    assert(0 <= index && index < (int)points.size());
    keep[index] = false;
  }
  
  vector<Point> new_points;
  for (int p = 0; p < (int)points.size(); ++p) {
    if (keep[p])
      new_points.push_back(points[p]);
  }

  new_points.swap(points);
  Update();
}

void PointCloud::GetObjectIndice(int objectid, vector<int>&indices) const{
  for(int i=0; i<GetNumPoints(); i++){
    const Point& curpt = GetPoint(i);
    if(curpt.object_id == objectid)
      indices.push_back(i);
  }
}

void PointCloud::GetObjectPoints(int objectid, vector<Point>&object_points) const{
    object_points.clear();
    for(const auto& pt: points){
	if(pt.object_id == objectid)
	    object_points.push_back(pt);
    }
}

void PointCloud::GetObjectBoundingbox(int objectid, vector<double>&bbox) const{
    bbox.clear();
    bbox.resize(6);
    bbox[0] = numeric_limits<double>::max();
    bbox[1] = numeric_limits<double>::min();
    bbox[2] = numeric_limits<double>::max();
    bbox[3] = numeric_limits<double>::min();
    bbox[4] = numeric_limits<double>::max();
    bbox[5] = numeric_limits<double>::min();
    for(int ptid=0; ptid<GetNumPoints(); ptid++){
	Point curpt = GetPoint(ptid);
	if(curpt.object_id == objectid){
	    bbox[0] = std::min(bbox[0], curpt.position[0]);
	    bbox[1] = std::max(bbox[1], curpt.position[0]);
	    bbox[2] = std::min(bbox[2], curpt.position[1]);
	    bbox[3] = std::max(bbox[3], curpt.position[1]);
	    bbox[4] = std::min(bbox[4], curpt.position[2]);
	    bbox[5] = std::max(bbox[5], curpt.position[2]);
	}
    }
}
    
void PointCloud::SetAllColor(float r,float g,float b){
  for(auto &v: points){
    v.color[0] = r;
    v.color[1] = g;
    v.color[2] = b;
  }
}

void PointCloud::SetColor(int ind, float r,float g,float b){
     assert(ind >= 0 && ind < (int)points.size());
     points[ind].color[0] = r;
     points[ind].color[1] = g;
     points[ind].color[2] = b;
}

// yasu To follow the convention in the class, this name should be
// really Update instead of update. This update makes it very
// difficult to understand this class.  It is not very clear at first
// sight, what members are reset, and what members are not. Have to
// read the comment carefully and compare against the header file.
// Ideally, one should be able to understand the functionality of a
// function easily without reading the code or a comment carefully.
//
// To be simple, this class should update all the variables in my opinion.
//
// 
// Update point cloud center, depth_width, depth_height, bounding_box. 
// Note that num_object is not changed, to avoid confusing.
void PointCloud::Update(){
  InitializeMembers();

  for (const auto& point : points) {
    center += point.position;

    depth_width = max(point.depth_position[1] + 1, depth_width);
    depth_height = max(point.depth_position[0] + 1, depth_height);

    num_objects = max(num_objects, point.object_id + 1);
    
    bounding_box[0] = min(point.position[0],bounding_box[0]);
    bounding_box[1] = max(point.position[0],bounding_box[1]);
    bounding_box[2] = min(point.position[1],bounding_box[2]);
    bounding_box[3] = max(point.position[1],bounding_box[3]);
    bounding_box[4] = min(point.position[2],bounding_box[4]);
    bounding_box[5] = max(point.position[2],bounding_box[5]);
  }
  if (!points.empty())
    center /= (int)points.size();
}
   
double PointCloud::GetBoundingboxVolume(){
  if(bounding_box.size() == 0)
    return 0;
  return (bounding_box[1]-bounding_box[0])*(bounding_box[3]-bounding_box[2])*(bounding_box[5]-bounding_box[4]);
}

double PointCloud::GetObjectBoundingboxVolume(const int objectid){
    vector<double>bbox;
    GetObjectBoundingbox(objectid, bbox);
    if(bbox[1] <= bbox[0] || bbox[3] <= bbox[2] || bbox[5] <= bbox[4])
	return 0;
    return (bbox[1]-bbox[0]) * (bbox[3] - bbox[2]) * (bbox[5] - bbox[4]);
}
  
void PointCloud::Transform(const Eigen::Matrix4d& transformation) {
  for (auto& point : points) {
    Vector4d position4(point.position[0], point.position[1], point.position[2], 1.0);
    position4 = transformation * position4;
    point.position = Vector3d(position4[0], position4[1], position4[2]);

    Vector4d normal4(point.normal[0], point.normal[1], point.normal[2], 0.0);
    normal4 = transformation * normal4;
    point.normal = Vector3d(normal4[0], normal4[1], normal4[2]);
  }
  Update();
}

void PointCloud::RandomSampleScale(const double scale) {
  const int target_size = static_cast<int>(scale * points.size());
  random_shuffle(points.begin(), points.end());
  points.resize(target_size);
}

void PointCloud::RandomSampleCount(const int max_count) {
  if (max_count < (int)points.size()) {
    random_shuffle(points.begin(), points.end());
    points.resize(max_count);
  }
}
  
//----------------------------------------------------------------------
void ReadPointClouds(const FileIO& file_io, std::vector<PointCloud>* point_clouds) {
  cout << "Reading pointclouds" << flush;
  const int num_panoramas = GetNumPanoramas(file_io);

  point_clouds->clear();
  point_clouds->resize(num_panoramas);
  for (int p = 0; p < num_panoramas; ++p) {
    cout << '.' << flush;
    point_clouds->at(p).Init(file_io, p);
    point_clouds->at(p).ToGlobal(file_io, p);
  }
  cout << " done." << endl;
}  

void ReadObjectPointClouds(const FileIO& file_io,
                           const int num_rooms,
                           std::vector<PointCloud>* object_point_clouds) {
  object_point_clouds->clear();
  object_point_clouds->resize(num_rooms);
  cout << "Reading object clouds..." << flush;
  for (int room = 0; room < num_rooms; ++room) {
    cout << '.' << flush;
    //object_point_clouds->at(room).Init(file_io.GetRefinedObjectClouds(room));
    object_point_clouds->at(room).Init(file_io.GetObjectPointClouds(room));
  }
  cout << "done" << endl;
}

void ReadRefinedObjectPointClouds(const FileIO& file_io,
			   const int num_rooms,
			   std::vector<PointCloud>* object_point_clouds) {
    object_point_clouds->clear();
    object_point_clouds->resize(num_rooms);
    cout << "Reading object clouds..." << flush;
    for (int room = 0; room < num_rooms; ++room) {
	cout << '.' << flush;
	//object_point_clouds->at(room).Init(file_io.GetRefinedObjectClouds(room));
	object_point_clouds->at(room).Init(file_io.GetRefinedObjectClouds(room));
    }
    cout << "done" << endl;
}
  
}  // namespace structured_indoor_modeling
