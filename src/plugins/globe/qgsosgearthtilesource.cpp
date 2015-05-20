/***************************************************************************
    qgsosgearthtilesource.cpp
    ---------------------
    begin                : August 2010
    copyright            : (C) 2010 by Pirmin Kalberer
    email                : pka at sourcepole dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <osgEarth/TileSource>
#include <osgEarth/Registry>
#include <osgEarth/ImageUtils>

#include <osg/Notify>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <sstream>

#include "qgsosgearthtilesource.h"

#include "qgsapplication.h"
#include <qgslogger.h>
#include <qgisinterface.h>
#include <qgsmapcanvas.h>
#include "qgsproviderregistry.h"
#include "qgsmaplayerregistry.h"
#include "qgsmaptopixel.h"
#include "qgspallabeling.h"
#include "qgsproject.h"

#include <QFile>
#include <QPainter>

using namespace osgEarth;
using namespace osgEarth::Drivers;


QgsOsgEarthTileSource::QgsOsgEarthTileSource( QStringList mapLayers, QgsMapCanvas* canvas, const TileSourceOptions& options )
    : TileSource( options )
    , mMapLayers( mapLayers )
    , mCanvas( canvas )
    , mCoordTransform( 0 )
{
}

void QgsOsgEarthTileSource::initialize( const std::string& referenceURI, const Profile* overrideProfile )
{
  Q_UNUSED( referenceURI );
  Q_UNUSED( overrideProfile );

  setProfile( osgEarth::Registry::instance()->getGlobalGeodeticProfile() );

  QgsCoordinateReferenceSystem destCRS;
  destCRS.createFromOgcWmsCrs( GEO_EPSG_CRS_AUTHID );

  if ( mCanvas->mapSettings().destinationCrs().authid().compare( GEO_EPSG_CRS_AUTHID, Qt::CaseInsensitive ) != 0 )
  {
    // FIXME: crs from canvas or first layer?
    QgsCoordinateReferenceSystem srcCRS( mCanvas->mapSettings().destinationCrs() );
    QgsDebugMsg( QString( "transforming from %1 to %2" ).arg( srcCRS.authid() ).arg( destCRS.authid() ) );
    mCoordTransform = new QgsCoordinateTransform( srcCRS, destCRS );
  }
  else
  {
    mCoordTransform = 0;
  }
  mMapRenderer = new QgsMapRenderer();
  mMapRenderer->setDestinationCrs( destCRS );
  mMapRenderer->setProjectionsEnabled( true );
  mMapRenderer->setOutputUnits( mCanvas->mapRenderer()->outputUnits() );
  mMapRenderer->setMapUnits( QGis::Degrees );
}

osg::Image* QgsOsgEarthTileSource::createImage( const TileKey& key, ProgressCallback* progress )
{
  QString kname = key.str().c_str();
  kname.replace( '/', '_' );

  Q_UNUSED( progress );

  //Get the extents of the tile
  int tileSize = getPixelsPerTile();
  if ( tileSize <= 0 )
  {
    QgsDebugMsg( "Tile size too small." );
    return ImageUtils::createEmptyImage();
  }

  QgsRectangle viewExtent = mCanvas->fullExtent();
  if ( mCoordTransform )
  {
    QgsDebugMsg( QString( "vext0:%1" ).arg( viewExtent.toString( 5 ) ) );
    viewExtent = mCoordTransform->transformBoundingBox( viewExtent );
  }

  QgsDebugMsg( QString( "vext1:%1" ).arg( viewExtent.toString( 5 ) ) );

  double xmin, ymin, xmax, ymax;
  key.getExtent().getBounds( xmin, ymin, xmax, ymax );
  QgsRectangle tileExtent( xmin, ymin, xmax, ymax );

  QgsDebugMsg( QString( "text0:%1" ).arg( tileExtent.toString( 5 ) ) );
  if ( !viewExtent.intersects( tileExtent ) )
  {
    QgsDebugMsg( QString( "earth tile key:%1 ext:%2: NO INTERSECT" ).arg( kname ).arg( tileExtent.toString( 5 ) ) );
    return ImageUtils::createEmptyImage();
  }

  QImage *qImage = createQImage( tileSize, tileSize );
  if ( !qImage )
  {
    QgsDebugMsg( QString( "earth tile key:%1 ext:%2: EMPTY IMAGE" ).arg( kname ).arg( tileExtent.toString( 5 ) ) );
    return ImageUtils::createEmptyImage();
  }

  mMapRenderer->setLayerSet( mCanvas->mapRenderer()->layerSet() );
  mMapRenderer->setOutputSize( QSize( tileSize, tileSize ), qImage->logicalDpiX() );
  mMapRenderer->setExtent( tileExtent );

  QPainter thePainter( qImage );
  mMapRenderer->render( &thePainter );

  QgsDebugMsg( QString( "earth tile key:%1 ext:%2" ).arg( kname ).arg( tileExtent.toString( 5 ) ) );
#if 0
  qImage->save( QString( "/tmp/tile-%1.png" ).arg( kname ) );
#endif

  osg::ref_ptr<osg::Image> image = new osg::Image;

  //The pixel format is always RGBA to support transparency
  image->setImage( tileSize, tileSize, 1, 4, // width, height, depth, pixelFormat?
                   GL_BGRA, GL_UNSIGNED_BYTE, //Why not GL_RGBA - Qt bug?
                   qImage->bits(),
                   osg::Image::NO_DELETE, 1 );

  image->flipVertical();


  //Create a transparent image if we don't have an image
  if ( !image.valid() )
  {
    QgsDebugMsg( "image is invalid" );
    return ImageUtils::createEmptyImage();
  }
  QgsDebugMsg( "returning image" );
  return image.release();
}

void QgsOsgEarthTileSource::addLayers( QSet<QgsMapLayer*> layers )
{
  Q_FOREACH( QgsMapLayer* layer, layers )
  {
    mMapLayers << layer->id();
  }
  mMapRenderer->setLayerSet( mMapLayers );
}

QImage* QgsOsgEarthTileSource::createQImage( int width, int height ) const
{
  if ( width < 0 || height < 0 )
  {
    return 0;
  }

  QImage* qImage = 0;

  //is format jpeg?
  bool jpeg = false;
  //transparent parameter
  bool transparent = true;

  //use alpha channel only if necessary because it slows down performance
  if ( transparent && !jpeg )
  {
    qImage = new QImage( width, height, QImage::Format_ARGB32_Premultiplied );
    qImage->fill( 0 );
  }
  else
  {
    qImage = new QImage( width, height, QImage::Format_RGB32 );
    qImage->fill( qRgb( 255, 255, 255 ) );
  }

  if ( !qImage )
  {
    return 0;
  }

  //apply DPI parameter if present.
#if 0
  int dpm = dpi / 0.0254;
  qImage->setDotsPerMeterX( dpm );
  qImage->setDotsPerMeterY( dpm );
#endif
  return qImage;
}
