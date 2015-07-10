/***************************************************************************
    qgsglobetilesource.cpp
    ---------------------
    begin                : August 2010
    copyright            : (C) 2010 by Pirmin Kalberer
                           (C) 2015 Sandro Mani
    email                : pka at sourcepole dot ch
                           smani at sourcepole dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <osgEarth/Registry>
#include <osgEarth/ImageUtils>
#include <QPainter>

#include "qgsglobetilesource.h"
#include "qgscoordinatetransform.h"
#include "qgslogger.h"
#include "qgsmapcanvas.h"
#include "qgsmaprenderer.h"


QgsGlobeTile::QgsGlobeTile( const QgsGlobeTileSource* tileSource, const QgsRectangle& tileExtent )
    : osg::Image()
    , mTileSource( tileSource )
    , mTileExtent( tileExtent )
    , mLastUpdateTime( osgEarth::DateTime().asTimeStamp() )
{}

bool QgsGlobeTile::requiresUpdateCall() const
{
  return mLastUpdateTime < mTileSource->getLastModifiedTime();
}

void QgsGlobeTile::update( osg::NodeVisitor * )
{
  QgsDebugMsg( QString( "Updating tile image for extent %1" ).arg( mTileExtent.toString() ) );
  osg::ref_ptr<osg::Image> image = mTileSource->renderImage( s(), mTileExtent );
  if ( image )
  {
    image->setAllocationMode( osg::Image::NO_DELETE ); // since image->data is passed to this
    setImage( image->s(), image->t(), image->r(), image->getInternalTextureFormat(), image->getPixelFormat(), image->getDataType(), image->data(), osg::Image::USE_NEW_DELETE, image->getPacking() );
    mLastUpdateTime = osgEarth::DateTime().asTimeStamp();
  }
}


QgsGlobeTileSource::QgsGlobeTileSource( QgsMapCanvas* canvas, const osgEarth::TileSourceOptions& options )
    : TileSource( options )
    , mCanvas( canvas )
    , mCoordTransform( 0 )
    , mLastModifiedTime( 0 )
{
}

osgEarth::TileSource::Status QgsGlobeTileSource::initialize( const osgDB::Options* /*dbOptions*/ )
{
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
  mMapRenderer = new QgsMapRenderer();
  mMapRenderer->setDestinationCrs( destCRS );
  mMapRenderer->setProjectionsEnabled( true );
  mMapRenderer->setOutputUnits( mCanvas->mapRenderer()->outputUnits() );
  mMapRenderer->setMapUnits( QGis::Degrees );

  mLastModifiedTime = osgEarth::DateTime().asTimeStamp();
  return STATUS_OK;
}

osg::Image* QgsGlobeTileSource::createImage( const osgEarth::TileKey& key, osgEarth::ProgressCallback* progress )
{
  Q_UNUSED( progress );

  //Get the extents of the tile
  int tileSize = getPixelsPerTile();
  if ( tileSize <= 0 )
  {
    QgsDebugMsg( "Tile size too small." );
    return osgEarth::ImageUtils::createEmptyImage();
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
  QgsDebugMsg( QString( "earth tile key:%1 ext:%2" ).arg( key.str().c_str() ).arg( tileExtent.toString( 5 ) ) );

  QgsDebugMsg( QString( "text0:%1" ).arg( tileExtent.toString( 5 ) ) );
  if ( !( viewExtent.intersects( tileExtent ) || viewExtent.contains( tileExtent ) ) )
  {
    QgsDebugMsg( QString( "earth tile key:%1 ext:%2: NO INTERSECT" ).arg( key.str().c_str() ).arg( tileExtent.toString( 5 ) ) );
    return osgEarth::ImageUtils::createEmptyImage();
  }

  osg::Image* image = renderImage( tileSize, tileExtent );
  if ( image != 0 )
    return image;
  else
    return osgEarth::ImageUtils::createEmptyImage();
}

QgsGlobeTile* QgsGlobeTileSource::renderImage( int tileSize, const QgsRectangle& tileExtent ) const
{
  unsigned char* data = new unsigned char[tileSize * tileSize * 4];
  QImage qImage( data, tileSize, tileSize, QImage::Format_ARGB32_Premultiplied );
  if ( qImage.isNull() )
    return 0;
  qImage.fill( 0 );

  mMapRenderer->setOutputSize( QSize( tileSize, tileSize ), qImage.logicalDpiX() );
  mMapRenderer->setExtent( tileExtent );

  QPainter painter( &qImage );
  mMapRenderer->render( &painter );

  QgsGlobeTile* image = new QgsGlobeTile( this, tileExtent );
  image->setImage( tileSize, tileSize, 1, 4, // width, height, depth, internal_format
                   GL_BGRA, GL_UNSIGNED_BYTE,
                   data, osg::Image::USE_NEW_DELETE, 1 );
  image->flipVertical();
  return image;
}

void QgsGlobeTileSource::updateModifiedTime()
{
  osgEarth::TimeStamp old = mLastModifiedTime;
  mLastModifiedTime = osgEarth::DateTime().asTimeStamp();
  QgsDebugMsg( QString( "Updated QGIS map layer modified time from %1 to %2" ).arg( old ).arg( mLastModifiedTime ) );
}

void QgsGlobeTileSource::setLayerSet( const QStringList &layerSet )
{
  mMapRenderer->setLayerSet( layerSet );
}

const QStringList& QgsGlobeTileSource::layerSet() const
{
  return mMapRenderer->layerSet();
}
