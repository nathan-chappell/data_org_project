//larson_kalja.h
#pragma once

#include <cmath>
#include <cstdint>
#include <list>
#include <memory>
#include <vector>

#include "header_array.h"
#include "storage_model.h"
#include "universal_hash.h"


namespace data_org_project_names {

void HashError()
{
    printf("unrealistic hash conditions, aborting\n");
    assert(false);
}

/*
 * Mini-Structs
 */

struct LkDirEntry {
  PageId pageId;
  size_t separator;

  std::string 
  ToString() const
  {
    return "{pageId: "         + std::to_string(pageId) +
      ", separator: "       + std::to_string(separator) + "}";
  }
};

template<typename Key, typename Data>
struct LkPageEntry {
  Key    key;
  Data   data;

  void   AdvanceHashIx() { ++hashIx_; }
  size_t hashIx() const { return hashIx_; }

  std::string 
  ToString() const
  {
    return "{key: "         + std::to_string(key) +
      ", data: "       + std::to_string(data) +
      ", hashIx_: " + std::to_string(hashIx_) + "}";
  }

 private:

  size_t hashIx_;
};

using LkHeader = HeaderBase;

template<typename Key, typename Data>
using LkPage = HeaderArray<LkHeader, LkPageEntry<Key, Data>>;

using Directory = std::vector<LkDirEntry>;

/*
 * ToString(Directory)
 */
std::string
ToString(const Directory& directory)
{
  std::string dir = "{Directory:\n";
  for (auto&& dirEntry : directory) dir += dirEntry.ToString() + "\n";
  dir += "}";

  return dir;
}

/*
 * LkHash
 *
 * Maintains the sequence of hash functions to locate directories and
 * calcluate signatures for an LkTable
 * public:
 *
 * size_t Signature(const PageEntry& pageEntry) const;
 * size_t Advance(overflowEntry, directory)
 * 
 * std::pair<bool,LkDirEntry> Search(key, dir) const
 *
 * private:
 * void Expand()
 * std::vector<LkHx<Key>> lkHx_;
 *
 */
template< 
  typename Key,
  typename Data,
  typename DirHashFn = UniHash<Key>,
  typename SigHashFn = UniHash<Key>
  >
class LkHash {
 public:
  using PageEntry = LkPageEntry<Key, Data>;
  using DirHash = DirHashFn;
  using SigHash = SigHashFn;
  using Header = LkHeader;
  
  struct LkHx {
    DirHash DirIx;
    SigHash Sig;
  };

  /*
   * Member Functions
   */

  LkHash(size_t maxDir) : lkHx_(1), maxDir_(maxDir) {}

  /*
   * Signature
   */
  size_t
  Signature(const PageEntry& pageEntry) const
  {
    return lkHx_[pageEntry.hashIx()].Sig(pageEntry.key);
  }
  /*
   * DirIx
   */
  size_t
  DirIx(const PageEntry& pageEntry) const
  {
    return lkHx_[pageEntry.hashIx()].DirIx(pageEntry.key) % maxDir_;
  }
  /*
   * Search
   */
  std::pair<bool,LkDirEntry>
  Search(
      const Key&       key,
      const Directory& dir) const
  {
    for (const auto& hx : lkHx_) {

      size_t dirIx = hx.DirIx(key) % maxDir_;

      if (hx.Sig(key) < dir[dirIx].separator) return {true, dir[dirIx]}; } 
    return {false,{0,0}};
  }
  /*
   * Advance
   *
   * Increases the entrie's sigIx until an appropriate signature is found
   * (that the key may be inserted)
   */
  size_t
  Advance(
      PageEntry& overflowEntry,
      const Directory& directory)
  {
    size_t hashIx = overflowEntry.hashIx();

    while (true) 
    {
      //expand the number of available hash functions if necessary
      if (hashIx == lkHx_.size()) Expand();

      PageId dirIx        = DirIx(overflowEntry);
      size_t keySignature = lkHx_[hashIx].Sig(overflowEntry.key);

      if (keySignature < directory[dirIx].separator) return dirIx;

      overflowEntry.AdvanceHashIx();
    }
  }

 private:

  /*
   * Expand - won't let the hash sequence get longer that 0x10000 function
   * pairs.  As of right now, it just aborts, but this could be handled
   * better.
   */
  void
  Expand()
  {
    if (lkHx_.size() >= 0x10000) HashError(); 
    lkHx_.resize(lkHx_.size()*2);
  }

