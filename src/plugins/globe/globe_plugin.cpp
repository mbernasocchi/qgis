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
#include "qgsglobestyleutils.h"
#include "globefeatureidentify.h"

#ifdef HAVE_OSGEARTHQT
#include <osgEarthQt/ViewerWidget>
#else
#include "osgEarthQt/ViewerWidget"
#endif

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

#include <QAction>
#include <QDir>
#include <QToolBar>
#include <QMessageBox>

#include <osgGA/TrackballManipulator>
#include <osgDB/ReadFile>
#include <osgDB/Registry>

#include <osgGA/StateSetManipulator>
#include <osgGA/GUIEventHandler>

#include <osg/MatrixTransform>
#include <osg/ShapeDrawable>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgEarth/Notify>
#include <osgEarth/Map>
#include <osgEarth/MapNode>
#include <osgEarth/TileSource>
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

#include "qgsglobelayerpropertiesfactory.h"
#include "qgsglobevectorlayerconfig.h"
#include "qgspallabeling.h"


using namespace osgEarth::Drivers;
using namespace osgEarth::Util;

#define MOVE_OFFSET 0.05

static const QString sName = QObject::tr( "Globe" );
static const QString sDescription = QObject::tr( "Overlay data on a 3D globe" );
static const QString sCategory = QObject::tr( "Plugins" );
static const QString sPluginVersion = QObject::tr( "Version 0.1" );
static const QgisPlugin::PLUGINTYPE sPluginType = QgisPlugin::UI;
static const QString sIcon = ":/globe/icon.svg";
static const QString sExperimental = QString( "true" );

#if 0
#include <qgsmessagelog.h>

class QgsMsgTrap : public std::streambuf
{
  public:
    inline virtual int_type overflow( int_type c = std::streambuf::traits_type::eof() )
    {
      if ( c == std::streambuf::traits_type::eof() )
        return std::streambuf::traits_type::not_eof( c );

      switch ( c )
      {
        case '\r':
          break;
        case '\n':
          QgsMessageLog::logMessage( buf, QObject::tr( "Globe" ) );
          buf.clear();
          break;
        default:
          buf += c;
          break;
      }
      return c;
    }

  private:
    QString buf;
} msgTrap;
#endif


//constructor
GlobePlugin::GlobePlugin( QgisInterface* theQgisInterface )
    : QgisPlugin( sName, sDescription, sCategory, sPluginVersion, sPluginType )
    , mQGisIface( theQgisInterface )
    , mQActionPointer( 0 )
    , mQActionSettingsPointer( 0 )
    , mQActionUnload( 0 )
    , mOsgViewer( 0 )
    , mViewerWidget( 0 )
    , mRootNode( 0 )
    , mMapNode( 0 )
    , mBaseLayer( 0 )
    , mQgisMapLayer( 0 )
    , mTileSource( 0 )
    , mControlCanvas( 0 )
    , mElevationManager( 0 )
    , mObjectPlacer( 0 )
    , mSelectedLat( 0. )
    , mSelectedLon( 0. )
    , mSelectedElevation( 0. )
    , mDockWidget( 0 )
    , mGlobeInterface( this )
{
  mIsGlobeRunning = false;
  //needed to be "seen" by other plugins by doing
  //iface.mainWindow().findChild( QObject, "globePlugin" )
  //needed until https://trac.osgeo.org/qgis/changeset/15224
  setObjectName( "globePlugin" );
  setParent( theQgisInterface->mainWindow() );

// update path to osg plugins on Mac OS X
#ifdef Q_OS_MACX
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

  mSettingsDialog = new QgsGlobePluginDialog( this, theQgisInterface->mainWindow(), QgisGui::ModalDialogFlags );

  connect( QgsProject::instance(), SIGNAL( readMapLayer( QgsMapLayer*, QDomElement ) ), this, SLOT( onLayerRead( QgsMapLayer*, QDomElement ) ) );
  connect( QgsProject::instance(), SIGNAL( writeMapLayer( QgsMapLayer*, QDomElement&, QDomDocument& ) ), this, SLOT( onLayerWrite( QgsMapLayer*, QDomElement&, QDomDocument& ) ) );
}

//destructor
GlobePlugin::~GlobePlugin()
{
  delete( mSettingsDialog );
}

struct PanControlHandler : public NavigationControlHandler
{
  PanControlHandler( osgEarth::Util::EarthManipulator* manip, double dx, double dy ) : _manip( manip ), _dx( dx ), _dy( dy ) { }
  virtual void onMouseDown( Control* /*control*/, int /*mouseButtonMask*/ ) override
  {
    _manip->pan( _dx, _dy );
  }
private:
  osg::observer_ptr<osgEarth::Util::EarthManipulator> _manip;
  double _dx;
  double _dy;
};

struct RotateControlHandler : public NavigationControlHandler
{
  RotateControlHandler( osgEarth::Util::EarthManipulator* manip, double dx, double dy ) : _manip( manip ), _dx( dx ), _dy( dy ) { }
  virtual void onMouseDown( Control* /*control*/, int /*mouseButtonMask*/ ) override
  {
    if ( 0 == _dx && 0 == _dy )
    {
      _manip->setRotation( osg::Quat() );
    }
    else
    {
      _manip->rotate( _dx, _dy );
    }
  }
private:
  osg::observer_ptr<osgEarth::Util::EarthManipulator> _manip;
  double _dx;
  double _dy;
};

struct ZoomControlHandler : public NavigationControlHandler
{
  ZoomControlHandler( osgEarth::Util::EarthManipulator* manip, double dx, double dy ) : _manip( manip ), _dx( dx ), _dy( dy ) { }
  virtual void onMouseDown( Control* /*control*/, int /*mouseButtonMask*/ ) override
  {
    _manip->zoom( _dx, _dy );
  }
private:
  osg::observer_ptr<osgEarth::Util::EarthManipulator> _manip;
  double _dx;
  double _dy;
};

