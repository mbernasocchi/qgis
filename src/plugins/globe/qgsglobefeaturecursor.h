/***************************************************************************
    qgsglobefeaturecursor.h
     --------------------------------------
    Date                 : 11.7.2013
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

#ifndef QGSGLOBEFEATURECURSOR_H
#define QGSGLOBEFEATURECURSOR_H

#include <qgsfeature.h>
#include <qgsfeatureiterator.h>
#include <qgslogger.h>

#include "qgsglobefeatureutils.h"


namespace osgEarth
{
  namespace Features
  {
    class QgsGlobeFeatureCursor : public FeatureCursor
    {
      public:
        QgsGlobeFeatureCursor( const QgsFields& fields, QgsFeatureIterator iterator )
            : mIterator( iterator )
            , mFields( fields )
        {
          mIterator.nextFeature( mFeature );
        }

        virtual bool hasMore() const
        {
          return mFeature.isValid();
        }

        virtual Feature* nextFeature()
        {
          if ( mFeature.isValid() )
          {
            Feature *feat = QgsGlobeFeatureUtils::featureFromQgsFeature( mFields, mFeature );
            mIterator.nextFeature( mFeature );
            return feat;
          }
          else
          {
            QgsDebugMsg( "WARNING: Returning NULL feature to osgEarth" );
            return NULL;
          }
        }

      private:
        // The iterator
        QgsFeatureIterator mIterator;
        // Cached feature which will be returned next.
        // Always contains the next feature which will be returned
        // (Because hasMore() needs to know if we are able to return a next feature)
        QgsFeature mFeature;
        // fields
        QgsFields mFields;
    };
  }
}
#endif // QGSGLOBEFEATURECURSOR_H