  std::vector<LkHx> lkHx_;
  size_t            maxDir_;
};


/**
 * LkTable is an honest to goodness hash table that works with a
 * LkDirectory and a storage_model to get pages.
 *
 * public:
 *  void                         insert(const Key& key, Data data)
 *  bool                         erase(const Key& key)
 *  std::pair<bool, LkPageEntry> find(const Key& key) const
 *  std::string                  ToString()           const
 *  inline size_t                size()               const
 *  inline size_t                capacity()           const
 *  inline double                LoadFactor()         const
 *
 *  friend class LkTableIterator
 *
 * private:
 *  void CreatePages();
 *
 *  storage_model* model_;
 *  LkHash         lkHash_;
 *  Directory      directory_;
 *  size_t         size_;
 *  size_t         capacity_;
 *
*/

template<
  typename Key,
  typename Data,
  typename Hash
  >
class LkTableIterator;

template<
  typename Key,
  typename Data,
  typename Hash = UniHash<Key>
  >
class LkTable : public HashInterface<Key, Data, Hash> {
 public:
  using iterator     = LkTableIterator<Key, Data>;
  using PageEntry    = LkPageEntry<Key, Data>;
  using Page         = LkPage<Key, Data>;
  using Hash         = LkHash<Key, Data>;
  using OverflowList = std::list<PageEntry>;
  using Header       = LkHeader;


  LkTable(storage_model* model,
          size_t numPages) : 
      model_(model),
      lkHash_(numPages),
      directory_(numPages)
  {
    CreatePages();
  }
    /*
     * find
     */
  iterator
  find(const Key& key) const
  {
    auto searchResult = lkHash_.Search(key, directory_);
    if (!searchResult.first) return {false, {0,0}};

    size_t pageId = searchResult.second.pageId;
    auto   page   = (Page*)model_->load_page(pageId);

    //try to find the key in the page
    PageEntry* keyMatch = page->find(
        [key](const PageEntry& e) { return e.key == key; }
    );

    return {keyMatch, page, this};
  }
    /*
     * insert
     */
  void
  insert(const Key& key, const Data& data) 
  {
    OverflowList Q;
    OverflowList pageOverflow;
    bool firstLoop = true;

    Q.push_back({key, data, 0});

    while (!Q.empty()) {

      PageEntry iEntry = Q.front();
      Q.pop_front();

      size_t dirIx = lkHash_.Advance(iEntry, directory_);
      auto   page  = (Page*)model_->load_page(dirIx);

      /*
       * First Loop: check to see if our size must increase
       */
      if (firstLoop) 
      {
        PageEntry* match = page->find(
            [key](const PageEntry& e) { return e.key == key; }
        );

        if (match == page->end()) ++size_; //inserting new key
        firstLoop = false;

      } else if (page->full()) {

        pageOverflow = PageOverflow(page, iEntry, lkHash_);

        directory_[dirIx].separator = 
          lkHash_.Signature(pageOverflow.front());

        Q.splice(Q.end(), pageOverflow);

      } else {

        PageInsertNonFull(page, iEntry);
      }
    }
  }
  /*
   * erase
   */
  bool
  erase(const Key& key)
  {
    auto directorySearch = lkHash_.Search(key, directory_);
    if (!directorySearch.first) return false;

    size_t dirIx = directorySearch.second.pageId;
    auto   page  = (Page*)model_->load_page(directory_[dirIx].pageId);

    PageEntry* eraseEntry = page->find(
        [key](const PageEntry& e) { return e.key == key; }
    );

    if (eraseEntry == page->end()) return false; //don't have it

    page->erase(eraseEntry);
    --size_;

    return true;
  }
  /*
   * LkTable ToString()
   */
  std::string
  ToString() const
  {
    std::string str;
    str += "\nPages:\n";

    for (auto dirEntry : directory_) 
    {
      auto page = (Page*) model_->load_page(dirEntry.pageId);
      str += page->ToString() + "\n";
    }
    return str + "\n";
  }

  inline size_t size()       const { return size_; }
  inline size_t capacity()   const { return capacity_; }
  inline double LoadFactor() const { return size_ / (double)capacity_; }

  friend class LkTableIterator<Key, Data, Hash>;

