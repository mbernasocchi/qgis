#ifndef QGSGLOBEFEATURESOURCE_H
#define QGSGLOBEFEATURESOURCE_H

#include <osgEarthFeatures/FeatureSource>

#include "qgsosgearthfeatureoptions.h"

class QgisInterface;

namespace osgEarth
{
  namespace Features
  {
    using namespace osgEarth;
    using namespace osgEarth::Symbology;
    using namespace osgEarth::Drivers;

    class QgsGlobeFeatureSource : public FeatureSource
    {
      public:
        QgsGlobeFeatureSource( const QgsGlobeFeatureOptions& options = ConfigOptions() );

        FeatureCursor* createFeatureCursor( const Symbology::Query& query = Symbology::Query() );

        int getFeatureCount() const;
        Feature* getFeature( FeatureID fid );
        Geometry::Type getGeometryType() const;

        const char* className() const { return "QGISFeatureSource"; }
        const char* libraryName() const { return "QGIS"; }

        void initialize( const osgDB::Options* dbOptions );

      protected:
        const FeatureProfile* createFeatureProfile();
        const FeatureSchema& getSchema() const;

        ~QgsGlobeFeatureSource() {}

      private:
        QgsGlobeFeatureOptions mOptions;
        QgsVectorLayer* mLayer;
        FeatureProfile* mProfile;
        FeatureSchema mSchema;
    };
  } // namespace Features
} // namespace osgEarth

#endif
