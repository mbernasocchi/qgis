/***************************************************************************
    qgsmaptooltouch.cpp  -  map tool for zooming and panning using qgestures
    ----------------------
    begin                : February 2012
    copyright            : (C) 2012 by Marco Bernasocchi
    email                : marco at bernawebdesign.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsmaptooltouch.h"
#include "qgsmapcanvas.h"
#include "qgsmapcanvasmap.h"
#include "qgscursors.h"
#include "qgsmaptopixel.h"
#include <QBitmap>
#include <QCursor>
#include <QMouseEvent>
#include <qgslogger.h>


QgsMapToolTouch::QgsMapToolTouch( QgsMapCanvas* canvas )
    : QgsMapTool( canvas ), mDragging( false ), mPinching( false )
{
  // set cursor
//  QBitmap panBmp = QBitmap::fromData( QSize( 16, 16 ), pan_bits );
//  QBitmap panBmpMask = QBitmap::fromData( QSize( 16, 16 ), pan_mask_bits );
//  mCursor = QCursor( panBmp, panBmpMask, 5, 5 );

}

QgsMapToolTouch::~QgsMapToolTouch()
{
  mCanvas->ungrabGesture( Qt::PinchGesture );
}

void QgsMapToolTouch::activate()
{
  mCanvas->grabGesture( Qt::PinchGesture );
  QgsMapTool::activate();
}

void QgsMapToolTouch::deactivate()
{
  mCanvas->ungrabGesture( Qt::PinchGesture );
  QgsMapTool::deactivate();
}

void QgsMapToolTouch::canvasMoveEvent( QMouseEvent * e )
{
  if ( !mPinching )
  {
    if (( e->buttons() & Qt::LeftButton ) )
    {
      mDragging = true;
      // move map and other canvas items
      mCanvas->panAction( e );
    }
  }
}

void QgsMapToolTouch::canvasReleaseEvent( QMouseEvent * e )
{
  if ( !mPinching )
  {
    if ( e->button() == Qt::LeftButton )
    {
      if ( mDragging )
      {
        mCanvas->panActionEnd( e->pos() );
        mDragging = false;
      }
      else // add pan to mouse cursor
      {
        // transform the mouse pos to map coordinates
        QgsPoint center = mCanvas->getCoordinateTransform()->toMapPoint( e->x(), e->y() );
        mCanvas->setExtent( QgsRectangle( center, center ) );
        mCanvas->refresh();
      }
    }
  }
}

void QgsMapToolTouch::canvasDoubleClickEvent( QMouseEvent *e )
{
  if ( !mPinching )
  {
    mCanvas->zoomWithCenter( e->x(), e->y(), true );
  }
}

bool QgsMapToolTouch::gestureEvent( QGestureEvent *event )
{
  qDebug() << "gesture " << event;
  if ( QGesture *gesture = event->gesture( Qt::PinchGesture ) )
  {
    mPinching = true;
    pinchTriggered( static_cast<QPinchGesture *>( gesture ) );
  }
  return true;
}


void QgsMapToolTouch::pinchTriggered( QPinchGesture *gesture )
{
  QPoint currentPos = mCanvas->mapFromGlobal( gesture->centerPoint().toPoint() );
  QgsMapCanvasMap * map = mCanvas->map();
  if ( gesture->state() == Qt::GestureStarted )
  {
        mPinchStartPoint = currentPos;
        qDebug() << "GestureStarted" << mPinchStartPoint;
  }
  if ( gesture->state() == Qt::GestureUpdated )
  {
      QPoint newPos = currentPos - mPinchStartPoint;
//      mCanvas->panAction( newPos );
      if ( ! ( 0.98 < gesture->totalScaleFactor()  && gesture->totalScaleFactor() < 1.02 ) )
      {
          qDebug() << "scale" << map->scale();
          map->setTransformOriginPoint(currentPos);
          map->setScale(gesture->totalScaleFactor());
      }
  }
  if ( gesture->state() == Qt::GestureFinished )
  {
    qDebug() << "GestureFinished";
    //a very small totalScaleFactor indicates a two finger tap (pinch gesture without pinching)
    if ( 0.98 < gesture->totalScaleFactor()  && gesture->totalScaleFactor() < 1.02 )
    {
      mCanvas->zoomOut();
    }
    else
    {
      qDebug() << "GestureFinished" << currentPos << mPinchStartPoint;
//      mCanvas->panActionEnd( currentPos );
      QgsRectangle extent = mCanvas->extent();
      QgsPoint center  = mCanvas->getCoordinateTransform()->toMapPoint( currentPos.x(), currentPos.y() );
      //reset the "live zooming"
      map->setScale(1);
      extent.scale( 1 / gesture->totalScaleFactor(), &center );
      mCanvas->setExtent( extent );
      mCanvas->refresh();
    }
    mPinching = false;
  }
}
