/***************************************************************************
    qgsgpsmarker.cpp  - canvas item which shows a gps position marker
    ---------------------
    begin                : December 2009
    copyright            : (C) 2009 by Tim Sutton
    email                : tim at linfiniti dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QPainter>

#include "qgsgpsmarker.h"
#include "qgscoordinatetransform.h"
#include "qgsmapcanvas.h"
#include "qgsmaprenderer.h"


QgsGpsMarker::QgsGpsMarker( QgsMapCanvas* mapCanvas )
    : QgsMapCanvasItem( mapCanvas )
{
  mSize = 16;
  mColor = Qt::black;
  mWgs84CRS.createFromOgcWmsCrs( "EPSG:4326" );
}

void QgsGpsMarker::setSize( int theSize )
{
  mSize = theSize;
}

void QgsGpsMarker::setColor( QColor theColor )
{
  mColor = theColor;
}

void QgsGpsMarker::setCenter( const QgsPoint& point )
{
  //transform to map crs
  if ( mMapCanvas )
  {
    QgsCoordinateTransform t( mWgs84CRS, mMapCanvas->mapSettings().destinationCrs() );
    try
    {
      mCenter = t.transform( point );
    }
    catch ( QgsCsException &e ) //silently ignore transformation exceptions
    {
      Q_UNUSED( e );
      return;
    }
  }
  else
  {
    mCenter = point;
  }

  QPointF pt = toCanvasCoordinates( mCenter );
  setPos( pt );
}

void QgsGpsMarker::paint( QPainter* p )
{
  // this needs to be done when the canvas is repainted to make for smoother map rendering
  // if not done the map could be panned, but the cursor position won't be updated until the next valid GPS fix is received
  QPointF pt = toCanvasCoordinates( mCenter );
  setPos( pt );

  float myHalfSize = mSize / 2.0;

  QPen pen(mColor);
  int penWidth = mSize / 10.0;
  pen.setWidth( penWidth );
  pen.setCapStyle(Qt::RoundCap);
  p->setPen(pen);

  int diameter = mSize - penWidth * 2;
  p->drawEllipse(-myHalfSize + penWidth, -myHalfSize + penWidth, diameter, diameter);
  float myHalfLineSize = myHalfSize * 0.5;
  QLineF vLine(0, -myHalfLineSize, 0, myHalfLineSize);
  QLineF hLine(-myHalfLineSize, 0, myHalfLineSize, 0);
  p->drawLine(vLine);
  p->drawLine(hLine);
}


QRectF QgsGpsMarker::boundingRect() const
{
  float myHalfSize = mSize / 2.0;
  return QRectF( -myHalfSize, -myHalfSize, 2.0 * myHalfSize, 2.0 * myHalfSize );
}

void QgsGpsMarker::updatePosition()
{
  setCenter( mCenter );
}
