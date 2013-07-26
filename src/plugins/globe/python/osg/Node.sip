/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2006 Robert Osfield
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/

/** A vector of Nodes pointers which is used to describe the path from a root node to a descendant.*/
typedef std::vector< Node* > NodePath;

/** A vector of NodePath, typically used to describe all the paths from a node to the potential root nodes it has.*/
typedef std::vector< NodePath > NodePathList;

/** A vector of NodePath, typically used to describe all the paths from a node to the potential root nodes it has.*/
typedef std::vector< Matrix > MatrixList;

/** Base class for all internal nodes in the scene graph.
    Provides interface for most common node operations (Composite Pattern).
*/
class Node
{
%TypeHeaderCode
#include <osg/Node>
using namespace osg;
%End
    public:

        /** Construct a node.
            Initialize the parent list to empty, node name to "" and
            bounding sphere dirty flag to true.*/
        Node();
};
