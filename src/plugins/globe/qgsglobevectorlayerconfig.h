/***************************************************************************
    QgsGlobeVectorLayerConfig.h
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

#ifndef QGSGLOBEVECTORLAYERCONFIG_H
#define QGSGLOBEVECTORLAYERCONFIG_H

#include <QObject>

#include <iostream>

class QgsGlobeVectorLayerConfig
{
  public:
    enum AltitudeClamping
    {
      AltitudeClampingNone,
      AltitudeClampingTerrain,
      AltitudeClampingRelative,
      AltitudeClampingAbsolute
    };

    enum AltitudeTechnique
    {
      AltitudeTechniqueMap,
      AltitudeTechniqueDrape,
      AltitudeTechniqueGpu,
      AltitudeTechniqueScene
    };

    enum AltitudeBinding
    {
      AltitudeBindingVertex,
      AltitudeBindingCentroid
    };

  public:
    QgsGlobeVectorLayerConfig()
        : mAltitudeClamping( AltitudeClampingTerrain )
        , mAltitudeTechnique( AltitudeTechniqueDrape )
        , mAltitudeBinding( AltitudeBindingVertex )
    {
    }

    QgsGlobeVectorLayerConfig( AltitudeClamping altitudeClamping, AltitudeTechnique altitudeTechnique, AltitudeBinding altitudeBinding )
        : mAltitudeClamping( altitudeClamping )
        , mAltitudeTechnique( altitudeTechnique )
        , mAltitudeBinding( altitudeBinding )
    {
    }

    ~QgsGlobeVectorLayerConfig() {}

    AltitudeClamping altitudeClamping() { return mAltitudeClamping; }
    AltitudeTechnique altitudeTechnique() { return mAltitudeTechnique; }
    AltitudeBinding altitudeBinding() { return mAltitudeBinding; }

    QString dump() const { return QString( "Altitude { Clamping %1, Technique %2, Binding %3 }" ).arg( mAltitudeClamping ).arg( mAltitudeTechnique ).arg( mAltitudeBinding ); }

  protected:
    AltitudeClamping mAltitudeClamping;
    AltitudeTechnique mAltitudeTechnique;
    AltitudeBinding mAltitudeBinding;
};

Q_DECLARE_METATYPE( QgsGlobeVectorLayerConfig )
Q_DECLARE_METATYPE( QgsGlobeVectorLayerConfig::AltitudeClamping )
Q_DECLARE_METATYPE( QgsGlobeVectorLayerConfig::AltitudeTechnique )
Q_DECLARE_METATYPE( QgsGlobeVectorLayerConfig::AltitudeBinding )

#endif // QGSGLOBEVECTORLAYERCONFIG_H
