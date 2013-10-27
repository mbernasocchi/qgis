/***************************************************************************
    globefrustumhighlight.h
     --------------------------------------
    Date                 : 27.10.2013
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

#ifndef GLOBEFRUSTUMHIGHLIGHT_H
#define GLOBEFRUSTUMHIGHLIGHT_H

#include <qgsrubberband.h>
#include <osg/NodeCallback>
#include <osgEarth/Terrain>

struct GlobeFrustumHighlightCallback : public osg::NodeCallback
{
  // NodeCallback interface
public:
  GlobeFrustumHighlightCallback( osg::View* view, osgEarth::Terrain* terrain, QgsMapCanvas* mapCanvas, QColor color );

  virtual void operator()( osg::Node* node, osg::NodeVisitor* nv );

private:
  osg::View* mView;
  osgEarth::Terrain* mTerrain;
  osg::Vec3d mCorners[4];
  osg::Vec3d mTmpCorners[4];
  QgsRubberBand mRubberBand;
  osgEarth::SpatialReference* mSrs;
};

#endif // GLOBEFRUSTUMHIGHLIGHT_H