struct HomeControlHandler : public NavigationControlHandler
{
  HomeControlHandler( osgEarth::Util::EarthManipulator* manip ) : _manip( manip ) { }
  virtual void onClick( Control* /*control*/, int /*mouseButtonMask*/, const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa ) override
  {
    _manip->home( ea, aa );
  }
private:
  osg::observer_ptr<osgEarth::Util::EarthManipulator> _manip;
};

struct RefreshControlHandler : public ControlEventHandler
{
  RefreshControlHandler( GlobePlugin* globe ) : mGlobe( globe ) { }
  virtual void onClick( Control* /*control*/, int /*mouseButtonMask*/ ) override
  {
    mGlobe->canvasLayersChanged();
  }
private:
  GlobePlugin* mGlobe;
};

struct SyncExtentControlHandler : public ControlEventHandler
{
  SyncExtentControlHandler( GlobePlugin* globe ) : mGlobe( globe ) { }
  virtual void onClick( Control* /*control*/, int /*mouseButtonMask*/ ) override
  {
    mGlobe->syncExtent();
  }
private:
  GlobePlugin* mGlobe;
};

void GlobePlugin::initGui()
{
  delete mQActionPointer;
  delete mQActionSettingsPointer;
  delete mQActionUnload;

  // Create the action for tool
  mQActionPointer = new QAction( QIcon( ":/globe/globe.png" ), tr( "Launch Globe" ), this );
  mQActionPointer->setObjectName( "mQActionPointer" );
  mQActionSettingsPointer = new QAction( QIcon( ":/globe/globe.png" ), tr( "Globe Settings" ), this );
  mQActionSettingsPointer->setObjectName( "mQActionSettingsPointer" );
  mQActionUnload = new QAction( tr( "Unload Globe" ), this );
  mQActionUnload->setObjectName( "mQActionUnload" );

  // Set the what's this text
  mQActionPointer->setWhatsThis( tr( "Overlay data on a 3D globe" ) );
  mQActionSettingsPointer->setWhatsThis( tr( "Settings for 3D globe" ) );
  mQActionUnload->setWhatsThis( tr( "Unload globe" ) );

  // Connect actions
  connect( mQActionPointer, SIGNAL( triggered() ), this, SLOT( run() ) );
  connect( mQActionSettingsPointer, SIGNAL( triggered() ), this, SLOT( settings() ) );
  connect( mQActionUnload, SIGNAL( triggered() ), this, SLOT( reset() ) );

  // Add the icon to the toolbar
  mQGisIface->addToolBarIcon( mQActionPointer );

  //Add menu
  mQGisIface->addPluginToMenu( tr( "&Globe" ), mQActionPointer );
  mQGisIface->addPluginToMenu( tr( "&Globe" ), mQActionSettingsPointer );
  mQGisIface->addPluginToMenu( tr( "&Globe" ), mQActionUnload );

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

#if 0
  mCoutRdBuf = std::cout.rdbuf( &msgTrap );
  mCerrRdBuf = std::cerr.rdbuf( &msgTrap );
#endif
}

void GlobePlugin::run()
{
  if ( mViewerWidget == 0 )
  {
    QSettings settings;

#if 0
    if ( !getenv( "OSGNOTIFYLEVEL" ) ) osgEarth::setNotifyLevel( osg::DEBUG_INFO );
#endif
    osgEarth::setNotifyLevel( osg::DEBUG_INFO );

    mOsgViewer = new osgViewer::Viewer();

    // install the programmable manipulator.
    osgEarth::Util::EarthManipulator* manip = new osgEarth::Util::EarthManipulator();
    mOsgViewer->setCameraManipulator( manip );

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
    setSkyParameters( settings.value( "/Plugin-Globe/skyEnabled", false ).toBool()
                      , settings.value( "/Plugin-Globe/skyDateTime", QDateTime() ).toDateTime()
                      , settings.value( "/Plugin-Globe/skyAutoAmbient", false ).toBool() );

    // create a surface to house the controls
#if OSGEARTH_VERSION_GREATER_OR_EQUAL( 2, 1, 1 )
    mControlCanvas = ControlCanvas::get( mOsgViewer );
#else
    mControlCanvas = new ControlCanvas( mOsgViewer );
#endif

    mRootNode->addChild( mControlCanvas );

    mOsgViewer->setSceneData( mRootNode );
    mOsgViewer->setThreadingModel( osgViewer::Viewer::SingleThreaded );

    mOsgViewer->addEventHandler( new osgViewer::StatsHandler() );
    mOsgViewer->addEventHandler( new osgViewer::WindowSizeHandler() );
    mOsgViewer->addEventHandler( new osgViewer::ThreadingHandler() );
    mOsgViewer->addEventHandler( new osgViewer::LODScaleHandler() );
    mOsgViewer->addEventHandler( new osgGA::StateSetManipulator( mOsgViewer->getCamera()->getOrCreateStateSet() ) );
#if OSGEARTH_VERSION_LESS_THAN( 2, 2, 0 )
    // add a handler that will automatically calculate good clipping planes
    mOsgViewer->addEventHandler( new osgEarth::Util::AutoClipPlaneHandler() );
#else
    mOsgViewer->getCamera()->addCullCallback( new AutoClipPlaneCullCallback( mMapNode ) );
#endif

    // osgEarth benefits from pre-compilation of GL objects in the pager. In newer versions of
    // OSG, this activates OSG's IncrementalCompileOpeartion in order to avoid frame breaks.
    mOsgViewer->getDatabasePager()->setDoPreCompile( true );

#ifdef GLOBE_OSG_STANDALONE_VIEWER
    mOsgViewer->run();
#endif

    mDockWidget = new QDockWidget( tr( "Globe" ), mQGisIface->mainWindow() );
    mViewerWidget = new osgEarth::QtGui::ViewerWidget( mOsgViewer );

    if ( settings.value( "/Plugin-Globe/anti-aliasing", true ).toBool() )
    {
      QGLFormat glf = QGLFormat::defaultFormat();
      glf.setSampleBuffers( true );
      bool aaLevelIsInt;
      int aaLevel;
      QString aaLevelStr = settings.value( "/Plugin-Globe/anti-aliasing-level", "" ).toString();
      aaLevel = aaLevelStr.toInt( &aaLevelIsInt );
      if ( aaLevelIsInt )
      {
        glf.setSamples( aaLevel );
      }
      mViewerWidget->setFormat( glf );
    }

    mDockWidget->setWidget( mViewerWidget );
    mDockWidget->show();
    mQGisIface->addDockWidget( Qt::BottomDockWidgetArea, mDockWidget );

    if ( settings.value( "/Plugin-Globe/anti-aliasing", true ).toBool() )
    {
      QGLFormat glf = QGLFormat::defaultFormat();
      glf.setSampleBuffers( true );
      bool aaLevelIsInt;
      int aaLevel;
      QString aaLevelStr = settings.value( "/Plugin-Globe/anti-aliasing-level", "" ).toString();
      aaLevel = aaLevelStr.toInt( &aaLevelIsInt );
      if ( aaLevelIsInt )
      {
        glf.setSamples( aaLevel );
      }
      mViewerWidget->setFormat( glf );
    }

    setupControls();

    // Set a home viewpoint
    manip->setHomeViewpoint(
      osgEarth::Util::Viewpoint( osg::Vec3d( -90, 0, 0 ), 0.0, -90.0, 2e7 ),
      1.0 );

    manip->home( 0 );

    // add our handlers
    mOsgViewer->addEventHandler( new FlyToExtentHandler( this ) );
    mOsgViewer->addEventHandler( new KeyboardControlHandler( manip ) );

#ifndef HAVE_OSGEARTH_ELEVATION_QUERY
    mOsgViewer->addEventHandler( new QueryCoordinatesHandler( this, mElevationManager,
                                 mMapNode->getMap()->getProfile()->getSRS() )
                               );
#endif

    mFeatureQueryTool = new osgEarth::Util::FeatureQueryTool( mMapNode, new GlobeFeatureIdentifyCallback( mQGisIface->mapCanvas() ) );
    mFeatureQueryTool->addCallback( new FeatureHighlightCallback() );
  }
  else
  {
    mDockWidget->show();
  }
}

