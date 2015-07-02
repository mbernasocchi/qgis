/***************************************************************************
    globe_plugin.cpp

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

// Include this first to avoid _POSIX_C_SOURCE redefined warnings
// see http://bytes.com/topic/python/answers/30009-warning-_posix_c_source-redefined
#include "qgsglobeinterface.h"

#include "globe_plugin.h"
#include "globe_plugin_dialog.h"
#include "qgsosgearthtilesource.h"
#include "qgsosgearthfeaturesource.h"
#include "qgsosgearthfeatureoptions.h"
#include "qgsglobestyleutils.h"
#include "qgsglobelayerpropertiesfactory.h"
#include "qgsglobevectorlayerconfig.h"
#include "globefeatureidentify.h"
#include "globefrustumhighlight.h"

#include <qgisinterface.h>
#include <qgisgui.h>
#include <qgslogger.h>
#include <qgsapplication.h>
#include <qgsmapcanvas.h>
#include <qgsmaplayerregistry.h>
#include <qgsfeature.h>
#include <qgsgeometry.h>
#include <qgspoint.h>
#include <qgsdistancearea.h>
#include <symbology-ng/qgsrendererv2.h>
#include <symbology-ng/qgssymbolv2.h>
#include <qgspallabeling.h>

#include <QAction>
#include <QDir>
#include <QDockWidget>
#include <QStringList>

#include <osgDB/ReadFile>
#include <osgDB/Registry>

#include <osgGA/StateSetManipulator>
#include <osgGA/GUIEventHandler>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgEarthQt/ViewerWidget>
#include <osgEarth/Notify>
#include <osgEarth/Map>
#include <osgEarth/MapNode>
#include <osgEarth/TileSource>
#include <osgEarthUtil/EarthManipulator>
#if OSGEARTH_VERSION_LESS_THAN( 2, 6, 0 )
#include <osgEarthUtil/SkyNode>
#else
#include <osgEarthUtil/Sky>
#endif
#include <osgEarthUtil/AutoClipPlaneHandler>
#include <osgEarthDrivers/gdal/GDALOptions>
#include <osgEarthDrivers/tms/TMSOptions>

#if OSGEARTH_VERSION_GREATER_OR_EQUAL( 2, 2, 0 )
#include <osgEarthDrivers/cache_filesystem/FileSystemCache>
#endif
#if OSGEARTH_VERSION_GREATER_OR_EQUAL( 2, 5, 0 )
#include <osgEarthUtil/VerticalScale>
#endif
#include <osgEarthDrivers/model_feature_geom/FeatureGeomModelOptions>
#include <osgEarthUtil/FeatureQueryTool>
#include <osgEarthFeatures/FeatureDisplayLayout>

#define MOVE_OFFSET 0.05

static const QString sName = QObject::tr( "Globe" );
static const QString sDescription = QObject::tr( "Overlay data on a 3D globe" );
static const QString sCategory = QObject::tr( "Plugins" );
static const QString sPluginVersion = QObject::tr( "Version 0.1" );
static const QgisPlugin::PLUGINTYPE sPluginType = QgisPlugin::UI;
static const QString sIcon = ":/globe/icon.svg";
static const QString sExperimental = QString( "true" );


class NavigationControlHandler : public osgEarth::Util::Controls::ControlEventHandler
{
  public:
    virtual void onMouseDown() { }
    virtual void onClick( const osgGA::GUIEventAdapter& /*ea*/, osgGA::GUIActionAdapter& /*aa*/ ) {}
};

class ZoomControlHandler : public NavigationControlHandler
{
  public:
    ZoomControlHandler( osgEarth::Util::EarthManipulator* manip, double dx, double dy )
        : _manip( manip ), _dx( dx ), _dy( dy ) { }
    virtual void onMouseDown() override
    {
      _manip->zoom( _dx, _dy );
    }
  private:
    osg::observer_ptr<osgEarth::Util::EarthManipulator> _manip;
    double _dx;
    double _dy;
};

class HomeControlHandler : public NavigationControlHandler
{
  public:
    HomeControlHandler( osgEarth::Util::EarthManipulator* manip ) : _manip( manip ) { }
    virtual void onClick( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa ) override
    {
      _manip->home( ea, aa );
    }
  private:
    osg::observer_ptr<osgEarth::Util::EarthManipulator> _manip;
};

struct RefreshControlHandler : public NavigationControlHandler
{
  RefreshControlHandler( GlobePlugin* globe ) : mGlobe( globe ) { }
  virtual void onClick( const osgGA::GUIEventAdapter& /*ea*/, osgGA::GUIActionAdapter& /*aa*/ ) override
  {
    mGlobe->canvasLayersChanged();
  }
private:
  GlobePlugin* mGlobe;
};

class SyncExtentControlHandler : public NavigationControlHandler
{
  public:
    SyncExtentControlHandler( GlobePlugin* globe ) : mGlobe( globe ) { }
    virtual void onClick( const osgGA::GUIEventAdapter& /*ea*/, osgGA::GUIActionAdapter& /*aa*/ ) override
    {
      mGlobe->syncExtent();
    }
  private:
    GlobePlugin* mGlobe;
};


class PanControlHandler : public NavigationControlHandler
{
  public:
    PanControlHandler( osgEarth::Util::EarthManipulator* manip, double dx, double dy ) : _manip( manip ), _dx( dx ), _dy( dy ) { }
    virtual void onMouseDown() override
    {
      _manip->pan( _dx, _dy );
    }
  private:
    osg::observer_ptr<osgEarth::Util::EarthManipulator> _manip;
    double _dx;
    double _dy;
};

class RotateControlHandler : public NavigationControlHandler
{
  public:
    RotateControlHandler( osgEarth::Util::EarthManipulator* manip, double dx, double dy ) : _manip( manip ), _dx( dx ), _dy( dy ) { }
    virtual void onMouseDown() override
    {
      if ( 0 == _dx && 0 == _dy )
        _manip->setRotation( osg::Quat() );
      else
        _manip->rotate( _dx, _dy );
    }
  private:
    osg::observer_ptr<osgEarth::Util::EarthManipulator> _manip;
    double _dx;
    double _dy;
};

class FlyToExtentHandler : public osgGA::GUIEventHandler
{
  public:
    FlyToExtentHandler( GlobePlugin* globe ) : mGlobe( globe ) { }

