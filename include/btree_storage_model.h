//../btree/Btree.h
//Btree.h
#pragma once

#include <functional>
#include <vector>

#include "btree.h"
#include "hash_interface.h"
#include "header_array.h"
#include "storage_model.h"

/**Here shall be some implementation of a btree using a class
 * implementing "storage_model_."  It is assumed that a reasonable
 * number of pages from the model_ can be held in memory at once.
 * Specifically, there should be at least enough memory to store all
 * the pages needed to be read to find any given node, as maintaining
 * this path is an implementation design.
 */

namespace data_org_project_names {

template<
  typename Key,
  typename Data,
  typename HashFunction = UniHash<Key>,
  typename LessThan = std::less<Key>
  >
class BtreeIterator;

template<
  typename Key,
  typename Data,
  typename HashFunction = UniHash<Key>,
  typename LessThan = std::less<Key>
  >
class Btree : public HashInterface<Key, Data, HashFunction> {

 public:

  using LeafEntry     = Entry<Key,Data>;
  using InteriorEntry = Entry<Key,PageId>;

  using LeafNode      = BtreePage<Key,Data>;
  using InteriorNode  = BtreePage<Key,PageId>;

  using Header = BtreeHeader;
  using Pages = std::vector<Header*>;

  using Table = Btree<Key, Data, HashFunction, LessThan>;

  using iterator = TableIterator<Key, Data, Table, PageIterator>;

