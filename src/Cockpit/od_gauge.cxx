// Owner Drawn Gauge helper class
//
// Written by Harald JOHNSEN, started May 2005.
//
// Copyright (C) 2005  Harald JOHNSEN
//
// Ported to OSG by Tim Moore - Jun 2007
//
// Heavily modified to be usable for the 2d Canvas by Thomas Geymayer - April 2012
// Supports now multisampling/mipmapping, usage of the stencil buffer and placing
// the texture in the scene by certain filter criteria
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <osg/Texture2D>
#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/Camera>
#include <osg/Geode>
#include <osg/NodeVisitor>
#include <osg/Matrix>
#include <osg/PolygonMode>
#include <osg/ShadeModel>
#include <osg/StateSet>
#include <osg/FrameBufferObject> // for GL_DEPTH_STENCIL_EXT on Windows

#include <osgDB/FileNameUtils>

#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/util/RenderConstants.hxx>

#include <Main/globals.hxx>
#include <Viewer/renderer.hxx>
#include <Scenery/scenery.hxx>
#include "od_gauge.hxx"

#include <cassert>

//------------------------------------------------------------------------------
static void cbAddCamera(osg::Camera* cam)
{
  globals->get_renderer()->addCamera(cam, false);
}

//------------------------------------------------------------------------------
static void cbRemoveCamera(osg::Camera* cam)
{
  globals->get_renderer()->removeCamera(cam);
}

//------------------------------------------------------------------------------
FGODGauge::FGODGauge():
  simgear::ODGauge(cbAddCamera, cbRemoveCamera)
{

}

//------------------------------------------------------------------------------
FGODGauge::~FGODGauge()
{

}

/**
 * Replace a texture in the airplane model with the gauge texture.
 */
class ReplaceStaticTextureVisitor:
  public osg::NodeVisitor
{
  public:

    ReplaceStaticTextureVisitor( const char* name,
                                 osg::Texture2D* new_texture ):
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
        _tex_name( osgDB::getSimpleFileName(name) ),
        _new_texture(new_texture)
    {}

    ReplaceStaticTextureVisitor( const SGPropertyNode* placement,
                                 osg::Texture2D* new_texture,
                                 osg::NodeCallback* cull_callback = 0 ):
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
        _tex_name( osgDB::getSimpleFileName(
          placement->getStringValue("texture"))
        ),
        _node_name( placement->getStringValue("node") ),
        _parent_name( placement->getStringValue("parent") ),
        _new_texture(new_texture),
        _cull_callback(cull_callback)
    {
      if(    _tex_name.empty()
          && _node_name.empty()
          && _parent_name.empty() )
        SG_LOG
        (
          SG_GL,
          SG_WARN,
          "No filter criterion for replacing texture. "
          " Every texture will be replaced!"
        );
    }

    /**
     * Get a list of groups which have been inserted into the scene graph to
     * replace the given texture
     */
    canvas::Placements& getPlacements()
    {
      return _placements;
    }

    virtual void apply(osg::Geode& node)
    {
      simgear::EffectGeode* eg = dynamic_cast<simgear::EffectGeode*>(&node);
      if( !eg )
        return;

      osg::StateSet* ss = eg->getEffect()->getDefaultStateSet();
      if( !ss )
        return;

      osg::Group *parent = node.getParent(0);
      if( !_node_name.empty() && parent->getName() != _node_name )
        return;

      if( !_parent_name.empty() )
      {
        // Traverse nodes upwards starting at the parent node (skip current
        // node)
        const osg::NodePath& np = getNodePath();
        bool found = false;
        for( int i = static_cast<int>(np.size()) - 2; i >= 0; --i )
        {
          const osg::Node* path_segment = np[i];
          const osg::Node* path_parent = path_segment->getParent(0);

          // A node without a name is always the parent of the root node of
          // the model just containing the file name
          if( path_parent && path_parent->getName().empty() )
            return;

          if( path_segment->getName() == _parent_name )
          {
            found = true;
            break;
          }
        }

        if( !found )
          return;
      }

      for( size_t unit = 0; unit < ss->getNumTextureAttributeLists(); ++unit )
      {
        osg::Texture2D* tex = dynamic_cast<osg::Texture2D*>
        (
          ss->getTextureAttribute(unit, osg::StateAttribute::TEXTURE)
        );

        if( !tex || !tex->getImage() || tex == _new_texture )
          continue;

        if( !_tex_name.empty() )
        {
          std::string tex_name = tex->getImage()->getFileName();
          std::string tex_name_simple = osgDB::getSimpleFileName(tex_name);
          if( !osgDB::equalCaseInsensitive(_tex_name, tex_name_simple) )
            continue;
        }

        // insert a new group between the geode an it's parent which overrides
        // the texture
        osg::ref_ptr<osg::Group> group = new osg::Group;
        group->setName("canvas texture group");
        group->addChild(eg);
        parent->removeChild(eg);
        parent->addChild(group);

        if( _cull_callback )
          group->setCullCallback(_cull_callback);

        _placements.push_back(
          canvas::PlacementPtr(new ObjectPlacement(group))
        );

        osg::StateSet* stateSet = group->getOrCreateStateSet();
        stateSet->setTextureAttribute( unit, _new_texture,
                                             osg::StateAttribute::OVERRIDE );
        stateSet->setTextureMode( unit, GL_TEXTURE_2D,
                                        osg::StateAttribute::ON );

        SG_LOG
        (
          SG_GL,
          SG_INFO,
             "Replaced texture '" << _tex_name << "'"
          << " for object '" << parent->getName() << "'"
          << (!_parent_name.empty() ? " with parent '" + _parent_name + "'"
                                    : "")
        );
        return;
      }
    }

  protected:

    class ObjectPlacement:
      public canvas::Placement
    {
      public:
        ObjectPlacement(osg::ref_ptr<osg::Group> group):
          _group(group)
        {}

        /**
         * Remove placement from the scene
         */
        virtual ~ObjectPlacement()
        {
          assert( _group->getNumChildren() == 1 );
          osg::Node *child = _group->getChild(0);

          if( _group->getNumParents() )
          {
            osg::Group *parent = _group->getParent(0);
            parent->addChild(child);
            parent->removeChild(_group);
          }

          _group->removeChild(child);
        }

      private:
        osg::ref_ptr<osg::Group> _group;
    };

    std::string _tex_name,      ///<! Name of texture to be replaced
                _node_name,     ///<! Only replace if node name matches
                _parent_name;   ///<! Only replace if any parent node matches
                                ///   given name (all the tree upwards)
    osg::Texture2D     *_new_texture;
    osg::NodeCallback  *_cull_callback;

    canvas::Placements _placements;
};

//------------------------------------------------------------------------------
canvas::Placements FGODGauge::set_texture( const char* name,
                                           osg::Texture2D* new_texture )
{
  osg::Group* root = globals->get_scenery()->get_aircraft_branch();
  ReplaceStaticTextureVisitor visitor(name, new_texture);
  root->accept(visitor);
  return visitor.getPlacements();
}

//------------------------------------------------------------------------------
canvas::Placements FGODGauge::set_texture( const SGPropertyNode* placement,
                                           osg::Texture2D* new_texture,
                                           osg::NodeCallback* cull_callback )
{
  osg::Group* root = globals->get_scenery()->get_aircraft_branch();
  ReplaceStaticTextureVisitor visitor(placement, new_texture, cull_callback);
  root->accept(visitor);
  return visitor.getPlacements();
}