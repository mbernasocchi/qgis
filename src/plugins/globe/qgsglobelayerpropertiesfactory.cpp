/***************************************************************************
    qgsglobelayerpropertiesfactory.cpp
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

#include "qgsglobelayerpropertiesfactory.h"

#include "qgsglobevectorlayerpropertiespage.h"

QgsGlobeLayerPropertiesFactory::QgsGlobeLayerPropertiesFactory()
    : QObject()
    , QgsMapLayerPropertiesFactory()
{
}


QgsVectorLayerPropertiesPage* QgsGlobeLayerPropertiesFactory::createVectorLayerPropertiesPage( QgsVectorLayer* layer, QWidget* parent )
{
  QgsGlobeVectorLayerPropertiesPage* propsPage = new QgsGlobeVectorLayerPropertiesPage( layer, parent );
  connect( propsPage, SIGNAL( layerSettingsChanged( QgsMapLayer* ) ), this, SIGNAL( layerSettingsChanged( QgsMapLayer* ) ) );
  return propsPage;
}

QListWidgetItem* QgsGlobeLayerPropertiesFactory::createVectorLayerPropertiesItem( QgsVectorLayer* layer, QListWidget* view )
{
  Q_UNUSED( layer );
  return new QListWidgetItem( QIcon( ":/globe/icon.svg" ), tr( "Globe" ), view );
}
