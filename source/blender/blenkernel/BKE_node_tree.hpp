#pragma once

#include "DNA_node_types.h"

#include "BLI_string_ref.hpp"
#include "BLI_array_ref.hpp"
#include "BLI_map.hpp"
#include "BLI_vector.hpp"
#include "BLI_listbase_wrapper.hpp"
#include "BLI_multimap.hpp"
#include "BLI_monotonic_allocator.hpp"

#include "RNA_access.h"

namespace BKE {

using BLI::ArrayRef;
using BLI::ListBaseWrapper;
using BLI::Map;
using BLI::MonotonicAllocator;
using BLI::MultiMap;
using BLI::StringRef;
using BLI::StringRefNull;
using BLI::Vector;

using bNodeList = ListBaseWrapper<struct bNode *, true>;
using bLinkList = ListBaseWrapper<struct bNodeLink *, true>;
using bSocketList = ListBaseWrapper<struct bNodeSocket *, true>;

struct SocketWithNode {
  bNodeSocket *socket;
  bNode *node;
};

struct SingleOriginLink {
  bNodeSocket *from;
  bNodeSocket *to;
  bNodeLink *source_link;
};

/**
 * The DNA structure of a node tree is difficult to parse, since it does not support e.g. the
 * following queries efficiently:
 *   - Which nodes have a specific type?
 *   - Which node corresponds to a socket?
 *   - Which other sockets are connected to a socket (with and without reroutes)?
 *
 * This data structure does some preprocessing to make these queries more efficient. It is only
 * valid as long as the underlying node tree is not modified.
 */
class IndexedNodeTree {
 public:
  IndexedNodeTree(bNodeTree *btree);

  bNodeTree *btree() const
  {
    return m_btree;
  }

  ID *btree_id() const
  {
    return &m_btree->id;
  }

  PointerRNA rna(bNode *bnode) const
  {
    PointerRNA rna;
    RNA_pointer_create(this->btree_id(), &RNA_Node, bnode, &rna);
    return rna;
  }

  /**
   * Get all nodes that are in the btree->nodes list.
   */
  ArrayRef<bNode *> original_nodes() const
  {
    return m_original_nodes;
  }

  /**
   * Get all links that are in the btree->links list.
   */
  ArrayRef<bNodeLink *> original_links() const
  {
    return m_original_links;
  }

  /**
   * Get all nodes that are not reroutes or frames.
   */
  ArrayRef<bNode *> actual_nodes() const
  {
    return m_actual_nodes;
  }

  bNode *node_of_socket(bNodeSocket *bsocket) const
  {
    return m_node_by_socket.lookup(bsocket);
  }

  ArrayRef<SingleOriginLink> single_origin_links() const;
  ArrayRef<bNode *> nodes_with_idname(StringRef idname) const;
  ArrayRef<SocketWithNode> linked(bNodeSocket *bsocket) const;

 private:
  bool is_reroute(bNode *bnode) const;
  bool is_frame(bNode *bnode) const;

  void find_connected_sockets_left(bNodeSocket *bsocket, Vector<SocketWithNode> &r_sockets) const;
  void find_connected_sockets_right(bNodeSocket *bsocket, Vector<SocketWithNode> &r_sockets) const;

  bNodeTree *m_btree;
  Vector<bNode *> m_original_nodes;
  Vector<bNodeLink *> m_original_links;
  Vector<bNode *> m_actual_nodes;
  Map<bNodeSocket *, bNode *> m_node_by_socket;
  MultiMap<bNodeSocket *, SocketWithNode> m_direct_links;
  MultiMap<bNodeSocket *, SocketWithNode> m_links;
  MultiMap<std::string, bNode *> m_nodes_by_idname;
  Vector<SingleOriginLink> m_single_origin_links;
};

class VirtualNode;
class VirtualSocket;
class VirtualLink;

class VirtualNodeTree {
 private:
  bool m_frozen = false;
  Vector<VirtualNode *> m_nodes;
  Vector<VirtualLink *> m_links;
  Vector<VirtualSocket *> m_inputs_with_links;
  MultiMap<std::string, VirtualNode *> m_nodes_by_idname;
  MonotonicAllocator<> m_allocator;
 public:
  void add_all_of_tree(bNodeTree *btree);
  VirtualNode *add_bnode(bNodeTree *btree, bNode *bnode);
  void add_link(VirtualSocket *a, VirtualSocket *b);

