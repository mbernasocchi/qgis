// qt includes
#include <QList>

// osg / osgearth includes
#include <osgDB/ReaderWriter>
#include <osgDB/FileNameUtils>

#include <osgEarthFeatures/FeatureSource>
#include <osgEarth/Registry>

// qgis includes
#include <qgisinterface.h>
#include <qgsfeature.h>
#include <qgsfeatureiterator.h>
#include <qgsgeometry.h>
#include <qgslogger.h>
#include <qgsmapcanvas.h>
#include <qgsrectangle.h>
#include <qgsvectordataprovider.h>
#include "qgsvectorlayer.h"

// plugin includes
#include "qgsglobefeaturecursor.h"
#include "qgsglobefeatureutils.h"
#include "qgsosgearthfeaturesource.h"

using namespace osgEarth::Features;

namespace osgEarth
{
  namespace Features
  {
    using namespace osgEarth;
    using namespace osgEarth::Symbology;
    using namespace osgEarth::Drivers;

    QgsGlobeFeatureSource::QgsGlobeFeatureSource( const QgsGlobeFeatureOptions& options ) :
        mOptions( options ),
        mLayer( 0 ),
        mProfile( 0 )
    {
    }


    void QgsGlobeFeatureSource::initialize( const osgDB::Options* dbOptions )
    {
#if 0
      std::string layerName = options_.getConfig().value( std::string( "layerId" ), std::string( "" ) );

      layer_ = 0;
      QList<QgsMapLayer*> layers = iface_->mapCanvas()->layers();
      for ( QList<QgsMapLayer*>::const_iterator lit = layers.begin(); lit != layers.end(); ++lit )
      {
        if (( *lit )->name().toStdString() == layerName )
        {
          layer_ = dynamic_cast<QgsVectorLayer*>( *lit );
          if ( !layer_ )
            continue;

          break;
        }
      }
      if ( !layer_ )
      {
        std::cout << "Cannot find layer with name " << layerName << std::endl;
        return;
      }
#endif
      mLayer = mOptions.layer();

      QgsVectorDataProvider* provider = mLayer->dataProvider();

      // create the profile
      SpatialReference* ref = SpatialReference::create( provider->crs().toWkt().toStdString() );
      if ( 0 == ref )
      {
        std::cout << "Cannot find the spatial reference" << std::endl;
        return;
      }
      QgsRectangle next = provider->extent();
      GeoExtent ext( ref, next.xMinimum(), next.yMinimum(), next.xMaximum(), next.yMaximum() );
      mProfile = new FeatureProfile( ext );
#if 0
      Q_FOREACH( const QgsField& field, mLayer->pendingFields() )
      {
        mSchema.push_back( field.name().toStdString(), ATTRTYPE_DOUBLE );
      }
#endif
    }

    const FeatureProfile* QgsGlobeFeatureSource::createFeatureProfile()
    {
      return mProfile;
    }

    const FeatureSchema& QgsGlobeFeatureSource::getSchema() const
    {
      return FeatureSource::getSchema();
    }

    FeatureCursor* QgsGlobeFeatureSource::createFeatureCursor( const Symbology::Query& query )
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

    Feature* QgsGlobeFeatureSource::getFeature( FeatureID fid )
    {
      QgsFeature feat;
      mLayer->getFeatures( QgsFeatureRequest().setFilterFid( fid ) ).nextFeature( feat );
      return QgsGlobeFeatureUtils::featureFromQgsFeature( mLayer->pendingFields(), feat );
    }

    Geometry::Type QgsGlobeFeatureSource::getGeometryType() const
    {
      switch ( mLayer->geometryType() )
      {
        case  QGis::Point:
          return Geometry::TYPE_POINTSET;
          break;

        case QGis::Line:
          return Geometry::TYPE_LINESTRING;
          break;

        case QGis::Polygon:
          return Geometry::TYPE_POLYGON;
          break;
        case QGis::UnknownGeometry:
        case QGis::NoGeometry:
          return Geometry::TYPE_UNKNOWN;
          break;
      }

      return Geometry::TYPE_POLYGON;
    }

    int QgsGlobeFeatureSource::getFeatureCount() const
    {
      std::cout << "QGISFeatureSource::getFeatureCount [" << mLayer->featureCount() << "]" << std::endl;
      return mLayer->featureCount();
    }
  }
}

class QGISFeatureSourceFactory : public FeatureSourceDriver
{
  public:
    QGISFeatureSourceFactory()
    {
      supportsExtension( "osgearth_feature_qgis", "QGIS feature driver for osgEarth" );
    }

    virtual const char* className()
    {
      return "QGIS Feature Reader";
    }

    virtual ReadResult readObject( const std::string& file_name, const Options* options ) const
    {
      // this function seems to be called for every plugin
      // we declare supporting the special extension "osgearth_feature_qgis"
      if ( !acceptsExtension( osgDB::getLowerCaseFileExtension( file_name ) ) )
        return ReadResult::FILE_NOT_HANDLED;

      return ReadResult( new QgsGlobeFeatureSource( getFeatureSourceOptions( options ) ) );
    }
};

REGISTER_OSGPLUGIN( osgearth_feature_qgis, QGISFeatureSourceFactory )
