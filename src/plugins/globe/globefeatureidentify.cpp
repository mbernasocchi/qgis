/***************************************************************************
    globefeatureidentify.cpp
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

#include "globefeatureidentify.h"
#include "qgsosgearthfeaturesource.h"

#include <qgsvectorlayer.h>
#include <qgsmapcanvas.h>

GlobeFeatureIdentifyCallback::GlobeFeatureIdentifyCallback( QgsMapCanvas* mapCanvas )
    : mRubberBand( new QgsRubberBand( mapCanvas, QGis::Polygon ) )
{
  QColor color( Qt::green );
  color.setAlpha( 190 );

  mRubberBand->setColor( color );
}

GlobeFeatureIdentifyCallback::~GlobeFeatureIdentifyCallback()
{
  delete mRubberBand;
}

void GlobeFeatureIdentifyCallback::onHit( osgEarth::Features::FeatureSourceIndexNode* index, osgEarth::Features::FeatureID fid, const EventArgs& args )
{
  Q_UNUSED( args )
  osgEarth::Features::FeatureSource* featSource = index->getFeatureSource();
  const osgEarth::Features::QgsGlobeFeatureSource* globeSource = dynamic_cast<const osgEarth::Features::QgsGlobeFeatureSource*>( featSource );

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