void GlobePlugin::settings()
{
  mSettingsDialog->updatePointLayers();
  if ( mSettingsDialog->exec() )
  {
    //viewer stereo settings set by mSettingsDialog and stored in QSettings
  }
}

void GlobePlugin::setupMap()
{
  QSettings settings;
  QString cacheDirectory = settings.value( "cache/directory", QgsApplication::qgisSettingsDirPath() + "cache" ).toString();

#if OSGEARTH_VERSION_GREATER_OR_EQUAL( 2, 2, 0 )
  FileSystemCacheOptions cacheOptions;
  cacheOptions.rootPath() = cacheDirectory.toStdString();
#else
  TMSCacheOptions cacheOptions;
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
  mTileSource = new QgsOsgEarthTileSource( QStringList(), mQGisIface->mapCanvas() );
  mTileSource->initialize( "", 0 );

  // Add layers to the map
  layersAdded( mQGisIface->mapCanvas()->layers() );
  elevationLayersChanged();

  // Create the frustum highlight callback
  QColor color( Qt::black );
  color.setAlpha( 50 );
  mFrustumHighlightCallback = new GlobeFrustumHighlightCallback(
    mOsgViewer
    , mMapNode->getTerrain()
    , mQGisIface->mapCanvas()
    , color );


  // model placement utils
#ifdef HAVE_OSGEARTH_ELEVATION_QUERY
#else
  mElevationManager = new osgEarth::Util::ElevationManager( mMapNode->getMap() );
  mElevationManager->setTechnique( osgEarth::Util::ElevationManager::TECHNIQUE_GEOMETRIC );
  mElevationManager->setMaxTilesToCache( 50 );

  mObjectPlacer = new osgEarth::Util::ObjectPlacer( mMapNode );

  // place 3D model on point layer
  if ( mSettingsDialog->modelLayer() && !mSettingsDialog->modelPath().isEmpty() )
  {
    osg::Node* model = osgDB::readNodeFile( mSettingsDialog->modelPath().toStdString() );
    if ( model )
    {
      QgsVectorLayer* layer = mSettingsDialog->modelLayer();
      QgsFeatureIterator fit = layer->getFeatures( QgsFeatureRequest().setSubsetOfAttributes( QgsAttributeList() ) ); //TODO: select only visible features
      QgsFeature feature;
      while ( fit.nextFeature( feature ) )
      {
        QgsPoint point = feature.geometry()->asPoint();
        placeNode( model, point.y(), point.x() );
      }
    }
  }
#endif

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
  // show x y on status bar
  //OE_NOTICE << "lon: " << lon << " lat: " << lat <<std::endl;
  QgsPoint coord = QgsPoint( lon, lat );
  emit xyCoordinates( coord );
}

void GlobePlugin::setSelectedCoordinates( osg::Vec3d coords )
{
  mSelectedLon = coords.x();
  mSelectedLat = coords.y();
  mSelectedElevation = coords.z();
  QgsPoint coord = QgsPoint( mSelectedLon, mSelectedLat );
  emit newCoordinatesSelected( coord );
}

osg::Vec3d GlobePlugin::getSelectedCoordinates()
{
  osg::Vec3d coords = osg::Vec3d( mSelectedLon, mSelectedLat, mSelectedElevation );
  return coords;
}

