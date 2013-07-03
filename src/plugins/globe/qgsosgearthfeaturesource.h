#ifndef QGIS_FEATURE_SOURCE_H
#define QGIS_FEATURE_SOURCE_H

#include <osgEarthFeatures/FeatureSource>

#include "qgsvectorlayer.h"

#include "qgsosgearthfeatureoptions.h"

class QgisInterface;

namespace osgEarth
{
  namespace Features
  {
    using namespace osgEarth;
    using namespace osgEarth::Symbology;
    using namespace osgEarth::Drivers;

    class QGISFeatureSource : public FeatureSource
    {
      public:
        QGISFeatureSource( const QGISFeatureOptions& options = ConfigOptions() );

        FeatureCursor* createFeatureCursor( const Symbology::Query& query = Symbology::Query() );

        int getFeatureCount() const;
        Feature* getFeature( FeatureID fid );
        Geometry::Type getGeometryType() const;

        const char* className() const { return "QGISFeatureSource"; }
        const char* libraryName() const { return "QGIS"; }

        void initialize( const std::string& referenceURI );

      protected:
        const FeatureProfile* createFeatureProfile();

        ~QGISFeatureSource() {}

      private:
        QGISFeatureOptions options_;
        QgsVectorLayer* mLayer;
        QgisInterface* iface_;
        FeatureProfile* profile_;
    };
  } // namespace Features
} // namespace osgEarth

#endif
