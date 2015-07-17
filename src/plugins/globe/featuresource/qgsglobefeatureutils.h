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
#include <qgspoint.h>

class QgsGlobeFeatureUtils
{
  public:
    static inline osg::Vec3d pointFromQgsPoint( const QgsPoint& pt )
    {
      return osg::Vec3d( pt.x(), pt.y(), pt.is3D() ? pt.z() : 0.0 );
    }

    static inline QgsPoint qgsPointFromPoint( const osg::Vec3d& pt )
    {
      return QgsPoint( pt.x(), pt.y() );
    }

    static inline osgEarth::Features::Polygon* polygonFromQgsPolygon( const QgsPolygon& poly )
    {
      Q_ASSERT( poly.size() > 0 );

      // a ring for osg earth is open (first point != last point)
      // an outer ring is oriented CCW, an inner ring is oriented CW
      osgEarth::Features::Polygon* retPoly = new osgEarth::Features::Polygon();

      // the outer ring
      foreach( const QgsPoint& pt, poly[0] )
      {
        retPoly->push_back( pointFromQgsPoint( pt ) );
      }
      retPoly->rewind( osgEarth::Symbology::Ring::ORIENTATION_CCW );

      size_t nRings = poly.size();
      for ( size_t i = 1; i < nRings; ++i )
      {
        osgEarth::Features::Ring* innerRing = new osgEarth::Features::Ring();
        foreach( const QgsPoint& pt, poly[i] )
        {
          innerRing->push_back( pointFromQgsPoint( pt ) );
        }
        innerRing->rewind( osgEarth::Symbology::Ring::ORIENTATION_CW );
        retPoly->getHoles().push_back( osg::ref_ptr<osgEarth::Features::Ring>( innerRing ) );
      }
      return retPoly;
    }

    static inline osgEarth::Features::Geometry* geometryFromQgsGeometry( const QgsGeometry& geom )
    {
#if 0
      // test srid
      std::cout << "geom = " << &geom << std::endl;
      std::cout << "wkb = " << geom.asWkb() << std::endl;
      uint32_t srid;
      memcpy( &srid, geom.asWkb() + 2 + sizeof( void* ), sizeof( uint32_t ) );
      std::cout << "srid = " << srid << std::endl;
#endif

      osgEarth::Features::Geometry* retGeom = NULL;

      switch ( QGis::flatType( geom.wkbType() ) )
      {
        case QGis::WKBPoint:
        {
          osgEarth::Features::PointSet* retPt = new osgEarth::Features::PointSet();
          retPt->push_back( pointFromQgsPoint( geom.asPoint() ) );
          retGeom = retPt;
        }
        break;

        case QGis::WKBLineString:
        {
          osgEarth::Features::LineString* retLs = new osgEarth::Features::LineString();
          foreach( const QgsPoint& pt, geom.asPolyline() )
          {
            retLs->push_back( pointFromQgsPoint( pt ) );
          }
          retGeom = retLs;
        }
        break;

        case QGis::WKBPolygon:
        {
          retGeom = polygonFromQgsPolygon( geom.asPolygon() );
        }
        break;

        case QGis::WKBMultiPoint:
        {
          osgEarth::Features::PointSet* retPt = new osgEarth::Features::PointSet();
          foreach( const QgsPoint& point, geom.asMultiPoint() )
          {
            retPt->push_back( pointFromQgsPoint( point ) );
          }
          retGeom = retPt;
        }
        break;

        case QGis::WKBMultiLineString:
        {
          osgEarth::Features::MultiGeometry* retMulti = new osgEarth::Features::MultiGeometry();

          foreach( const QgsPolyline& line, geom.asMultiPolyline() )
          {
            osgEarth::Features::LineString* retLs = new osgEarth::Features::LineString();

            foreach( const QgsPoint& pt, line )
            {
              retLs->push_back( pointFromQgsPoint( pt ) );
            }

            retMulti->getComponents().push_back( retLs );
          }
          retGeom = retMulti;
        }
        break;


        case QGis::WKBMultiPolygon:
        {
          osgEarth::Features::MultiGeometry* retMulti = new osgEarth::Features::MultiGeometry();

          foreach( const QgsPolygon& poly, geom.asMultiPolygon() )
          {
            retMulti->getComponents().push_back( osg::ref_ptr<osgEarth::Features::Polygon>( polygonFromQgsPolygon( poly ) ) );
          }
          retGeom = retMulti;
        }
        break;


        default:
          break;
      }
      return retGeom;
    }

    static osgEarth::Features::Feature* featureFromQgsFeature( const QgsFields& fields, QgsFeature& feat )
    {
      osgEarth::Features::Geometry* nGeom = geometryFromQgsGeometry( *feat.geometry() );
      osgEarth::Features::Feature* retFeat = new osgEarth::Features::Feature( nGeom, 0, osgEarth::Style(), feat.id() );

      const QgsAttributes& attrs = feat.attributes();

      for ( int idx = 0, numFlds = fields.size(); idx < numFlds; ++idx )
      {
        const QgsField& fld = fields.at( idx );
        std::string name = fld.name().toStdString();

        switch ( fld.type() )
        {
          case QVariant::Bool:
            if ( !attrs[idx].isNull() )
              retFeat->set( name, attrs[idx].toBool() );
            else
              retFeat->setNull( name, osgEarth::Features::ATTRTYPE_BOOL );

            break;

          case QVariant::Int:
          case QVariant::UInt:
          case QVariant::LongLong:
          case QVariant::ULongLong:
            if ( !attrs[idx].isNull() )
              retFeat->set( name, attrs[idx].toInt() );
            else
              retFeat->setNull( name, osgEarth::Features::ATTRTYPE_INT );

            break;

          case QVariant::Double:
            if ( !attrs[idx].isNull() )
              retFeat->set( name, attrs[idx].toDouble() );
            else
              retFeat->setNull( name, osgEarth::Features::ATTRTYPE_DOUBLE );

            break;

          case QVariant::Char:
          case QVariant::String:
          default:
            if ( !attrs[idx].isNull() )
              retFeat->set( name, attrs[idx].toString().toStdString() );
            else
              retFeat->setNull( name, osgEarth::Features::ATTRTYPE_STRING );

            break;
        }
      }

      return retFeat;
    }

    static osgEarth::Features::FeatureSchema schemaForFields(const QgsFields& fields)
    {
      osgEarth::Features::FeatureSchema schema;

      for ( int idx = 0, numFlds = fields.size(); idx < numFlds; ++idx )
      {
        const QgsField& fld = fields.at( idx );
        std::string name = fld.name().toStdString();

        switch ( fld.type() )
        {
          case QVariant::Bool:
            schema.insert(std::make_pair(name, osgEarth::Features::ATTRTYPE_BOOL));
            break;

          case QVariant::Int:
          case QVariant::UInt:
          case QVariant::LongLong:
          case QVariant::ULongLong:
            schema.insert(std::make_pair(name, osgEarth::Features::ATTRTYPE_INT));
            break;

          case QVariant::Double:
            schema.insert(std::make_pair(name, osgEarth::Features::ATTRTYPE_DOUBLE));
            break;

          case QVariant::Char:
          case QVariant::String:
          default:
            schema.insert(std::make_pair(name, osgEarth::Features::ATTRTYPE_STRING));
            break;
        }
      }
      return schema;
    }
};

#endif // QGSGLOBEFEATUREUTILS_H
