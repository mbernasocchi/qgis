/***************************************************************************
    qgsglobetilesource.h
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

#ifndef QGSGLOBETILESOURCE_H
#define QGSGLOBETILESOURCE_H

#include <osgEarth/TileSource>
#include "qgsrectangle.h"

class QgsCoordinateTransform;
class QgsMapCanvas;
class QgsMapRenderer;
class QgsGlobeTileSource;


class QgsGlobeTile : public osg::Image
{
  public:
    QgsGlobeTile( const QgsGlobeTileSource* tileSource, const QgsRectangle& tileExtent );
    bool requiresUpdateCall() const;
    void update( osg::NodeVisitor * );

  private:
    const QgsGlobeTileSource* mTileSource;
    QgsRectangle mTileExtent;
    osgEarth::TimeStamp mLastUpdateTime;
};


class QgsGlobeTileSource : public osgEarth::TileSource
{
  public:
    QgsGlobeTileSource( QgsMapCanvas* canvas, const osgEarth::TileSourceOptions& options = osgEarth::TileSourceOptions() );
    Status initialize( const osgDB::Options *dbOptions ) override;

    osg::Image* createImage( const osgEarth::TileKey& key, osgEarth::ProgressCallback* progress );
    QgsGlobeTile *renderImage( int tileSize, const QgsRectangle& tileExtent ) const;

    osg::HeightField* createHeightField( const osgEarth::TileKey &/*key*/, osgEarth::ProgressCallback* /*progress*/ )
    {
      OE_WARN << "[QGIS] Driver does not support heightfields" << std::endl;
      return 0;
    }

    osgEarth::CachePolicy getCachePolicyHint( const osgEarth::Profile* ) const
    {
      return osgEarth::CachePolicy::NO_CACHE;
    }

    bool supportsPersistentCaching() const { return false; }
    bool isDynamic() const { return true; }
    osgEarth::TimeStamp getLastModifiedTime() const { return mLastModifiedTime; }
    void updateModifiedTime();
    void setLayerSet( const QStringList& layerSet );
    const QStringList &layerSet() const;

  private:
    QgsMapRenderer *mMapRenderer;
    QgsMapCanvas* mCanvas;
    QgsCoordinateTransform *mCoordTransform;
    osgEarth::TimeStamp mLastModifiedTime;
};

#endif // QGSGLOBETILESOURCE_H