    bool handle( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& /*aa*/ ) override
    {
      if ( ea.getEventType() == ea.KEYDOWN && ea.getKey() == '1' )
      {
        mGlobe->syncExtent();
      }
      return false;
    }

  private:
    GlobePlugin* mGlobe;

};

class KeyboardControlHandler : public osgGA::GUIEventHandler
{
  public:
    KeyboardControlHandler( osgEarth::Util::EarthManipulator* manip ) : _manip( manip ) { }

    bool handle( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa ) override;

  private:
    osg::observer_ptr<osgEarth::Util::EarthManipulator> _manip;
};

class NavigationControl : public osgEarth::Util::Controls::ImageControl
{
  public:
    NavigationControl( osg::Image* image = 0 ) : ImageControl( image ),  mMousePressed( false ) {}

  protected:
    bool handle( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa, osgEarth::Util::Controls::ControlContext& cx ) override;

  private:
    bool mMousePressed;
};


GlobePlugin::GlobePlugin( QgisInterface* theQgisInterface )
    : QgisPlugin( sName, sDescription, sCategory, sPluginVersion, sPluginType )
    , mQGisIface( theQgisInterface )
    , mActionLaunch( 0 )
    , mActionSettings( 0 )
    , mActionUnload( 0 )
    , mOsgViewer( 0 )
    , mViewerWidget( 0 )
    , mDockWidget( 0 )
    , mSettingsDialog( 0 )
    , mRootNode( 0 )
    , mMapNode( 0 )
    , mBaseLayer( 0 )
    , mQgisMapLayer( 0 )
    , mTileSource( 0 )
    , mControlCanvas( 0 )
    , mIsGlobeRunning( false )
    , mSelectedLat( 0. )
    , mSelectedLon( 0. )
    , mSelectedElevation( 0. )
    , mGlobeInterface( this )
    , mFeatureQueryTool( 0 )
{
#ifdef Q_OS_MACX
  // update path to osg plugins on Mac OS X
  if ( !getenv( "OSG_LIBRARY_PATH" ) )
  {
    // OSG_PLUGINS_PATH value set by CMake option
    QString ogsPlugins( OSG_PLUGINS_PATH );
    QString bundlePlugins = QgsApplication::pluginPath() + "/../osgPlugins";
    if ( QFile::exists( bundlePlugins ) )
    {
      // add internal osg plugin path if bundled osg
      ogsPlugins = bundlePlugins;
    }
    if ( QFile::exists( ogsPlugins ) )
    {
      osgDB::Registry::instance()->setLibraryFilePathList( QDir::cleanPath( ogsPlugins ).toStdString() );
    }
  }
#endif
  connect( QgsProject::instance(), SIGNAL( readMapLayer( QgsMapLayer*, QDomElement ) ), this, SLOT( onLayerRead( QgsMapLayer*, QDomElement ) ) );
  connect( QgsProject::instance(), SIGNAL( writeMapLayer( QgsMapLayer*, QDomElement&, QDomDocument& ) ), this, SLOT( onLayerWrite( QgsMapLayer*, QDomElement&, QDomDocument& ) ) );
}

GlobePlugin::~GlobePlugin() {}

void GlobePlugin::initGui()
{
  mSettingsDialog = new QgsGlobePluginDialog( this, mQGisIface->mainWindow(), QgisGui::ModalDialogFlags );

  // Create the action for tool
  mActionLaunch = new QAction( QIcon( ":/globe/globe.png" ), tr( "Launch Globe" ), this );
  mActionLaunch->setWhatsThis( tr( "Overlay data on a 3D globe" ) );
  mActionSettings = new QAction( QIcon( ":/globe/globe.png" ), tr( "Globe Settings" ), this );
  mActionSettings->setWhatsThis( tr( "Settings for 3D globe" ) );
  mActionUnload = new QAction( tr( "Unload Globe" ), this );
  mActionUnload->setWhatsThis( tr( "Unload globe" ) );

  // Connect actions
  connect( mActionLaunch, SIGNAL( triggered() ), this, SLOT( run() ) );
  connect( mActionSettings, SIGNAL( triggered() ), this, SLOT( showSettings() ) );
  connect( mActionUnload, SIGNAL( triggered() ), this, SLOT( reset() ) );

  // Add the icon to the toolbar
  mQGisIface->addToolBarIcon( mActionLaunch );

  //Add menu
  mQGisIface->addPluginToMenu( tr( "&Globe" ), mActionLaunch );
  mQGisIface->addPluginToMenu( tr( "&Globe" ), mActionSettings );
  mQGisIface->addPluginToMenu( tr( "&Globe" ), mActionUnload );

  connect( mQGisIface->mapCanvas(), SIGNAL( extentsChanged() ),
           this, SLOT( extentsChanged() ) );
  connect( mQGisIface->mapCanvas(), SIGNAL( layersChanged() ),
           this, SLOT( imageLayersChanged() ) );
  // Add layer properties pages
  mLayerPropertiesFactory = new QgsGlobeLayerPropertiesFactory();
  mQGisIface->registerMapLayerPropertiesFactory( mLayerPropertiesFactory );

  connect( mLayerPropertiesFactory, SIGNAL( layerSettingsChanged( QgsMapLayer* ) ), this, SLOT( layerSettingsChanged( QgsMapLayer* ) ) );

  QgsMapLayerRegistry* layerRegistry = QgsMapLayerRegistry::instance();

  connect( layerRegistry , SIGNAL( layersAdded( QList<QgsMapLayer*> ) ),
           this, SLOT( layersAdded( QList<QgsMapLayer*> ) ) );
  connect( layerRegistry , SIGNAL( layersRemoved( QStringList ) ),
           this, SLOT( layersRemoved( QStringList ) ) );
  connect( mSettingsDialog, SIGNAL( elevationDatasourcesChanged() ),
           this, SLOT( elevationLayersChanged() ) );
  connect( mQGisIface->mainWindow(), SIGNAL( projectRead() ), this,
           SLOT( projectReady() ) );
  connect( mQGisIface, SIGNAL( newProjectCreated() ), this,
           SLOT( blankProjectReady() ) );
  connect( this, SIGNAL( xyCoordinates( const QgsPoint & ) ),
           mQGisIface->mapCanvas(), SIGNAL( xyCoordinates( const QgsPoint & ) ) );
}

