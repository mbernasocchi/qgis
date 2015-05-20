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
#include <QDomElement>
#include <QDomDocument>
#include <QMetaEnum>

#include <qgsvectorlayer.h>
#include <qgslogger.h>

class QgsGlobeVectorLayerConfig : public QObject
{
    Q_OBJECT

  public:
    enum AltitudeClamping
    {
      AltitudeClampingTerrain,
      AltitudeClampingNone,
      AltitudeClampingRelative,
      AltitudeClampingAbsolute
    };
    Q_ENUMS( AltitudeClamping )

    enum AltitudeTechnique
    {
      AltitudeTechniqueDrape,
      AltitudeTechniqueMap,
      AltitudeTechniqueGpu,
      AltitudeTechniqueScene
    };
    Q_ENUMS( AltitudeTechnique )

    enum AltitudeBinding
    {
      AltitudeBindingVertex,
      AltitudeBindingCentroid
    };
    Q_ENUMS( AltitudeBinding )

  public:
    QgsGlobeVectorLayerConfig( QObject* parent )
        : QObject( parent )
        , mAltitudeClamping( AltitudeClampingTerrain )
        , mAltitudeTechnique( AltitudeTechniqueDrape )
        , mAltitudeBinding( AltitudeBindingVertex )
        , mExtrusionEnabled( false )
        , mExtrusionHeight( 10 )
        , mExtrusionFlatten( false )
        , mExtrusionWallGradient( 0.5 )
        , mLighting( true )
    {
    }

    QgsGlobeVectorLayerConfig( AltitudeClamping altitudeClamping, AltitudeTechnique altitudeTechnique, AltitudeBinding altitudeBinding, QObject* parent )
        : QObject( parent )
        , mAltitudeClamping( altitudeClamping )
        , mAltitudeTechnique( altitudeTechnique )
        , mAltitudeBinding( altitudeBinding )
        , mExtrusionEnabled( false )
        , mExtrusionHeight( 10 )
        , mExtrusionFlatten( false )
        , mExtrusionWallGradient( 0.5 )
        , mLighting( true )
    {}

    ~QgsGlobeVectorLayerConfig() {}

    void setAltitudeClamping( AltitudeClamping altitudeClamping ) { mAltitudeClamping = altitudeClamping; }
    AltitudeClamping altitudeClamping() { return mAltitudeClamping; }

    void setAltitudeTechnique( AltitudeTechnique altitudeTechnique ) { mAltitudeTechnique = altitudeTechnique; }
    AltitudeTechnique altitudeTechnique() { return mAltitudeTechnique; }


    void setAltitudeBinding( AltitudeBinding altitudeBinding ) { mAltitudeBinding = altitudeBinding; }
    AltitudeBinding altitudeBinding() { return mAltitudeBinding; }

    void setExtrusionEnabled( bool enabled ) { mExtrusionEnabled = enabled; }
    bool extrusionEnabled() { return mExtrusionEnabled; }

    void setExtrusionHeight( QString height ) { mExtrusionHeight = height; }
    QString extrusionHeight() { return mExtrusionHeight; }

    bool isExtrusionHeightNumeric() { bool ok; mExtrusionHeight.toDouble( &ok ); return ok; }
    double extrusionHeightNumeric() { return mExtrusionHeight.toDouble(); }

    void setExtrusionFlatten( bool flatten ) { mExtrusionFlatten = flatten; }
    bool extrusionFlatten() { return mExtrusionFlatten; }

    void setExtrusionWallGradient( float wallGradient ) { mExtrusionWallGradient = wallGradient; }
    float extrusionWallGradient() { return mExtrusionWallGradient; }

    void setLighting( bool lighting ) { mLighting = lighting; }
    bool lighting() { return mLighting; }

    void setLabelingEnabled( bool enabled ) { mLabelingEnabled = enabled; }
    bool labelingEnabled() { return mLabelingEnabled; }

    void setLabelingField( const QString& field ) { mLabelingField = field; }
    const QString& labelingField() { return mLabelingField; }

    void setLabelingDeclutter( bool declutter ) { mLabelingDeclutter = declutter; }
    bool labelingDeclutter() { return mLabelingDeclutter; }


    /**
     * Helper method
     */
    QString enumToQString( const char* name, int val )
    {
      int idx = metaObject()->indexOfEnumerator( name );
      QMetaEnum e = metaObject()->enumerator( idx );
      return e.valueToKey( val );
    }

    QString dump() const { return QString( "Altitude { Clamping %1, Technique %2, Binding %3 }" ).arg( mAltitudeClamping ).arg( mAltitudeTechnique ).arg( mAltitudeBinding ); }

    template <typename T>
    static T qStringToEnum( const char* name, const QString& val )
    {
      int idx = staticMetaObject.indexOfEnumerator( name );
      QMetaEnum e = staticMetaObject.enumerator( idx );

      T ret = ( T )e.keyToValue( val.toUtf8().constData() );

      if ( ret == -1 )
      {
        ret = ( T )0;
      }

      return ret;
    }

