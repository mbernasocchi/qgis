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
#include "qgsvectorlayer.h"
#include <qgsfeatureiterator.h>
#include <qgsfeature.h>
#include <qgsgeometry.h>
#include <qgslogger.h>

using namespace osgEarth::Features;

namespace osgEarth
{
  namespace Features
  {
    using namespace osgEarth;
    using namespace osgEarth::Symbology;
    using namespace osgEarth::Drivers;

    static inline osg::Vec3d pointFromQgsPoint( const QgsPoint& pt )
    {
      return osg::Vec3d( pt.x(), pt.y(), pt.is3D() ? pt.z() : 0.0 );
    }

    static inline Polygon* polygonFromQgsPolygon( const QgsPolygon& poly )
    {
      Q_ASSERT( poly.size() > 0 );

      // a ring for osg earth is open (first point != last point)
      // an outer ring is oriented CCW, an inner ring is oriented CW
      Polygon* retPoly = new Polygon();

      // the outer ring
      Q_FOREACH( const QgsPoint& pt, poly[0] )
      {
        retPoly->push_back( pointFromQgsPoint( pt ) );
      }
      retPoly->rewind( Symbology::Ring::ORIENTATION_CCW );

      size_t nRings = poly.size();
      for ( size_t i = 1; i < nRings; ++i )
      {
        Ring* innerRing = new Ring();
        Q_FOREACH( const QgsPoint& pt, poly[i] )
        {
          innerRing->push_back( pointFromQgsPoint( pt ) );
        }
        innerRing->rewind( Symbology::Ring::ORIENTATION_CW );
        retPoly->getHoles().push_back( osg::ref_ptr<Ring>( innerRing ) );
      }
      return retPoly;
    }

    static inline Geometry* geometryFromQgsGeometry( const QgsGeometry& geom )
    {
#if 0
      // test srid
      std::cout << "geom = " << &geom << std::endl;
      std::cout << "wkb = " << geom.asWkb() << std::endl;
      uint32_t srid;
      memcpy( &srid, geom.asWkb() + 2 + sizeof( void* ), sizeof( uint32_t ) );
      std::cout << "srid = " << srid << std::endl;
#endif

      Geometry* retGeom = NULL;

      switch ( geom.wkbType() )
      {
        case QGis::WKBPoint:
        case QGis::WKBPoint25D:
        {
          PointSet* retPt = new PointSet();
          retPt->push_back( pointFromQgsPoint( geom.asPoint() ) );
          retGeom = retPt;
        }
        break;


        case QGis::WKBLineString:
        case QGis::WKBLineString25D:
        {
          LineString* retLs = new LineString();

          Q_FOREACH( const QgsPoint& pt, geom.asPolyline() )
          {
            retLs->push_back( pointFromQgsPoint( pt ) );
          }
          retGeom = retLs;
        }
        break;


        case QGis::WKBPolygon:
        case QGis::WKBPolygon25D:
        {
          retGeom = polygonFromQgsPolygon( geom.asPolygon() );
        }
        break;


        case QGis::WKBMultiPoint:
        case QGis::WKBMultiPoint25D:
        {
          PointSet* retPt = new PointSet();

          Q_FOREACH( const QgsPoint& point, geom.asMultiPoint() )
          {
            retPt->push_back( pointFromQgsPoint( point ) );
          }
          retGeom = retPt;
        }
        break;


        case QGis::WKBMultiLineString:
        case QGis::WKBMultiLineString25D:
        {
          MultiGeometry* retMulti = new MultiGeometry();

          Q_FOREACH( const QgsPolyline& line, geom.asMultiPolyline() )
          {
            LineString* retLs = new LineString();

            Q_FOREACH( const QgsPoint& pt, line )
            {
              retLs->push_back( pointFromQgsPoint( pt ) );
            }

            retMulti->getComponents().push_back( retLs );
          }
          retGeom = retMulti;
        }
        break;


        case QGis::WKBMultiPolygon:
        case QGis::WKBMultiPolygon25D:
        {
          MultiGeometry* retMulti = new MultiGeometry();

          Q_FOREACH( const QgsPolygon& poly, geom.asMultiPolygon() )
          {
            retMulti->getComponents().push_back( osg::ref_ptr<Polygon>( polygonFromQgsPolygon( poly ) ) );
          }
          retGeom = retMulti;
        }
        break;


        default:
          break;
      }
      return retGeom;
    }