void GlobePlugin::run()
{
  if ( mViewerWidget == 0 )
  {
    QSettings settings;

#if 0
    if ( !getenv( "OSGNOTIFYLEVEL" ) ) osgEarth::setNotifyLevel( osg::DEBUG_INFO );
#endif

    mOsgViewer = new osgViewer::Viewer();

    // Set camera manipulator with default home position
    osgEarth::Util::EarthManipulator* manip = new osgEarth::Util::EarthManipulator();
    mOsgViewer->setCameraManipulator( manip );
    manip->setHomeViewpoint( osgEarth::Util::Viewpoint( osg::Vec3d( -90, 0, 0 ), 0., -90., 2e7 ), 1. );
    manip->home( 0 );

    mIsGlobeRunning = true;
    setupProxy();

    if ( getenv( "GLOBE_MAPXML" ) )
    {
      char* mapxml = getenv( "GLOBE_MAPXML" );
      QgsDebugMsg( mapxml );
      osg::Node* node = osgDB::readNodeFile( mapxml );
      if ( !node )
      {
        QgsDebugMsg( "Failed to load earth file " );
        return;
      }
      mMapNode = MapNode::findMapNode( node );
      mRootNode = new osg::Group();
      mRootNode->addChild( node );
    }
    else
    {
      setupMap();
    }

    // Initialize the sky node if required
    setSkyParameters( settings.value( "/Plugin-Globe/skyEnabled", false ).toBool(),
                      settings.value( "/Plugin-Globe/skyDateTime", QDateTime() ).toDateTime(),
                      settings.value( "/Plugin-Globe/skyAutoAmbient", false ).toBool() );

    // create a surface to house the controls
    mControlCanvas = osgEarth::Util::Controls::ControlCanvas::get( mOsgViewer );
    mRootNode->addChild( mControlCanvas );

    mOsgViewer->setSceneData( mRootNode );
    mOsgViewer->setThreadingModel( osgViewer::Viewer::SingleThreaded );

    mOsgViewer->addEventHandler( new FlyToExtentHandler( this ) );
    mOsgViewer->addEventHandler( new KeyboardControlHandler( manip ) );
    mOsgViewer->addEventHandler( new osgViewer::StatsHandler() );
    mOsgViewer->addEventHandler( new osgViewer::WindowSizeHandler() );
    mOsgViewer->addEventHandler( new osgViewer::ThreadingHandler() );
    mOsgViewer->addEventHandler( new osgViewer::LODScaleHandler() );
    mOsgViewer->addEventHandler( new osgGA::StateSetManipulator( mOsgViewer->getCamera()->getOrCreateStateSet() ) );
#if OSGEARTH_VERSION_LESS_THAN( 2, 2, 0 )
    // add a handler that will automatically calculate good clipping planes
    mOsgViewer->addEventHandler( new osgEarth::Util::AutoClipPlaneHandler() );
#else
    mOsgViewer->getCamera()->addCullCallback( new osgEarth::Util::AutoClipPlaneCullCallback( mMapNode ) );
#endif

    // osgEarth benefits from pre-compilation of GL objects in the pager. In newer versions of
    // OSG, this activates OSG's IncrementalCompileOpeartion in order to avoid frame breaks.
    mOsgViewer->getDatabasePager()->setDoPreCompile( true );

    mViewerWidget = new osgEarth::QtGui::ViewerWidget( mOsgViewer );
    if ( settings.value( "/Plugin-Globe/anti-aliasing", true ).toBool() &&
         settings.value( "/Plugin-Globe/anti-aliasing-level", "" ).toInt() > 0 )
    {
      QGLFormat glf = QGLFormat::defaultFormat();
      glf.setSampleBuffers( true );
      glf.setSamples( settings.value( "/Plugin-Globe/anti-aliasing-level", "" ).toInt() );
      mViewerWidget->setFormat( glf );
    }

    setupControls();

    mFeatureQueryTool = new osgEarth::Util::FeatureQueryTool( mMapNode );
    mFeatureQueryTool->addCallback( new GlobeFeatureIdentifyCallback( mQGisIface->mapCanvas() ) );
    mFeatureQueryTool->addCallback( new osgEarth::Util::FeatureHighlightCallback() );

    mDockWidget = new QDockWidget( tr( "Globe" ), mQGisIface->mainWindow() );
    mDockWidget->setWidget( mViewerWidget );
    mQGisIface->addDockWidget( Qt::BottomDockWidgetArea, mDockWidget );
  }
  else
  {
    mDockWidget->show();
  }
}

void GlobePlugin::setupMap()
{
  QSettings settings;
  QString cacheDirectory = settings.value( "cache/directory", QgsApplication::qgisSettingsDirPath() + "cache" ).toString();

#if OSGEARTH_VERSION_GREATER_OR_EQUAL( 2, 2, 0 )
  osgEarth::Drivers::FileSystemCacheOptions cacheOptions;
  cacheOptions.rootPath() = cacheDirectory.toStdString();
#else
  osgEarth::Drivers::TMSCacheOptions cacheOptions;
  cacheOptions.setPath( cacheDirectory.toStdString() );
#endif

  MapOptions mapOptions;
  mapOptions.cache() = cacheOptions;
  osgEarth::Map *map = new osgEarth::Map( mapOptions );

  //Default image layer
  /*
  GDALOptions driverOptions;
  driverOptions.url() = QDir::cleanPath( QgsApplication::pkgDataPath() + "/globe/world.tif" ).toStdString();
  ImageLayerOptions layerOptions( "world", driverOptions );
  map->addImageLayer( new osgEarth::ImageLayer( layerOptions ) );
  */

  MapNodeOptions nodeOptions;
  //nodeOptions.proxySettings() =
  //nodeOptions.enableLighting() = false;

  //LoadingPolicy loadingPolicy( LoadingPolicy::MODE_SEQUENTIAL );
  TerrainOptions terrainOptions;
  //terrainOptions.loadingPolicy() = loadingPolicy;
#warning "FIXME?"
#if OSGEARTH_VERSION_LESS_THAN(2, 6, 0)
  terrainOptions.compositingTechnique() = TerrainOptions::COMPOSITING_MULTITEXTURE_FFP;
#endif
  //terrainOptions.lodFallOff() = 6.0;
  nodeOptions.setTerrainOptions( terrainOptions );

  // The MapNode will render the Map object in the scene graph.
  mMapNode = new osgEarth::MapNode( map, nodeOptions );

  if ( settings.value( "/Plugin-Globe/baseLayerEnabled", true ).toBool() )
  {
    setBaseMap( settings.value( "/Plugin-Globe/baseLayerURL", "http://readymap.org/readymap/tiles/1.0.0/7/" ).toString() );
  }

  mRootNode = new osg::Group();
  mRootNode->addChild( mMapNode );

  //add QGIS layer
  mTileSource = new osgEarth::Drivers::QgsOsgEarthTileSource( QStringList(), mQGisIface->mapCanvas() );
  mTileSource->initialize( "", 0 );

  // Add layers to the map
  layersAdded( mQGisIface->mapCanvas()->layers() );
  elevationLayersChanged();

  // Create the frustum highlight callback
  mFrustumHighlightCallback = new GlobeFrustumHighlightCallback(
    mOsgViewer, mMapNode->getTerrain(), mQGisIface->mapCanvas(), QColor( 0, 0, 0, 50 ) );
}

