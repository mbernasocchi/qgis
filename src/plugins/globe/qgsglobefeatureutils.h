/***************************************************************************
    qgsglobefeatureutils.h
     --------------------------------------
    Date                 : 11.7.2013
    Copyright            : (C) 2013 Matthias Kuhn
    Email                : matthias dot kuhn at gmx dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSGLOBEFEATUREUTILS_H
#define QGSGLOBEFEATUREUTILS_H

#include <osgEarthFeatures/Feature>

#include <qgsfield.h>
#include <qgsgeometry.h>
#include <qgslogger.h>
#include <qgspoint.h>


using namespace osgEarth::Features;

class QgsGlobeFeatureUtils
{
  public:
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
      Feature* retFeat = new Feature( nGeom, 0, Style(), feat.id() );

      const QgsAttributes& attrs = feat.attributes();

      int numFlds = fields.size();

      for ( int idx = 0; idx < numFlds; idx ++ )
      {
        const QgsField& fld = fields.at( idx );
        std::string name = fld.name().toStdString();

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
};

#endif // QGSGLOBEFEATUREUTILS_H