void GlobePlugin::showSelectedCoordinates()
{
  QString lon, lat, elevation;
  lon.setNum( mSelectedLon );
  lat.setNum( mSelectedLat );
  elevation.setNum( mSelectedElevation );
  QMessageBox m;
  m.setText( "selected coordinates are:\nlon: " + lon + "\nlat: " + lat + "\nelevation: " + elevation );
  m.exec();
}

double GlobePlugin::getSelectedLon()
{
  return mSelectedLon;
}

double GlobePlugin::getSelectedLat()
{
  return mSelectedLat;
}

double GlobePlugin::getSelectedElevation()
{
  return mSelectedElevation;
}

void GlobePlugin::syncExtent()
{
  QgsMapCanvas* mapCanvas = mQGisIface->mapCanvas();
  const QgsMapSettings &mapSettings = mapCanvas->mapSettings();
  QgsRectangle extent = mapCanvas->extent();

  long epsgGlobe = 4326;
  QgsCoordinateReferenceSystem globeCrs;
  globeCrs.createFromOgcWmsCrs( QString( "EPSG:%1" ).arg( epsgGlobe ) );

  // transform extent to WGS84
  if ( mapSettings.destinationCrs().authid().compare( QString( "EPSG:%1" ).arg( epsgGlobe ), Qt::CaseInsensitive ) != 0 )
  {
    QgsCoordinateReferenceSystem srcCRS( mapSettings.destinationCrs() );
    QgsCoordinateTransform* coordTransform = new QgsCoordinateTransform( srcCRS, globeCrs );
    extent = coordTransform->transformBoundingBox( extent );
    delete coordTransform;
  }

  osgEarth::Util::EarthManipulator* manip = dynamic_cast<osgEarth::Util::EarthManipulator*>( mOsgViewer->getCameraManipulator() );
  //rotate earth to north and perpendicular to camera
  manip->setRotation( osg::Quat() );

  QgsDistanceArea dist;

  dist.setSourceCrs( globeCrs );
  dist.setEllipsoidalMode( true );
  dist.setEllipsoid( "WGS84" );

  QgsPoint ll = QgsPoint( extent.xMinimum(), extent.yMinimum() );
  QgsPoint ul = QgsPoint( extent.xMinimum(), extent.yMaximum() );
  double height = dist.measureLine( ll, ul );

  //double height = dist.computeDistanceBearing( ll, ul );

  //camera viewing angle
  double viewAngle = 30;
  //camera distance
  double distance = height / tan( viewAngle * osg::PI / 180 ); //c = b*cotan(B(rad))

  OE_NOTICE << "map extent: " << height << " camera distance: " << distance << std::endl;

  osgEarth::Util::Viewpoint viewpoint( osg::Vec3d( extent.center().x(), extent.center().y(), 0.0 ), 0.0, -90.0, distance );
  manip->setViewpoint( viewpoint, 4.0 );
}

#if OSGEARTH_VERSION_GREATER_OR_EQUAL( 2, 5, 0 )
void GlobePlugin::setVerticalScale( double value )
{
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
}
#endif

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

  osg::Image* yawPitchWheelImg = osgDB::readImageFile( imgDir + "/YawPitchWheel.png" );
  ImageControl* yawPitchWheel = new ImageControl( yawPitchWheelImg );
  int imgLeft = 16;
  int imgTop = 20;
  yawPitchWheel->setPosition( imgLeft, imgTop );
  mControlCanvas->addControl( yawPitchWheel );

//ROTATE CONTROLS
  Control* rotateCCW = new NavigationControl();
  rotateCCW->setHeight( 22 );
  rotateCCW->setWidth( 20 );
  rotateCCW->setPosition( imgLeft + 0, imgTop + 18 );
  rotateCCW->addEventHandler( new RotateControlHandler( manip, MOVE_OFFSET, 0 ) );
  mControlCanvas->addControl( rotateCCW );

  Control* rotateCW = new NavigationControl();
  rotateCW->setHeight( 22 );
  rotateCW->setWidth( 20 );
  rotateCW->setPosition( imgLeft + 36, imgTop + 18 );
  rotateCW->addEventHandler( new RotateControlHandler( manip, -MOVE_OFFSET, 0 ) );
  mControlCanvas->addControl( rotateCW );

  //Rotate Reset
  Control* rotateReset = new NavigationControl();
  rotateReset->setHeight( 22 );
  rotateReset->setWidth( 16 );
  rotateReset->setPosition( imgLeft + 20, imgTop + 18 );
  rotateReset->addEventHandler( new RotateControlHandler( manip, 0, 0 ) );
  mControlCanvas->addControl( rotateReset );

