#include <Eigen/Dense>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include <gflags/gflags.h>

#include "../../base/file_io.h"
#include "../../base/indoor_polygon.h"

using namespace Eigen;
using namespace structured_indoor_modeling;
using namespace std;

namespace {

struct Polygon {
  vector<int> boundary;
  vector<vector<int> > holes;
};

void SetPolygon(const Segment& segment, const int offset, vector<Polygon>* polygons) {
  const int num_vertices = segment.vertices.size();
  // For each vertex, in-out vertex pair.
  vector<vector<pair<int, int> > > intervals(num_vertices);
  {
    for (const auto& triangle : segment.triangles) {
      for (int index0 = 0; index0 < 3; ++index0) {
        const int index1 = (index0 + 1) % 3;
        const int index2 = (index0 + 2) % 3;
        intervals[triangle.indices[index0]].push_back(make_pair(triangle.indices[index1],
                                                                triangle.indices[index2]));
      }
    }
  }

  const int kInvalid = -1;
  const pair<int, int> kInvalidPair(kInvalid, kInvalid);
  // Find concatenated intervals.
  vector<vector<pair<int, int> > > concatenated_intervals(num_vertices);
  {
    for (int v = 0; v < num_vertices; ++v) {
      const vector<pair<int, int> >& candidates = intervals[v];
      vector<bool> used(candidates.size(), false);
      while (true) {
        // Pick unused candidate.
        int first_index = kInvalid;
        for (int i = 0; i < (int)used.size(); ++i) {
          if (!used[i]) {
            first_index = i;
            break;
          }
        }
        
        if (first_index == kInvalid)
          break;
        
        used[first_index] = true;
        
        pair<int, int> interval = candidates[first_index];
        // Search forward.
        while (true) {
          // Find an unused candidate, which starts with interval.second.
          int index = kInvalid;
          for (int i = 0; i < (int)used.size(); ++i) {
            if (!used[i] && candidates[i].first == interval.second) {
              index = i;
              break;
            }
          }
          if (index == kInvalid)
            break;
          
          used[index] = true;
          interval.second = candidates[index].second;
        }
        // Search backward.
        while (true) {
          // Find an unused candidate, which starts with interval.second.
          int index = kInvalid;
          for (int i = 0; i < (int)used.size(); ++i) {
            if (!used[i] && candidates[i].second == interval.first) {
              index = i;
              break;
            }
          }
          if (index == kInvalid)
            break;
          
          used[index] = true;
          interval.first = candidates[index].first;
        }
        
        // If interval.first and interval.second are the same, vanish.
        if (interval.first == interval.second)
          continue;
        
        concatenated_intervals[v].push_back(interval);
      }
    }
  }

  // Now trace.
  vector<vector<int> > chains;
  while (true) {
    // Find a vertex that has some concatenated intervals.
    int first_vertex = kInvalid;
    pair<int, int> first_interval = kInvalidPair;
    vector<int> chain;
    for (int v = 0; v < (int)concatenated_intervals.size(); ++v) {
      if (first_vertex != kInvalid)
        break;
      for (int i = 0; i < (int)concatenated_intervals[v].size(); ++i) {
        if (concatenated_intervals[v][i].first != kInvalid) {
          first_vertex = v;
          first_interval = concatenated_intervals[v][i];

          chain.push_back(v);
          concatenated_intervals[v][i] = kInvalidPair;
          break;
        }
      }
    }

    if (first_vertex == kInvalid)
      break;
    
    // Start from first_vertex.
    // first_interval.first ---> first_vertex ---> first_interval.second.

    int current_vertex = first_vertex;
    pair<int, int> current_interval = first_interval;
    
    while (true) {
      const int next_vertex = current_interval.second;
      pair<int, int> next_interval = kInvalidPair;

      for (int i = 0; i < (int)concatenated_intervals[next_vertex].size(); ++i) {
        if (concatenated_intervals[next_vertex][i].first == current_vertex) {
          next_interval = concatenated_intervals[next_vertex][i];

          chain.push_back(next_vertex);
          concatenated_intervals[next_vertex][i] = kInvalidPair;
          break;
        }
      }
      if (next_interval == kInvalidPair) {
        cerr << "Impossible. Cannot come back..." << endl;
        exit (1);
      }

      // Cycle detected.
      if (next_interval.second == first_vertex)
        break;

      current_vertex = next_vertex;
      current_interval = next_interval;
    }
    chains.push_back(chain);
  }
  
  // If multiple chains, identify holes.
  if (chains.empty()) {
    cerr << "No chain extracted." << endl;
    exit (1);
  }
  
  if ((int)chains.size() > 1) {
    // Identify holes.
    vector<double> volumes(chains.size(), 0);

    for (int c = 0; c < chains.size(); ++c) {
      Vector3d min_xyz, max_xyz;
      for (int i = 0; i < (int)chains[c].size(); ++i) {
        const Vector3d& position = segment.vertices[chains[c][i]];
        if (i == 0) {
          min_xyz = max_xyz = position;
        } else {
          for (int a = 0; a < 3; ++a) {
            min_xyz[a] = min(min_xyz[a], position[a]);
            max_xyz[a] = max(max_xyz[a], position[a]);
          }
        }
      }
      volumes[c] = (max_xyz[0] - min_xyz[0]) + (max_xyz[1] - min_xyz[1]) + (max_xyz[2] - min_xyz[2]);
    }

    const int max_index = max_element(volumes.begin(), volumes.end()) - volumes.begin();
    if (max_index != 0)
      swap(chains[0], chains[max_index]);
  }

  // Now chains[0] is the boundary.
  // Add offset.
  for (auto& chain : chains) {
    for (auto& value : chain)
      value += offset;
  }

  Polygon polygon;
  polygon.boundary = chains[0];
  for (int i = 1; i < (int)chains.size(); ++i) {
    reverse(chains[i].begin(), chains[i].end());
    polygon.holes.push_back(chains[i]);
  }

  polygons->push_back(polygon);
  /*
  set<pair<int, int> > edges;
  for (const auto& triangle : segment.triangles) {
    for (int lhs = 0; lhs < 3; ++lhs) {
      const int rhs = (lhs + 1) % 3;
      const pair<int, int> forward(triangle.indices[lhs], triangle.indices[rhs]);
      const pair<int, int> backward(triangle.indices[rhs], triangle.indices[lhs]);

      if (edges.find(backward) != edges.end()) {
        edges.erase(backward);
      } else {
        edges.insert(forward);
      }
    }
  }

  //----------------------------------------------------------------------
  // Form chains.
  vector<vector<int> > chains;
  while (!edges.empty()) {
    vector<int> chain;
    

    chains.push_back(chain);
  }
  */
}
  
}  // namespace