    static QgsGlobeVectorLayerConfig* configForLayer( QgsVectorLayer* vLayer );

    static void setConfigForLayer( QgsVectorLayer* vLayer, QgsGlobeVectorLayerConfig* newConfig );

    static void readLayer( QgsVectorLayer* vLayer, QDomElement elem )
    {
      QgsGlobeVectorLayerConfig* layerConfig = new QgsGlobeVectorLayerConfig( vLayer );

      QDomElement globeElem = elem.firstChildElement( "globe" );

      if ( !globeElem.isNull() )
      {
        QDomElement renderingElem = globeElem.firstChildElement( "rendering" );
        layerConfig->setLighting( renderingElem.attribute( "lighting", "1" ).toInt() == 1 );

        QDomElement extrusionElem = globeElem.firstChildElement( "extrusion" );
        layerConfig->setExtrusionEnabled( extrusionElem.attribute( "enabled" ).toInt() == 1 );
        layerConfig->setExtrusionHeight( extrusionElem.attribute( "height" ) );
        layerConfig->setExtrusionFlatten( extrusionElem.attribute( "flatten" ).toInt() == 1 );
        layerConfig->setExtrusionWallGradient( extrusionElem.attribute( "wall-gradient" ).toDouble() );

        QDomElement altitudeElem = globeElem.firstChildElement( "altitude" );
        layerConfig->setAltitudeClamping( qStringToEnum<AltitudeClamping>( "AltitudeClamping", altitudeElem.attribute( "clamping" ) ) );
        layerConfig->setAltitudeTechnique( qStringToEnum<AltitudeTechnique>( "AltitudeTechnique", altitudeElem.attribute( "technique" ) ) );
        layerConfig->setAltitudeBinding( qStringToEnum<AltitudeBinding>( "AltitudeBinding", altitudeElem.attribute( "binding" ) ) );

        QDomElement labelingElem = globeElem.firstChildElement( "labeling" );
        layerConfig->setLabelingEnabled( labelingElem.attribute( "enabled", "0" ).toInt() == 1 );
        layerConfig->setLabelingField( labelingElem.attribute( "field" ) );
        layerConfig->setLabelingDeclutter( labelingElem.attribute( "declutter", "1" ).toInt() == 1 );
      }

      setConfigForLayer( vLayer, layerConfig );
    }

    static void writeLayer( QgsVectorLayer* vLayer, QDomElement& elem, QDomDocument& doc )
    {
      QgsGlobeVectorLayerConfig* layerConfig = configForLayer( vLayer );

      QDomElement globeElem = doc.createElement( "globe" );

      QDomElement renderingElem = doc.createElement( "rendering" );
      renderingElem.setAttribute( "lighting", layerConfig->lighting() );
      globeElem.appendChild( renderingElem );

      QDomElement extrusionElem = doc.createElement( "extrusion" );
      extrusionElem.setAttribute( "enabled", layerConfig->extrusionEnabled() );
      extrusionElem.setAttribute( "height", layerConfig->extrusionHeight() );
      extrusionElem.setAttribute( "flatten", layerConfig->extrusionFlatten() );
      extrusionElem.setAttribute( "wall-gradient", layerConfig->extrusionWallGradient() );
      globeElem.appendChild( extrusionElem );

      QDomElement altitudeElem = doc.createElement( "altitude" );
      altitudeElem.setAttribute( "clamping", layerConfig->enumToQString( "AltitudeClamping", layerConfig->altitudeClamping() ) );
      altitudeElem.setAttribute( "technique", layerConfig->enumToQString( "AltitudeTechnique", layerConfig->altitudeTechnique() ) );
      altitudeElem.setAttribute( "binding", layerConfig->enumToQString( "AltitudeBinding", layerConfig->altitudeBinding() ) );
      globeElem.appendChild( altitudeElem );

      QDomElement labelingElem = doc.createElement( "labeling" );
      labelingElem.setAttribute( "enabled", layerConfig->labelingEnabled() );
      labelingElem.setAttribute( "field", layerConfig->labelingField() );
      labelingElem.setAttribute( "declutter", layerConfig->labelingDeclutter() );
      globeElem.appendChild( labelingElem );

      elem.appendChild( globeElem );
    }

  protected:
    AltitudeClamping mAltitudeClamping;
    AltitudeTechnique mAltitudeTechnique;
    AltitudeBinding mAltitudeBinding;

    bool mExtrusionEnabled;
    QString mExtrusionHeight;
    bool mExtrusionFlatten;
    float mExtrusionWallGradient;

    bool mLabelingEnabled;
    QString mLabelingField;
    bool mLabelingDeclutter;

    bool mLighting;
};

Q_DECLARE_METATYPE( QgsGlobeVectorLayerConfig* )
Q_DECLARE_METATYPE( QgsGlobeVectorLayerConfig::AltitudeClamping )
Q_DECLARE_METATYPE( QgsGlobeVectorLayerConfig::AltitudeTechnique )
Q_DECLARE_METATYPE( QgsGlobeVectorLayerConfig::AltitudeBinding )

#endif // QGSGLOBEVECTORLAYERCONFIG_H
