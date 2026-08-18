/**
 * A self-balancing associative container implemented with an AVL tree.
 */
#ifndef SJTU_MAP_HPP
#define SJTU_MAP_HPP

#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {

template<class Key, class T, class Compare = std::less<Key> >
class map {
 private:
  typedef pair<const Key, T> stored_value_type;

  struct Node {
    stored_value_type *value;
    Node *left, *right, *parent, *all_next;
    int height;
    bool alive;
    Node(const stored_value_type &v, Node *p)
        : value(new stored_value_type(v)), left(nullptr), right(nullptr), parent(p),
          all_next(nullptr), height(1), alive(true) {}
    ~Node() { delete value; }
  };

  Node *root;
  Node *all_nodes;
  size_t node_count;
  Compare compare;

  static int height_of(Node *p) { return p == nullptr ? 0 : p->height; }
  static int maximum_int(int a, int b) { return a > b ? a : b; }
  static void update_height(Node *p) {
    if (p != nullptr) p->height = maximum_int(height_of(p->left), height_of(p->right)) + 1;
  }
  static int balance_of(Node *p) { return height_of(p->left) - height_of(p->right); }
  static Node *minimum(Node *p) {
    while (p != nullptr && p->left != nullptr) p = p->left;
    return p;
  }
  static Node *maximum(Node *p) {
    while (p != nullptr && p->right != nullptr) p = p->right;
    return p;
  }
  static Node *next_node(Node *p) {
    if (p->right != nullptr) return minimum(p->right);
    Node *q = p->parent;
    while (q != nullptr && p == q->right) { p = q; q = q->parent; }
    return q;
  }
  static Node *previous_node(Node *p) {
    if (p->left != nullptr) return maximum(p->left);
    Node *q = p->parent;
    while (q != nullptr && p == q->left) { p = q; q = q->parent; }
    return q;
  }

  void rotate_left(Node *p) {
    Node *q = p->right;
    p->right = q->left;
    if (q->left != nullptr) q->left->parent = p;
    q->parent = p->parent;
    if (p->parent == nullptr) root = q;
    else if (p == p->parent->left) p->parent->left = q;
    else p->parent->right = q;
    q->left = p;
    p->parent = q;
    update_height(p);
    update_height(q);
  }
  void rotate_right(Node *p) {
    Node *q = p->left;
    p->left = q->right;
    if (q->right != nullptr) q->right->parent = p;
    q->parent = p->parent;
    if (p->parent == nullptr) root = q;
    else if (p == p->parent->right) p->parent->right = q;
    else p->parent->left = q;
    q->right = p;
    p->parent = q;
    update_height(p);
    update_height(q);
  }
  void rebalance_from(Node *p) {
    while (p != nullptr) {
      update_height(p);
      int b = balance_of(p);
      if (b > 1) {
        if (balance_of(p->left) < 0) rotate_left(p->left);
        Node *parent = p->parent;
        rotate_right(p);
        p = parent;
      } else if (b < -1) {
        if (balance_of(p->right) > 0) rotate_right(p->right);
        Node *parent = p->parent;
        rotate_left(p);
        p = parent;
      } else {
        p = p->parent;
      }
    }
  }
  static void destroy(Node *p) {
    while (p != nullptr) {
      Node *next = p->all_next;
      delete p;
      p = next;
    }
  }
  void transplant(Node *from, Node *to) {
    if (from->parent == nullptr) root = to;
    else if (from == from->parent->left) from->parent->left = to;
    else from->parent->right = to;
    if (to != nullptr) to->parent = from->parent;
  }
  Node *find_node(const Key &key) const {
    Node *p = root;
    while (p != nullptr) {
      if (compare(key, p->value->first)) p = p->left;
      else if (compare(p->value->first, key)) p = p->right;
      else return p;
    }
    return nullptr;
  }

 public:
  typedef pair<const Key, T> value_type;
  class const_iterator;

  class iterator {
   private:
    Node *node;
    map *owner;
    iterator(Node *p, map *m) : node(p), owner(m) {}
    void validate_node() const {
      if (owner == nullptr || node == nullptr || !node->alive) throw invalid_iterator();
    }
    friend class map;
    friend class const_iterator;
   public:
    iterator() : node(nullptr), owner(nullptr) {}
    iterator(const iterator &other) : node(other.node), owner(other.owner) {}
    iterator operator++(int) { iterator result(*this); ++*this; return result; }
    iterator &operator++() {
      validate_node();
      Node *n = next_node(node);
      node = n;
      return *this;
    }
    iterator operator--(int) { iterator result(*this); --*this; return result; }
    iterator &operator--() {
      if (owner == nullptr) throw invalid_iterator();
      if (node == nullptr) {
        node = maximum(owner->root);
        if (node == nullptr) throw invalid_iterator();
        return *this;
      }
      validate_node();
      Node *p = previous_node(node);
      if (p == nullptr) throw invalid_iterator();
      node = p;
      return *this;
    }
    value_type &operator*() const { validate_node(); return *node->value; }
    bool operator==(const iterator &rhs) const { return owner == rhs.owner && node == rhs.node; }
    bool operator==(const const_iterator &rhs) const;
    bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
    bool operator!=(const const_iterator &rhs) const;
    value_type *operator->() const noexcept { return node == nullptr ? nullptr : &*node->value; }
  };

