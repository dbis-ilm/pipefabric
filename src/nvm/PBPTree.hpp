#ifndef PBPTree_hpp_
#define PBPTree_hpp_

#include <array>

#include "nvml/include/libpmemobj++/make_persistent.hpp"
#include "nvml/include/libpmemobj++/p.hpp"
#include "nvml/include/libpmemobj++/persistent_ptr.hpp"
#include "nvml/include/libpmemobj++/transaction.hpp"
#include "nvml/include/libpmemobj++/utils.hpp"

#define BRANCH_PADDING  0
#define LEAF_PADDING    0

namespace pfabric { namespace nvm {

using nvml::obj::delete_persistent;
using nvml::obj::make_persistent;
using nvml::obj::p;
using nvml::obj::persistent_ptr;
using nvml::obj::transaction;

//const std::string LAYOUT = "PBPTree";

/**
 * A persistent memory implementation of a B+ tree.
 *
 * @tparam KeyType the data type of the key
 * @tparam ValueType the data type of the values associated with the key
 * @tparam N the maximum number of keys on a branch node
 * @tparam M the maximum number of keys on a leaf node
 */
template <typename KeyType, typename ValueType, int N, int M>
class PBPTree {
  // we need at least two keys on a branch node to be able to split
  static_assert(N > 2, "number of branch keys has to be >2.");
  // we need at least one key on a leaf node
  static_assert(M > 0, "number of leaf keys should be >0.");
#ifndef UNIT_TESTS
 private:
#else
 public:
#endif

  // Forward declarations
  struct LeafNode;
  struct BranchNode;
  struct LeafOrBranchNode {
    LeafOrBranchNode() : tag(BLANK){};
    LeafOrBranchNode(persistent_ptr<LeafNode> leaf_) : tag(LEAF), leaf(leaf_) {};
    LeafOrBranchNode(persistent_ptr<BranchNode> branch_) : tag(BRANCH), branch(branch_) {};
    LeafOrBranchNode(const LeafOrBranchNode& other) { copy(other); };

    void copy (const LeafOrBranchNode& other) throw() {
//      NodeType tempTag = tag;
      tag = other.tag;
//      other.tag = tempTag;

      switch (tag) {
      case LEAF: {
//        persistent_ptr<LeafNode> tempLeaf(leaf);
        leaf = other.leaf;
//        other.leaf = tempLeaf;
        break;
      }
      case BRANCH: {
//        persistent_ptr<BranchNode> tempBranch(branch);
        branch = other.branch;
//        other.branch = tempBranch;
        break;
      }
      default: break;
      }
    }

    LeafOrBranchNode& operator=(LeafOrBranchNode other) {
      copy(other);
      return *this;
    }

    enum NodeType {BLANK, LEAF, BRANCH} tag;
    union {
      persistent_ptr<LeafNode> leaf;
      persistent_ptr<BranchNode> branch;
    };
  };

  /**
   * A structure for passing information about a node split to
   * the caller.
   */
  struct SplitInfo {
    KeyType key;                  //< the key at which the node was split
    LeafOrBranchNode leftChild;   //< the resulting lhs child node
    LeafOrBranchNode rightChild;  //< the resulting rhs child node
  };

  p<unsigned int> depth;          //< the depth of the tree, i.e. the number of levels (0 => rootNode is LeafNode)

  LeafOrBranchNode rootNode;      //< pointer to the root node (an instance of @c LeafNode or
                                  //< @c BranchNode). This pointer is never @c nullptr.

 public:
  /**
  * Iterator for iterating over the leaf nodes
  */
  class iterator {
    persistent_ptr<LeafNode> currentNode;
    std::size_t currentPosition;

    public:
    iterator() : currentNode(nullptr), currentPosition(0) {}
    iterator(const LeafOrBranchNode &root, std::size_t d) {
      // traverse to left-most key
      auto node = root;
      while (d-- > 0) {
        auto n = node.branch;
        node = n->children->at(0);
      }
      currentNode = node.leaf;
      currentPosition = 0;
    }

    iterator& operator++() {
      if (currentPosition >= currentNode->numKeys-1) {
        currentNode = currentNode->nextLeaf;
        currentPosition = 0;
      } else {
        currentPosition++;
      }
      return *this;
    }
    iterator operator++(int) {iterator retval = *this; ++(*this); return retval;}

    bool operator==(iterator other) const {return (currentNode == other.currentNode &&
                                                   currentPosition == other.currentPosition);}
    bool operator!=(iterator other) const {return !(*this == other);}

    std::pair<KeyType, ValueType> operator*() {
      return std::make_pair(currentNode->keys->at(currentPosition), currentNode->values->at(currentPosition));
    }

    // iterator traits
    using difference_type = long;
    using value_type = std::pair<KeyType, ValueType>;
    using pointer = const std::pair<KeyType, ValueType>*;
    using reference = const std::pair<KeyType, ValueType>&;
    using iterator_category = std::forward_iterator_tag;
  };
  iterator begin() { return iterator(rootNode, depth); }
  iterator end() { return iterator(); }

