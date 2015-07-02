/***************************************************************************
    globe.h

    Globe Plugin
    a QGIS plugin
     --------------------------------------
    Date                 : 08-Jul-2010
    Copyright            : (C) 2010 by Sourcepole
    Email                : info at sourcepole.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGS_GLOBE_PLUGIN_H
#define QGS_GLOBE_PLUGIN_H

#include <qgisplugin.h>
#include <QObject>
#include <osg/ref_ptr>

#include "qgsglobeinterface.h"

class QAction;
class QDateTime;
class QDockWidget;
class QDomDocument;
class QDomElement;
class QgsGlobeLayerPropertiesFactory;
class QgsGlobeInterface;
class QgsGlobePluginDialog;
class QgsMapLayer;
class QgsPoint;
class GlobeFrustumHighlightCallback;
class QgsOsgEarthTileSource;

namespace osg
{
  class Group;
  class Vec3d;
}
namespace osgViewer { class Viewer; }

namespace osgEarth
{
  class ImageLayer;
  class MapNode;
  namespace QtGui { class ViewerWidget; }
  namespace Util
  {
    class FeatureQueryTool;
    class SkyNode;
    class VerticalScale;
    namespace Controls
    {
      class Control;
      class ControlCanvas;
      class ControlEventHandler;
    }
  }
}


class GLOBE_EXPORT GlobePlugin : public QObject, public QgisPlugin
{
    Q_OBJECT

  public:
    GlobePlugin( QgisInterface* theQgisInterface );
    ~GlobePlugin();

    //! offer an interface for python plugins
    virtual QgsPluginInterface* pluginInterface();
    //! init the gui
    virtual void initGui() override;
    //!  Reset globe
    void reset();
    //! unload the plugin
    void unload() override;

    //! Set a different base map (QString::NULL will disable the base map)
    void setBaseMap( QString url );
    //! Set sky parameters
    void setSkyParameters( bool enabled, const QDateTime& dateTime, bool autoAmbience );
    //! Called when the extents of the map change
    void extentsChanged();
    //! Sync globe extent to mapCanavas
    void syncExtent();
    //! Enable or disable frustum highlight
    void enableFrustumHighlight( bool statu );
    //! Enable or disable feature identification
    void enableFeatureIdentification( bool status );

    //! set the globe coordinates of a user right-click on the globe
    void setSelectedCoordinates( const osg::Vec3d& coords );
    //! get a coordinates vector
    osg::Vec3d getSelectedCoordinates();
    //! emits signal with current mouse coordinates
    void showCurrentCoordinates( double lon, double lat );
    //! get longitude of user right click
    double getSelectedLon() const { return mSelectedLon; }
    //! get latitude of user right click
    double getSelectedLat() const { return mSelectedLat; }
    //! get elevation of user right click
    double getSelectedElevation() { return mSelectedElevation; }

    //! Get the OSG viewer
    osgViewer::Viewer* osgViewer() { return mOsgViewer; }
    //! Get OSG map node
    osgEarth::MapNode* mapNode() { return mMapNode; }

  public slots:
    void run();
    void refreshQGISMapLayer();
    void setVerticalScale( double scale );

  private:
    QgisInterface *mQGisIface;
    QAction * mActionLaunch;
    QAction * mActionSettings;
    QAction * mActionUnload;
    osgViewer::Viewer* mOsgViewer;
    osgEarth::QtGui::ViewerWidget* mViewerWidget;
    QDockWidget* mDockWidget;
    QgsGlobePluginDialog *mSettingsDialog;
    osg::Group* mRootNode;
    osgEarth::MapNode* mMapNode;
    osg::ref_ptr<osgEarth::ImageLayer> mBaseLayer;
    osg::ref_ptr<osgEarth::Util::SkyNode> mSkyNode;
    osg::ref_ptr<osgEarth::Util::VerticalScale> mVerticalScale;
    osgEarth::ImageLayer* mQgisMapLayer;
    QgsOsgEarthTileSource* mTileSource;
    osgEarth::Util::Controls::ControlCanvas* mControlCanvas;
    bool mIsGlobeRunning;
    //! coordinates of the right-clicked point on the globe
    double mSelectedLat, mSelectedLon, mSelectedElevation;

    QgsGlobeInterface mGlobeInterface;
    //! Creates additional pages in the layer properties for adjusting 3D properties
    QgsGlobeLayerPropertiesFactory* mLayerPropertiesFactory;
    GlobeFrustumHighlightCallback* mFrustumHighlightCallback;
    osgEarth::Util::FeatureQueryTool* mFeatureQueryTool;

    void setupProxy();
    void setupMap();
    void addControl( osgEarth::Util::Controls::Control* control, int x, int y, int w, int h, osgEarth::Util::Controls::ControlEventHandler* handler );
    void addImageControl( const std::string &imgPath, int x, int y, osgEarth::Util::Controls::ControlEventHandler* handler = 0 );
    void setupControls();

  private slots:
    void elevationLayersChanged();
    void projectReady();
    void blankProjectReady();
    void showSettings();
    void layersAdded( const QList<QgsMapLayer*>& );
    void layersRemoved( const QStringList& layerIds );
    void layerSettingsChanged( QgsMapLayer* layer );
    void onLayerRead( QgsMapLayer* mapLayer, const QDomElement &elem );
    void onLayerWrite( QgsMapLayer* mapLayer, QDomElement& elem, QDomDocument& doc );

  signals:
    //! emits current mouse position
    void xyCoordinates( const QgsPoint & p );
    //! emits position of right click on globe
    void newCoordinatesSelected( const QgsPoint & p );
};

#endif // QGS_GLOBE_PLUGIN_H
