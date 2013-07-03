#include <osgDB/ReaderWriter>
#include <osgDB/FileNameUtils>

#include <osgEarthFeatures/FeatureSource>
#include <osgEarth/Registry>

#include "qgsosgearthfeaturesource.h"

#include <QList>

#include <qgisinterface.h>
#include <qgsmapcanvas.h>
#include <qgsrectangle.h>
#include <qgsvectordataprovider.h>
#include <qgsfeatureiterator.h>
#include <qgsfeature.h>
#include <qgsgeometry.h>

using namespace osgEarth::Features;

namespace osgEarth
{
  namespace Features
  {
    using namespace osgEarth;
    using namespace osgEarth::Symbology;
    using namespace osgEarth::Drivers;

    Geometry* geometryFromQgsGeometry( const QgsGeometry& geom )
    {
#if 0
      // test srid
      std::cout << "geom = " << &geom << std::endl;
      std::cout << "wkb = " << geom.asWkb() << std::endl;
      uint32_t srid;
      memcpy( &srid, geom.asWkb() + 2 + sizeof( void* ), sizeof( uint32_t ) );
      std::cout << "srid = " << srid << std::endl;
#endif

      switch ( geom.wkbType() )
      {
        case QGis::WKBPoint:
        case QGis::WKBPoint25D:
        {
          QgsPoint pt = geom.asPoint();
          PointSet* retPt = new PointSet();
          retPt->push_back( osg::Vec3d( pt.x(), pt.y(), pt.is3D() ? pt.z() : 0.0 ) );
          return retPt;
        }
        break;
        case QGis::WKBLineString:
        case QGis::WKBLineString25D:
        {
          QgsPolyline line = geom.asPolyline();
          LineString* retLs = new LineString();

          size_t nPts = line.size();
          for ( size_t i = 0; i < nPts; ++i )
          {
            retLs->push_back( osg::Vec3d( line[i].x(), line[i].y(), line[i].is3D() ? line[i].z() : 0.0 ) );
          }
          return retLs;
        }
        break;
        case QGis::WKBPolygon:
        case QGis::WKBPolygon25D:
        {
          QgsPolygon poly = geom.asPolygon();
          // a ring for osg earth is open (first point != last point)
          // an outer ring is oriented CCW, an inner ring is oriented CW

          Polygon* retPoly = new Polygon();

          // the outer ring
          size_t nPts = poly[0].size();
          for ( size_t j = 0; j < nPts; ++j )
          {
            const QgsPoint& pt = poly[0][j];
            retPoly->push_back( osg::Vec3d( pt.x(), pt.y(), pt.is3D() ? pt.z() : 0.0 ) );
          }
          retPoly->rewind( Symbology::Ring::ORIENTATION_CCW );

          size_t nRings = poly.size();
          for ( size_t i = 1; i < nRings; ++i )
          {
            size_t nPts = poly[i].size();
            Ring* innerRing = new Ring();
            for ( size_t j = 0; j < nPts; ++j )
            {
              const QgsPoint& pt = poly[i][j];
              innerRing->push_back( osg::Vec3d( pt.x(), pt.y(), pt.is3D() ? pt.z() : 0.0 ) );
            }
            innerRing->rewind( Symbology::Ring::ORIENTATION_CW );
            retPoly->getHoles().push_back( osg::ref_ptr<Ring>( innerRing ) );
          }
          return retPoly;
        }
        break;
        case QGis::WKBMultiPoint:
        case QGis::WKBMultiPoint25D:
        {
          QgsMultiPoint multiPt = geom.asMultiPoint();
          PointSet* retPt = new PointSet();
          size_t nPts = multiPt.size();
          for ( size_t i = 0; i < nPts; ++i )
          {
            const QgsPoint& pt = multiPt[i];
            retPt->push_back( osg::Vec3d( pt.x(), pt.y(), pt.is3D() ? pt.z() : 0.0 ) );
          }
          return retPt;
        }
        break;
        case QGis::WKBMultiLineString:
        case QGis::WKBMultiLineString25D:
        {
          QgsMultiPolyline multiLine = geom.asMultiPolyline();

          MultiGeometry* retMulti = new MultiGeometry();

          size_t nGeoms = multiLine.size();
          for ( size_t m = 0; m < nGeoms; ++m )
          {
            QgsPolyline& line = multiLine[m];
            LineString* retLs = new LineString();

            size_t nPts = line.size();
            for ( size_t i = 0; i < nPts; ++i )
            {
              retLs->push_back( osg::Vec3d( line[i].x(), line[i].y(), line[i].is3D() ? line[i].z() : 0.0 ) );
            }

            retMulti->getComponents().push_back( retLs );
          }
          return retMulti;
        }
        break;
        case QGis::WKBMultiPolygon:
        case QGis::WKBMultiPolygon25D:
        {
          QgsMultiPolygon mpoly = geom.asMultiPolygon();

          MultiGeometry* retMulti = new MultiGeometry();
          size_t nGeoms = mpoly.size();
          for ( size_t m = 0; m < nGeoms; ++m )
          {
            QgsPolygon& poly = mpoly[m];

            Polygon* retPoly = new Polygon();
            size_t nRings = poly.size();

            // the outer ring
            size_t nPts = poly[0].size();
            for ( size_t j = 0; j < nPts; ++j )
            {
              const QgsPoint& pt = poly[0][j];
              retPoly->push_back( osg::Vec3d( pt.x(), pt.y(), pt.is3D() ? pt.z() : 0.0 ) );
            }
            retPoly->rewind( Symbology::Ring::ORIENTATION_CCW );

            // inner rings, if any
            for ( size_t i = 1; i < nRings; ++i )
            {
              size_t nPts = poly[i].size();
              Ring* innerRing = new Ring();
              for ( size_t j = 0; j < nPts; ++j )
              {
                const QgsPoint& pt = poly[i][j];
                innerRing->push_back( osg::Vec3d( pt.x(), pt.y(), pt.is3D() ? pt.z() : 0.0 ) );
              }
              innerRing->rewind( Symbology::Ring::ORIENTATION_CW );
              retPoly->getHoles().push_back( osg::ref_ptr<Ring>( innerRing ) );
            }
            // add polygon to the returned multi geometry
            retMulti->getComponents().push_back( osg::ref_ptr<Polygon>( retPoly ) );
          }
          return retMulti;
        }
        break;
        default:
          break;
      }
      return 0;
    }

