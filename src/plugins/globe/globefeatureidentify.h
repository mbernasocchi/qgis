/***************************************************************************
    globefeatureidentify.h
     --------------------------------------
    Date                 : 27.10.2013
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

#ifndef GLOBEFEATUREIDENTIFY_H
#define GLOBEFEATUREIDENTIFY_H

#include <qgsrubberband.h>
#include <osgEarthUtil/FeatureQueryTool>

struct GlobeFeatureIdentifyCallback : public osgEarth::Util::FeatureQueryTool::Callback
{
public:
  GlobeFeatureIdentifyCallback( QgsMapCanvas* mapCanvas );
  ~GlobeFeatureIdentifyCallback();

  virtual void onHit( osgEarth::Features::FeatureSourceIndexNode* index, osgEarth::Features::FeatureID fid, const EventArgs& args );

private:
  QgsRubberBand* mRubberBand;
};

#endif // GLOBEFEATUREIDENTIFY_H