  void freeze_and_index();

  ArrayRef<VirtualNode *> nodes()
  {
    return m_nodes;
  }

  ArrayRef<VirtualLink *> links()
  {
    return m_links;
  }

  ArrayRef<VirtualSocket *> inputs_with_links()
  {
    BLI_assert(m_frozen);
    return m_inputs_with_links;
  }

  ArrayRef<VirtualNode *> nodes_with_idname(StringRef idname)
  {
    BLI_assert(m_frozen);
    return m_nodes_by_idname.lookup_default(idname.to_std_string());
  }

  bool is_frozen()
  {
    return m_frozen;
  }

 private:
  void initialize_direct_links();
  void initialize_links();
  void initialize_nodes_by_idname();
};

class VirtualNode {
 private:
  friend VirtualNodeTree;
  friend VirtualSocket;

  VirtualNodeTree *m_backlink;
  bNodeTree *m_btree;
  bNode *m_bnode;
  ArrayRef<VirtualSocket *> m_inputs;
  ArrayRef<VirtualSocket *> m_outputs;

 public:
  ArrayRef<VirtualSocket *> inputs()
  {
    return m_inputs;
  }

  ArrayRef<VirtualSocket *> outputs()
  {
    return m_outputs;
  }

  VirtualSocket *input(uint index)
  {
    return m_inputs[index];
  }

  VirtualSocket *output(uint index)
  {
    return m_outputs[index];
  }

  bNode *bnode()
  {
    return m_bnode;
  }

  bNodeTree *btree()
  {
    return m_btree;
  }

  ID *btree_id()
  {
    return &m_btree->id;
  }

  PointerRNA rna()
  {
    PointerRNA rna;
    RNA_pointer_create(&m_btree->id, &RNA_Node, m_bnode, &rna);
    return rna;
  }

  const char *name()
  {
    return m_bnode->name;
  }

  const char *idname()
  {
    return m_bnode->idname;
  }
};

class VirtualSocket {
 private:
  friend VirtualNodeTree;

  VirtualNode *m_vnode;
  bNodeTree *m_btree;
  bNodeSocket *m_bsocket;

  ArrayRef<VirtualSocket *> m_direct_links;
  ArrayRef<VirtualSocket *> m_links;

 public:
  bool is_input() const
  {
    return this->m_bsocket->in_out == SOCK_IN;
  }

  bool is_output() const
  {
    return this->m_bsocket->in_out == SOCK_OUT;
  }

  bNodeSocket *bsocket()
  {
    return m_bsocket;
  }

  bNodeTree *btree()
  {
    return m_btree;
  }

  ID *btree_id()
  {
    return &m_btree->id;
  }

  VirtualNode *vnode()
  {
    return m_vnode;
  }

  ArrayRef<VirtualSocket *> direct_links()
  {
    BLI_assert(m_vnode->m_backlink->is_frozen());
    return m_direct_links;
  }

  ArrayRef<VirtualSocket *> links()
  {
    BLI_assert(m_vnode->m_backlink->is_frozen());
    return m_links;
  }

  PointerRNA rna()
  {
    PointerRNA rna;
    RNA_pointer_create(&m_btree->id, &RNA_NodeSocket, m_bsocket, &rna);
    return rna;
  }

  const char *name()
  {
    return m_bsocket->name;
  }
};

class VirtualLink {
 private:
  friend VirtualNodeTree;

  VirtualSocket *m_from;
  VirtualSocket *m_to;
};

}  // namespace BKE