//TILT CONTROLS
  Control* tiltUp = new NavigationControl();
  tiltUp->setHeight( 19 );
  tiltUp->setWidth( 24 );
  tiltUp->setPosition( imgLeft + 20, imgTop + 0 );
  tiltUp->addEventHandler( new RotateControlHandler( manip, 0, MOVE_OFFSET ) );
  mControlCanvas->addControl( tiltUp );

  Control* tiltDown = new NavigationControl();
  tiltDown->setHeight( 19 );
  tiltDown->setWidth( 24 );
  tiltDown->setPosition( imgLeft + 16, imgTop + 36 );
  tiltDown->addEventHandler( new RotateControlHandler( manip, 0, -MOVE_OFFSET ) );
  mControlCanvas->addControl( tiltDown );

  // -------

  osg::Image* moveWheelImg = osgDB::readImageFile( imgDir + "/MoveWheel.png" );
  ImageControl* moveWheel = new ImageControl( moveWheelImg );
  imgTop = 80;
  moveWheel->setPosition( imgLeft, imgTop );
  mControlCanvas->addControl( moveWheel );

  //MOVE CONTROLS
  Control* moveLeft = new NavigationControl();
  moveLeft->setHeight( 22 );
  moveLeft->setWidth( 20 );
  moveLeft->setPosition( imgLeft + 0, imgTop + 18 );
  moveLeft->addEventHandler( new PanControlHandler( manip, -MOVE_OFFSET, 0 ) );
  mControlCanvas->addControl( moveLeft );

  Control* moveRight = new NavigationControl();
  moveRight->setHeight( 22 );
  moveRight->setWidth( 20 );
  moveRight->setPosition( imgLeft + 36, imgTop + 18 );
  moveRight->addEventHandler( new PanControlHandler( manip, MOVE_OFFSET, 0 ) );
  mControlCanvas->addControl( moveRight );

  Control* moveUp = new NavigationControl();
  moveUp->setHeight( 19 );
  moveUp->setWidth( 24 );
  moveUp->setPosition( imgLeft + 20, imgTop + 0 );
  moveUp->addEventHandler( new PanControlHandler( manip, 0, MOVE_OFFSET ) );
  mControlCanvas->addControl( moveUp );

  Control* moveDown = new NavigationControl();
  moveDown->setHeight( 19 );
  moveDown->setWidth( 24 );
  moveDown->setPosition( imgLeft + 16, imgTop + 36 );
  moveDown->addEventHandler( new PanControlHandler( manip, 0, -MOVE_OFFSET ) );
  mControlCanvas->addControl( moveDown );

  //Zoom Reset
  Control* zoomHome = new NavigationControl();
  zoomHome->setHeight( 22 );
  zoomHome->setWidth( 16 );
  zoomHome->setPosition( imgLeft + 20, imgTop + 18 );
  zoomHome->addEventHandler( new HomeControlHandler( manip ) );
  mControlCanvas->addControl( zoomHome );

  // -------

  osg::Image* backgroundImg = osgDB::readImageFile( imgDir + "/button-background.png" );
  ImageControl* backgroundGrp1 = new ImageControl( backgroundImg );
  imgTop = imgTop + 62;
  backgroundGrp1->setPosition( imgLeft + 12, imgTop );
  mControlCanvas->addControl( backgroundGrp1 );

  osg::Image* plusImg = osgDB::readImageFile( imgDir + "/zoom-in.png" );
  ImageControl* zoomIn = new NavigationControl( plusImg );
  zoomIn->setPosition( imgLeft + 12 + 3, imgTop + 3 );
  zoomIn->addEventHandler( new ZoomControlHandler( manip, 0, -MOVE_OFFSET ) );
  mControlCanvas->addControl( zoomIn );

  osg::Image* minusImg = osgDB::readImageFile( imgDir + "/zoom-out.png" );
  ImageControl* zoomOut = new NavigationControl( minusImg );
  zoomOut->setPosition( imgLeft + 12 + 3, imgTop + 3 + 23 + 2 );
  zoomOut->addEventHandler( new ZoomControlHandler( manip, 0, MOVE_OFFSET ) );
  mControlCanvas->addControl( zoomOut );

  // -------

  ImageControl* backgroundGrp2 = new ImageControl( backgroundImg );
  imgTop = imgTop + 60;
  backgroundGrp2->setPosition( imgLeft + 12, imgTop );
  mControlCanvas->addControl( backgroundGrp2 );

  //Zoom Reset
#if ENABLE_HOME_BUTTON
  osg::Image* homeImg = osgDB::readImageFile( imgDir + "/zoom-home.png" );
  ImageControl* home = new NavigationControl( homeImg );
  home->setPosition( imgLeft + 12 + 3, imgTop + 2 );
  imgTop = imgTop + 23 + 2;
  home->addEventHandler( new HomeControlHandler( manip ) );
  mControlCanvas->addControl( home );
#endif

  //refresh layers
  osg::Image* refreshImg = osgDB::readImageFile( imgDir + "/refresh-view.png" );
  ImageControl* refresh = new NavigationControl( refreshImg );
  refresh->setPosition( imgLeft + 12 + 3, imgTop + 3 );
  imgTop = imgTop + 23 + 2;
  refresh->addEventHandler( new RefreshControlHandler( this ) );
  mControlCanvas->addControl( refresh );

  //Sync Extent
  osg::Image* syncImg = osgDB::readImageFile( imgDir + "/sync-extent.png" );
  ImageControl* sync = new NavigationControl( syncImg );
  sync->setPosition( imgLeft + 12 + 3, imgTop + 2 );
  sync->addEventHandler( new SyncExtentControlHandler( this ) );
  mControlCanvas->addControl( sync );
}

void GlobePlugin::setupProxy()
{
  QSettings settings;
  settings.beginGroup( "proxy" );
  if ( settings.value( "/proxyEnabled" ).toBool() )
  {
    ProxySettings proxySettings( settings.value( "/proxyHost" ).toString().toStdString(),
                                 settings.value( "/proxyPort" ).toInt() );
    if ( !settings.value( "/proxyUser" ).toString().isEmpty() )
    {
      QString auth = settings.value( "/proxyUser" ).toString() + ":" + settings.value( "/proxyPassword" ).toString();
#ifdef WIN32
      putenv( QString( "OSGEARTH_CURL_PROXYAUTH=%1" ).arg( auth ).toAscii() );
#else
      setenv( "OSGEARTH_CURL_PROXYAUTH", auth.toStdString().c_str(), 0 );
#endif
    }
    //TODO: settings.value("/proxyType")
    //TODO: URL exlusions
    HTTPClient::setProxySettings( proxySettings );
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

void GlobePlugin::layersAdded( QList<QgsMapLayer*> mapLayers )
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

        QgsGlobeFeatureOptions  featureOpt;
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
        FeatureGeomModelOptions geomOpt;
        geomOpt.featureOptions() = featureOpt;
        geomOpt.styles() = new StyleSheet();
        geomOpt.styles()->addStyle( style );

        geomOpt.featureIndexing() = FeatureSourceIndexOptions();

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
      tileLayersChanged();
    }
  }
}