  /**
   * Typedef for a function passed to the scan method.
   */
  typedef std::function<void(const KeyType &key, const ValueType &val)>
      ScanFunc;

  /**
   * Constructor for creating a new B+ tree.
   */
  PBPTree() : depth(0) {
    rootNode = newLeafNode();
    std::cout << "sizeof(BranchNode) = " << sizeof(BranchNode)
      << ", sizeof(LeafNode) = " << sizeof(LeafNode) << std::endl;
  }

  /**
   * Destructor for the B+ tree. Should delete all allocated nodes.
   */
  ~PBPTree() {
    //TODO: Necessary in Pmem case?
    // Nodes are deleted automatically by releasing leafPool and branchPool.
  }

  /**
   * Insert an element (a key-value pair) into the B+ tree. If the key @c key
   * already exists, the corresponding value is replaced by @c val.
   *
   * @param key the key of the element to be inserted
   * @param val the value that is associated with the key
   */
  void insert(const KeyType &key, const ValueType &val) {
    auto pop = nvml::obj::pool_by_vptr(this);
    transaction::exec_tx(pop, [&] {
        SplitInfo splitInfo;

        bool wasSplit = false;
        if (depth == 0) {
          // the root node is a leaf node
          auto n = rootNode.leaf;
          wasSplit = insertInLeafNode(n, key, val, &splitInfo);
        } else {
          // the root node is a branch node
          auto n = rootNode.branch;
          wasSplit = insertInBranchNode(n, depth, key, val, &splitInfo);
        }
        if (wasSplit) {
          // we had an overflow in the node and therefore the node is split
          auto root = newBranchNode();
          root->keys->at(0) = splitInfo.key;
          root->children->at(0) = splitInfo.leftChild;
          root->children->at(1) = splitInfo.rightChild;
          root->numKeys = root->numKeys + 1;
          rootNode.branch = root;
          depth = depth + 1;
        }
   });
  }

  /**
   * Find the given @c key in the B+ tree and if found return the
   * corresponding value.
   *
   * @param key the key we are looking for
   * @param[out] val a pointer to memory where the value is stored
   *                 if the key was found
   * @return true if the key was found, false otherwise
   */
  bool lookup(const KeyType &key, ValueType *val) const {
    assert(val != nullptr);
    bool result = false;

    auto leafNode = findLeafNode(key);
    auto pos = lookupPositionInLeafNode(leafNode, key);
    if (pos < leafNode->numKeys && leafNode->keys->at(pos) == key) {
      // we found it!
      *val = leafNode->values->at(pos);
      result = true;
    }
    return result;
  }

  /**
   * Delete the entry with the given key @c key from the tree.
   *
   * @param key the key of the entry to be deleted
   * @return true if the key was found and deleted
   */
  bool erase(const KeyType &key) {
    if (depth == 0) {
      // special case: the root node is a leaf node and
      // there is no need to handle underflow
      auto node = rootNode.leaf;
      assert(node != nullptr);
      return eraseFromLeafNode(node, key);
    } else {
      auto node = rootNode.branch;
      assert(node != nullptr);
      return eraseFromBranchNode(node, depth, key);
    }
  }

  /**
   * Print the structure and content of the B+ tree to stdout.
   */
  void print() const {
    if (depth == 0) {
      // the trivial case
      printLeafNode(0, rootNode.leaf);
    } else {
      auto n = rootNode;
      printBranchNode(0u, n.branch);
    }
  }

  /**
   * Perform a scan over all key-value pairs stored in the B+ tree.
   * For each entry the given function @func is called.
   *
   * @param func the function called for each entry
   */
  void scan(ScanFunc func) const {
    // we traverse to the leftmost leaf node
    auto node = rootNode;
    auto d = depth.get_ro();
    while ( d-- > 0) {
      // as long as we aren't at the leaf level we follow the path down
      auto n = node.branch;
      node = n->children->at(0);
    }
    auto leaf = node.leaf;
    while (leaf != nullptr) {
      // for each key-value pair call func
      for (auto i = 0u; i < leaf->numKeys; i++) {
        auto &key = leaf->keys->at(i);
        auto &val = leaf->values->at(i);
        func(key, val);
      }
      // move to the next leaf node
      leaf = leaf->nextLeaf;
    }
  }

  /**
   * Perform a range scan over all elements within the range [minKey, maxKey]
   * and for each element call the given function @c func.
   *
   * @param minKey the lower boundary of the range
   * @param maxKey the upper boundary of the range
   * @param func the function called for each entry
   */
  void scan(const KeyType &minKey, const KeyType &maxKey, ScanFunc func) const {
    auto leaf = findLeafNode(minKey);

    while (leaf != nullptr) {
      // for each key-value pair within the range call func
      for (auto i = 0u; i < leaf->numKeys; i++) {
        auto &key = leaf->keys->at(i);
        if (key > maxKey) return;

        auto &val = leaf->values->at(i);
        func(key, val);
      }
      // move to the next leaf node
      leaf = leaf->nextLeaf;
    }
  }

#ifndef UNIT_TESTS
  private:
#endif
  /* ------------------------------------------------------------------- */
  /*                        DELETE AT LEAF LEVEL                         */
  /* ------------------------------------------------------------------- */

