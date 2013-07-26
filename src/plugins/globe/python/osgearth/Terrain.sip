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
     * This object is passed to terrain callbacks to provide context information
     * and a feedback interface to terrain callback consumers.
     */
    class TerrainCallbackContext
    {
%TypeHeaderCode
#include <osgEarth/Terrain>
using namespace osgEarth;
%End
    public:
        const Terrain* getTerrain() const;

        /** Indicate that the callback should be removed immediately */
        void remove();

        /** whether the user called remove(). */
        bool markedForRemoval() const;


    public:
        TerrainCallbackContext(Terrain* terrain);

        /** dtor */
        virtual ~TerrainCallbackContext();
    };




    class /*interface-only*/ TerrainHeightProvider
    {
%TypeHeaderCode
#include <osgEarth/Terrain>
using namespace osgEarth;
%End
    public:
        virtual bool getHeight(
            const SpatialReference* srs,
            double                  x,
            double                  y,
            double*                 out_heightAboveMSL,
            double*                 out_heightAboveEllipsoid =0) const =0;

        /** dtor */
        virtual ~TerrainHeightProvider();
    };


    /**
     * Services for interacting with the live terrain graph. This differs from
     * the Map model; Map represents the parametric data backing the terrain, 
     * while Terrain represents the actual geometry in memory.
     *
     * All returned map coordinate values are in the units conveyed in the
     * spatial reference at getSRS().
     */
    class Terrain
    {
%TypeHeaderCode
#include <osgEarth/Terrain>
using namespace osgEarth;
using namespace osg;
%End
    public:
        /**
         * Gets the profile of the map with which this terrain is associated.
         */
/*
        const Profile* getProfile() const;
*/
        /**
         * Gets the spatial reference of the map with which this terrain is
         * associated.
         */
        const SpatialReference* getSRS() const;

        /**
         * Whether the terrain is in geocentric (ECEF) coordinates
         */
        bool isGeocentric() const;


    public: // Intersection Utilities

        /**
         * Intersects the terrain at the location x, y and returns the height data.
         *
         * @param srs
         *      Spatial reference system of (x,y) coordinates
         * @param x, y
         *      Coordinates at which to query the height
         * @param out_heightAboveMSL
         *      The resulting relative height goes here. The height is relative to MSL
         *      (mean sea level) as expressed by the map's vertical datum.
         * @param out_heightAboveEllipsoid
         *      The resulting geodetic height goes here. The height is relative to the
         *      geodetic ellipsoid expressed by the map's SRS.
         */
        bool getHeight(
            const SpatialReference* srs,
            double                  x,
            double                  y,
            double*                 out_heightAboveMSL,
            double*                 out_heightAboveEllipsoid =0) const;

        /**
         * Save as above, but specify a subgraph patch.
         */
        bool getHeight(
            Node*                   patch,
            const SpatialReference* srs,
            double                  x,
            double                  y,
            double*                 out_heightAboveMSL,
            double*                 out_heightAboveEllipsoid =0) const;

        /**
         * Returns the world coordinates under the mouse.
         * @param view
         *      View in which to do the query
         * @param mx, my
         *      Mouse coordinates
         * @param out_coords
         *      Stores the world coordinates under the mouse (when returning true)
         */
/*
        bool getWorldCoordsUnderMouse(
            osg::View*  view,
            float       mx,
            float       my,
            Vec3d& out_world ) const;

        bool getWorldCoordsUnderMouse(
            osg::View*  view,
            float       mx,
            float       my,
            osg::Vec3d& out_world,
            osg::ref_ptr<Node>& out_node ) const;
*/
    public:
        /**
         * Adds a terrain callback.
         *
         * @param callback
         *      Terrain callback to add. This will get called whenever tile data changes in
         *      the active terrain graph
         */
/*
        void addTerrainCallback( TerrainCallback* callback);
*/
        /**
         * Removes a terrain callback.
         */
/*
        void removeTerrainCallback( TerrainCallback* callback );
*/
    public:

        /**
         * Accept a node visitor on the terrain's scene graph.
         */
/*
        void accept( NodeVisitor& nv );
*/
        // access the raw terrain graph
        Node* getGraph();
/*
        // queues the onTileAdded callback (internal)
        void notifyTileAdded( const TileKey& key, Node* tile );
        // fires the onTileAdded callback (internal)
        void fireTileAdded( const TileKey& key, Node* tile );
*/
        /** dtor */
        virtual ~Terrain();

      private:
        Terrain();
    };


    /**
     * A TerrainPatch is a standalone subset of terrain that you can use for
     * height queries. The intention is that this be used for a "disconnected"
     * terrain tile, i.e. one that is not in the scene graph yet -- making it
     * MT-safe.
     */
   class TerrainPatch : TerrainHeightProvider
   {
%TypeHeaderCode
#include <osgEarth/Terrain>
using namespace osgEarth;
using namespace osg;
%End
   public:
       TerrainPatch( Node* patch, const Terrain* terrain );

       /**
        * Queries the elevation under the specified point. This method is
        * identical to calling Terrain::getHeight with the specified patch.
        */
      bool getHeight(
          const SpatialReference* srs,
          double                  x,
          double                  y,
          double*                 out_heightAboveMSL,
          double*                 out_heightAboveEllipsoid =0) const;
   };
