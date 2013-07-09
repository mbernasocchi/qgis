/***************************************************************************
    qgsglobevectorlayerpropertiespage.cpp
     --------------------------------------
    Date                 : 9.7.2013
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

#include "qgsglobevectorlayerpropertiespage.h"

#include <qgsvectorlayer.h>
#include <qgslogger.h>

#include "qgsglobevectorlayerconfig.h"

QgsGlobeVectorLayerPropertiesPage::QgsGlobeVectorLayerPropertiesPage( QgsVectorLayer* layer, QWidget *parent )
    : QgsVectorLayerPropertiesPage( parent )
    , mLayer( layer )
{
  setupUi( this );

  mCbxAltitudeClamping->setModel( new QgsEnumComboBox<QgsGlobeVectorLayerConfig::AltitudeClamping>( mCbxAltitudeClamping ) );
  mCbxAltitudeTechnique->setModel( new QgsEnumComboBox<QgsGlobeVectorLayerConfig::AltitudeTechnique>( mCbxAltitudeTechnique ) );
  mCbxAltitudeBinding->setModel( new QgsEnumComboBox<QgsGlobeVectorLayerConfig::AltitudeBinding>( mCbxAltitudeBinding ) );

  // Fill altitude combo boxes
  mCbxAltitudeClamping->addItem( tr( "None" ), QVariant::fromValue<QgsGlobeVectorLayerConfig::AltitudeClamping>( QgsGlobeVectorLayerConfig::AltitudeClampingNone ) );
  mCbxAltitudeClamping->addItem( tr( "Terrain" ), QVariant::fromValue<QgsGlobeVectorLayerConfig::AltitudeClamping>( QgsGlobeVectorLayerConfig::AltitudeClampingTerrain ) );
  mCbxAltitudeClamping->addItem( tr( "Relative" ), QVariant::fromValue<QgsGlobeVectorLayerConfig::AltitudeClamping>( QgsGlobeVectorLayerConfig::AltitudeClampingRelative ) );
  mCbxAltitudeClamping->addItem( tr( "Absolute" ), QVariant::fromValue<QgsGlobeVectorLayerConfig::AltitudeClamping>( QgsGlobeVectorLayerConfig::AltitudeClampingAbsolute ) );

  mCbxAltitudeTechnique->addItem( tr( "Map" ), QVariant::fromValue<QgsGlobeVectorLayerConfig::AltitudeTechnique>( QgsGlobeVectorLayerConfig::AltitudeTechniqueMap ) );
  mCbxAltitudeTechnique->addItem( tr( "Drape" ), QVariant::fromValue<QgsGlobeVectorLayerConfig::AltitudeTechnique>( QgsGlobeVectorLayerConfig::AltitudeTechniqueDrape ) );
  mCbxAltitudeTechnique->addItem( tr( "GPU" ), QVariant::fromValue<QgsGlobeVectorLayerConfig::AltitudeTechnique>( QgsGlobeVectorLayerConfig::AltitudeTechniqueGpu ) );
  mCbxAltitudeTechnique->addItem( tr( "Scene" ), QVariant::fromValue<QgsGlobeVectorLayerConfig::AltitudeTechnique>( QgsGlobeVectorLayerConfig::AltitudeTechniqueScene ) );

  mCbxAltitudeBinding->addItem( tr( "Vertex" ), QVariant::fromValue<QgsGlobeVectorLayerConfig::AltitudeBinding>( QgsGlobeVectorLayerConfig::AltitudeBindingVertex ) );
  mCbxAltitudeBinding->addItem( tr( "Centroid" ), QVariant::fromValue<QgsGlobeVectorLayerConfig::AltitudeBinding>( QgsGlobeVectorLayerConfig::AltitudeBindingCentroid ) );

  QgsGlobeVectorLayerConfig layerConfig = mLayer->property( "globe-config" ).value<QgsGlobeVectorLayerConfig>();

  mCbxAltitudeClamping->setCurrentIndex( mCbxAltitudeClamping->findData( QVariant::fromValue<QgsGlobeVectorLayerConfig::AltitudeClamping>( layerConfig.altitudeClamping() ) ) );
  mCbxAltitudeTechnique->setCurrentIndex( mCbxAltitudeTechnique->findData( QVariant::fromValue<QgsGlobeVectorLayerConfig::AltitudeTechnique>( layerConfig.altitudeTechnique() ) ) );
  mCbxAltitudeBinding->setCurrentIndex( mCbxAltitudeBinding->findData( QVariant::fromValue<QgsGlobeVectorLayerConfig::AltitudeBinding>( layerConfig.altitudeBinding() ) ) );
}

void QgsGlobeVectorLayerPropertiesPage::apply()
{
  QgsGlobeVectorLayerConfig layerConfig(
    mCbxAltitudeClamping->itemData( mCbxAltitudeClamping->currentIndex() ).value<QgsGlobeVectorLayerConfig::AltitudeClamping>(),
    mCbxAltitudeTechnique->itemData( mCbxAltitudeTechnique->currentIndex() ).value<QgsGlobeVectorLayerConfig::AltitudeTechnique>(),
    mCbxAltitudeBinding->itemData( mCbxAltitudeBinding->currentIndex() ).value<QgsGlobeVectorLayerConfig::AltitudeBinding>()
  );

  mLayer->setProperty( "globe-config", QVariant::fromValue<QgsGlobeVectorLayerConfig>( layerConfig ) );

  emit layerSettingsChanged( mLayer );
}