  /*
   * Btree
   */
  Btree(storage_model* model, size_t n = 0) : model_(model), size_(0) 
  {
    rootId_ = model_->create_page();
  }
  /*
   * BtreePath
   */
  Path
  BtreePath(const Key& key)
  {
    return GetSearchPath(
        rootId_,
        key, 
        [this](PageId pageId) { return this->model_->load_page(pageId); }
    );
  }
  /*
   * GetNext (leaf)
   */
  LeafEntry*
  GetNext(LeafNode* leaf, const Key& key)
  {
    LeafEntry dummyEntry {key, Data()};

    return std::lower_bound(
        leaf->begin(),
        leaf->end(),
        dummyEntry,
        [](const LeafNode& l, const LeafNode& r) { 
          return LessThan(l.key, r.key); 
        }
    );
  }
  const LeafEntry*
  GetNext(LeafNode* leaf, const Key& key) const
  {
    return const_cast<Btree*>(this)->GetNext(leaf, key);
  }
  /*
   * IsMatch
   */
  /*
   * IsMatch const
   */
  static bool
  IsMatch(
      const LeafNode*  leafNode,
      const LeafEntry* leafMatch,
      const Key&       key)
  {
    return leafMatch != leafNode->end() && leafMatch->key == key;
  }
  /*
   * Find
   */
  iterator
  find(const Key& key) const override
  {
    Path searchPath = BtreePath(key);
    auto leafNode = (LeafNode*)searchPath.back().header;

    LeafEntry* leafMatch = GetNext(leafNode, key);

    if (IsMatch(leafNode, leafMatch, key)) return {true, leafMatch->data};
    else return {false, Data()};
  }
  /*
   * Page Info (struct)
   */
  struct PageInfo {
    PageId       id;
    Header* header;
  };
  /*
   * CreateNewPage
   */
  PageInfo
  CreateNewPage() 
  {
    PageId newPageId    = model_->create_page();
    Header* header = load_page(newPageId);

    InitializeHeader<LeafEntry>(*header, model_->get_page_size(), newPageId);
    header->nodeHeight = 0;

    return {newPageId, header};
  }
  /*
   * Split
   */
  void Split(
      InteriorNode*  parentNode,
      InteriorEntry* childEntry,
      Header*        childHeader) 
  {
    PageInfo newPage = CreateNewPage();

    if (childHeader->IsLeaf())
    {
      SplitBtreeNode(
          parentNode,
          childEntry,
          (LeafEntry*)childHeader,
          newPage.header
      );

    } else {

      SplitBtreeNode(
          parentNode,
          childEntry,
          (InteriorEntry*)childHeader,
          newPage.header
      );
    }
  }
  /*
   * SplitRoot
   */
  void SplitRoot() 
  {
    Header* rootHeader  = load_page(rootId_);
    PageInfo     newRootPage = CreateNewPage();

    newRootPage.header->nodeHeight = rootHeader->nodeHeight + 1;

    auto newRoot = (InteriorNode*)newRootPage.header;

    newRoot->begin()->data = rootId_;

    Split(
        newRoot,
        newRoot->begin(),
        rootHeader 
    );

    rootId_ = newRootPage.id;
  }
  /*
   * CanInsertKey
   * 
   * (If the key exists or the page is not full)
   */
  bool CanInsertKey(Path& path, const Key& key)
  {
    auto leaf = (LeafNode*)path.back().header;
    
    if (!leaf->IsFull()) return true;

    LeafEntry* iPoint = GetNext(leaf, key);

    if (IsMatch(leaf,iPoint,key)) return true;

    return false;
  }
  /*
   * MergeRoot
   */
  void
  MergeRoot()
  {
    auto root  = (InteriorNode*)load_page(rootId_);
    auto left  = (InteriorNode*)load_page(root->begin()->data);
    auto right = (InteriorNode*)load_page(root->end()->data);

    MergeNode(root, root->begin(), left, right);
    
    rootId_ = left->header()->pageId;
  }
  /*
   * PrepareInsertPath
   */
  Path
  PrepareInsertPath(const Key& key)
  {
    Path searchPath = BtreePath(key); 

    while (!CanInsertKey(searchPath, key)) 
    {
      auto splitBegin = std::find_if(
          searchPath.rbegin(),
          searchPath.rend(),
          [](const Header* header) { return !header->IsFull(); }
      );

      if (splitBegin == searchPath.rend()) SplitRoot();

      else Split(
              (InteriorNode*)splitBegin->header,
              (InteriorEntry*)splitBegin->childEntry,
              --splitBegin->header
      );

      searchPath = BtreePath(key);
    }

    return searchPath;
  }
  /*
   * Merge (SearchPath)
   */
  void
  Merge(Path& path, Path::iterator mergeNode)
  {
    auto parentIt  = std::prev(mergeNode);
    auto parent    = (InteriorNode*)parentIt->header;
    auto leftEntry = (InteriorEntry*)parent->childEntry;

    if (leftEntry == parent->end()) --leftEntry;

    Header* leftHeader = load_page(leftEntry->data);

    if (leftHeader->IsLeaf())
    {
      auto left = (LeafNode*)leftHeader;
      auto right = std::next(left);

      MergeNode(parent, leftEntry, left, right);

    } else {

      auto left = (InteriorNode*)leftHeader;
      auto right = std::next(left);

      MergeNode(parent, leftEntry, left, right);
    }
  }
  /*
   * CanEraseKey
   */
  bool CanEraseKey(const Path& path, const Key& key)
  {
    BtreeHeader* leaf = path.back().header;
    if (leaf->IsHalf()) return true;

    LeafEntry* ePoint = GetNext(leaf, key);
    if (!IsMatch(leaf,ePoint,key)) return true;

    return false;
  }
  /*
   * PrepareErasePath
   */
  Path
  PrepareErasePath(const Key& key)
  {
    Path searchPath = BtreePath(key); 

    while (!CanEraseKey(searchPath, key)) 
    {
      auto mergeBegin = std::find_if(
          searchPath.rbegin(),
          searchPath.rend(),
          [](const Header* header) { return !header->IsHalf(); }
      );

      if (mergeBegin == searchPath.rend() &&
          mergeBegin->header->size == 1) MergeRoot();
      else Merge(searchPath, mergeBegin);

      searchPath = BtreePath(key);
    }
    
    return searchPath;
  }
  /*
   * erase
   */
  bool 
  erase(const Key& key) override
  {
    Path searchPath = PrepareErasePath(key);

    auto leaf = (LeafNode*)searchPath.back().header;

    LeafEntry* ePoint = GetNext(leaf, key);

    if (!IsMatch(leaf,ePoint,key)) return false;

    leaf->erase(ePoint);
    return true;
  }
  /*
   * insert
   */
  void
  insert(const Key& key, const Data& data) override
  {
    LeafEntry iEntry = {key, data};

    Path searchPath = PrepareInsertPath(key);

    auto leaf = (LeafNode*)searchPath.back();
    
    LeafEntry* iPoint = GetNext(leaf, key);

    if (IsMatch(leaf,iPoint,key))
    {
      iPoint->data = data;
      return;
    }

    leaf->insert(iPoint, iEntry);
  }
  /*
   * VerifyHeight
   */
  static bool
  VerifyHeight(const Pages& pages)
  {
    if (pages.empty()) return true;
    size_t height = pages.front()->nodeHeight;

    std::vector<size_t> heights;
    std::back_insert_iterator<std::vector<size_t> > inserter(heights);
    
    std::transform(
        pages.begin(),
        pages.end(),
        inserter,
        [](Header* p) { return p->nodeHeight; }
    );
    
    return std::all_of(
        heights.begin(),
        heights.end(),
        [height](size_t h) { return h == height; }
    );
  }
  /*
   * VerifyOrder
   */
  static bool
  VerifyOrder(const Pages& pages, storage_model* model)
  {
    if (pages.empty()) return true;

    bool valid = true;
    Key* prev = nullptr;

    if (pages.front()->IsLeaf())
    {
      for (auto&& p : pages) 
        for (auto&& entry : *(LeafNode*)p)
        {
          if (!prev) {
            prev = new Key(entry.key);
          } else {
            valid &= *prev < entry.key;
            *prev = entry.key;
          }
        }

    } else {

      for (auto&& p : pages) 
        for (auto&& entry : *(InteriorNode*)p)
        {
          if (!prev) {
            prev = new Key(entry.key);
          } else {
            valid &= *prev < entry.key;
            *prev = entry.key;
          }
        }
    }

    delete prev;
  }
  /*
   * Verify
   */
  bool
  Verify(const Pages& pages)
  {
    bool valid = true;

    if (pages.empty()) return valid;

    valid &= VerifyHeight(pages);
    valid &= VerifyOrder(pages, model_);

    return valid;
  }
  /*
   * GetPages
   */
  static Pages 
  GetPages(const std::vector<PageId>& level)
  {
    Pages pages;
    for (auto&& pid : level) pages.push_back(load_page(pid));
    return pages;
  }
  /*
   * GetNextLevel
   */
  std::vector<PageId>
  GetNextLevel(const Pages& pages)
  {
    if (pages.empty() || pages.front()->IsLeaf()) return {};

    std::vector<PageId> nextLevel;
    auto inserter = std::back_inserter(nextLevel);
    
    for (auto&& page : pages) {
      auto node = *(InteriorNode*)page;
      std::copy(node->begin(), node->end(), inserter);
    }

    return nextLevel;
  }
  /*
   * Verify
   */
  bool
  Verify() const
  {
    bool valid = true;

    std::vector<PageId> level = {rootId_};
    
    while (!level.empty()) {
      Pages pages = GetPages(level);
      valid &= Verify(pages);
      level = GetNextLevel(pages);
    }
    return valid;
  }
  /*
   * IsEnd
   */
  bool 
  IsEnd(const void* entry, const Header* header) const
  {
    if (header->IsLeaf()) return ((LeafNode*)header)->end() == entry;
    else return ((InteriorNode*)header)->end() == entry;
  }
  /*
   * IsBegin
   */
  bool 
  IsBegin(const void* entry, const Header* header) const
  {
    if (header->IsLeaf()) return ((LeafNode*)header)->begin() == entry;
    else return ((InteriorNode*)header)->begin() == entry;
  }
  /*
   * Successor
   */
  iterator&
  Successor(iterator& it)
  {
      }
  /*
   * Predecessor
   */
  iterator&
  Predecessor(iterator& it)
  {
    Path searchPath = GetSearchPath(
        rootId_,
        it->entry_->key, 
        [this](PageId pageId) { return this->model_->load_page(pageId); }
    );

    LeafNode* leaf = (LeafEntry*)searchPath.back().header;

    if (searchPath.back().childEntry != leaf->begin()) 
    {
      --it->entry_;
      return it;
    }

    auto branch = std::find_if(
        searchPath.begin(),
        searchPath.end(),
        [](const PathVertex& v) { 
          return !IsBegin(v.childEntry, v.header);
        }
    );

    it->page_ = GetMinSubtree(*--branch);
    it->entry_ = it->page_->begin();
    return it;
  }
  