  /**
   * Delete the element with the given key from the given leaf node.
   *
   * @param node the leaf node from which the element is deleted
   * @param key the key of the element to be deleted
   * @return true of the element was deleted
   */
  bool eraseFromLeafNode(persistent_ptr<LeafNode> node, const KeyType &key) {
    bool deleted = false;
    auto pos = lookupPositionInLeafNode(node, key);
    if (node->keys->at(pos) == key) {
      for (auto i = pos; i < node->numKeys - 1; i++) {
        node->keys->at(i) = node->keys->at(i + 1);
        node->values->at(i) = node->values->at(i + 1);
      }
      node->numKeys--;
      deleted = true;
    }
    return deleted;
  }

  /**
   * Handle the case that during a delete operation a underflow at node @c leaf
   * occured. If possible this is handled
   * (1) by rebalancing the elements among the leaf node and one of its siblings
   * (2) if not possible by merging with one of its siblings.
   *
   * @param node the parent node of the node where the underflow occured
   * @param pos the position of the child node @leaf in the @c children array of
   * the branch node
   * @param leaf the node at which the underflow occured
   */
  void underflowAtLeafLevel(persistent_ptr<BranchNode> node, unsigned int pos, persistent_ptr<LeafNode> leaf) {
    assert(pos <= node->numKeys);

    unsigned int middle = (M + 1) / 2;
    // 1. we check whether we can rebalance with one of the siblings
    // but only if both nodes have the same direct parent
    if (pos > 0 && leaf->prevLeaf->numKeys > middle) {
      // we have a sibling at the left for rebalancing the keys
      balanceLeafNodes(leaf->prevLeaf, leaf);
      node->keys->at(pos) = leaf->keys->at(0);
    } else if (pos < node->numKeys && leaf->nextLeaf->numKeys > middle) {
      // we have a sibling at the right for rebalancing the keys
      balanceLeafNodes(leaf->nextLeaf, leaf);
      node->keys->at(pos) = leaf->nextLeaf->keys->at(0);
    } else {
      // 2. if this fails we have to merge two leaf nodes
      // but only if both nodes have the same direct parent
      persistent_ptr<LeafNode> survivor = nullptr;
      if (pos > 0 && leaf->prevLeaf->numKeys <= middle) {
        survivor = mergeLeafNodes(leaf->prevLeaf, leaf);
        deleteLeafNode(leaf);
      } else if (pos < node->numKeys && leaf->nextLeaf->numKeys <= middle) {
        // because we update the pointers in mergeLeafNodes
        // we keep it here
        auto l = leaf->nextLeaf;
        survivor = mergeLeafNodes(leaf, leaf->nextLeaf);
        deleteLeafNode(l);
      } else {
        // this shouldn't happen?!
        assert(false);
      }
      if (node->numKeys > 1) {
        if (pos > 0) pos--;
        // just remove the child node from the current branch node
        for (auto i = pos; i < node->numKeys - 1; i++) {
          node->keys->at(i) = node->keys->at(i + 1);
          node->children->at(i + 1) = node->children->at(i + 2);
        }
        node->children->at(pos) = survivor;
        node->numKeys--;
      } else {
        // This is a special case that happens only if
        // the current node is the root node. Now, we have
        // to replace the branch root node by a leaf node.
        rootNode = survivor;
        depth = depth - 1;
      }
    }
  }

  /**
   * Merge two leaf nodes by moving all elements from @c node2 to
   * @c node1.
   *
   * @param node1 the target node of the merge
   * @param node2 the source node
   * @return the merged node (always @c node1)
   */
  persistent_ptr<LeafNode> mergeLeafNodes(persistent_ptr<LeafNode> node1, persistent_ptr<LeafNode> node2) {
    assert(node1 != nullptr);
    assert(node2 != nullptr);
    assert(node1->numKeys + node2->numKeys <= M);

    // we move all keys/values from node2 to node1
    for (auto i = 0u; i < node2->numKeys; i++) {
      node1->keys->at(node1->numKeys + i) = node2->keys->at(i);
      node1->values->at(node1->numKeys + i) = node2->values->at(i);
    }
    node1->numKeys += node2->numKeys;
    node1->nextLeaf = node2->nextLeaf;
    node2->numKeys = 0;
    if (node2->nextLeaf != nullptr) node2->nextLeaf->prevLeaf = node1;
    return node1;
  }