int main(int argc, char* argv[]) {
  if (argc < 2) {
    cerr << "Usage: " << argv[0] << " data_directory" << endl;
    return 1;
  }
#ifdef __APPLE__
  google::ParseCommandLineFlags(&argc, &argv, true);
#else
  gflags::ParseCommandLineFlags(&argc, &argv, true);
#endif

  FileIO file_io(argv[1]);
  vector<string> indoor_polygon_files;

  indoor_polygon_files.push_back(file_io.GetIndoorPolygonSimple());
  indoor_polygon_files.push_back(file_io.GetIndoorPolygon());
  indoor_polygon_files.push_back(file_io.GetIndoorPolygonWithCeiling());
  vector<string> collada_files;
  collada_files.push_back(file_io.GetColladaSimple());
  collada_files.push_back(file_io.GetCollada());
  collada_files.push_back(file_io.GetColladaWithCeiling());

  const int kNumFiles = 2;
  for (int i = 0; i < kNumFiles; ++i) {
    IndoorPolygon indoor_polygon(indoor_polygon_files[i]);

    vector<Vector3d> vertices;
    vector<Polygon> polygons;
    
    for (int s = 0; s < indoor_polygon.GetNumSegments(); ++s) {
      const Segment& segment = indoor_polygon.GetSegment(s);
      // Do not include ceiling for the first type.
      if (i == 0 && segment.type == Segment::CEILING)
        continue;
      
      const int offset = vertices.size();
      for (const auto& vertex : segment.vertices)
        vertices.push_back(vertex);
      
      SetPolygon(segment, offset, &polygons);
    }
    
    ofstream ofstr;
    ofstr.open(collada_files[i]);
    
    ofstr << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>" << endl
          << "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">" << endl
          << "<asset>" << endl
          << "<up_axis>Z_UP</up_axis>" << endl
          << "</asset>" << endl
          << "<library_visual_scenes>" << endl
          << "<visual_scene id=\"ID1\">" << endl
          << "<node name=\"SketchUp\">" << endl
          << "<instance_geometry url=\"#ID10\">" << endl
          << "</instance_geometry>" << endl
          << "</node>" << endl
          << "</visual_scene>" << endl
          << "</library_visual_scenes>" << endl
          << "<library_geometries>" << endl
          << "<geometry id=\"ID10\">" << endl
          << "<mesh>" << endl
          << "<source id=\"ID11\">" << endl
          << "<float_array id=\"ID14\" count=\"" << (int)vertices.size() * 3 << "\">";
    for (const auto& vertex : vertices) {
      ofstr << vertex[0] << ' ' << vertex[1] << ' ' << vertex[2] << ' ';
    }
    ofstr << "</float_array>" << endl
          << "<technique_common>" << endl
          << "<accessor count=\"" << (int)vertices.size() << "\" source=\"#ID14\" stride=\"3\">" << endl
          << "<param name=\"X\" type=\"float\" />" << endl
          << "<param name=\"Y\" type=\"float\" />" << endl
          << "<param name=\"Z\" type=\"float\" />" << endl
          << "</accessor>" << endl
          << "</technique_common>" << endl
          << "</source>" << endl
          << "<vertices id=\"ID13\">" << endl
          << "<input semantic=\"POSITION\" source=\"#ID11\" />" << endl
          << "</vertices>" << endl
          << "<polygons count=\"" << (int)polygons.size() << "\" material=\"Material2\">" << endl
          << "<input offset=\"0\" semantic=\"VERTEX\" source=\"#ID13\" />" << endl;
    for (const auto& polygon : polygons) {
      if (polygon.holes.empty()) {
        ofstr << "<p>";
        for (const auto& index : polygon.boundary)
          ofstr << index << ' ';
        ofstr << "</p>" << endl;
      } else {
        ofstr << "<ph>" << endl
              << "<p>";
        for (const auto& index : polygon.boundary)
          ofstr << index << ' ';
        ofstr << "</p>" << endl;
        for (const auto& hole : polygon.holes) {
          ofstr << "<h>";
          for (const auto& index : hole)
            ofstr << index << ' ';
          ofstr << "</h>" << endl;
        }
        ofstr << "</ph>" << endl;
      }
    }
    ofstr << "</polygons>" << endl
          << "</mesh>" << endl
          << "</geometry>" << endl
          << "</library_geometries>" << endl
          << "<scene>" << endl
          << "<instance_visual_scene url=\"#ID1\" />" << endl
          << "</scene>" << endl
          << "</COLLADA>" << endl;
    
    ofstr.close();
  }
  
  /*
  Floorplan floorplan(floorplan_file);

  // vcoords
  vector<Vector3d> vertices;
  vector<int> vcounts;
  for (int room = 0; room < floorplan.GetNumRooms(); ++room) {
    for (int wall = 0; wall < floorplan.GetNumWalls(room); ++wall) {
      vcounts.push_back(4);
      const int next_wall = (wall + 1) % floorplan.GetNumWalls(room);

      {
        Vector2d current = floorplan.GetRoomVertexLocal(room, wall);
        Vector2d next = floorplan.GetRoomVertexLocal(room, next_wall);

        const double floor = floorplan.GetFloorHeight(room);
        const double ceiling = floorplan.GetCeilingHeight(room);
        vertices.push_back(Vector3d(current[0], current[1], floor));
        vertices.push_back(Vector3d(next[0], next[1], floor));
        vertices.push_back(Vector3d(next[0], next[1], ceiling));
        vertices.push_back(Vector3d(current[0], current[1], ceiling));
      }
    }

    vcounts.push_back(floorplan.GetNumWalls(room));
    for (int wall = 0; wall < floorplan.GetNumWalls(room); ++wall) {
      Vector2d current = floorplan.GetRoomVertexLocal(room, wall);
      const double floor = floorplan.GetFloorHeight(room);
      vertices.push_back(Vector3d(current[0], current[1], floor));
    }
  }  
  
  cout << "<source id=\"ID5\">" << endl
       << "<float_array id=\"ID8\" count=\"" << (int)vertices.size() * 3  << "\">";

  for (const auto& vertex : vertices)
    cout << vertex[0] << ' ' << vertex[1] << ' ' << vertex[2] << ' ';
    
  cout << "</float_array>" << endl
       << "<technique_common>" << endl
       << "<accessor count=\"" << (int)vertices.size() << "\" source=\"#ID8\" stride=\"3\">" << endl
       << "<param name=\"X\" type=\"float\" />" << endl
       << "<param name=\"Y\" type=\"float\" />" << endl
       << "<param name=\"Z\" type=\"float\" />" << endl
       << "</accessor>" << endl
       << "</technique_common>" << endl
       << "</source>" << endl;

  //----------------------------------------------------------------------
  cout << endl << endl;
  //----------------------------------------------------------------------

  cout << "<polylist count=\"" << (int)vcounts.size() << "\" material=\"Material2\">" << endl
       << "<input offset=\"0\" semantic=\"VERTEX\" source=\"#ID7\" />" << endl
       << "<vcount>";

  for (const auto& vcount : vcounts)
    cout << vcount << ' ';
  cout << "</vcount>" << endl
       << "<p>";

  for (int i = 0; i < (int)vertices.size(); ++i)
    cout << i << ' ';

  cout << "</p>" << endl
       << "</polylist>" << endl;
  */
  return 0;
}