 private:

  /*
   * GetMinSubtree
   */
  LeafNode*
  GetMinSubtree(PathVertex v) //TODO Better interface here..
  {
    while (!v.header->IsLeaf()) {
      PageId pageId = ((InteriorEntry*)v.childEntry)->data;
      v.header = load_page(pageId);
      v.childEntry = v.header + 1;
    }

    return (LeafNode*)v.header;
  }
  /*
   * GetMaxAfter
   */
  LeafNode*
  GetMaxAfter(PathVertex v)
  {
    while (!v.header->IsLeaf()) {
      PageId pageId = ((InteriorEntry*)v.childEntry)->data;
      v.header = load_page(pageId);
      v.childEntry = v.header + 1;
    }

    return (LeafNode*)v.header;
  }
 
  /*
   * load_page
   */
  Header*
  load_page(PageId pageId) const 
  { 
    return (Header*)model_->load_page(pageId);
  }

  PageId         rootId_;
  storage_model* model_;
  size_t         size_;

 public:

  class PageIterator : public PageIteratorBase<LeafNode, Table> {
    public:
      using PageIteratorBase<LeafNode, Table>::page_;
      using PageIteratorBase<LeafNode, Table>::table_;

      PageIterator& operator++() override {
        storage_model* model_  = table_->model_;
        PageId         rootId_ = table_->rootId_;

        if (page_ == nullptr)
        {
          page_ = model_->load_page(rootId_);
        }
        else
        {
          Path searchPath = GetSearchPath(
              rootId_,
              page_->begin()->key;
              [table_](PageId pageId) { return table_->model_->load_page(pageId); }
              );

          auto branch = std::find_if(
              searchPath.rbegin(),
              searchPath.rend(),
              [](const PathVertex& v) { return !IsEnd(v.childEntry, v.header); }
              );

          if (branch == searchPath.rend())
          {
            page_ =  nullptr;
          }
          else
          {
            NextChildEntry(branch);
            while (!branch->header->IsLeaf())
            {
              PageId nextPage = ((InteriorEntry*)branch->childEntry)->data;
              branch->header = table_->model_->load_page(nextPage);
              branch->childEntry = ((InteriorPage*)branch->header)->begin();
            }
            page_ = branch->header;
          }
        }
        return *this;
      }
      PageIterator& operator--() override {
        storage_model* model_  = table_->model_;
        PageId         rootId_ = table_->rootId_;

        if (page_ == nullptr)
        {
          page_ = model_->load_page(rootId_);
        }
        else
        {
          Path searchPath = GetSearchPath(
              rootId_,
              page_->begin()->key;
              [table_](PageId pageId) { return table_->model_->load_page(pageId); }
              );

          auto branch = std::find_if(
              searchPath.rbegin(),
              searchPath.rend(),
              [](const PathVertex& v) { return !IsBegin(v.childEntry, v.header); }
              );

          if (branch == searchPath.rend())
          {
            page_ =  nullptr;
          }
          else
          {
            PrevChildEntry(branch);
            while (!branch->header->IsLeaf())
            {
              PageId nextPage = ((InteriorEntry*)branch->childEntry)->data;
              branch->header = table_->model_->load_page(nextPage);
              branch->childEntry = --((InteriorPage*)branch->header)->end();
            }
            page_ = branch->header;
          }
        }
        return *this;
      }

     private:

      void AdvanceChildEntry(PathVertex& v) {
        if (v.header->IsLeaf())
        {
          ++((LeafEntry*)v.childEntry);
        }
        else
        {
          ++((InteriorEntry*)v.childEntry);
        }
      }
      void PrevChildEntry(PathVertex& v) {
        if (v.header->IsLeaf())
        {
          --((LeafEntry*)v.childEntry);
        }
        else
        {
          --((InteriorEntry*)v.childEntry);
        }
      }
  };

}; //struct Btree



}; //data_org_project_names
