#ifndef QGSGLOBEFEATURESOURCE_H
#define QGSGLOBEFEATURESOURCE_H

#include <osgEarthFeatures/FeatureSource>

#include "qgsglobefeatureoptions.h"

class QgsGlobeFeatureSource : public osgEarth::Features::FeatureSource
{
  public:
    QgsGlobeFeatureSource( const QgsGlobeFeatureOptions& options = osgEarth::Features::ConfigOptions() );

    osgEarth::Features::FeatureCursor* createFeatureCursor( const osgEarth::Symbology::Query& query = osgEarth::Symbology::Query() ) override;

    int getFeatureCount() const override;
    osgEarth::Features::Feature* getFeature( osgEarth::Features::FeatureID fid ) override;
    osgEarth::Features::Geometry::Type getGeometryType() const override;

    QgsVectorLayer* layer() const { return mLayer; }

    const char* className() const override { return "QGISFeatureSource"; }
    const char* libraryName() const override { return "QGIS"; }

    void initialize( const osgDB::Options* dbOptions );

  protected:
    const osgEarth::Features::FeatureProfile* createFeatureProfile() override { return mProfile; }
    const osgEarth::Features::FeatureSchema& getSchema() const override { return mSchema; }

    ~QgsGlobeFeatureSource() {}

  private:
    QgsGlobeFeatureOptions mOptions;
    QgsVectorLayer* mLayer;
    osgEarth::Features::FeatureProfile* mProfile;
    osgEarth::Features::FeatureSchema mSchema;
};

#endif // QGSGLOBEFEATURESOURCE_H