void GlobePlugin::showSettings()
{
  mSettingsDialog->exec();
}

void GlobePlugin::projectReady()
{
  blankProjectReady();
  mSettingsDialog->readElevationDatasources();
}

void GlobePlugin::blankProjectReady()
{ //needs at least http://trac.osgeo.org/qgis/changeset/14452
  mSettingsDialog->resetElevationDatasources();
}

void GlobePlugin::showCurrentCoordinates( double lon, double lat )
{
  emit xyCoordinates( QgsPoint( lon, lat ) );
}

void GlobePlugin::setSelectedCoordinates( const osg::Vec3d &coords )
{
  mSelectedLon = coords.x();
  mSelectedLat = coords.y();
  mSelectedElevation = coords.z();
  emit newCoordinatesSelected( QgsPoint( mSelectedLon, mSelectedLat ) );
}

osg::Vec3d GlobePlugin::getSelectedCoordinates()
{
  return osg::Vec3d( mSelectedLon, mSelectedLat, mSelectedElevation );
}

void GlobePlugin::syncExtent()
{
  const QgsMapSettings& mapSettings = mQGisIface->mapCanvas()->mapSettings();
  QgsRectangle extent = mQGisIface->mapCanvas()->extent();

  long epsgGlobe = 4326;
  QgsCoordinateReferenceSystem globeCrs;
  globeCrs.createFromOgcWmsCrs( QString( "EPSG:%1" ).arg( epsgGlobe ) );

  // transform extent to WGS84
  if ( mapSettings.destinationCrs().authid().compare( QString( "EPSG:%1" ).arg( epsgGlobe ), Qt::CaseInsensitive ) != 0 )
  {
    QgsCoordinateReferenceSystem srcCRS( mapSettings.destinationCrs() );
    extent = QgsCoordinateTransform( srcCRS, globeCrs ).transformBoundingBox( extent );
  }

  QgsDistanceArea dist;
  dist.setSourceCrs( globeCrs );
  dist.setEllipsoidalMode( true );
  dist.setEllipsoid( "WGS84" );

  QgsPoint ll = QgsPoint( extent.xMinimum(), extent.yMinimum() );
  QgsPoint ul = QgsPoint( extent.xMinimum(), extent.yMaximum() );
  double height = dist.measureLine( ll, ul );
//  double height = dist.computeDistanceBearing( ll, ul );

  double camViewAngle = 30;
  double camDistance = height / tan( camViewAngle * osg::PI / 180 ); //c = b*cotan(B(rad))
  osgEarth::Util::Viewpoint viewpoint( osg::Vec3d( extent.center().x(), extent.center().y(), 0.0 ), 0.0, -90.0, camDistance );

  OE_NOTICE << "map extent: " << height << " camera distance: " << camDistance << std::endl;

  osgEarth::Util::EarthManipulator* manip = dynamic_cast<osgEarth::Util::EarthManipulator*>( mOsgViewer->getCameraManipulator() );
  manip->setRotation( osg::Quat() );
  manip->setViewpoint( viewpoint, 4.0 );
}

void GlobePlugin::setVerticalScale( double value )
{
#if OSGEARTH_VERSION_GREATER_OR_EQUAL( 2, 5, 0 )
  if ( mMapNode )
  {
    if ( !mVerticalScale.get() || mVerticalScale->getScale() != value )
    {
      mMapNode->getTerrainEngine()->removeEffect( mVerticalScale );
      mVerticalScale = new osgEarth::Util::VerticalScale();
      mVerticalScale->setScale( value );
      mMapNode->getTerrainEngine()->addEffect( mVerticalScale );
    }
  }
#else
  Q_UNUSED( value );
#endif
}

void GlobePlugin::addControl( osgEarth::Util::Controls::Control* control, int x, int y, int w, int h, osgEarth::Util::Controls::ControlEventHandler* handler )
{
  control->setPosition( x, y );
  control->setHeight( h );
  control->setWidth( w );
  control->addEventHandler( handler );
  mControlCanvas->addControl( control );
}

void GlobePlugin::addImageControl( const std::string& imgPath, int x, int y, Util::Controls::ControlEventHandler *handler )
{
  osg::Image* image = osgDB::readImageFile( imgPath );
  osgEarth::Util::Controls::ImageControl* control = new NavigationControl( image );
  control->setPosition( x, y );
  if ( handler )
    control->addEventHandler( handler );
  mControlCanvas->addControl( control );
}