  /**
   * Redistribute (key, value) pairs from the leaf node @c donor to
   * the leaf node @c receiver such that both nodes have approx. the same
   * number of elements. This method is used in case of an underflow
   * situation of a leaf node.
   *
   * @param donor the leaf node from which the elements are taken
   * @param receiver the sibling leaf node getting the elements from @c donor
   */
  void balanceLeafNodes(persistent_ptr<LeafNode> donor, persistent_ptr<LeafNode> receiver) {
    assert(donor->numKeys > receiver->numKeys);

    unsigned int balancedNum = (donor->numKeys + receiver->numKeys) / 2;
    unsigned int toMove = donor->numKeys - balancedNum;
    if (toMove == 0) return;

    if (donor->keys->at(0) < receiver->keys->at(0)) {
      // move from one node to a node with larger keys
      unsigned int i = 0, j = 0;
      for (i = receiver->numKeys; i > 0; i--) {
        // reserve space on receiver side
        receiver->keys->at(i + toMove - 1) = receiver->keys->at(i - 1);
        receiver->values->at(i + toMove - 1) = receiver->values->at(i - 1);
      }
      // move toMove keys/values from donor to receiver
      for (i = balancedNum; i < donor->numKeys; i++, j++) {
        receiver->keys->at(j) = donor->keys->at(i);
        receiver->values->at(j) = donor->values->at(i);
        receiver->numKeys++;
      }
    } else {
      // mode from one node to a node with smaller keys
      unsigned int i = 0;
      // move toMove keys/values from donor to receiver
      for (i = 0; i < toMove; i++) {
        receiver->keys->at(receiver->numKeys) = donor->keys->at(i);
        receiver->values->at(receiver->numKeys) = donor->values->at(i);
        receiver->numKeys++;
      }
      // on donor node move all keys and values to the left
      for (i = 0; i < donor->numKeys - toMove; i++) {
        donor->keys->at(i) = donor->keys->at(toMove + i);
        donor->values->at(i) = donor->values->at(toMove + i);
      }
    }
    donor->numKeys -= toMove;
  }

  /* ------------------------------------------------------------------- */
  /*                        DELETE AT INNER LEVEL                        */
  /* ------------------------------------------------------------------- */
  /**
   * Delete an entry from the tree by recursively going down to the leaf level
   * and handling the underflows.
   *
   * @param node the current branch node
   * @param d the current depth of the traversal
   * @param key the key to be deleted
   * @return true if the entry was deleted
   */
  bool eraseFromBranchNode(persistent_ptr<BranchNode> node, unsigned int d, const KeyType &key) {
    assert(d >= 1);
    bool deleted = false;
    // try to find the branch
    auto pos = lookupPositionInBranchNode(node, key);
    auto n = node->children->at(pos);
    if (d == 1) {
      // the next level is the leaf level
      auto leaf = n.leaf;
      assert(leaf != nullptr);
      deleted = eraseFromLeafNode(leaf, key);
      unsigned int middle = (M + 1) / 2;
      if (leaf->numKeys < middle) {
        // handle underflow
        underflowAtLeafLevel(node, pos, leaf);
      }
    } else {
      auto child = n.branch;
      deleted = eraseFromBranchNode(child, d - 1, key);

      pos = lookupPositionInBranchNode(node, key);
      unsigned int middle = (N + 1) / 2;
      if (child->numKeys < middle) {
        // handle underflow
        child = underflowAtBranchLevel(node, pos, child);
        if (d == depth && node->numKeys == 0) {
          // special case: the root node is empty now
          rootNode = child;
          depth = depth - 1;
        }
      }
    }
    return deleted;
  }

  /**
   * Merge two branch nodes my moving all keys/children from @c node to @c
   * sibling and put the key @c key from the parent node in the middle. The node
   * @c node should be deleted by the caller.
   *
   * @param sibling the left sibling node which receives all keys/children
   * @param key the key from the parent node that is between sibling and node
   * @param node the node from which we move all keys/children
   */
  void mergeBranchNodes(persistent_ptr<BranchNode> sibling, const KeyType &key,
                       persistent_ptr<BranchNode> node) {
    assert(key <= node->keys->at(0));
    assert(sibling != nullptr);
    assert(node != nullptr);
    assert(sibling->keys->at(sibling->numKeys - 1) < key);

    sibling->keys->at(sibling->numKeys) = key;
    sibling->children->at(sibling->numKeys + 1) = node->children->at(0);
    for (auto i = 0u; i < node->numKeys; i++) {
      sibling->keys->at(sibling->numKeys + i + 1) = node->keys->at(i);
      sibling->children->at(sibling->numKeys + i + 2) = node->children->at(i + 1);
    }
    sibling->numKeys += node->numKeys + 1;
  }

