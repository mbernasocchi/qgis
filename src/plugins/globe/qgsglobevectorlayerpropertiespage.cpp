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

  mCbxAltitudeClamping->setModel( new QgsEnumComboBoxModel<QgsGlobeVectorLayerConfig::AltitudeClamping>( mCbxAltitudeClamping ) );
  mCbxAltitudeTechnique->setModel( new QgsEnumComboBoxModel<QgsGlobeVectorLayerConfig::AltitudeTechnique>( mCbxAltitudeTechnique ) );
  mCbxAltitudeBinding->setModel( new QgsEnumComboBoxModel<QgsGlobeVectorLayerConfig::AltitudeBinding>( mCbxAltitudeBinding ) );

  QgsGlobeVectorLayerConfig* layerConfig = QgsGlobeVectorLayerConfig::configForLayer( mLayer );

  mChkLighting->setChecked( layerConfig->lighting() );

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

  // Set altitude combobox values
  mCbxAltitudeClamping->setCurrentIndex( mCbxAltitudeClamping->findData( QVariant::fromValue<QgsGlobeVectorLayerConfig::AltitudeClamping>( layerConfig->altitudeClamping() ) ) );
  mCbxAltitudeTechnique->setCurrentIndex( mCbxAltitudeTechnique->findData( QVariant::fromValue<QgsGlobeVectorLayerConfig::AltitudeTechnique>( layerConfig->altitudeTechnique() ) ) );
  mCbxAltitudeBinding->setCurrentIndex( mCbxAltitudeBinding->findData( QVariant::fromValue<QgsGlobeVectorLayerConfig::AltitudeBinding>( layerConfig->altitudeBinding() ) ) );

  mGbxExtrusion->setChecked( layerConfig->extrusionEnabled() );
  mTxtHeight->setText( layerConfig->extrusionHeight() );
  mChkExtrusionFlatten->setChecked( layerConfig->extrusionFlatten() );
  mTxtWallGradient->setValue( layerConfig->extrusionWallGradient() );

  // Set labeling values
  mGbxLabelingEnabled->setChecked( layerConfig->labelingEnabled() );
  mCbxLabelingDeclutter->setChecked( layerConfig->labelingDeclutter() );

  // Trigger visibility determination for dynamicly visible widgets
  on_mCbxAltitudeClamping_currentIndexChanged( mCbxAltitudeClamping->currentIndex() );
}

void QgsGlobeVectorLayerPropertiesPage::apply()
{
  QgsGlobeVectorLayerConfig* layerConfig = new QgsGlobeVectorLayerConfig(
    mCbxAltitudeClamping->itemData( mCbxAltitudeClamping->currentIndex() ).value<QgsGlobeVectorLayerConfig::AltitudeClamping>(),
    mCbxAltitudeTechnique->itemData( mCbxAltitudeTechnique->currentIndex() ).value<QgsGlobeVectorLayerConfig::AltitudeTechnique>(),
    mCbxAltitudeBinding->itemData( mCbxAltitudeBinding->currentIndex() ).value<QgsGlobeVectorLayerConfig::AltitudeBinding>(),
    mLayer
  );

  layerConfig->setExtrusionEnabled( mGbxExtrusion->isChecked() );
  layerConfig->setExtrusionHeight( mTxtHeight->text() );
  layerConfig->setExtrusionFlatten( mChkExtrusionFlatten->isChecked() );
  layerConfig->setExtrusionWallGradient( mTxtWallGradient->value() );

  layerConfig->setLighting( mChkLighting->isChecked() );

  layerConfig->setLabelingEnabled( mGbxLabelingEnabled->isChecked() );
  layerConfig->setLabelingDeclutter( mCbxLabelingDeclutter->isChecked() );

  QgsGlobeVectorLayerConfig::setConfigForLayer( mLayer, layerConfig );

  emit layerSettingsChanged( mLayer );
}

void QgsGlobeVectorLayerPropertiesPage::on_mCbxAltitudeClamping_currentIndexChanged( int index )
{
  QgsGlobeVectorLayerConfig::AltitudeClamping clamping = mCbxAltitudeClamping->itemData( index ).value<QgsGlobeVectorLayerConfig::AltitudeClamping>();

  if ( clamping == QgsGlobeVectorLayerConfig::AltitudeClampingTerrain )
  {
    mLblTechnique->show();
    mCbxAltitudeTechnique->show();
  }
  else
  {
    mLblTechnique->hide();
    mCbxAltitudeTechnique->hide();
  }

  on_mCbxAltitudeTechnique_currentIndexChanged( mCbxAltitudeTechnique->currentIndex() );
}

void QgsGlobeVectorLayerPropertiesPage::on_mCbxAltitudeTechnique_currentIndexChanged( int index )
{
  QgsGlobeVectorLayerConfig::AltitudeTechnique technique = mCbxAltitudeTechnique->itemData( index ).value<QgsGlobeVectorLayerConfig::AltitudeTechnique>();

  if ( technique == QgsGlobeVectorLayerConfig::AltitudeTechniqueMap && !mCbxAltitudeTechnique->isHidden() )
  {
    mLblBinding->show();
    mCbxAltitudeBinding->show();
    mLblResolution->show();
    mTxtAltitudeResolution->show();
  }
  else
  {
    mLblBinding->hide();
    mCbxAltitudeBinding->hide();
    mLblResolution->hide();
    mTxtAltitudeResolution->hide();
  }
}


QgsGlobeVectorLayerConfig* QgsGlobeVectorLayerConfig::configForLayer( QgsVectorLayer* vLayer )
{
  QVariant v = vLayer->property( "globe-config" );

  QgsGlobeVectorLayerConfig* layerConfig = v.value<QgsGlobeVectorLayerConfig*>();

  if ( !layerConfig )
  {
    layerConfig = new QgsGlobeVectorLayerConfig( vLayer );
  }

  return layerConfig;
}

void QgsGlobeVectorLayerConfig::setConfigForLayer( QgsVectorLayer* vLayer, QgsGlobeVectorLayerConfig* newConfig )
{
  QgsGlobeVectorLayerConfig* oldConfig = vLayer->property( "globe-config" ).value<QgsGlobeVectorLayerConfig*>();

  if ( oldConfig != newConfig )
  {
    if ( oldConfig && oldConfig->parent() == vLayer )
      delete oldConfig;

    vLayer->setProperty( "globe-config", QVariant::fromValue<QgsGlobeVectorLayerConfig*>( newConfig ) );
  }
}
