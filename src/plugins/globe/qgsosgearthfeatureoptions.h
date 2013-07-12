/* -*-c++-*- */
/*
 * osgEarth is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */
#ifndef QGSGLOBEFEATUREOPTIONS_H
#define QGSGLOBEFEATUREOPTIONS_H

#include <osgEarth/Common>
#include <osgEarthFeatures/FeatureSource>

class QgsVectorLayer;

namespace osgEarth
{
  namespace Drivers
  {
    using namespace osgEarth;
    using namespace osgEarth::Features;

    class QgsGlobeFeatureOptions : public FeatureSourceOptions // NO EXPORT; header only
    {
      public:
        template <class T>
        class RefPtr : public osg::Referenced
        {
          public:
            RefPtr( T* ptr )
            {
              mPtr = ptr;
            }

            T* ptr() { return mPtr; }

          private:
            T* mPtr;
        };

      public:
        optional<std::string>& layerId() { return mLayerId; }
        const optional<std::string>& layerId() const { return mLayerId; }

        QgsVectorLayer* layer() { return mLayer; }
        void setLayer( QgsVectorLayer* layer ) { mLayer = layer; }

      public:
        QgsGlobeFeatureOptions( const ConfigOptions& opt = ConfigOptions() ) : FeatureSourceOptions( opt )
        {
          // -> this is the important thing here
          // it will call the driver declared as "osgearth_feature_qgis"
          std::cout << "QGIS options CTOR" << std::endl;
          setDriver( "qgis" );
          fromConfig( _conf );
        }

      public:
        Config getConfig() const
        {
          std::cout << "QGIS options::getConfig" << std::endl;
          Config conf = FeatureSourceOptions::getConfig();
          conf.updateIfSet( "layerId", mLayerId );
          conf.updateNonSerializable( "layer", new RefPtr< QgsVectorLayer >( mLayer ) );
          return conf;
        }

      protected:
        void mergeConfig( const Config& conf )
        {
          FeatureSourceOptions::mergeConfig( conf );
          fromConfig( conf );
        }

      private:
        void fromConfig( const Config& conf )
        {
          conf.getIfSet( "layerId", mLayerId );
          RefPtr< QgsVectorLayer > *layer_ptr = conf.getNonSerializable< RefPtr< QgsVectorLayer > >( "layer" );
          mLayer = layer_ptr ? layer_ptr->ptr() : 0;
        }

        optional<std::string> mLayerId;
        QgsVectorLayer*       mLayer;
    };
  }
} // namespace osgEarth::Drivers

#endif // QGSGLOBEFEATUREOPTIONS_H