    static Feature* featureFromQgsFeature( const QgsFields& fields, const QgsFeature& feat )
    {
      QgsDebugMsg( QString( "featureFromQgsFeature" ) );
      const QgsGeometry* geom = feat.geometry();

      Geometry* nGeom = geometryFromQgsGeometry( *geom );
      Feature* retFeat = new Feature( nGeom, 0 );

      const QgsAttributes& attrs = feat.attributes();

      int numFlds = fields.size();

      for ( int idx = 0; idx < numFlds; idx ++ )
      {
        const QgsField& fld = fields.at( idx );
        std::string name = fld.name().toStdString();

        std::cout << "feature " << feat.id() << " field " << idx << std::endl;

        switch ( fld.type() )
        {
          case QVariant::Bool:
            if ( !attrs[idx].isNull() )
              retFeat->set( name, attrs[idx].toBool() );
            else
              retFeat->setNull( name, ATTRTYPE_BOOL );

            break;

          case QVariant::Int:
          case QVariant::UInt:
          case QVariant::LongLong:
          case QVariant::ULongLong:
            if ( !attrs[idx].isNull() )
              retFeat->set( name, attrs[idx].toInt() );
            else
              retFeat->setNull( name, ATTRTYPE_INT );

            break;

          case QVariant::Double:
            if ( !attrs[idx].isNull() )
              retFeat->set( name, attrs[idx].toDouble() );
            else
              retFeat->setNull( name, ATTRTYPE_DOUBLE );

            break;

          case QVariant::Char:
          case QVariant::String:
          default:
            if ( !attrs[idx].isNull() )
              retFeat->set( name, attrs[idx].toString().toStdString() );
            else
              retFeat->setNull( name, ATTRTYPE_STRING );

            break;
        }
      }

      return retFeat;
    }


    class QGISFeatureCursor : public FeatureCursor
    {
      public:
        QGISFeatureCursor( const QgsFields& fields, QgsFeatureIterator iterator )
            : mIterator( iterator )
            , mFields( fields )
        {
        }

        virtual bool hasMore() const
        {
          return !mIterator.isClosed();
        }

        virtual Feature* nextFeature()
        {
          if ( mIterator.nextFeature( mFeature ) )
            return featureFromQgsFeature( mFields, mFeature );
          else
            return NULL;
        }
      private:
        // current iterator
        QgsFeatureIterator mIterator;
        // dummy feature
        QgsFeature mFeature;
        // fields
        QgsFields mFields;
    };

    QGISFeatureSource::QGISFeatureSource( const QGISFeatureOptions& options ) :
        options_( options ),
        mLayer( 0 ),
        mProfile( 0 )
    {
      iface_ = options_.qgis();
    }


    void QGISFeatureSource::initialize( const osgDB::Options* dbOptions )
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

    const FeatureProfile* QGISFeatureSource::createFeatureProfile()
    {
      return mProfile;
    }

    const FeatureSchema& QGISFeatureSource::getSchema() const
    {
      return FeatureSource::getSchema();
    }

    FeatureCursor* QGISFeatureSource::createFeatureCursor( const Symbology::Query& query )
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
      return new QGISFeatureCursor( mLayer->pendingFields(), it );
    }

    Feature* QGISFeatureSource::getFeature( FeatureID fid )
    {
      QgsFeature feat;
      mLayer->getFeatures( QgsFeatureRequest().setFilterFid( fid ) ).nextFeature( feat );
      return featureFromQgsFeature( mLayer->pendingFields(), feat );
    }

    Geometry::Type QGISFeatureSource::getGeometryType() const
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

    int QGISFeatureSource::getFeatureCount() const
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

      return ReadResult( new QGISFeatureSource( getFeatureSourceOptions( options ) ) );
    }
};

REGISTER_OSGPLUGIN( osgearth_feature_qgis, QGISFeatureSourceFactory )
