class QgsGlobeFeatureUtils
{
%TypeHeaderCode
#include <qgsglobefeatureutils.h>
using namespace osg;
%End
  public:
    static Vec3d pointFromQgsPoint( const QgsPoint& pt );

    static Polygon* polygonFromQgsPolygon( const QgsPolygon& poly );
/*
    static Geometry* geometryFromQgsGeometry( const QgsGeometry& geom );
*/
    static Feature* featureFromQgsFeature( const QgsFields& fields, const QgsFeature& feat );
};

