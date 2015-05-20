/***************************************************************************
    qgsglobelayerpropertiesfactory.h
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

#ifndef QGSGLOBELAYERPROPERTIESFACTORY_H
#define QGSGLOBELAYERPROPERTIESFACTORY_H

#include <qgsmaplayerpropertiesfactory.h>

#include <QObject>

class QgsMapLayer;

class QgsGlobeLayerPropertiesFactory : public QObject, public QgsMapLayerPropertiesFactory
{
    Q_OBJECT

  public:
    explicit QgsGlobeLayerPropertiesFactory();

    // QgsMapLayerPropertiesFactory interface
  public:
    QgsVectorLayerPropertiesPage* createVectorLayerPropertiesPage( QgsVectorLayer* layer, QWidget* parent );
    QListWidgetItem* createVectorLayerPropertiesItem( QgsVectorLayer* layer, QListWidget* view );

  signals:
    void layerSettingsChanged( QgsMapLayer* layer );
};

#endif // QGSGLOBELAYERPROPERTIESFACTORY_H