  /**
   * Handle the case that during a delete operation a underflow at node @c child
   * occured where @c node is the parent node. If possible this is handled
   * (1) by rebalancing the elements among the node @c child and one of its
   * siblings
   * (2) if not possible by merging with one of its siblings.
   *
   * @param node the parent node of the node where the underflow occured
   * @param pos the position of the child node @child in the @c children array
   * of the branch node
   * @param child the node at which the underflow occured
   * @return the (possibly new) child node (in case of a merge)
   */
  persistent_ptr<BranchNode> underflowAtBranchLevel(persistent_ptr<BranchNode> node, unsigned int pos,
                             persistent_ptr<BranchNode> child) {
    assert(node != nullptr);
    assert(child != nullptr);

    persistent_ptr<BranchNode> newChild = child;
    unsigned int middle = (N + 1) / 2;
    // 1. we check whether we can rebalance with one of the siblings
    if (pos > 0 &&
        (node->children->at(pos - 1)).branch->numKeys >
            middle) {
      // we have a sibling at the left for rebalancing the keys
      persistent_ptr<BranchNode> sibling = (node->children->at(pos - 1)).branch;
      balanceBranchNodes(sibling, child, node, pos - 1);
      // node->keys->at(pos) = child->keys->at(0);
      return newChild;
    } else if (pos < node->numKeys && (node->children->at(pos + 1)).branch->numKeys > middle) {
      // we have a sibling at the right for rebalancing the keys
      auto sibling = (node->children->at(pos + 1)).branch;
      balanceBranchNodes(sibling, child, node, pos);
      return newChild;
    } else {
      // 2. if this fails we have to merge two branch nodes
      persistent_ptr<BranchNode> lSibling = nullptr, rSibling = nullptr;
      unsigned int prevKeys = 0, nextKeys = 0;

      if (pos > 0) {
        lSibling = (node->children->at(pos - 1)).branch;
        prevKeys = lSibling->numKeys;
      }
      if (pos < node->numKeys) {
        rSibling = (node->children->at(pos + 1)).branch;
        nextKeys = rSibling->numKeys;
      }

      persistent_ptr<BranchNode> witnessNode = nullptr;
      auto ppos = pos;
      if (prevKeys > 0) {
        mergeBranchNodes(lSibling, node->keys->at(pos - 1), child);
        ppos = pos - 1;
        witnessNode = child;
        newChild = lSibling;
        // pos -= 1;
      } else if (nextKeys > 0) {
        mergeBranchNodes(child, node->keys->at(pos), rSibling);
        witnessNode = rSibling;
      } else
        // shouldn't happen
        assert(false);

      // remove node->keys->at(pos) from node
      for (auto i = ppos; i < node->numKeys - 1; i++) {
        node->keys->at(i) = node->keys->at(i + 1);
      }
      if (pos == 0) pos++;
      for (auto i = pos; i < node->numKeys; i++) {
        if (i + 1 <= node->numKeys)
          node->children->at(i) = node->children->at(i + 1);
      }
      node->numKeys--;

      deleteBranchNode(witnessNode);
      return newChild;
    }
  }

  /* ---------------------------------------------------------------------- */
  /**
   * Rebalance two branch nodes by moving some key-children pairs from the node
   * @c donor to the node @receiver via the parent node @parent. The position of
   * the key between the two nodes is denoted by @c pos.
   *
   * @param donor the branch node from which the elements are taken
   * @param receiver the sibling branch node getting the elements from @c donor
   * @param parent the parent node of @c donor and @c receiver
   * @param pos the position of the key in node @c parent that lies between
   *      @c donor and @c receiver
   */
  void balanceBranchNodes(persistent_ptr<BranchNode> donor, persistent_ptr<BranchNode> receiver,
                         persistent_ptr<BranchNode> parent, unsigned int pos) {
    assert(donor->numKeys > receiver->numKeys);

    unsigned int balancedNum = (donor->numKeys + receiver->numKeys) / 2;
    unsigned int toMove = donor->numKeys - balancedNum;
    if (toMove == 0) return;

    if (donor->keys->at(0) < receiver->keys->at(0)) {
      // move from one node to a node with larger keys
      unsigned int i = 0;

      // 1. make room
      receiver->children->at(receiver->numKeys + toMove) =
          receiver->children->at(receiver->numKeys);
      for (i = receiver->numKeys; i > 0; i--) {
        // reserve space on receiver side
        receiver->keys->at(i + toMove - 1) = receiver->keys->at(i - 1);
        receiver->children->at(i + toMove - 1) = receiver->children->at(i - 1);
      }
      // 2. move toMove keys/children from donor to receiver
      for (i = 0; i < toMove; i++) {
        receiver->children->at(i) =
            donor->children->at(donor->numKeys - toMove + 1 + i);
      }
      for (i = 0; i < toMove - 1; i++) {
        receiver->keys->at(i) = donor->keys->at(donor->numKeys - toMove + 1 + i);
      }
      receiver->keys->at(toMove - 1) = parent->keys->at(pos);
      assert(parent->numKeys > pos);
      parent->keys->at(pos) = donor->keys->at(donor->numKeys - toMove);
      receiver->numKeys += toMove;
    } else {
      // mode from one node to a node with smaller keys
      unsigned int i = 0, n = receiver->numKeys;

      // 1. move toMove keys/children from donor to receiver
      for (i = 0; i < toMove; i++) {
        receiver->children->at(n + 1 + i) = donor->children->at(i);
        receiver->keys->at(n + 1 + i) = donor->keys->at(i);
      }
      // 2. we have to move via the parent node: take the key from
      // parent->keys->at(pos)
      receiver->keys->at(n) = parent->keys->at(pos);
      receiver->numKeys += toMove;
      KeyType key = donor->keys->at(toMove - 1);

      // 3. on donor node move all keys and values to the left
      for (i = 0; i < donor->numKeys - toMove; i++) {
        donor->keys->at(i) = donor->keys->at(toMove + i);
        donor->children->at(i) = donor->children->at(toMove + i);
      }
      donor->children->at(donor->numKeys - toMove) =
          donor->children->at(donor->numKeys);
      // and replace this key by donor->keys->at(0)
      assert(parent->numKeys > pos);
      parent->keys->at(pos) = key;
    }
    donor->numKeys -= toMove;
  }