void GlobePlugin::setupControls()
{
  std::string imgDir = QDir::cleanPath( QgsApplication::pkgDataPath() + "/globe/gui" ).toStdString();
  if ( QgsApplication::isRunningFromBuildDir() )
  {
    imgDir = QDir::cleanPath( QgsApplication::buildSourcePath() + "/src/plugins/globe/images/gui" ).toStdString();
  }
  osgEarth::Util::EarthManipulator* manip = dynamic_cast<osgEarth::Util::EarthManipulator*>( mOsgViewer->getCameraManipulator() );

  // Set scroll sensitivity
  osgEarth::Util::EarthManipulator::Settings* settings = manip->getSettings();
  QgsDebugMsg( QString( "Scroll sensitivity %1" ).arg( mSettingsDialog->scrollSensitivity() ) );
  settings->setScrollSensitivity( mSettingsDialog->scrollSensitivity() );
  if ( !mSettingsDialog->invertScrollWheel() )
  {
    settings->bindScroll( osgEarth::Util::EarthManipulator::ACTION_ZOOM_IN, osgGA::GUIEventAdapter::SCROLL_UP );
    settings->bindScroll( osgEarth::Util::EarthManipulator::ACTION_ZOOM_OUT, osgGA::GUIEventAdapter::SCROLL_DOWN );
  }
  manip->applySettings( settings );

  // Rotate and tiltcontrols
  int imgLeft = 16;
  int imgTop = 20;
  addImageControl( imgDir + "/YawPitchWheel.png", 16, 20 );
  addControl( new NavigationControl, imgLeft, imgTop + 18, 20, 22, new RotateControlHandler( manip, MOVE_OFFSET, 0 ) );
  addControl( new NavigationControl, imgLeft + 36, imgTop + 18, 20, 22, new RotateControlHandler( manip, -MOVE_OFFSET, 0 ) );
  addControl( new NavigationControl, imgLeft + 20, imgTop + 18, 16, 22, new RotateControlHandler( manip, 0, 0 ) );
  addControl( new NavigationControl, imgLeft + 20, imgTop, 24, 19, new RotateControlHandler( manip, 0, MOVE_OFFSET ) );
  addControl( new NavigationControl, imgLeft + 16, imgTop + 36, 24, 19, new RotateControlHandler( manip, 0, -MOVE_OFFSET ) );

  // Move controls
  imgTop = 80;
  addImageControl( imgDir + "/MoveWheel.png", imgLeft, imgTop );
  addControl( new NavigationControl, imgLeft, imgTop + 18, 20, 22, new PanControlHandler( manip, -MOVE_OFFSET, 0 ) );
  addControl( new NavigationControl, imgLeft + 36, imgTop + 18, 20, 22, new PanControlHandler( manip, MOVE_OFFSET, 0 ) );
  addControl( new NavigationControl, imgLeft + 20, imgTop, 24, 19, new PanControlHandler( manip, 0, MOVE_OFFSET ) );
  addControl( new NavigationControl, imgLeft + 16, imgTop + 36, 24, 19, new PanControlHandler( manip, 0, -MOVE_OFFSET ) );
  addControl( new NavigationControl, imgLeft + 20, imgTop + 18, 16, 22, new HomeControlHandler( manip ) );

  // Zoom controls
  imgLeft = 28;
  imgTop = imgTop + 62;
  addImageControl( imgDir + "/button-background.png", imgLeft, imgTop );
  addImageControl( imgDir + "/zoom-in.png", imgLeft + 3, imgTop + 3, new ZoomControlHandler( manip, 0, -MOVE_OFFSET ) );
  addImageControl( imgDir + "/zoom-out.png", imgLeft + 3, imgTop + 28, new ZoomControlHandler( manip, 0, MOVE_OFFSET ) );

  // Refresh and sync controls
  imgTop = imgTop + 60;
  addImageControl( imgDir + "/button-background.png", imgLeft, imgTop );
  addImageControl( imgDir + "/refresh-view.png", imgLeft + 3, imgTop + 3, new RefreshControlHandler( this ) );
  addImageControl( imgDir + "/sync-extent.png", imgLeft + 3, imgTop + 28, new SyncExtentControlHandler( this ) );
}

void GlobePlugin::setupProxy()
{
  QSettings settings;
  settings.beginGroup( "proxy" );
  if ( settings.value( "/proxyEnabled" ).toBool() )
  {
    osgEarth::ProxySettings proxySettings( settings.value( "/proxyHost" ).toString().toStdString(),
                                           settings.value( "/proxyPort" ).toInt() );
    if ( !settings.value( "/proxyUser" ).toString().isEmpty() )
    {
      QString auth = settings.value( "/proxyUser" ).toString() + ":" + settings.value( "/proxyPassword" ).toString();
      qputenv( "OSGEARTH_CURL_PROXYAUTH", auth.toLocal8Bit() );
    }
    //TODO: settings.value("/proxyType")
    //TODO: URL exlusions
    osgEarth::HTTPClient::setProxySettings( proxySettings );
  }
  settings.endGroup();
}

void GlobePlugin::extentsChanged()
{
  QgsDebugMsg( "extentsChanged: " + mQGisIface->mapCanvas()->extent().toString() );
}

void GlobePlugin::canvasLayersChanged()
{
#if 0
  if ( mIsGlobeRunning )
  {
    QgsDebugMsg( "imageLayersChanged: Globe Running, executing" );
    osg::ref_ptr<Map> map = mMapNode->getMap();

    if ( map->getNumImageLayers() > 1 )
    {
      mOsgViewer->getDatabasePager()->clear();
    }

    //remove QGIS layer
    if ( mQgisMapLayer )
    {
      QgsDebugMsg( "removeMapLayer" );
      map->removeImageLayer( mQgisMapLayer );
    }

    //add QGIS layer
    mTileSource = new QgsOsgEarthTileSource( QStringList(), mQGisIface->mapCanvas() );
    mTileSource->initialize( "", 0 );
    ImageLayerOptions options( "QGIS" );
#if OSGEARTH_VERSION_GREATER_OR_EQUAL( 2, 2, 0 )
    options.cachePolicy() = CachePolicy::NO_CACHE;
#endif
    mQgisMapLayer = new ImageLayer( options, mTileSource );
    map->addImageLayer( mQgisMapLayer );
  }
  else
  {
    QgsDebugMsg( "layersChanged: Globe NOT running, skipping" );
    return;
  }
#endif
}

