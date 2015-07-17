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
#include "qgsglobeplugindialog.h"

class QAction;
class QDateTime;
class QDockWidget;
class QgsGlobeLayerPropertiesFactory;
class QgsGlobeInterface;
class QgsGlobePluginDialog;
class QgsMapLayer;
class QgsPoint;
class QgsGlobeFrustumHighlightCallback;
class QgsGlobeFeatureIdentifyCallback;
class QgsGlobeTileSource;

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
    class FeatureHighlightCallback;
    class FeatureQueryTool;
    class SkyNode;
    class VerticalScale;
    namespace Controls
    {
      class Control;
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
    //! unload the plugin
    void unload() override;

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

  private:
    QgisInterface *mQGisIface;

    QAction* mActionLaunch;
    QAction* mActionSettings;
    QAction* mActionUnload;
    osgEarth::QtGui::ViewerWidget* mViewerWidget;
    QDockWidget* mDockWidget;
    QgsGlobePluginDialog* mSettingsDialog;

    QgsGlobeInterface mGlobeInterface;
    bool mIsGlobeRunning;
    QString mBaseLayerUrl;
    QList<QgsGlobePluginDialog::ElevationDataSource> mElevationSources;
    double mSelectedLat, mSelectedLon, mSelectedElevation;

    osg::ref_ptr<osgViewer::Viewer> mOsgViewer;
    osg::ref_ptr<osgEarth::MapNode> mMapNode;
    osg::ref_ptr<osg::Group> mRootNode;
    osg::ref_ptr<osgEarth::Util::SkyNode> mSkyNode;
    osg::ref_ptr<osgEarth::ImageLayer> mBaseLayer;
    osg::ref_ptr<osgEarth::ImageLayer> mQgisMapLayer;
    osg::ref_ptr<osgEarth::Util::VerticalScale> mVerticalScale;
    osg::ref_ptr<QgsGlobeTileSource> mTileSource;

    //! Creates additional pages in the layer properties for adjusting 3D properties
    QgsGlobeLayerPropertiesFactory* mLayerPropertiesFactory;
    osg::ref_ptr<QgsGlobeFrustumHighlightCallback> mFrustumHighlightCallback;
    osg::ref_ptr<QgsGlobeFeatureIdentifyCallback> mFeatureQueryToolIdentifyCb;
    osg::ref_ptr<osgEarth::Util::FeatureHighlightCallback> mFeatureQueryToolHighlightCb;
    osg::ref_ptr<osgEarth::Util::FeatureQueryTool> mFeatureQueryTool;

    void setupProxy();
    void setupMap();
    void addControl( osgEarth::Util::Controls::Control* control, int x, int y, int w, int h, osgEarth::Util::Controls::ControlEventHandler* handler );
    void addImageControl( const std::string &imgPath, int x, int y, osgEarth::Util::Controls::ControlEventHandler* handler = 0 );
    void setupControls();
    void applyProjectSettings();

  private slots:
    void reset();
    void projectRead();
    void showSettings();
    void applySettings();
    void layersAdded( const QList<QgsMapLayer*>& );
    void layersRemoved( const QStringList& layerIds );
    void layerChanged( QgsMapLayer* layer );

  signals:
    //! emits current mouse position
    void xyCoordinates( const QgsPoint & p );
    //! emits position of right click on globe
    void newCoordinatesSelected( const QgsPoint & p );
};

#endif // QGS_GLOBE_PLUGIN_H