 private:
  /*
   * CreatePages
   *
   * Asks the storage model to create pages to fill up the directory.
   * After loading the page, its header is initialized and the page is
   * released.
   */
  void
  CreatePages() 
  {
    size_t pageSize = model_->get_page_size();

    for (auto& dirEntry : directory_) 
    {
      PageId newPageId = model_->create_page();

      dirEntry.pageId  = newPageId;
      dirEntry.separator = SIZE_MAX;

      auto pageHeader = (LkHeader*)model_->load_page(newPageId);

      InitializeHeader<LkHeader, PageEntry>(
          pageHeader,
          pageSize,
          newPageId
          );

      capacity_ += pageHeader->max_size;

      model_->release_page(newPageId);
    }
  }
  /*
   * PageInsertNonFull(page, iEntry, lkHashers_)
   */
  PageEntry*
  PageInsertNonFull(Page*      page, 
              const PageEntry& iEntry)
  {
    PageEntry* insertionPoint;
    
    insertionPoint = page->find(
      [iEntry, this](const PageEntry& e) {
        return this->lkHash_.Signature(iEntry) <= this->lkHash_.Signature(e);
      });

    //if found...
    if (insertionPoint      != page->end() && 
        insertionPoint->key == iEntry.key)
    {
      *insertionPoint = iEntry;

    } else {

      page->insert(insertionPoint, iEntry);
    }

    return insertionPoint;
  }
  /*
   * PageOverflow
   */
  OverflowList
  PageOverflow(Page*    page, 
         const PageEntry& iEntry,
         const Hash&    lkHash)
  {
    OverflowList pageOverflow;
    auto endSignature = lkHash.Signature(page->back());
    auto keySignature = lkHash.Signature(iEntry);

    PageEntry* overflowBegin = page->find_last(
      [endSignature, lkHash](const PageEntry& e) {
        return endSignature == lkHash.Signature(e);
      });

    PageEntry* insertionPoint = page->find(
      [keySignature, lkHash](const PageEntry& e) {
        return keySignature <= lkHash.Signature(e);
      });

    bool iEntryOverflow = (overflowBegin == page->end() ||
           lkHash.Signature(*insertionPoint) == endSignature);

    while (overflowBegin != page->end()) 
    {
      pageOverflow.push_back(*overflowBegin);
      page->erase(overflowBegin);
    }

    if (iEntryOverflow) pageOverflow.push_back(iEntry);
    if (!iEntryOverflow) page->insert(insertionPoint, iEntry);

    return pageOverflow;
  }

  storage_model* model_;
  Hash           lkHash_;
  Directory      directory_;
  size_t         size_;
  size_t         capacity_;
};


template<typename Key, typename Data, >
class LkTableIterator :
  public TableIteratorBase<
  Key, Data, LkPageEntry, FaginTable<Key,Data,Hash>
  > {
  using iterator = LkTableIterator<Key, Data>;

  LkTableIterator() : entry_(nullptr) {}

  reference       operator*()        { return *entry_; }
  const_reference operator*()  const { return *entry_; }
  pointer         operator->()       { return entry_; }
  const_pointer   operator->() const { return entry_; }
  bool operator==(const iterator& it) const {
    return entry_ == it.entry_;
  }
  bool operator!=(const iterator& it) const {
    return !(entry_ == it.entry_);
  }
  iterator& operator++() {
    if (entry_ != page_->end()) {

      ++entry_;
      return *this;
    } 

    auto dirIt = std::find_if(
        table->directory_.begin(),
        table->directory_.end(),
        [page](PageId pageId) { return pageId == page->pageId; }
    );
    
    if (dirIt_ != table_->directory_.end()) {

      ++dirIt_;
      page = (Page*)table_->model_->load_page(*dirIt);
      entry_ = page_->begin();

    } else {

      entry_ = nullptr;
      page_  = nullptr;
    }

    return *this;
  }
  iterator& operator--() {
    if (entry_ != page_->begin()) {

      ++entry_;
      return *this;

    } 

    auto dirIt = std::find_if(
        table->directory_.rbegin(),
        table->directory_.rend(),
        [page](PageId pageId) { return pageId == page->pageId; }
    );
    
    if (dirIt_ != table_->directory_.rend()) {

      ++dirIt_; //reverse iterator!
      page = (Page*)table_->model_->load_page(*dirIt);
      entry_ = page_->begin();

    } else {

      entry_ = nullptr;
      page_  = nullptr;
    }

    return *this;
  }
  iterator operator++(int) {
    LkTableIterator tmp = *this;
    operator++();
    return tmp;
  }
  iterator operator--(int) {
    LkTableIterator tmp = *this;
    operator--();
    return tmp;
  }

 private:

  Directory::iterator  dirIt_;
};

}; //data_org_project_names
