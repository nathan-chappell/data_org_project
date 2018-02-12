#pragma once

#include <memory>
#include <cstring>
#include <string>

#include "universal_hash.h"

/*
 * Defines an interface for a hash-container designed to work with
 * records in
 * secondary storage.
 */

namespace data_org_project_names {

typedef size_t PageId;

template<
  typename Key,
  typename Data,
  template<typename, typename> typename Entry,
  template TableType
  >
class TableIteratorBase {

 public:

  using value_type      = Entry<const Key, Data>;
  using pointer         = value_type*;
  using reference       = value_type&;
  using const_pointer   = const pointer;
  using const_reference = const reference;
  using difference_type = std::ptrdiff_t;

  using Table = TableType;
  using Page  = HeaderArray<Table::Header, value_type>;

  reference       operator*()        { return *entry_; }
  const_reference operator*()  const { return *entry_; }
  pointer         operator->()       { return entry_; }
  const_pointer   operator->() const { return entry_; }
  iterator        operator++(int) { 
    auto prev = *this;
    this->operator++();
    return prev;
  }
  iterator        operator--(int) { 
    auto prev = *this;
    operator--();
    return prev;
  }
  bool operator==(const TableIteratorBase& it) {
    return entry_ == it.entry_ && page_ == it.page_;
  }
  bool operator!=(const TableIteratorBase& it) {
    return !(it == *this);
  }

 private:

  pointer entry_;
  Page*   page_;
  Table*  table_;
};

/*
 * Implements an abstract standard hash interface.
 */
template<
  typename Key,
  typename Data,
  typename HashFunction
  >
class HashInterface {
 public:
  /*
   * TODO: C++ Concepts: Unordered Associative Container
   */

  virtual void
  insert(const Key& key, const Data& data) = 0;

  virtual bool
  erase(const Key& key) = 0;

  virtual std::pair<bool, Data>
  find(const Key& key) const = 0;
};

}; //namespace secondary_storage_hash_names
