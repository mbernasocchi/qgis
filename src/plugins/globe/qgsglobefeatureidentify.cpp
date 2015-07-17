/***************************************************************************
    qgsglobefeatureidentify.cpp
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

#include "qgsglobefeatureidentify.h"
#include "featuresource/qgsglobefeaturesource.h"

#include <qgsvectorlayer.h>
#include <qgsrubberband.h>

QgsGlobeFeatureIdentifyCallback::QgsGlobeFeatureIdentifyCallback( QgsMapCanvas* mapCanvas )
    : mRubberBand( new QgsRubberBand( mapCanvas, QGis::Polygon ) )
{
  QColor color( Qt::green );
  color.setAlpha( 190 );

  mRubberBand->setColor( color );
}

QgsGlobeFeatureIdentifyCallback::~QgsGlobeFeatureIdentifyCallback()
{
  delete mRubberBand;
}

void QgsGlobeFeatureIdentifyCallback::onHit( osgEarth::Features::FeatureSourceIndexNode* index, osgEarth::Features::FeatureID fid, const EventArgs& /*args*/ )
{
  QgsGlobeFeatureSource* globeSource = dynamic_cast<QgsGlobeFeatureSource*>( index->getFeatureSource() );

  if ( globeSource )
  {
    QgsFeature feat;
    QgsVectorLayer* lyr = globeSource->layer();

    lyr->getFeatures( QgsFeatureRequest().setFilterFid( fid ) ).nextFeature( feat );

    if ( feat.isValid() )
      mRubberBand->setToGeometry( feat.geometry(), lyr );
    else
      mRubberBand->reset( QGis::Polygon );
  }
  else
  {
    QgsDebugMsg( "Clicked feature was not on a QGIS layer" );
  }
}

void QgsGlobeFeatureIdentifyCallback::onMiss(const EventArgs &/*args*/)
{
  mRubberBand->reset( QGis::Polygon );
}
