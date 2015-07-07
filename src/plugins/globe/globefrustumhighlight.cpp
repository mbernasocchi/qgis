/***************************************************************************
    globefrustumhighlight.cpp
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

#include <qgsmapcanvas.h>
#include <qgsmaprenderer.h>

#include "qgsglobefeatureutils.h"

#include "globefrustumhighlight.h"

GlobeFrustumHighlightCallback::GlobeFrustumHighlightCallback( osg::View* view, osgEarth::Terrain* terrain, QgsMapCanvas* mapCanvas, QColor color )
  : osg::NodeCallback()
  , mView( view )
  , mTerrain( terrain )
  , mRubberBand( new QgsRubberBand( mapCanvas, QGis::Polygon ) )
  , mSrs( osgEarth::SpatialReference::create( mapCanvas->mapRenderer()->destinationCrs().toWkt().toStdString() ) )
{
  mRubberBand->setColor( color );
}

GlobeFrustumHighlightCallback::~GlobeFrustumHighlightCallback()
{
  delete mRubberBand;
}

void GlobeFrustumHighlightCallback::operator()( osg::Node* node, osg::NodeVisitor* nv )
{
  Q_UNUSED( node )
  Q_UNUSED( nv )

  const osg::Viewport::value_type &width = mView->getCamera()->getViewport()->width();
  const osg::Viewport::value_type &height = mView->getCamera()->getViewport()->height();

  mTerrain->getWorldCoordsUnderMouse( mView, 0,     0,      mTmpCorners[0] );
  mTerrain->getWorldCoordsUnderMouse( mView, 0,     height - 1, mTmpCorners[1] );
  mTerrain->getWorldCoordsUnderMouse( mView, width - 1, height - 1, mTmpCorners[2] );
  mTerrain->getWorldCoordsUnderMouse( mView, width - 1, 0,      mTmpCorners[3] );

  for ( int i = 0; i < 4; i++ )
  {
    if ( mTmpCorners[i] != mCorners[i] )
    {
      mRubberBand.reset( QGis::Polygon );
      for ( int j = 0; j < 4; j++ )
      {
        osg::Vec3d localCoords;
        mCorners[j] = mTmpCorners[j];
        mSrs->transformFromWorld( mCorners[j], localCoords );

        const QgsPoint&pt = QgsGlobeFeatureUtils::qgsPointFromPoint( localCoords );

        // mRubberBand.addPoint( pt, j == 3 ? true : false );
        mRubberBand.addPoint( pt );
      }
      break;
    }
  }
}
