/***************************************************************************
    qgsosgstyleutils.h
     --------------------------------------
    Date                 : 17.7.2013
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

#ifndef QGSGLOBESTYLEUTILS_H
#define QGSGLOBESTYLEUTILS_H

#include <QColor>

#include <osgEarthFeatures/Feature>

using namespace osgEarth::Symbology;
class QgsGlobeStyleUtils
{
  public:
    static Color QColorToOsgColor( const QColor& color )
    {
      return Color( color.redF(), color.greenF(), color.blueF(), color.alphaF() );
    }


};


#endif // QGSGLOBESTYLEUTILS_H
