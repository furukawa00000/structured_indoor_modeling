#ifndef OBJECT_RENDERER_H_
#define OBJECT_RENDERER_H_

#include <Eigen/Dense>
#include <QGLFunctions>
#include <fstream>
#include <vector>
#include <string>

#include "../base/detection.h"

namespace structured_indoor_modeling {

class Floorplan;
class IndoorPolygon;
class TreeOrganizer;

typedef std::pair<Eigen::Vector3d, Eigen::Vector3d> ColoredPoint;
typedef std::vector<ColoredPoint> ColoredPointCloud;

struct BoundingBox {
  Eigen::Vector3d corners[4];
};

 class ObjectRenderer : protected QGLFunctions {
 public:
   ObjectRenderer(const Floorplan& floorplan,
                  const IndoorPolygon& indoor_polygon,
                  const std::string& detection_file);
  virtual ~ObjectRenderer();

  void Init(const std::string data_directory);
  void InitGL();

  void RenderAll(const double position);
  void RenderAll(const TreeOrganizer& tree_organizer,
                 const double air_to_tree_progress,
                 const double animation) const;

  void RenderIcons(const double alpha, const double animation);

  int GetNumRooms() const;
  int GetNumObjects(const int room) const;
  const std::vector<float>& GetObject(const int room, const int object) const;

  bool Toggle();
  
 private:
  void ComputeBoundingBoxes();
  void RenderDesk(const Detection& detection,
                  const Eigen::Vector3d bounding_boxes[4],
                  const double animation) const;
  void RenderSofa(const Detection& detection,
                  const Eigen::Vector3d vs[4],
                  const double animation) const;
  void RenderLamp(const Detection& detection,
                  const Eigen::Vector3d vs[4],
                  const double animation) const;
  void RenderDefault(const Detection& detection,
                     const Eigen::Vector3d vs[4],
                     const double animation) const;

  void RotateToFrontal(const Detection& detection,
                       const Eigen::Vector3d vs[4],
                       Eigen::Vector3d rotated_vs[4]) const;
    
  const Floorplan& floorplan;
  const IndoorPolygon& indoor_polygon;
  // private:
  bool render;
  // For each room, for each object, a colored point cloud.
  // std::vector<std::vector<ColoredPointCloud> > colored_point_clouds;

  std::vector<std::vector<std::vector<float> > > vertices;
  std::vector<std::vector<std::vector<float> > > colors;

  std::vector<std::vector<std::vector<float> > > vertices_org;
  std::vector<std::vector<std::vector<float> > > colors_org;

  // Bounding boxes for each object.
  std::vector<std::vector<BoundingBox> > bounding_boxes;

  std::vector<Detection> detections;
  
};

}  // namespace structured_indoor_modeling 

#endif  // OBJECT_RENDERER_H_
