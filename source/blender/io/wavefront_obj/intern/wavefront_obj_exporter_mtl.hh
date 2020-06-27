
/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2008 Blender Foundation.
 * All rights reserved.
 */

/** \file
 * \ingroup obj
 */

#ifndef __WAVEFRONT_OBJ_EXPORTER_MTL_HH__
#define __WAVEFRONT_OBJ_EXPORTER_MTL_HH__

#include "BKE_node_tree_ref.hh"

#include "BLI_path_util.h"

#include "wavefront_obj_exporter_mesh.hh"

namespace io {
namespace obj {

class MTLWriter {
 public:
  MTLWriter(const char *obj_filepath)
  {
    BLI_strncpy(_mtl_filepath, obj_filepath, PATH_MAX);
    BLI_path_extension_replace(_mtl_filepath, PATH_MAX, ".mtl");
  }
  /** Append an object's material to the .mtl file. */
  void append_material(OBJMesh &mesh_to_export);

 private:
  FILE *_mtl_outfile;
  char _mtl_filepath[PATH_MAX];
  /** An object's material, to be exported. */
  Material *_export_mtl;

  /** First bsdf node encountered in the object's nodes. */
  bNode *_bsdf_node;
  void init_bsdf_node();

  /** Copy the float property from the bNode to given buffer. */
  void float_property_from_node(float *r_property, const bNode *curr_node, const char *identifier);
  /** Copy the float3 property from the bNode to given buffer.
   * Mostly used for color, without alpha.
   */
  void float3_property_from_node(float *r_property,
                                 const bNode *curr_node,
                                 const char *identifier);

  /**
   * Collect all the source sockets linked to the destination socket in a destination node.
   */
  void linked_sockets_to_dest_id(blender::Vector<const BKE::OutputSocketRef *> *r_linked_sockets,
                                 const bNode *dest_node,
                                 BKE::NodeTreeRef &node_tree,
                                 const char *dest_socket_id);

  /**
   * From a list of sockets, get the parent node which is of the given node type.
   */
  const bNode *linked_node_of_type(
      const blender::Vector<const BKE::OutputSocketRef *> &sockets_list, uint sh_node_type);
  /**
   * From a texture image shader node, get the image's filepath.
   * Filepath is the exact string the node contains, relative or absolute.
   */
  const char *get_image_filepath(const bNode *tex_node);
};

}  // namespace obj
}  // namespace io
#endif