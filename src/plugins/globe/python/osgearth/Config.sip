/* -*-c++-*-
/* osgEarth - Dynamic map generation toolkit for OpenSceneGraph
 * Copyright 2008-2013 Pelican Mapping
 * http://osgearth.org
 *
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

    /**
     * Config is a general-purpose container for serializable data. You store an object's members
     * to Config, and then translate the Config to a particular format (like XML or JSON). Likewise,
     * the object can de-serialize a Config back into member data. Config support the optional<>
     * template for optional values.
     */
    class Config
    {
%TypeHeaderCode
#include <osgEarth/Config>
%End
    public:
        Config();

        virtual ~Config();

        /** Context for resolving relative URIs that occur in this Config */
/*
        void setReferrer( const std::string& value );
        void inheritReferrer( const std::string& value );

        const std::string& referrer() const { return _referrer; }
        bool fromXML( std::istream& in );

        std::string toJSON( bool pretty =false ) const;

        bool fromJSON( const std::string& json );
*/

        bool empty() const;

        bool isSimple() const;
/*
        std::string& key() { return _key; }
        const std::string& key() const;

        const std::string& value() const;
        std::string& value();

        const ConfigSet& children() const;

        const ConfigSet children( const std::string& key ) const;

        bool hasChild( const std::string& key );

        void remove( const std::string& key );

        Config child( const std::string& key ) const;

        const Config* child_ptr( const std::string& key ) const;

        Config* mutable_child( const std::string& key );
*/
        void merge( const Config& rhs );
/*
        Config* find( const std::string& key, bool checkThis =true );
        const Config* find( const std::string& key, bool checkThis =true) const;
*/
        void add( const Config& conf );
/*
        void add( const std::string& key, const Config& conf );

        void add( const ConfigSet& set );
*/
        void update( const Config& conf );
/*
        bool hasValue( const std::string& key ) const;

        const std::string value( const std::string& key ) const;

        const std::string referrer( const std::string& key ) const;
*/
        bool boolValue( bool fallback ) const;

        /** support for conveying non-serializable objects in a Config (in memory only) */

/*
        typedef std::map<std::string, osg::ref_ptr<osg::Referenced> > RefMap;

        void addNonSerializable( const std::string& key, osg::Referenced* obj );

        void updateNonSerializable( const std::string& key, osg::Referenced* obj );

*/
        // remove everything from (this) that also appears in rhs
        Config operator - ( const Config& rhs ) const;
    };


    /**
     * Base class for all serializable options classes.
     */
    class ConfigOptions
    {
%TypeHeaderCode
#include <osgEarth/Config>
using namespace osgEarth;
%End
    public:
        ConfigOptions( const Config& conf =Config() );
        ConfigOptions( const ConfigOptions& rhs );

        virtual ~ConfigOptions();
/*
        const std::string& referrer() const;
        ConfigOptions& operator= ( const ConfigOptions& rhs );
*/

        void merge( const ConfigOptions& rhs );

        virtual Config getConfig() const;

        virtual Config getConfig( bool isolate ) const;

        Config newConfig() const;

        bool empty() const;

    protected:
        virtual void mergeConfig( const Config& conf );
    };

    /**
     * Base configoptions class for driver options.
     */
    class DriverConfigOptions
    {
%TypeHeaderCode
#include <osgEarth/Config>
%End
    public:
        DriverConfigOptions( const ConfigOptions& rhs =ConfigOptions() );

        /** dtor */
        virtual ~DriverConfigOptions();

        /** Gets or sets the name of the driver to load */
/*
        void setDriver( const std::string& value );
        const std::string& getDriver() const;
*/
    public:
        virtual Config getConfig() const;

        virtual Config getConfig( bool isolate ) const;

        virtual void mergeConfig( const Config& conf );

    public:
        void fromConfig( const Config& conf );
    };
