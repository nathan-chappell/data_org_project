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

/*
 * The following base class shall be suitable for iterators of the
 * tables in this library.  Note that the only thing left to do for
 * the derived iterators is to implement operator++ and operator--.
 *
 * TODO This can be made even more general purpose by enforcing/
 * implementing a predecessor/ successor function for every table.
 * Should have done that from the start...
 *
 */
template<
  typename Key,
  typename Data,
  template<typename, typename> typename Entry,
  typename PageIterator
  >
class TableIterator {

 public:

  using value_type      = Entry<Key, Data>; //TODO "Key" -> "const Key"
  using pointer         = value_type*;
  using reference       = value_type&;
  using const_pointer   = const pointer;
  using const_reference = const reference;
  using difference_type = std::ptrdiff_t;
  using iterator_category = std::bidirectional_iterator_tag;
  using iterator        = TableIterator<Key, Data, Entry, PageIterator>;

  reference       operator*()        { return *entry_; }
  const_reference operator*()  const { return *entry_; }
  pointer         operator->()       { return entry_; }
  const_pointer   operator->() const { return entry_; }
  iterator&       operator++() {
    if (pageIt_.get() == nullptr) {
      ++pageIt_;
    }
    if (entry_ != pageIt_->end())
    {
      ++entry_;
    }
    else
    {
      ++pageIt_;
      if (pageIt_.get() != nullptr) entry_ = pageIt_->begin();
      else entry_ = nullptr;
    }
    return *this;
  }      
  iterator&       operator--() {
    if (pageIt_.get() == nullptr) {
      --pageIt_;
    }
    if (entry_ != pageIt_->begin())
    {
      --entry_;
    }
    else
    {
      --pageIt_;
      if (pageIt_.get() != nullptr) entry_ = --pageIt_->end();
      else entry_ = nullptr;
    }
    return *this;
  }
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
  bool operator==(const iterator& it) {
    return entry_ == it.entry_ && pageIt_ == it.pageIt_;
  }
  bool operator!=(const iterator& it) {
    return !(it == *this);
  }

  TableIterator(pointer entry, PageIterator page) :
    entry_(entry), pageIt_(page) {}

  friend PageIterator;

 protected:

  pointer      entry_;
  PageIterator pageIt_;
};

/*
 * Boilerplate for Table's PageIterators
 */
template<typename PageType, typename TableType>
class PageIteratorBase {
  public:
    using Table = TableType;
    using Page  = PageType;

    Page&       operator*()        { return *page_; }
    Page*       operator->()       { return page_; }
    Page*       get()              { return page_; }
    const Page& operator*()  const { return *page_; }
    const Page* operator->() const { return page_; }
    bool operator==(const PageIteratorBase& l) { 
      return l.page_ == page_ && l.table_ == table_; 
    }
    bool operator!=(const PageIteratorBase& l) { 
      return !(*this == l); 
    }
    PageIteratorBase(Page* page, Table* table) :
      page_(page), table_(table) {}

    PageIteratorBase() : page_(nullptr), table_(nullptr) {}

    virtual void operator++() = 0;
    virtual void operator--() = 0;

   protected:

    Page* page_;
    Table* table_;
};

/*
 * Implements an abstract standard hash interface.
 */
template<
  typename Key,
  typename Data
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