  /* ---------------------------------------------------------------------- */
  /*                                   DEBUGGING                            */
  /* ---------------------------------------------------------------------- */

  /**
   * Print the given branch node @c node and all its children
   * to standard output.
   *
   * @param d the current depth used for indention
   * @param node the tree node to print
   */
  void printBranchNode(unsigned int d, persistent_ptr<BranchNode> node) const {
    for (auto i = 0u; i < d; i++) std::cout << "  ";
    std::cout << d << " { ";
    for (auto k = 0u; k < node->numKeys; k++) {
      if (k > 0) std::cout << ", ";
      std::cout << node->keys->at(k);
    }
    std::cout << " }" << std::endl;
    for (auto k = 0u; k <= node->numKeys; k++) {
      if (d + 1 < depth) {
        auto child = node->children->at(k).branch;
        if (child != nullptr) printBranchNode(d + 1, child);
      } else {
        auto leaf = (node->children->at(k)).leaf;
        printLeafNode(d + 1, leaf);
      }
    }
  }

  /**
   * Print the keys of the given branch node @c node to standard
   * output.
   *
   * @param node the tree node to print
   */
  void printBranchNodeKeys(persistent_ptr<BranchNode> node) const {
    std::cout << "{ ";
    for (auto k = 0u; k < node->numKeys; k++) {
      if (k > 0) std::cout << ", ";
      std::cout << node->keys->at(k);
    }
    std::cout << " }" << std::endl;
  }

  /**
   * Print the given leaf node @c node to standard output.
   *
   * @param d the current depth used for indention
   * @param node the tree node to print
   */
  void printLeafNode(unsigned int d, persistent_ptr<LeafNode> node) const {
    for (auto i = 0u; i < d; i++) std::cout << "  ";
    std::cout << "[" << std::hex << node << std::dec << " : ";
    for (auto i = 0u; i < node->numKeys; i++) {
      if (i > 0) std::cout << ", ";
      std::cout << "{" << node->keys->at(i) << " -> " << node->values->at(i) << "}";
    }
    std::cout << "]" << std::endl;
  }

  /* ---------------------------------------------------------------------- */
  /*                                   INSERT                               */
  /* ---------------------------------------------------------------------- */

  /**
   * Insert a (key, value) pair into the corresponding leaf node. It is the
   * responsibility of the caller to make sure that the node @c node is
   * the correct node. The key is inserted at the correct position.
   *
   * @param node the node where the key-value pair is inserted.
   * @param key the key to be inserted
   * @param val the value associated with the key
   * @param splitInfo information about a possible split of the node
   */
  bool insertInLeafNode(persistent_ptr<LeafNode> node, const KeyType &key,
                        const ValueType &val, SplitInfo *splitInfo) {
    bool split = false;
    auto pos = lookupPositionInLeafNode(node, key);
    if (pos < node->numKeys && node->keys->at(pos) == key) {
      // handle insert of duplicates
      node->values->at(pos) = val;
      return false;
    }
    if (node->numKeys == M) {
      // the node is full, so we must split it
      // determine the split position
      unsigned int middle = (M + 1) / 2;
      // move all entries behind this position to a new sibling node
      persistent_ptr<LeafNode> sibling = newLeafNode();
      sibling->numKeys = node->numKeys - middle;
      for (auto i = 0u; i < sibling->numKeys; i++) {
        sibling->keys->at(i) = node->keys->at(i + middle);
        sibling->values->at(i) = node->values->at(i + middle);
      }
      node->numKeys = middle;

      // insert the new entry
      if (pos < middle)
        insertInLeafNodeAtPosition(node, pos, key, val);
      else
        insertInLeafNodeAtPosition(sibling, pos - middle, key, val);

      // setup the list of leaf nodes
      node->nextLeaf = sibling;
      sibling->prevLeaf = node;

      // and inform the caller about the split
      split = true;
      splitInfo->leftChild = node;
      splitInfo->rightChild = sibling;
      splitInfo->key = sibling->keys->at(0);
    } else {
      // otherwise, we can simply insert the new entry at the given position
      insertInLeafNodeAtPosition(node, pos, key, val);
    }
    return split;
  }

  /**
   * Insert a (key, value) pair at the given position @c pos into the leaf node
   * @c node. The caller has to ensure that
   * - there is enough space to insert the element
   * - the key is inserted at the correct position according to the order of
   * keys
   *
   * @oaram node the leaf node where the element is to be inserted
   * @param pos the position in the leaf node (0 <= pos <= numKeys < M)
   * @param key the key of the element
   * @param val the actual value corresponding to the key
   */
  void insertInLeafNodeAtPosition(persistent_ptr<LeafNode> node, unsigned int pos,
                                  const KeyType &key, const ValueType &val) {
    assert(pos < M);
    assert(pos <= node->numKeys);
    assert(node->numKeys < M);
    // we move all entries behind pos by one position
    for (unsigned int i = node->numKeys; i > pos; i--) {
      node->keys->at(i) = node->keys->at(i - 1);
      node->values->at(i) = node->values->at(i - 1);
    }
    // and then insert the new entry at the given position
    node->keys->at(pos) = key;
    node->values->at(pos) = val;
    node->numKeys = node->numKeys + 1;
  }