void GlobePlugin::layersAdded( const QList<QgsMapLayer *> &mapLayers )
{
  if ( mIsGlobeRunning )
  {
    QSet<QgsMapLayer*> tileLayers;

    Q_FOREACH( QgsMapLayer* mapLayer, mapLayers )
    {
      if ( false /*mapLayer->type() == QgsMapLayer::VectorLayer*/ )
      {
        // Add a new feature source for vector layers

        QgsVectorLayer* vLayer = dynamic_cast<QgsVectorLayer*>( mapLayer );
        Q_ASSERT( vLayer );

        QgsGlobeVectorLayerConfig* layerConfig = QgsGlobeVectorLayerConfig::configForLayer( vLayer );

        osgEarth::Drivers::QgsGlobeFeatureOptions  featureOpt;
        featureOpt.setLayer( vLayer );
        Style style;

        Q_FOREACH( QgsSymbolV2* sym, vLayer->rendererV2()->symbols() )
        {
          if ( sym->type() == QgsSymbolV2::Line )
          {

            LineSymbol* ls = style.getOrCreateSymbol<LineSymbol>();
            QColor color = sym->color();
            ls->stroke()->color() = osg::Vec4f( color.redF(), color.greenF(), color.blueF(), color.alphaF() );
            ls->stroke()->width() = 1.0f;
          }
          else if ( sym->type() == QgsSymbolV2::Fill )
          {
            // TODO access border color, etc.
            PolygonSymbol* poly = style.getOrCreateSymbol<PolygonSymbol>();
            QColor color = sym->color();
            poly->fill()->color() = osg::Vec4f( color.redF(), color.greenF(), color.blueF(), color.alphaF() );
            style.addSymbol( poly );
          }
        }

        AltitudeSymbol* altitudeSymbol = style.getOrCreateSymbol<AltitudeSymbol>();

        switch ( layerConfig->altitudeClamping() )
        {
          case QgsGlobeVectorLayerConfig::AltitudeClampingRelative:
            altitudeSymbol->clamping() = osgEarth::Symbology::AltitudeSymbol::CLAMP_RELATIVE_TO_TERRAIN;
            break;

          case QgsGlobeVectorLayerConfig::AltitudeClampingTerrain:
            altitudeSymbol->clamping() = osgEarth::Symbology::AltitudeSymbol::CLAMP_TO_TERRAIN;
            break;

          case QgsGlobeVectorLayerConfig::AltitudeClampingAbsolute:
            altitudeSymbol->clamping() = osgEarth::Symbology::AltitudeSymbol::CLAMP_ABSOLUTE;
            break;

          default:
            altitudeSymbol->clamping() = osgEarth::Symbology::AltitudeSymbol::CLAMP_NONE;
            break;
        }

        switch ( layerConfig->altitudeTechnique() )
        {
          case QgsGlobeVectorLayerConfig::AltitudeTechniqueMap:
            altitudeSymbol->technique() = osgEarth::Symbology::AltitudeSymbol::TECHNIQUE_MAP;
            break;

          case QgsGlobeVectorLayerConfig::AltitudeTechniqueDrape:
            altitudeSymbol->technique() = osgEarth::Symbology::AltitudeSymbol::TECHNIQUE_DRAPE;
            break;

          case QgsGlobeVectorLayerConfig::AltitudeTechniqueGpu:
            altitudeSymbol->technique() = osgEarth::Symbology::AltitudeSymbol::TECHNIQUE_GPU;
            break;

          default:
            altitudeSymbol->technique() = osgEarth::Symbology::AltitudeSymbol::TECHNIQUE_SCENE;
        }

        switch ( layerConfig->altitudeBinding() )
        {
          case QgsGlobeVectorLayerConfig::AltitudeBindingVertex:
            altitudeSymbol->binding() = osgEarth::Symbology::AltitudeSymbol::BINDING_VERTEX;
            break;

          default:
            altitudeSymbol->binding() = osgEarth::Symbology::AltitudeSymbol::BINDING_CENTROID;
            break;
        }

        style.addSymbol( altitudeSymbol );

        if ( layerConfig->extrusionEnabled() )
        {
          ExtrusionSymbol* extrusionSymbol = style.getOrCreateSymbol<ExtrusionSymbol>();

          if ( layerConfig->isExtrusionHeightNumeric() )
          {
            extrusionSymbol->height() = layerConfig->extrusionHeightNumeric();
          }
          else
          {
            extrusionSymbol->heightExpression() = layerConfig->extrusionHeight().toStdString();
          }

          extrusionSymbol->flatten() = layerConfig->extrusionFlatten();
          extrusionSymbol->wallGradientPercentage() = layerConfig->extrusionWallGradient();
          style.addSymbol( extrusionSymbol );
        }

        if ( layerConfig->labelingEnabled() )
        {
          TextSymbol* textSymbol = style.getOrCreateSymbol<TextSymbol>();
#if 0
          textSymbol->declutter() = layerConfig->labelingDeclutter();
#endif
          // load labeling settings from layer
          QgsPalLayerSettings lyr;
          lyr.readFromLayer( vLayer );
#if 0
          textSymbol->content() = QString( "[%1]" ).arg( lyr.fieldName ).toStdString();
#endif
          textSymbol->content() = std::string( "test" );
          textSymbol->font() = "Cantarell";
          textSymbol->size() = 20;
          /*
                    Stroke stroke;
                    stroke.color() = QgsGlobeStyleUtils::QColorToOsgColor( lyr.bufferColor );
                    textSymbol->halo() = stroke;
                    textSymbol->haloOffset() = lyr.bufferSize;
                    textSymbol->font() = lyr.textFont.family().toStdString();
                    textSymbol->size() = lyr.textFont.pointSize();
          */
          textSymbol->alignment() = TextSymbol::ALIGN_CENTER_TOP;

          style.addSymbol( textSymbol );
        }

        RenderSymbol* renderSymbol = style.getOrCreateSymbol<RenderSymbol>();
        renderSymbol->lighting() = layerConfig->lighting();

        style.addSymbol( renderSymbol );
#if 0
        FeatureDisplayLayout layout;

        layout.tileSizeFactor() = 45.0;
        layout.addLevel( FeatureLevel( 0.0f, 200000.0f ) );
#endif
        osgEarth::Drivers::FeatureGeomModelOptions geomOpt;
        geomOpt.featureOptions() = featureOpt;
        geomOpt.styles() = new StyleSheet();
        geomOpt.styles()->addStyle( style );

        geomOpt.featureIndexing() = osgEarth::Features::FeatureSourceIndexOptions();

#if 0
        geomOpt.layout() = layout;
#endif

        ModelLayerOptions modelOptions( vLayer->id().toStdString(), geomOpt );

        ModelLayer* nLayer = new ModelLayer( modelOptions );

        osg::ref_ptr<Map> map = mMapNode->getMap();

        map->addModelLayer( nLayer );
#if 0
        osgEarth::Features::FeatureModelSource *ms = dynamic_cast<osgEarth::Features::FeatureModelSource*>(
              map->getModelLayerByName( nLayer->getName() )->getModelSource() );
        QgsDebugMsg( QString( "Model source: %1" ).arg( ms->getFeatureSource()-> ) );
#endif
      }
      else
      {
        // Add to the draped image for non-vector layers
        tileLayers << mapLayer;
      }
    }

    if ( tileLayers.size() > 0 )
    {
      mTileSource->addLayers( tileLayers );
      osg::ref_ptr<Map> map = mMapNode->getMap();
      if ( map->getNumImageLayers() > 1 )
      {
        mOsgViewer->getDatabasePager()->clear();
      }

      if ( mQgisMapLayer )
      {
        map->removeImageLayer( mQgisMapLayer );
      }

      osgEarth::ImageLayerOptions options( "QGIS" );
      mQgisMapLayer = new osgEarth::ImageLayer( options, mTileSource );
      map->addImageLayer( mQgisMapLayer );
    }
  }
}