void GlobePlugin::layersRemoved( QStringList layerIds )
{
  QgsDebugMsg( QString( "layers removed [%1]" ).arg( layerIds.join( ", " ) ) );
  if ( mIsGlobeRunning )
  {
    osg::ref_ptr<Map> mapNode = mMapNode->getMap();

    Q_FOREACH( const QString &layer, layerIds )
    {
      ModelLayer* modelLayer = mapNode->getModelLayerByName( layer.toStdString() );
      mapNode->removeModelLayer( modelLayer );
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
    ElevationLayerVector list;
    map->getElevationLayers( list );
    for ( ElevationLayerVector::iterator i = list.begin(); i != list.end(); ++i )
    {
      map->removeElevationLayer( *i );
    }

    // Add elevation layers
    QSettings settings;
    QString cacheDirectory = settings.value( "cache/directory", QgsApplication::qgisSettingsDirPath() + "cache" ).toString();
    QTableWidget* table = mSettingsDialog->elevationDatasources();
    for ( int i = 0; i < table->rowCount(); ++i )
    {
      QString type = table->item( i, 0 )->text();
      //bool cache = table->item( i, 1 )->checkState();
      QString uri = table->item( i, 2 )->text();
      ElevationLayer* layer = 0;

      if ( "Raster" == type )
      {
        GDALOptions options;
        options.url() = uri.toStdString();
        layer = new osgEarth::ElevationLayer( uri.toStdString(), options );
      }
      else if ( "TMS" == type )
      {
        TMSOptions options;
        options.url() = uri.toStdString();
        layer = new osgEarth::ElevationLayer( uri.toStdString(), options );
      }
      map->addElevationLayer( layer );

      //if ( !cache || type == "Worldwind" ) layer->setCache( 0 ); //no tms cache for worldwind (use worldwind_cache)
    }
#if OSGEARTH_VERSION_GREATER_OR_EQUAL( 2, 5, 0 )
    double scale = QgsProject::instance()->readDoubleEntry( "Globe-Plugin", "/verticalScale", 1 );
    setVerticalScale( scale );
#endif
  }
  else
  {
    QgsDebugMsg( "layersChanged: Globe NOT running, skipping" );
    return;
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
      TMSOptions imagery;
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
        mSkyNode = SkyNode::create( mMapNode );

#warning "FIXME?"
#if OSGEARTH_VERSION_GREATER_OR_EQUAL( 2, 4, 0 ) and OSGEARTH_VERSION_LESS_THAN(2, 6, 0)
      mSkyNode->setAutoAmbience( autoAmbience );
#else
      Q_UNUSED( autoAmbience );
#endif
#if OSGEARTH_VERSION_GREATER_OR_EQUAL(2, 6, 0)
      mSkyNode->setDateTime( osgEarth::DateTime( dateTime.date().year(), dateTime.date().month(), dateTime.date().day(), dateTime.time().hour() + dateTime.time().minute() / 60.0 ) );
#else
      mSkyNode->setDateTime( dateTime.date().year()
                             , dateTime.date().month()
                             , dateTime.date().day()
                             , dateTime.time().hour() + dateTime.time().minute() / 60.0 );
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

void GlobePlugin::tileLayersChanged()
{
  if ( mIsGlobeRunning )
  {
    osg::ref_ptr<Map> map = mMapNode->getMap();

    if ( map->getNumImageLayers() > 1 )
    {
      mOsgViewer->getDatabasePager()->clear();
    }

    //remove QGIS layer
    if ( mQgisMapLayer )
    {
      map->removeImageLayer( mQgisMapLayer );
    }

    ImageLayerOptions options( "QGIS" );
    mQgisMapLayer = new ImageLayer( options, mTileSource );
    map->addImageLayer( mQgisMapLayer );
  }
}

void GlobePlugin::reset()
{
  if ( mViewerWidget )
  {
    delete mViewerWidget;
    mViewerWidget = 0;
    delete mDockWidget;
    mDockWidget = 0;
  }
  if ( mOsgViewer )
  {
    delete mOsgViewer;
    mOsgViewer = 0;
  }
  mQgisMapLayer = 0;

  setGlobeNotRunning();
}

void GlobePlugin::unload()
{
  reset();
  // remove the GUI
  mQGisIface->removePluginMenu( tr( "&Globe" ), mQActionPointer );
  mQGisIface->removePluginMenu( tr( "&Globe" ), mQActionSettingsPointer );
  mQGisIface->removePluginMenu( tr( "&Globe" ), mQActionUnload );

  mQGisIface->removeToolBarIcon( mQActionPointer );

  delete mQActionPointer;

#if 0
  if ( mCoutRdBuf )
    std::cout.rdbuf( mCoutRdBuf );
  if ( mCerrRdBuf )
    std::cerr.rdbuf( mCerrRdBuf );
#endif
  mQGisIface->unregisterMapLayerPropertiesFactory( mLayerPropertiesFactory );
  delete mLayerPropertiesFactory;
}

void GlobePlugin::help()
{
}

void GlobePlugin::placeNode( osg::Node* node, double lat, double lon, double alt /*= 0.0*/ )
{
#ifdef HAVE_OSGEARTH_ELEVATION_QUERY
  Q_UNUSED( node );
  Q_UNUSED( lat );
  Q_UNUSED( lon );
  Q_UNUSED( alt );
#else
  // get elevation
  double elevation = 0.0;
  double resolution = 0.0;
  mElevationManager->getElevation( lon, lat, 0, NULL, elevation, resolution );

  // place model
  osg::Matrix mat;
  mObjectPlacer->createPlacerMatrix( lat, lon, elevation + alt, mat );

  osg::MatrixTransform* mt = new osg::MatrixTransform( mat );
  mt->addChild( node );
  mRootNode->addChild( mt );
#endif
}

void GlobePlugin::copyFolder( QString sourceFolder, QString destFolder )
{
  QDir sourceDir( sourceFolder );
  if ( !sourceDir.exists() )
    return;
  QDir destDir( destFolder );
  if ( !destDir.exists() )
  {
    destDir.mkpath( destFolder );
  }
  QStringList files = sourceDir.entryList( QDir::Files );
  for ( int i = 0; i < files.count(); i++ )
  {
    QString srcName = sourceFolder + "/" + files[i];
    QString destName = destFolder + "/" + files[i];
    QFile::copy( srcName, destName );
  }
  files.clear();
  files = sourceDir.entryList( QDir::AllDirs | QDir::NoDotAndDotDot );
  for ( int i = 0; i < files.count(); i++ )
  {
    QString srcName = sourceFolder + "/" + files[i];
    QString destName = destFolder + "/" + files[i];
    copyFolder( srcName, destName );
  }
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

void GlobePlugin::onLayerRead( QgsMapLayer* mapLayer, QDomElement elem )
{
  if ( mapLayer->type() == QgsMapLayer::VectorLayer )
  {
    QgsVectorLayer* vLayer = dynamic_cast<QgsVectorLayer*>( mapLayer );
    QgsGlobeVectorLayerConfig::readLayer( vLayer, elem );
  }
}

void GlobePlugin::onLayerWrite( QgsMapLayer* mapLayer, QDomElement& elem, QDomDocument& doc )
{
  if ( mapLayer->type() == QgsMapLayer::VectorLayer )
  {
    QgsVectorLayer* vLayer = dynamic_cast<QgsVectorLayer*>( mapLayer );
    QgsGlobeVectorLayerConfig::writeLayer( vLayer, elem, doc );
  }
}

void GlobePlugin::setGlobeNotRunning()
{
  QgsDebugMsg( "Globe Closed" );
  mIsGlobeRunning = false;
}

bool NavigationControl::handle( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa, ControlContext& cx )
{
  switch ( ea.getEventType() )
  {
    case osgGA::GUIEventAdapter::PUSH:
      _mouse_down_event = &ea;
      break;
    case osgGA::GUIEventAdapter::FRAME:
      if ( _mouse_down_event )
      {
        _mouse_down_event = &ea;
      }
      break;
    case osgGA::GUIEventAdapter::RELEASE:
      for ( ControlEventHandlerList::const_iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i )
      {
        NavigationControlHandler* handler = dynamic_cast<NavigationControlHandler*>( i->get() );
        if ( handler )
        {
          handler->onClick( this, ea.getButtonMask(), ea, aa );
        }
      }
      _mouse_down_event = NULL;
      break;
    default:
      /* ignore */
      ;
  }
  if ( _mouse_down_event )
  {
    //OE_NOTICE << "NavigationControl::handle getEventType " << ea.getEventType() << std::endl;
    for ( ControlEventHandlerList::const_iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i )
    {
      NavigationControlHandler* handler = dynamic_cast<NavigationControlHandler*>( i->get() );
      if ( handler )
      {
        handler->onMouseDown( this, ea.getButtonMask() );
      }
    }
  }
  return Control::handle( ea, aa, cx );
}

bool KeyboardControlHandler::handle( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& /*aa*/ )
{
#if 0
  osgEarth::Util::EarthManipulator::Settings* _manipSettings = _manip->getSettings();
  _manip->getSettings()->bindKey( osgEarth::Util::EarthManipulator::ACTION_ZOOM_IN, osgGA::GUIEventAdapter::KEY_Space );
  //install default action bindings:
  osgEarth::Util::EarthManipulator::ActionOptions options;

  _manipSettings->bindKey( osgEarth::Util::EarthManipulator::ACTION_HOME, osgGA::GUIEventAdapter::KEY_Space );

  _manipSettings->bindMouse( osgEarth::Util::EarthManipulator::ACTION_PAN, osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON );

  // zoom as you hold the right button:
  options.clear();
  options.add( osgEarth::Util::EarthManipulator::OPTION_CONTINUOUS, true );
  _manipSettings->bindMouse( osgEarth::Util::EarthManipulator::ACTION_ROTATE, osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON, 0L, options );

  // zoom with the scroll wheel:
  _manipSettings->bindScroll( osgEarth::Util::EarthManipulator::ACTION_ZOOM_IN,  osgGA::GUIEventAdapter::SCROLL_DOWN );
  _manipSettings->bindScroll( osgEarth::Util::EarthManipulator::ACTION_ZOOM_OUT, osgGA::GUIEventAdapter::SCROLL_UP );

  // pan around with arrow keys:
  _manipSettings->bindKey( osgEarth::Util::EarthManipulator::ACTION_PAN_LEFT,  osgGA::GUIEventAdapter::KEY_Left );
  _manipSettings->bindKey( osgEarth::Util::EarthManipulator::ACTION_PAN_RIGHT, osgGA::GUIEventAdapter::KEY_Right );
  _manipSettings->bindKey( osgEarth::Util::EarthManipulator::ACTION_PAN_UP,    osgGA::GUIEventAdapter::KEY_Up );
  _manipSettings->bindKey( osgEarth::Util::EarthManipulator::ACTION_PAN_DOWN,  osgGA::GUIEventAdapter::KEY_Down );

  // double click the left button to zoom in on a point:
  options.clear();
  options.add( osgEarth::Util::EarthManipulator::OPTION_GOTO_RANGE_FACTOR, 0.4 );
  _manipSettings->bindMouseDoubleClick( osgEarth::Util::EarthManipulator::ACTION_GOTO, osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON, 0L, options );

  // double click the right button(or CTRL-left button) to zoom out to a point
  options.clear();
  options.add( osgEarth::Util::EarthManipulator::OPTION_GOTO_RANGE_FACTOR, 2.5 );
  _manipSettings->bindMouseDoubleClick( osgEarth::Util::EarthManipulator::ACTION_GOTO, osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON, 0L, options );
  _manipSettings->bindMouseDoubleClick( osgEarth::Util::EarthManipulator::ACTION_GOTO, osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON, osgGA::GUIEventAdapter::MODKEY_CTRL, options );

  _manipSettings->setThrowingEnabled( false );
  _manipSettings->setLockAzimuthWhilePanning( true );

  _manip->applySettings( _manipSettings );
#endif

  switch ( ea.getEventType() )
  {
    case( osgGA::GUIEventAdapter::KEYDOWN ) :
    {
      //move map
      if ( ea.getKey() == '4' )
      {
        _manip->pan( -MOVE_OFFSET, 0 );
      }
      if ( ea.getKey() == '6' )
      {
        _manip->pan( MOVE_OFFSET, 0 );
      }
      if ( ea.getKey() == '2' )
      {
        _manip->pan( 0, MOVE_OFFSET );
      }
      if ( ea.getKey() == '8' )
      {
        _manip->pan( 0, -MOVE_OFFSET );
      }
      //rotate
      if ( ea.getKey() == '/' )
      {
        _manip->rotate( MOVE_OFFSET, 0 );
      }
      if ( ea.getKey() == '*' )
      {
        _manip->rotate( -MOVE_OFFSET, 0 );
      }
      //tilt
      if ( ea.getKey() == '9' )
      {
        _manip->rotate( 0, MOVE_OFFSET );
      }
      if ( ea.getKey() == '3' )
      {
        _manip->rotate( 0, -MOVE_OFFSET );
      }
      //zoom
      if ( ea.getKey() == '-' )
      {
        _manip->zoom( 0, MOVE_OFFSET );
      }
      if ( ea.getKey() == '+' )
      {
        _manip->zoom( 0, -MOVE_OFFSET );
      }
      //reset
      if ( ea.getKey() == '5' )
      {
        //_manip->zoom( 0, 0 );
      }
      break;
    }

    default:
      break;
  }
  return false;
}

bool FlyToExtentHandler::handle( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& /*aa*/ )
{
  if ( ea.getEventType() == ea.KEYDOWN && ea.getKey() == '1' )
  {
    mGlobe->syncExtent();
  }
  return false;
}

#ifdef HAVE_OSGEARTH_ELEVATION_QUERY
#else
bool QueryCoordinatesHandler::handle( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa )
{
  if ( ea.getEventType() == osgGA::GUIEventAdapter::MOVE )
  {
    osgViewer::View* view = static_cast<osgViewer::View*>( aa.asView() );
    osg::Vec3d coords = getCoords( ea.getX(), ea.getY(), view, false );
    mGlobe->showCurrentCoordinates( coords.x(), coords.y() );
  }
  if ( ea.getEventType() == osgGA::GUIEventAdapter::PUSH
       && ea.getButtonMask() == osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON )
  {
    osgViewer::View* view = static_cast<osgViewer::View*>( aa.asView() );
    osg::Vec3d coords = getCoords( ea.getX(), ea.getY(), view, false );

    OE_NOTICE << "SelectedCoordinates set to:\nLon: " << coords.x() << " Lat: " << coords.y()
    << " Ele: " << coords.z() << std::endl;

    mGlobe->setSelectedCoordinates( coords );

    if ( ea.getModKeyMask() == osgGA::GUIEventAdapter::MODKEY_CTRL )
      //ctrl + rightclick pops up a QMessageBox
    {
      mGlobe->showSelectedCoordinates();
    }
  }

  return false;
}

osg::Vec3d QueryCoordinatesHandler::getCoords( float x, float y, osgViewer::View* view,  bool getElevation )
{
  osgUtil::LineSegmentIntersector::Intersections results;
  osg::Vec3d coords;
  if ( view->computeIntersections( x, y, results, 0x01 ) )
  {
    // find the first hit under the mouse:
    osgUtil::LineSegmentIntersector::Intersection first = *( results.begin() );
    osg::Vec3d point = first.getWorldIntersectPoint();

    // transform it to map coordinates:
    double lat_rad, lon_rad, height;
    _mapSRS->getEllipsoid()->convertXYZToLatLongHeight( point.x(), point.y(), point.z(), lat_rad, lon_rad, height );

    // query the elevation at the map point:
    double lon_deg = osg::RadiansToDegrees( lon_rad );
    double lat_deg = osg::RadiansToDegrees( lat_rad );
    double elevation = -99999;

    if ( getElevation )
    {
      osg::Matrixd out_mat;
      double query_resolution = 0.1; // 1/10th of a degree
      double out_resolution = 0.0;

      //TODO test elevation calculation
      //@see https://github.com/gwaldron/osgearth/blob/master/src/applications/osgearth_elevation/osgearth_elevation.cpp
      if ( _elevMan->getPlacementMatrix(
             lon_deg, lat_deg, 0,
             query_resolution, _mapSRS,
             //query_resolution, NULL,
             out_mat, elevation, out_resolution ) )
      {
        OE_NOTICE << "Elevation at " << lon_deg << ", " << lat_deg
        << " is " << elevation << std::endl;
      }
      else
      {
        OE_NOTICE << "getElevation FAILED! at (" << lon_deg << ", "
        << lat_deg << ")" << std::endl;
      }
    }

    coords = osg::Vec3d( lon_deg, lat_deg, elevation );
  }
  return coords;
}
#endif

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