  class const_iterator {
   private:
    Node *node;
    const map *owner;
    const_iterator(Node *p, const map *m) : node(p), owner(m) {}
    void validate_node() const {
      if (owner == nullptr || node == nullptr || !node->alive) throw invalid_iterator();
    }
    friend class map;
    friend class iterator;
   public:
    const_iterator() : node(nullptr), owner(nullptr) {}
    const_iterator(const const_iterator &other) : node(other.node), owner(other.owner) {}
    const_iterator(const iterator &other) : node(other.node), owner(other.owner) {}
    const_iterator operator++(int) { const_iterator result(*this); ++*this; return result; }
    const_iterator &operator++() {
      validate_node();
      node = next_node(node);
      return *this;
    }
    const_iterator operator--(int) { const_iterator result(*this); --*this; return result; }
    const_iterator &operator--() {
      if (owner == nullptr) throw invalid_iterator();
      if (node == nullptr) {
        node = maximum(owner->root);
        if (node == nullptr) throw invalid_iterator();
        return *this;
      }
      validate_node();
      Node *p = previous_node(node);
      if (p == nullptr) throw invalid_iterator();
      node = p;
      return *this;
    }
    const value_type &operator*() const { validate_node(); return *node->value; }
    bool operator==(const const_iterator &rhs) const { return owner == rhs.owner && node == rhs.node; }
    bool operator==(const iterator &rhs) const { return owner == rhs.owner && node == rhs.node; }
    bool operator!=(const const_iterator &rhs) const { return !(*this == rhs); }
    bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
    const value_type *operator->() const noexcept { return node == nullptr ? nullptr : &*node->value; }
  };

  map() : root(nullptr), all_nodes(nullptr), node_count(0), compare(Compare()) {}
  map(const map &other) : root(nullptr), all_nodes(nullptr), node_count(0), compare(other.compare) {
    for (Node *p = minimum(other.root); p != nullptr; p = next_node(p)) insert(*p->value);
  }
  map &operator=(const map &other) {
    if (this == &other) return *this;
    map temp(other);
    Node *old_root = root;
    Node *old_all_nodes = all_nodes;
    size_t old_count = node_count;
    Compare old_compare = compare;
    root = temp.root; all_nodes = temp.all_nodes; node_count = temp.node_count; compare = temp.compare;
    temp.root = old_root; temp.all_nodes = old_all_nodes; temp.node_count = old_count; temp.compare = old_compare;
    return *this;
  }
  ~map() { destroy(all_nodes); }

  T &at(const Key &key) {
    Node *p = find_node(key);
    if (p == nullptr) throw index_out_of_bound();
    return p->value->second;
  }
  const T &at(const Key &key) const {
    Node *p = find_node(key);
    if (p == nullptr) throw index_out_of_bound();
    return p->value->second;
  }
  T &operator[](const Key &key) {
    Node *p = find_node(key);
    if (p != nullptr) return p->value->second;
    return insert(value_type(key, T())).first->second;
  }
  const T &operator[](const Key &key) const { return at(key); }

  iterator begin() { return iterator(minimum(root), this); }
  const_iterator cbegin() const { return const_iterator(minimum(root), this); }
  iterator end() { return iterator(nullptr, this); }
  const_iterator cend() const { return const_iterator(nullptr, this); }
  bool empty() const { return node_count == 0; }
  size_t size() const { return node_count; }
  void clear() {
    for (Node *p = all_nodes; p != nullptr; p = p->all_next) {
      if (p->alive) { p->alive = false; delete p->value; p->value = nullptr; }
    }
    root = nullptr;
    node_count = 0;
  }

  pair<iterator, bool> insert(const value_type &value) {
    if (root == nullptr) {
      root = new Node(value, nullptr);
      root->all_next = all_nodes; all_nodes = root;
      ++node_count;
      return pair<iterator, bool>(iterator(root, this), true);
    }
    Node *p = root, *parent = nullptr;
    while (p != nullptr) {
      parent = p;
      if (compare(value.first, p->value->first)) p = p->left;
      else if (compare(p->value->first, value.first)) p = p->right;
      else return pair<iterator, bool>(iterator(p, this), false);
    }
    Node *added = new Node(value, parent);
    added->all_next = all_nodes; all_nodes = added;
    if (compare(value.first, parent->value->first)) parent->left = added;
    else parent->right = added;
    ++node_count;
    rebalance_from(parent);
    return pair<iterator, bool>(iterator(added, this), true);
  }

  void erase(iterator pos) {
    if (pos.owner != this || pos.node == nullptr || !pos.node->alive) throw invalid_iterator();
    Node *z = pos.node;
    Node *start;
    if (z->left == nullptr || z->right == nullptr) {
      start = z->parent;
      transplant(z, z->left != nullptr ? z->left : z->right);
    } else {
      Node *s = minimum(z->right);
      Node *old_parent = s->parent;
      if (old_parent != z) {
        transplant(s, s->right);
        s->right = z->right;
        s->right->parent = s;
        start = old_parent;
      } else {
        start = s;
      }
      transplant(z, s);
      s->left = z->left;
      s->left->parent = s;
      update_height(s);
    }
    z->alive = false;
    delete z->value;
    z->value = nullptr;
    --node_count;
    rebalance_from(start);
  }
  size_t count(const Key &key) const { return find_node(key) == nullptr ? 0 : 1; }
  iterator find(const Key &key) { return iterator(find_node(key), this); }
  const_iterator find(const Key &key) const { return const_iterator(find_node(key), this); }
};

template<class Key, class T, class Compare>
bool map<Key, T, Compare>::iterator::operator==(const const_iterator &rhs) const {
  return owner == rhs.owner && node == rhs.node;
}
template<class Key, class T, class Compare>
bool map<Key, T, Compare>::iterator::operator!=(const const_iterator &rhs) const {
  return !(*this == rhs);
}

}
#endif
