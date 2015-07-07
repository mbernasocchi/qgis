/***************************************************************************
*    qgsglobeplugindialog.cpp - settings dialog for the globe plugin
*     --------------------------------------
*    Date                 : 11-Nov-2010
*    Copyright            : (C) 2010 by Marco Bernasocchi
*    Email                : marco at bernawebdesign.ch
***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "qgsglobeplugindialog.h"
#include "globe_plugin.h"

#include <qgsproject.h>
#include <qgslogger.h>

#include <QFileDialog>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSettings>
#include <QTimer>

#include <osg/DisplaySettings>
#include <osgViewer/Viewer>
#include <osgEarth/Version>
#include <osgEarthUtil/EarthManipulator>

QgsGlobePluginDialog::QgsGlobePluginDialog( GlobePlugin* globe , QWidget* parent, Qt::WFlags fl )
    : QDialog( parent, fl )
    , mGlobe( globe )
{
  setupUi( this );

  mBaseLayerComboBox->addItem( "Readymap: NASA BlueMarble Imagery", "http://readymap.org/readymap/tiles/1.0.0/1/" );
  mBaseLayerComboBox->addItem( "Readymap: NASA BlueMarble with land removed, only ocean", "http://readymap.org/readymap/tiles/1.0.0/2/" );
  mBaseLayerComboBox->addItem( "Readymap: High resolution insets from various locations around the world Austin, TX; Kandahar, Afghanistan; Bagram, Afghanistan; Boston, MA; Washington, DC", "http://readymap.org/readymap/tiles/1.0.0/3/" );
  mBaseLayerComboBox->addItem( "Readymap: Global Land Cover Facility 15m Landsat", "http://readymap.org/readymap/tiles/1.0.0/6/" );
  mBaseLayerComboBox->addItem( "Readymap: NASA BlueMarble + Landsat + Ocean Masking Layer", "http://readymap.org/readymap/tiles/1.0.0/7/" );
  mBaseLayerComboBox->addItem( "[Custom]" );

  comboStereoMode->addItem( "OFF", -1 );
  comboStereoMode->addItem( "ANAGLYPHIC", osg::DisplaySettings::ANAGLYPHIC );
  comboStereoMode->addItem( "QUAD_BUFFER", osg::DisplaySettings::ANAGLYPHIC );
  comboStereoMode->addItem( "HORIZONTAL_SPLIT", osg::DisplaySettings::HORIZONTAL_SPLIT );
  comboStereoMode->addItem( "VERTICAL_SPLIT", osg::DisplaySettings::VERTICAL_SPLIT );


  mAANumSamples->setValidator( new QIntValidator( mAANumSamples ) );

  elevationPath->setText( QDir::homePath() );

#if OSGEARTH_VERSION_LESS_THAN( 2, 5, 0 )
  mTxtVerticalScale->setVisible( false );
#endif

  connect( buttonBox->button( QDialogButtonBox::Apply ), SIGNAL( clicked( bool ) ), this, SLOT( apply() ) );

  restoreSavedSettings();
}

void QgsGlobePluginDialog::restoreSavedSettings()
{
  QSettings settings;

  // Stereo settings
  comboStereoMode->setCurrentIndex( comboStereoMode->findText( settings.value( "/Plugin-Globe/stereoMode", "OFF" ).toString() ) );
  screenDistance->setValue( settings.value( "/Plugin-Globe/screenDistance",
                            osg::DisplaySettings::instance()->getScreenDistance() ).toDouble() );
  screenWidth->setValue( settings.value( "/Plugin-Globe/screenWidth",
                                         osg::DisplaySettings::instance()->getScreenWidth() ).toDouble() );
  screenHeight->setValue( settings.value( "/Plugin-Globe/screenHeight",
                                          osg::DisplaySettings::instance()->getScreenHeight() ).toDouble() );
  eyeSeparation->setValue( settings.value( "/Plugin-Globe/eyeSeparation",
                           osg::DisplaySettings::instance()->getEyeSeparation() ).toDouble() );
  splitStereoHorizontalSeparation->setValue( settings.value( "/Plugin-Globe/splitStereoHorizontalSeparation",
      osg::DisplaySettings::instance()->getSplitStereoHorizontalSeparation() ).toInt() );
  splitStereoVerticalSeparation->setValue( settings.value( "/Plugin-Globe/splitStereoVerticalSeparation",
      osg::DisplaySettings::instance()->getSplitStereoVerticalSeparation() ).toInt() );
  splitStereoHorizontalEyeMapping->setCurrentIndex( settings.value( "/Plugin-Globe/splitStereoHorizontalEyeMapping",
      osg::DisplaySettings::instance()->getSplitStereoHorizontalEyeMapping() ).toInt() );
  splitStereoVerticalEyeMapping->setCurrentIndex( settings.value( "/Plugin-Globe/splitStereoVerticalEyeMapping",
      osg::DisplaySettings::instance()->getSplitStereoVerticalEyeMapping() ).toInt() );

  // Navigation settings
  mScrollSensitivitySlider->setValue( settings.value( "/Plugin-Globe/scrollSensitivity", 20 ).toInt() );
  mInvertScrollWheel->setChecked( settings.value( "/Plugin-Globe/invertScrollWheel", 0 ).toInt() );

  // Video settings
  mAntiAliasingGroupBox->setChecked( settings.value( "/Plugin-Globe/anti-aliasing", false ).toBool() );
  mAANumSamples->setText( settings.value( "/Plugin-Globe/anti-aliasing-level", "" ).toString() );

  // Map settings
  mBaseLayerGroupBox->setChecked( settings.value( "/Plugin-Globe/baseLayerEnabled", true ).toBool() );
  mBaseLayerURL->setText( settings.value( "/Plugin-Globe/baseLayerURL", "http://readymap.org/readymap/tiles/1.0.0/7/" ).toString() );
  mBaseLayerComboBox->setCurrentIndex( mBaseLayerComboBox->findData( mBaseLayerURL->text() ) );
  mSkyGroupBox->setChecked( settings.value( "/Plugin-Globe/skyEnabled", false ).toBool() );
  mSkyAutoAmbient->setChecked( settings.value( "/Plugin-Globe/skyAutoAmbient", false ).toBool() );
  mSkyDateTime->setDateTime( settings.value( "/Plugin-Globe/skyDateTime", QDateTime::currentDateTime() ).toDateTime() );
}

void QgsGlobePluginDialog::on_buttonBox_accepted()
{
  apply();
  accept();
}

void QgsGlobePluginDialog::on_buttonBox_rejected()
{
  restoreSavedSettings();
  readProjectSettings();
  reject();
}

void QgsGlobePluginDialog::apply()
{
  QSettings settings;

  // Stereo settings
  settings.setValue( "/Plugin-Globe/stereoMode", comboStereoMode->currentText() );
  settings.setValue( "/Plugin-Globe/screenDistance", screenDistance->value() );
  settings.setValue( "/Plugin-Globe/screenWidth", screenWidth->value() );
  settings.setValue( "/Plugin-Globe/screenHeight", screenHeight->value() );
  settings.setValue( "/Plugin-Globe/eyeSeparation", eyeSeparation->value() );
  settings.setValue( "/Plugin-Globe/splitStereoHorizontalSeparation", splitStereoHorizontalSeparation->value() );
  settings.setValue( "/Plugin-Globe/splitStereoVerticalSeparation", splitStereoVerticalSeparation->value() );
  settings.setValue( "/Plugin-Globe/splitStereoHorizontalEyeMapping", splitStereoHorizontalEyeMapping->currentIndex() );
  settings.setValue( "/Plugin-Globe/splitStereoVerticalEyeMapping", splitStereoVerticalEyeMapping->currentIndex() );

  // Navigation settings
  settings.setValue( "/Plugin-Globe/scrollSensitivity", mScrollSensitivitySlider->value() );
  settings.setValue( "/Plugin-Globe/invertScrollWheel", mInvertScrollWheel->checkState() );

  // Video settings
  settings.setValue( "/Plugin-Globe/anti-aliasing", mAntiAliasingGroupBox->isChecked() );
  settings.setValue( "/Plugin-Globe/anti-aliasing-level", mAANumSamples->text() );

  // Map settings
  settings.setValue( "/Plugin-Globe/baseLayerEnabled", mBaseLayerGroupBox->isChecked() );
  settings.setValue( "/Plugin-Globe/baseLayerURL", mBaseLayerURL->text() );
  settings.setValue( "/Plugin-Globe/skyEnabled", mSkyGroupBox->isChecked() );
  settings.setValue( "/Plugin-Globe/skyAutoAmbient", mSkyAutoAmbient->isChecked() );
  settings.setValue( "/Plugin-Globe/skyDateTime", mSkyDateTime->dateTime() );

  writeProjectSettings();

  // Apply stereo settings
  int stereoMode = comboStereoMode->itemData( comboStereoMode->currentIndex() ).toInt();
  if ( stereoMode == -1 )
  {
    osg::DisplaySettings::instance()->setStereo( false );
  }
  else
  {
    osg::DisplaySettings::instance()->setStereo( true );
    osg::DisplaySettings::instance()->setStereoMode(
      static_cast<osg::DisplaySettings::StereoMode>( stereoMode ) );
    osg::DisplaySettings::instance()->setEyeSeparation( eyeSeparation->value() );
    osg::DisplaySettings::instance()->setScreenDistance( screenDistance->value() );
    osg::DisplaySettings::instance()->setScreenWidth( screenWidth->value() );
    osg::DisplaySettings::instance()->setScreenHeight( screenHeight->value() );
    osg::DisplaySettings::instance()->setSplitStereoVerticalSeparation(
      splitStereoVerticalSeparation->value() );
    osg::DisplaySettings::instance()->setSplitStereoVerticalEyeMapping(
      static_cast<osg::DisplaySettings::SplitStereoVerticalEyeMapping>(
        splitStereoVerticalEyeMapping->currentIndex() ) );
    osg::DisplaySettings::instance()->setSplitStereoHorizontalSeparation(
      splitStereoHorizontalSeparation->value() );
    osg::DisplaySettings::instance()->setSplitStereoHorizontalEyeMapping(
      static_cast<osg::DisplaySettings::SplitStereoHorizontalEyeMapping>(
        splitStereoHorizontalEyeMapping->currentIndex() ) );
  }

  emit settingsApplied();
}

void QgsGlobePluginDialog::readProjectSettings()
{
  // clear the widget
  elevationDatasourcesWidget->setRowCount( 0 );
  foreach ( const ElevationDataSource& ds, getElevationDataSources() )
  {
    int row = elevationDatasourcesWidget->rowCount();
    elevationDatasourcesWidget->setRowCount( row + 1 );
    elevationDatasourcesWidget->setItem( row, 0, new QTableWidgetItem( ds.type ) );
    QTableWidgetItem* chkBoxItem = new QTableWidgetItem();
    //chkBoxItem->setCheckState( ds.cache ? Qt::Checked : Qt::Unchecked );
    elevationDatasourcesWidget->setItem( row, 1, chkBoxItem );
    elevationDatasourcesWidget->setItem( row, 2, new QTableWidgetItem( ds.uri ) );
  }

#if OSGEARTH_VERSION_GREATER_OR_EQUAL( 2, 5, 0 )
  mTxtVerticalScale->setValue( QgsProject::instance()->readDoubleEntry( "Globe-Plugin", "/verticalScale", 1 ) );
#endif
}

void QgsGlobePluginDialog::writeProjectSettings()
{
  QgsProject::instance()->removeEntry( "Globe-Plugin", "/elevationDatasources/" );
  for ( int row = 0, nRows = elevationDatasourcesWidget->rowCount(); row < nRows; ++row )
  {
    QString type  = elevationDatasourcesWidget->item( row, 0 )->text();
    QString uri   = elevationDatasourcesWidget->item( row, 2 )->text();
    bool    cache = elevationDatasourcesWidget->item( row, 1 )->checkState();
    QString key = QString( "/elevationDatasources/L%1" ).arg( row );
    QgsProject::instance()->writeEntry( "Globe-Plugin", key + "/type", type );
    QgsProject::instance()->writeEntry( "Globe-Plugin", key + "/uri", uri );
    QgsProject::instance()->writeEntry( "Globe-Plugin", key + "/cache", cache );
  }

#if OSGEARTH_VERSION_GREATER_OR_EQUAL( 2, 5, 0 )
  QgsProject::instance()->writeEntry( "Globe-Plugin", "/verticalScale", mTxtVerticalScale->value() );
#endif
}

/// ELEVATION /////////////////////////////////////////////////////////////////

QString QgsGlobePluginDialog::validateElevationResource( QString type, QString uri )
{
  if ( "Raster" == type )
  {
    QFileInfo file( uri );
    if ( file.isFile() && file.isReadable() )
      return QString::null;
    else
      return tr( "Invalid Path: The file is either unreadable or does not exist" );
  }
  else
  {
    QNetworkAccessManager networkMgr;
    QNetworkReply* reply = nullptr;
    QUrl url( uri );
    int timeout = 5000; // 5 sec

    while ( true )
    {
      QNetworkRequest req( url );
      req.setRawHeader( "User-Agent" , "Wget/1.13.4" );
      reply = networkMgr.get( req );
      QTimer timer;
      QEventLoop loop;
      QObject::connect( &timer, SIGNAL( timeout() ), &loop, SLOT( quit() ) );
      QObject::connect( reply, SIGNAL( finished() ), &loop, SLOT( quit() ) );
      timer.setSingleShot( true );
      timer.start( timeout );
      loop.exec();
      if ( reply->isRunning() )
      {
        reply->close();
        delete reply;
        return tr( "Error contacting server: timeout" );
      }

      QUrl redirectUrl = reply->attribute( QNetworkRequest::RedirectionTargetAttribute ).toUrl();
      if ( redirectUrl.isValid() && url != redirectUrl )
      {
        delete reply;
        url = redirectUrl;
      }
      else
      {
        break;
      }
    }

    if ( reply->error() == QNetworkReply::NoError )
      return QString::null;
    else
      return tr( "Error contacting server: %1" ).arg( reply->errorString() );
  }
}

void QgsGlobePluginDialog::on_elevationCombo_currentIndexChanged( QString type )
{
  elevationPath->setEnabled( true );
  if ( "Raster" == type )
  {
    elevationActions->setCurrentIndex( 0 );
    elevationPath->setText( QDir::homePath() );
  }
  else if ( "Worldwind" == type )
  {
    elevationActions->setCurrentIndex( 1 );
    elevationPath->setText( "http://tileservice.worldwindcentral.com/getTile?bmng.topo.bathy.200401" );
    elevationPath->setEnabled( false );
  }
  else if ( "TMS" == type )
  {
    elevationActions->setCurrentIndex( 1 );
    elevationPath->setText( "http://readymap.org/readymap/tiles/1.0.0/9/" );
  }
}

void QgsGlobePluginDialog::on_elevationBrowse_clicked()
{
  //see http://www.gdal.org/formats_list.html
  QString filter = tr( "GDAL files" ) + " (*.dem *.tif *.tiff *.jpg *.jpeg *.asc);;"
                   + tr( "DEM files" ) + " (*.dem);;"
                   + tr( "All files" ) + " (*.*)";
  QString newPath = QFileDialog::getOpenFileName( this, tr( "Open raster file" ), QDir::homePath(), filter );
  if ( ! newPath.isEmpty() )
  {
    elevationPath->setText( newPath );
  }
}

void QgsGlobePluginDialog::on_elevationAdd_clicked()
{
  QString error = validateElevationResource( elevationCombo->currentText(), elevationPath->text() );
  QString question = tr( "Do you want to add the datasource anyway?" );
  if ( error.isNull() || QMessageBox::warning( this, tr( "Warning" ), error + "\n" + question, QMessageBox::Ok, QMessageBox::Cancel ) == QMessageBox::Ok )
  {
    int row = elevationDatasourcesWidget->rowCount();
    //cache->setCheckState(( elevationCombo->currentText() == "Worldwind" ) ? Qt::Checked : Qt::Unchecked ); //worldwind_cache will be active
    elevationDatasourcesWidget->setRowCount( row + 1 );
    elevationDatasourcesWidget->setItem( row, 0, new QTableWidgetItem( elevationCombo->currentText() ) );
    elevationDatasourcesWidget->setItem( row, 1, new QTableWidgetItem() );
    elevationDatasourcesWidget->setItem( row, 2, new QTableWidgetItem( elevationPath->text() ) );
    elevationDatasourcesWidget->setCurrentIndex( elevationDatasourcesWidget->model()->index( row, 0 ) );
  }
}

void QgsGlobePluginDialog::on_elevationRemove_clicked()
{
  elevationDatasourcesWidget->removeRow( elevationDatasourcesWidget->currentRow() );
}

void QgsGlobePluginDialog::on_elevationUp_clicked()
{
  moveRow( elevationDatasourcesWidget, -1 );
}

void QgsGlobePluginDialog::on_elevationDown_clicked()
{
  moveRow( elevationDatasourcesWidget, + 1 );
}

void QgsGlobePluginDialog::moveRow( QTableWidget* widget, int offset )
{
  if ( widget->currentItem() != 0 )
  {
    int srcRow = widget->currentItem()->row();
    int dstRow = srcRow + offset;
    if ( dstRow >= 0 && dstRow < widget->rowCount() )
    {
      for ( int col = 0, nCols = widget->columnCount(); col < nCols; ++col )
      {
        QTableWidgetItem* srcItem = widget->takeItem( srcRow, col );
        QTableWidgetItem* dstItem = widget->takeItem( dstRow, col );
        widget->setItem( dstRow, col, srcItem );
        widget->setItem( srcRow, col, dstItem );
      }
      widget->selectRow( dstRow );
    }
  }
}

QList<QgsGlobePluginDialog::ElevationDataSource> QgsGlobePluginDialog::getElevationDataSources() const
{
  int keysCount = QgsProject::instance()->subkeyList( "Globe-Plugin", "/elevationDatasources/" ).count();
  QList<ElevationDataSource> datasources;
  for ( int i = 0; i < keysCount; ++i )
  {
    QString key = QString( "/elevationDatasources/L%1" ).arg( i );
    ElevationDataSource datasource;
    datasource.type  = QgsProject::instance()->readEntry( "Globe-Plugin", key + "/type" );
    datasource.uri   = QgsProject::instance()->readEntry( "Globe-Plugin", key + "/uri" );
//  datasource.cache = QgsProject::instance()->readBoolEntry( "Globe-Plugin", key + "/cache" );
    datasources.append( datasource );
  }
  return datasources;
}

double QgsGlobePluginDialog::getVerticalScale() const
{
  return mTxtVerticalScale->value();
}

/// MAP ///////////////////////////////////////////////////////////////////////

void QgsGlobePluginDialog::on_mBaseLayerComboBox_currentIndexChanged( int index )
{
  QVariant url = mBaseLayerComboBox->itemData( index );
  if ( url.isValid() )
  {
    mBaseLayerURL->setEnabled( false );
    mBaseLayerURL->setText( url.toString() );
  }
  else
  {
    mBaseLayerURL->setEnabled( true );
  }
}

QString QgsGlobePluginDialog::getBaseLayerUrl() const
{
  return mBaseLayerGroupBox->isChecked() ? mBaseLayerURL->text() : QString::null;
}

bool QgsGlobePluginDialog::getSkyEnabled() const
{
  return QSettings().value( "/Plugin-Globe/skyEnabled", false ).toBool();
}

QDateTime QgsGlobePluginDialog::getSkyDateTime() const
{
  return QSettings().value( "/Plugin-Globe/skyDateTime", QDateTime() ).toDateTime();
}

bool QgsGlobePluginDialog::getSkyAutoAmbience() const
{
  return QSettings().value( "/Plugin-Globe/skyAutoAmbient", false ).toBool();
}

/// NAVIGATION ////////////////////////////////////////////////////////////////

float QgsGlobePluginDialog::getScrollSensitivity() const
{
  return mScrollSensitivitySlider->value() / 10;
}

bool QgsGlobePluginDialog::getInvertScrollWheel() const
{
  return mInvertScrollWheel->checkState();
}

/// STEREO ////////////////////////////////////////////////////////////////////
void QgsGlobePluginDialog::on_resetStereoDefaults_clicked()
{
  //http://www.openscenegraph.org/projects/osg/wiki/Support/UserGuides/StereoSettings
  comboStereoMode->setCurrentIndex( comboStereoMode->findText( "OFF" ) );
  screenDistance->setValue( 0.5 );
  screenHeight->setValue( 0.26 );
  screenWidth->setValue( 0.325 );
  eyeSeparation->setValue( 0.06 );
  splitStereoHorizontalSeparation->setValue( 42 );
  splitStereoVerticalSeparation->setValue( 42 );
  splitStereoHorizontalEyeMapping->setCurrentIndex( 0 );
  splitStereoVerticalEyeMapping->setCurrentIndex( 0 );
}

void QgsGlobePluginDialog::on_comboStereoMode_currentIndexChanged( int index )
{
  //http://www.openscenegraph.org/projects/osg/wiki/Support/UserGuides/StereoSettings
  //http://www.openscenegraph.org/documentation/OpenSceneGraphReferenceDocs/a00181.html

  int stereoMode = comboStereoMode->itemData( index ).toInt();
  bool stereoEnabled = stereoMode != -1;
  bool verticalSplit = stereoMode == osg::DisplaySettings::VERTICAL_SPLIT;
  bool horizontalSplit = stereoMode == osg::DisplaySettings::HORIZONTAL_SPLIT;

  screenDistance->setEnabled( stereoEnabled );
  screenHeight->setEnabled( stereoEnabled );
  screenWidth->setEnabled( stereoEnabled );
  eyeSeparation->setEnabled( stereoEnabled );
  splitStereoHorizontalSeparation->setEnabled( stereoEnabled && horizontalSplit );
  splitStereoHorizontalEyeMapping->setEnabled( stereoEnabled && horizontalSplit );
  splitStereoVerticalSeparation->setEnabled( stereoEnabled && verticalSplit );
  splitStereoVerticalEyeMapping->setEnabled( stereoEnabled && verticalSplit );
}
