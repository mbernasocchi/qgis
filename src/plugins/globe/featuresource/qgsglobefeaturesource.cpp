#include <osgDB/ReaderWriter>
#include <osgDB/FileNameUtils>

#include <qgsfeature.h>
#include <qgsfeatureiterator.h>
#include <qgsgeometry.h>
#include <qgslogger.h>
#include <qgsrectangle.h>
#include "qgsvectorlayer.h"

#include "qgsglobefeaturecursor.h"
#include "qgsglobefeatureutils.h"
#include "qgsglobefeaturesource.h"


QgsGlobeFeatureSource::QgsGlobeFeatureSource( const QgsGlobeFeatureOptions& options ) :
    mOptions( options ),
    mLayer( 0 ),
    mProfile( 0 )
{
}

void QgsGlobeFeatureSource::initialize( const osgDB::Options* dbOptions )
{
  Q_UNUSED( dbOptions )
  mLayer = mOptions.layer();

  // create the profile
  osgEarth::SpatialReference* ref = osgEarth::SpatialReference::create( mLayer->crs().toWkt().toStdString() );
  if ( 0 == ref )
  {
    std::cout << "Cannot find the spatial reference" << std::endl;
    return;
  }
  QgsRectangle ext = mLayer->extent();
  osgEarth::GeoExtent geoext( ref, ext.xMinimum(), ext.yMinimum(), ext.xMaximum(), ext.yMaximum() );
  mProfile = new osgEarth::Features::FeatureProfile( geoext );
  mSchema = QgsGlobeFeatureUtils::schemaForFields( mLayer->pendingFields() );
}

osgEarth::Features::FeatureCursor* QgsGlobeFeatureSource::createFeatureCursor( const osgEarth::Symbology::Query& query )
{
  QgsFeatureRequest request;

  if ( query.expression().isSet() )
  {
    QgsDebugMsg( QString( "Ignoring query expression '%1'" ). arg( query.expression().value().c_str() ) );
  }

  if ( query.bounds().isSet() )
  {
    QgsRectangle bounds( query.bounds()->xMin(), query.bounds()->yMin(), query.bounds()->xMax(), query.bounds()->yMax() );
    request.setFilterRect( bounds );
  }

  QgsFeatureIterator it = mLayer->getFeatures( request );
  return new QgsGlobeFeatureCursor( mLayer->pendingFields(), it );
}

osgEarth::Features::Feature* QgsGlobeFeatureSource::getFeature( osgEarth::Features::FeatureID fid )
{
  QgsFeature feat;
  mLayer->getFeatures( QgsFeatureRequest().setFilterFid( fid ) ).nextFeature( feat );
  return QgsGlobeFeatureUtils::featureFromQgsFeature( mLayer->pendingFields(), feat );
}

osgEarth::Features::Geometry::Type QgsGlobeFeatureSource::getGeometryType() const
{
  switch ( mLayer->geometryType() )
  {
    case  QGis::Point:
      return osgEarth::Features::Geometry::TYPE_POINTSET;

    case QGis::Line:
      return osgEarth::Features::Geometry::TYPE_LINESTRING;

    case QGis::Polygon:
      return osgEarth::Features::Geometry::TYPE_POLYGON;

    default:
      return osgEarth::Features::Geometry::TYPE_UNKNOWN;
  }

  return osgEarth::Features::Geometry::TYPE_UNKNOWN;
}

int QgsGlobeFeatureSource::getFeatureCount() const
{
  return mLayer->featureCount();
}

class QgsGlobeFeatureSourceFactory : public osgEarth::Features::FeatureSourceDriver
{
  public:
    QgsGlobeFeatureSourceFactory()
    {
      supportsExtension( "osgearth_feature_qgis", "QGIS feature driver for osgEarth" );
    }

    virtual osgDB::ReaderWriter::ReadResult readObject( const std::string& file_name, const osgDB::Options* options ) const override
    {
      // this function seems to be called for every plugin
      // we declare supporting the special extension "osgearth_feature_qgis"
      if ( !acceptsExtension( osgDB::getLowerCaseFileExtension( file_name ) ) )
      return osgDB::ReaderWriter::ReadResult::FILE_NOT_HANDLED;

      return osgDB::ReaderWriter::ReadResult( new QgsGlobeFeatureSource( getFeatureSourceOptions( options ) ) );
    }
  };

REGISTER_OSGPLUGIN( osgearth_feature_qgis, QgsGlobeFeatureSourceFactory )