void GlobePlugin::layersRemoved( const QStringList &layerIds )
{
  QgsDebugMsg( QString( "layers removed [%1]" ).arg( layerIds.join( ", " ) ) );
  if ( mIsGlobeRunning )
  {
    foreach ( const QString &layer, layerIds )
    {
      ModelLayer* modelLayer = mMapNode->getMap()->getModelLayerByName( layer.toStdString() );
      mMapNode->getMap()->removeModelLayer( modelLayer );
    }
  }
}

void GlobePlugin::elevationLayersChanged()
{
  if ( mIsGlobeRunning )
  {
    QgsDebugMsg( "elevationLayersChanged: Globe Running, executing" );
    osg::ref_ptr<Map> map = mMapNode->getMap();

    if ( map->getNumElevationLayers() > 1 )
    {
      mOsgViewer->getDatabasePager()->clear();
    }

    // Remove elevation layers
    osgEarth::ElevationLayerVector list;
    map->getElevationLayers( list );
    for ( ElevationLayerVector::iterator i = list.begin(); i != list.end(); ++i )
    {
      map->removeElevationLayer( *i );
    }

    // Add elevation layers
    QTableWidget* table = mSettingsDialog->elevationDatasources();
    for ( int i = 0; i < table->rowCount(); ++i )
    {
      QString type = table->item( i, 0 )->text();
      QString uri = table->item( i, 2 )->text();
      ElevationLayer* layer = 0;

      if ( "Raster" == type )
      {
        osgEarth::Drivers::GDALOptions options;
        options.url() = uri.toStdString();
        layer = new osgEarth::ElevationLayer( uri.toStdString(), options );
      }
      else if ( "TMS" == type )
      {
        osgEarth::Drivers::TMSOptions options;
        options.url() = uri.toStdString();
        layer = new osgEarth::ElevationLayer( uri.toStdString(), options );
      }
      map->addElevationLayer( layer );
    }
#if OSGEARTH_VERSION_GREATER_OR_EQUAL( 2, 5, 0 )
    double scale = QgsProject::instance()->readDoubleEntry( "Globe-Plugin", "/verticalScale", 1 );
    setVerticalScale( scale );
#endif
  }
}

void GlobePlugin::setBaseMap( QString url )
{
  if ( mMapNode )
  {
    mMapNode->getMap()->removeImageLayer( mBaseLayer );
    if ( url.isNull() )
    {
      mBaseLayer = 0;
    }
    else
    {
      osgEarth::Drivers::TMSOptions imagery;
      imagery.url() = url.toStdString();
      mBaseLayer = new ImageLayer( "Imagery", imagery );
      mMapNode->getMap()->insertImageLayer( mBaseLayer, 0 );
    }
  }
}

void GlobePlugin::setSkyParameters( bool enabled, const QDateTime& dateTime, bool autoAmbience )
{
  if ( mRootNode )
  {
    if ( enabled )
    {
      // Create if not yet done
      if ( !mSkyNode.get() )
        mSkyNode = osgEarth::Util::SkyNode::create( mMapNode );

#warning "FIXME?"
#if OSGEARTH_VERSION_GREATER_OR_EQUAL( 2, 4, 0 ) and OSGEARTH_VERSION_LESS_THAN(2, 6, 0)
      mSkyNode->setAutoAmbience( autoAmbience );
#else
      Q_UNUSED( autoAmbience );
#endif
#if OSGEARTH_VERSION_GREATER_OR_EQUAL(2, 6, 0)
      mSkyNode->setDateTime( osgEarth::DateTime(
                               dateTime.date().year(),
                               dateTime.date().month(),
                               dateTime.date().day(),
                               dateTime.time().hour() + dateTime.time().minute() / 60.0 ) );
#else
      mSkyNode->setDateTime( dateTime.date().year(),
                             dateTime.date().month(),
                             dateTime.date().day(),
                             dateTime.time().hour() + dateTime.time().minute() / 60.0 );
#endif
      //sky->setSunPosition( osg::Vec3(0,-1,0) );
      mSkyNode->attach( mOsgViewer );
      mRootNode->addChild( mSkyNode );
    }
    else
    {
      mRootNode->removeChild( mSkyNode );
    }
  }
}

void GlobePlugin::reset()
{
  delete mFeatureQueryTool;
  mFeatureQueryTool = 0;
  delete mViewerWidget;
  mViewerWidget = 0;
  delete mDockWidget;
  mDockWidget = 0;
  delete mOsgViewer;
  mOsgViewer = 0;
  mQgisMapLayer = 0;
  mIsGlobeRunning = false;
}