  /**
   * Split the given branch node @c node in the middle and move
   * half of the keys/children to the new sibling node.
   *
   * @param node the branch node to be split
   * @param splitKey the key on which the split of the child occured
   * @param splitInfo information about the split
   */
  void splitBranchNode(persistent_ptr<BranchNode> node, const KeyType &splitKey,
                      SplitInfo *splitInfo) {
    // we have an overflow at the branch node, let's split it
    // determine the split position
    unsigned int middle = (N + 1) / 2;
    // adjust the middle based on the key we have to insert
    if (splitKey > node->keys->at(middle)) middle++;
    // move all entries behind this position to a new sibling node
    persistent_ptr<BranchNode> sibling = newBranchNode();
    sibling->numKeys = node->numKeys - middle;
    for (auto i = 0u; i < sibling->numKeys; i++) {
      sibling->keys->at(i) = node->keys->at(middle + i);
      sibling->children->at(i) = node->children->at(middle + i);
    }
    sibling->children->at(sibling->numKeys) = node->children->at(node->numKeys);
    node->numKeys = middle - 1;

    splitInfo->key = node->keys->at(middle - 1);
    splitInfo->leftChild = node;
    splitInfo->rightChild = sibling;
  }

  /**
   * Insert a (key, value) pair into the tree recursively by following the path
   * down to the leaf level starting at node @c node at depth @c depth.
   *
   * @param node the starting node for the insert
   * @param depth the current depth of the tree (0 == leaf level)
   * @param key the key of the element
   * @param val the actual value corresponding to the key
   * @param splitInfo information about the split
   * @return true if a split was performed
   */
  bool insertInBranchNode(persistent_ptr<BranchNode> node, unsigned int depth,
                         const KeyType &key, const ValueType &val,
                         SplitInfo *splitInfo) {
    SplitInfo childSplitInfo;
    bool split = false, hasSplit = false;

    auto pos = lookupPositionInBranchNode(node, key);
    if (depth - 1 == 0) {
      // case #1: our children are leaf nodes
      auto child = node->children->at(pos).leaf;
      hasSplit = insertInLeafNode(child, key, val, &childSplitInfo);
    } else {
      // case #2: our children are branch nodes
      auto child = node->children->at(pos).branch;
      hasSplit = insertInBranchNode(child, depth - 1, key, val, &childSplitInfo);
    }
    if (hasSplit) {
      persistent_ptr<BranchNode> host = node;
      // the child node was split, thus we have to add a new entry
      // to our branch node

      if (node->numKeys == N) {
        splitBranchNode(node, childSplitInfo.key, splitInfo);

        host = (key < splitInfo->key ? splitInfo->leftChild
                                     : splitInfo->rightChild).branch;
        split = true;
        pos = lookupPositionInBranchNode(host, key);
      }
      if (pos < host->numKeys) {
        // if the child isn't inserted at the rightmost position
        // then we have to make space for it
        host->children->at(host->numKeys + 1) = host->children->at(host->numKeys);
        for (auto i = host->numKeys.get_ro(); i > pos; i--) {
          host->children->at(i) = host->children->at(i - 1);
          host->keys->at(i) = host->keys->at(i - 1);
        }
      }
      // finally, add the new entry at the given position
      host->keys->at(pos) = childSplitInfo.key;
      host->children->at(pos) = childSplitInfo.leftChild;
      host->children->at(pos + 1) = childSplitInfo.rightChild;
      host->numKeys = host->numKeys + 1;
    }
    return split;
  }

  /* ---------------------------------------------------------------------- */
  /*                                   LOOKUP                               */
  /* ---------------------------------------------------------------------- */

  /**
   * Traverse the tree starting at the root until the leaf node is found that
   * could contain the given @key. Note, that always a leaf node is returned
   * even if the key doesn't exist on this node.
   *
   * @param key the key we are looking for
   * @return the leaf node that would store the key
   */
  persistent_ptr<LeafNode> findLeafNode(const KeyType &key) const {
    auto node = rootNode;
    auto d = depth.get_ro();
    while (d-- > 0) {
      // as long as we aren't at the leaf level we follow the path down
      auto n = node.branch;
      auto pos = lookupPositionInBranchNode(n, key);
      node = n->children->at(pos);
    }
    return node.leaf;
  }

  /**
   * Lookup the search key @c key in the given branch node and return the
   * position which is the position in the list of keys + 1. in this way, the
   * position corresponds to the position of the child pointer in the
   * array @children.
   * If the search key is less than the smallest key, then @c 0 is returned.
   * If the key is greater than the largest key, then @c numKeys is returned.
   *
   * @param node the branch node where we search
   * @param key the search key
   * @return the position of the key + 1 (or 0 or @c numKey)
   */
  unsigned int lookupPositionInBranchNode(persistent_ptr<BranchNode> node,
                                         const KeyType &key) const {
    // we perform a simple linear search, perhaps we should try a binary
    // search instead?
    unsigned int pos = 0;
    const unsigned int num = node->numKeys;
    for (; pos < num && node->keys->at(pos) <= key; pos++)
      ;
    return pos;
  }