    Feature* featureFromQgsFeature( const QgsFeature& feat )
    {
      QgsGeometry* geom = feat.geometry();

      Geometry* nGeom = geometryFromQgsGeometry( *geom );
      Feature* retFeat = new Feature( nGeom, 0 );
      return retFeat;
    }

    class QGISFeatureCursor : public FeatureCursor
    {
      public:
        QGISFeatureCursor( QgsFeatureIterator iterator ) :
            mIterator( iterator )
        {
        }

        virtual bool hasMore() const
        {
          return mIterator.isClosed();
        }

        virtual Feature* nextFeature()
        {
          if ( mIterator.isClosed() )
            return NULL;

          mIterator.nextFeature( mFeature );

          return featureFromQgsFeature( mFeature );
        }
      private:
        // current iterator
        QgsFeatureIterator mIterator;
        // dummy feature
        QgsFeature mFeature;
    };

    QGISFeatureSource::QGISFeatureSource( const QGISFeatureOptions& options ) :
        options_( options ),
        mLayer( 0 ),
        profile_( 0 )
    {
      iface_ = options_.qgis();
    }

    void QGISFeatureSource::initialize( const std::string& /* uri */ )
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
      mLayer = options_.layer();
      std::cout << "got vector layer = " << mLayer << std::endl;

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
      profile_ = new FeatureProfile( ext );
    }

    const FeatureProfile* QGISFeatureSource::createFeatureProfile()
    {
      return profile_;
    }

    FeatureCursor* QGISFeatureSource::createFeatureCursor( const Symbology::Query& query )
    {
      Q_UNUSED( query );
      return new QGISFeatureCursor( mLayer->dataProvider()->getFeatures() );
    }

    Feature* QGISFeatureSource::getFeature( FeatureID fid )
    {
      std::cout << "QGISFeatureSource::geFeature " << fid << std::endl;
      QgsFeature feat;
      mLayer->dataProvider()->getFeatures( QgsFeatureRequest().setFilterFid( fid ) ).nextFeature( feat );
      return featureFromQgsFeature( feat );
    }

    Geometry::Type QGISFeatureSource::getGeometryType() const
    {
      std::cout << "QGISFeatureSource::getGeometryType" << std::endl;
      return Geometry::TYPE_POLYGON;
    }

    int QGISFeatureSource::getFeatureCount() const
    {
      std::cout << "QGISFeatureSource::getFeatureCount" << std::endl;
      return mLayer->dataProvider()->featureCount();
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

      return ReadResult( new QGISFeatureSource( getFeatureSourceOptions( options ) ) );
    }
};

REGISTER_OSGPLUGIN( osgearth_feature_qgis, QGISFeatureSourceFactory )