void GlobePlugin::unload()
{
  reset();
  mQGisIface->removePluginMenu( tr( "&Globe" ), mActionLaunch );
  mQGisIface->removePluginMenu( tr( "&Globe" ), mActionSettings );
  mQGisIface->removePluginMenu( tr( "&Globe" ), mActionUnload );
  mQGisIface->removeToolBarIcon( mActionLaunch );
  mQGisIface->unregisterMapLayerPropertiesFactory( mLayerPropertiesFactory );
  delete mLayerPropertiesFactory;
  mLayerPropertiesFactory = 0;
  delete mSettingsDialog;
  mSettingsDialog = 0;

  QgsMapLayerRegistry* layerRegistry = QgsMapLayerRegistry::instance();
  disconnect( layerRegistry , SIGNAL( layersAdded( QList<QgsMapLayer*> ) ),
              this, SLOT( refreshQGISMapLayer() ) );
  disconnect( layerRegistry , SIGNAL( layersRemoved( QStringList ) ),
              this, SLOT( refreshQGISMapLayer( ) ) );
  disconnect( mQGisIface->mapCanvas(), SIGNAL( extentsChanged() ),
              this, SLOT( extentsChanged() ) );
  disconnect( mQGisIface->mainWindow(), SIGNAL( projectRead() ), this,
              SLOT( projectReady() ) );
  disconnect( mQGisIface, SIGNAL( newProjectCreated() ), this,
              SLOT( blankProjectReady() ) );
  disconnect( this, SIGNAL( xyCoordinates( const QgsPoint & ) ),
              mQGisIface->mapCanvas(), SIGNAL( xyCoordinates( const QgsPoint & ) ) );
}

void GlobePlugin::enableFrustumHighlight( bool status )
{
  if ( status )
    mMapNode->getTerrainEngine()->addUpdateCallback( mFrustumHighlightCallback );
  else
    mMapNode->getTerrainEngine()->removeUpdateCallback( mFrustumHighlightCallback );
}

void GlobePlugin::enableFeatureIdentification( bool status )
{
  if ( status )
    mOsgViewer->addEventHandler( mFeatureQueryTool );
  else
    mOsgViewer->removeEventHandler( mFeatureQueryTool );
}

void GlobePlugin::layerSettingsChanged( QgsMapLayer* layer )
{
  layersRemoved( QStringList() << layer->id() );
  layersAdded( QList<QgsMapLayer*>() << layer );
}

void GlobePlugin::onLayerRead( QgsMapLayer* mapLayer, const QDomElement& elem )
{
  if ( mapLayer->type() == QgsMapLayer::VectorLayer )
  {
    QgsVectorLayer* vLayer = static_cast<QgsVectorLayer*>( mapLayer );
    QgsGlobeVectorLayerConfig::readLayer( vLayer, elem );
  }
}

void GlobePlugin::onLayerWrite( QgsMapLayer* mapLayer, QDomElement& elem, QDomDocument& doc )
{
  if ( mapLayer->type() == QgsMapLayer::VectorLayer )
  {
    QgsVectorLayer* vLayer = static_cast<QgsVectorLayer*>( mapLayer );
    QgsGlobeVectorLayerConfig::writeLayer( vLayer, elem, doc );
  }
}

bool NavigationControl::handle( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa, osgEarth::Util::Controls::ControlContext& cx )
{
  if ( ea.getEventType() == osgGA::GUIEventAdapter::PUSH )
  {
    mMousePressed = true;
  }
  else if ( ea.getEventType() == osgGA::GUIEventAdapter::FRAME && mMousePressed )
  {
    for ( osgEarth::Util::Controls::ControlEventHandlerList::const_iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i )
    {
      NavigationControlHandler* handler = dynamic_cast<NavigationControlHandler*>( i->get() );
      if ( handler )
      {
        handler->onMouseDown();
      }
    }
  }
  else if ( ea.getEventType() == osgGA::GUIEventAdapter::RELEASE )
  {
    for ( osgEarth::Util::Controls::ControlEventHandlerList::const_iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i )
    {
      NavigationControlHandler* handler = dynamic_cast<NavigationControlHandler*>( i->get() );
      if ( handler )
      {
        handler->onClick( ea, aa );
      }
    }
    mMousePressed = false;
  }
  return Control::handle( ea, aa, cx );
}

bool KeyboardControlHandler::handle( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa )
{
  if ( ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN )
  {
    //move map
    if ( ea.getKey() == '4' )
      _manip->pan( -MOVE_OFFSET, 0 );
    else if ( ea.getKey() == '6' )
      _manip->pan( MOVE_OFFSET, 0 );
    else if ( ea.getKey() == '2' )
      _manip->pan( 0, MOVE_OFFSET );
    else if ( ea.getKey() == '8' )
      _manip->pan( 0, -MOVE_OFFSET );
    //rotate
    else if ( ea.getKey() == '/' )
      _manip->rotate( MOVE_OFFSET, 0 );
    else if ( ea.getKey() == '*' )
      _manip->rotate( -MOVE_OFFSET, 0 );
    //tilt
    else if ( ea.getKey() == '9' )
      _manip->rotate( 0, MOVE_OFFSET );
    else if ( ea.getKey() == '3' )
      _manip->rotate( 0, -MOVE_OFFSET );
    //zoom
    else if ( ea.getKey() == '-' )
      _manip->zoom( 0, MOVE_OFFSET );
    else if ( ea.getKey() == '+' )
      _manip->zoom( 0, -MOVE_OFFSET );
    //reset
    else if ( ea.getKey() == '5' )
      _manip->home( ea, aa );
  }
  return false;
}

/**
 * Required extern functions needed  for every plugin
 * These functions can be called prior to creating an instance
 * of the plugin class
 */
// Class factory to return a new instance of the plugin class
QGISEXTERN QgisPlugin * classFactory( QgisInterface * theQgisInterfacePointer )
{
  return new GlobePlugin( theQgisInterfacePointer );
}
// Return the name of the plugin - note that we do not user class members as
// the class may not yet be insantiated when this method is called.
QGISEXTERN QString name()
{
  return sName;
}

// Return the description
QGISEXTERN QString description()
{
  return sDescription;
}

// Return the category
QGISEXTERN QString category()
{
  return sCategory;
}

// Return the type (either UI or MapLayer plugin)
QGISEXTERN int type()
{
  return sPluginType;
}

// Return the version number for the plugin
QGISEXTERN QString version()
{
  return sPluginVersion;
}

// Return the icon
QGISEXTERN QString icon()
{
  return sIcon;
}

// Return the experimental status for the plugin
QGISEXTERN QString experimental()
{
  return sExperimental;
}

// Delete ourself
QGISEXTERN void unload( QgisPlugin * thePluginPointer )
{
  delete thePluginPointer;
}

QgsPluginInterface* GlobePlugin::pluginInterface()
{
  return &mGlobeInterface;
}