  /**
   * Lookup the search key @c key in the given leaf node and return the
   * position.
   * If the search key was not found, then @c numKeys is returned.
   *
   * @param node the leaf node where we search
   * @param key the search key
   * @return the position of the key  (or @c numKey if not found)
   */
  unsigned int lookupPositionInLeafNode(persistent_ptr<LeafNode> node,
                                        const KeyType &key) const {
    // we perform a simple linear search, perhaps we should try a binary
    // search instead?
    unsigned int pos = 0;
    const unsigned int num = node->numKeys.get_ro();
    for (; pos < num && node->keys->at(pos) < key; pos++)
      ;
    return pos;
  }

  /* ---------------------------------------------------------------------- */

  /**
   * Create a new empty leaf node
   */
  persistent_ptr<LeafNode> newLeafNode() {
    auto pop = nvml::obj::pool_by_vptr(this);
    persistent_ptr<LeafNode> newNode = nullptr;
    transaction::exec_tx(pop, [&] {
      newNode = make_persistent<LeafNode>();
    });
    return newNode;
  }

  void deleteLeafNode(persistent_ptr<LeafNode> node) {
    auto pop = nvml::obj::pool_by_vptr(this);
    transaction::exec_tx(pop, [&] {
      delete_persistent<LeafNode>(node);
    });
  }

  /**
   * Create a new empty branch node
   */
  persistent_ptr<BranchNode> newBranchNode() {
    auto pop = nvml::obj::pool_by_vptr(this);
    persistent_ptr<BranchNode> newNode = nullptr;
    transaction::exec_tx(pop, [&] {
      newNode = make_persistent<BranchNode>();
    });
    return newNode;
  }

  void deleteBranchNode(persistent_ptr<BranchNode> node) {
    auto pop = nvml::obj::pool_by_vptr(this);
    transaction::exec_tx(pop, [&] {
      delete_persistent<BranchNode>(node);
    });
  }
  /* -----------------------------------------------------------------------
   */

  /**
   * A structure for representing a leaf node of a B+ tree.
   */
  struct LeafNode {
    /**
     * Constructor for creating a new empty leaf node.
     */
    LeafNode() : numKeys(0), nextLeaf(nullptr), prevLeaf(nullptr) {
      auto pop = nvml::obj::pool_by_vptr(this);
      transaction::exec_tx(pop, [&] {
        keys = make_persistent<std::array<KeyType, M>>();
        values = make_persistent<std::array<ValueType, M>>();
      });
    }
   // ~LeafNode() { std::cout << "~LeafNode: " << std::hex << this <<
   //    std::endl; }

    p<unsigned int> numKeys;                         //< the number of currently stored keys
    persistent_ptr<std::array<KeyType, M>> keys;     //< the actual keys
    persistent_ptr<std::array<ValueType, M>> values; //< the actual values
    persistent_ptr<LeafNode> nextLeaf;               //< pointer to the subsequent sibling
    persistent_ptr<LeafNode> prevLeaf;               //< pointer to the preceeding sibling
    p<unsigned char> pad_[LEAF_PADDING];            //<
  };

  /**
   * A structure for representing an branch node (branch node) of a B+ tree.
   */
  struct BranchNode {
    /**
     * Constructor for creating a new empty branch node.
     */
    BranchNode() : numKeys(0) {
      auto pop = nvml::obj::pool_by_vptr(this);
      transaction::exec_tx(pop, [&] {
        keys = make_persistent<std::array<KeyType, N>>();
        children = make_persistent<std::array<LeafOrBranchNode, N+1>>();
      });
    }
    // ~BranchNode() { std::cout << "~BranchNode: " << std::hex << this << std::dec <<
     //   std::endl; }

    p<unsigned int> numKeys;                     //< the number of currently stored keys
    persistent_ptr<std::array<KeyType, N>> keys; //< the actual keys
    persistent_ptr<std::array<LeafOrBranchNode, N + 1>>
        children;                                //< pointers to child nodes (BranchNode or LeafNode)
    p<unsigned char> pad_[BRANCH_PADDING];            //<
  };

}; /* end class PBPTree */

}} /* namespace pfabric::nvm */

/* POLYMORPHIC TYPES ARE NOT SUPPORTED BY PMEM
 *
template <typename KeyType, typename ValueType, int N, int M>
class PBPTree : public PBPTree_Base<KeyType, ValueType, N, M> {};

template <typename KeyType, int N, int M>
class PBPTree<KeyType, std::string, N, M> : public PBPTree_Base<KeyType, std::string, N, M> {
private:
  using LeafNode = typename PBPTree_Base<KeyType, std::string, N, M>::LeafNode;
  using SplitInfo = typename PBPTree_Base<KeyType, std::string, N, M>::SplitInfo;

  bool insertInLeafNode(persistent_ptr<LeafNode> node, const KeyType &key,
                        const std::string &val, SplitInfo *splitInfo) override {
    std::cout << "Specialization Test" << std::endl;
    return false;
  }
};

*/

#endif
