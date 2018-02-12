//HeaderArray.h
#pragma once

/**This header defines a simple array, with a header to manage some information.
 * It is intended that the array not deal with any memory management, that the
 * array is initialized by giving it a pointer to the header, then offering a
 * few simple interfaces from there
 */

//TODO Get all the btree specific stuff outta here

#include <algorithm>
#include <functional>
#include <set>
#include <string>

using PageId = size_t;

namespace data_org_project_names {

template<typename C>
std::string ContainerToString(const C& c)
{
  std::string containerString;

  std::for_each(
      c->begin(),
      c->end(),
      [&containerString](typename C::const_reference x) {
        containerString += "\t" + x.ToString() + "\n";
      }
  );

  return containerString;
}

struct HeaderBase {
  PageId pageId;
  size_t pageSize;
  size_t size;
  size_t max_size;

  std::string
  ToString() const
  {
    return "{pageId = "    + std::to_string(pageId) +
           ", pageSize = " + std::to_string(pageSize) +
           ", size = "     + std::to_string(size) +
           ", max_size = " + std::to_string(max_size) + "}";
  }
};

/* Maximum number of items of type T an array of size (@pageSize) bytes can
 * hold, given that it needs a Header.
 *0....+sizeof(Header)...+sizeof(T)...+sizeof(T)...+sizeof(T).pageSize
 *^          ^              ^              ^              ^   ^
 *[__HEADER__|_____T_1______|_____T_2______|_____T_x______|***]
 *
 * T_x above indicates the "past the end" entry.  It is maintained
 * every where as a bit of a simplification, since the "far right"
 * btrees need the "past the end" pointer, it's easier to just deal
 * with it generally.
 */
template<typename Header, typename T>
size_t
max_size(size_t pageSize) 
{
  size_t maxItems = (pageSize - sizeof(Header)) / sizeof(T) - 1;
  //printf("pageSize: %zu, header: %zu, T: %zu, maxItems: %zu\n", 
      //pageSize, sizeof(Header), sizeof(T), maxItems);
  return maxItems;
}
/*
 * InitializeHeader
 */
template<typename Header, typename T>
void
InitializeHeader(
    Header* page,
    size_t  pageSize,
    PageId  pageId)
{
  page->pageId = pageId;
  page->pageSize = pageSize;
  page->size = 0;
  page->max_size = max_size<Header, T>(pageSize);
}

template<typename Header, typename T>
class HeaderArray {
 public:
  /*
   * Member Types
   */
  typedef       T  value_type;
  typedef       T& reference;
  typedef       T& const_reference;
  typedef       T* pointer;
  typedef       T* const_pointer;
  typedef       T* iterator;
  typedef const T* const_iterator;
 
  /*
   * Member Functions
   */

  /*
   * Header Access
   */
  inline       Header* header()       { return (Header*) this; }
  inline const Header* header() const { return (Header*) this; }

  /*
   * Element Access
   */
  inline       T* begin()          { return  (T*)(header() + 1); }
  inline const T* begin()    const { return (const T*)(header() + 1); }
  inline       T* end()            { return begin() + header()->size; }
  inline const T* end()      const { return begin() + header()->size; }
  inline       T& front()          { return *(T*)(header() + 1); }
  inline       T& back()           { return *(end() - 1); }
  inline       T* ArrayEnd()       { return begin() + header()->max_size; }
  inline const T* ArrayEnd() const { return begin() + header()->max_size; }

  inline       T& operator[](size_t index)       { return *(begin() + index); }
  inline const T& operator[](size_t index) const { return *(begin() + index); }

  /*
   * Capacity
   */
  inline bool   full()     const { return max_size() == size(); }
  inline bool   empty()    const { return begin() == end(); }
  inline size_t max_size() const { return header()->max_size; }
  inline size_t size()     const { return header()->size; }

  /*
   * --Non-Modifying operations
   */
  /*
   * find
   */
  T*
  find(std::function<bool(const T&)> predicate)
  {
    for (auto& t : *this) if (predicate(t)) return &t;
    return end();
  }
  /*
   * find const
   */
  const T* 
  find(std::function<bool(const T&)> unaryPredicate) const
  {
    for (const auto& t : *this) if (unaryPredicate(t)) return &t;
    return end();
  }
  /*
   * find_last
   */
  T*
  find_last(std::function<bool(const T&)> predicate)
  {
    auto t = end();
    while (t != begin()) if (!predicate(*--t)) break;
    return ++t;
  }
  /*
   * find_last const
   */
  const T*
  find_last(std::function<bool(const T&)> unaryPredicate) const
  {
    const auto t = end();
    while (t != begin()) if (!predicate(*--t)) break;
    return ++t;
  }

  /*
   * --Modifying Operations
   */
  /*
   * erase
   */
  void
  erase(T* where)
  {
    RangeCheck(where);
    std::move(where + 1, end() + 1, where);
    --header()->size;
  }
  /*
   * insert
   */
  void
  insert( T* where,
    const T& what)
  {
    RangeCheck(where);
    std::move_backward(where, end() + 1, end() + 2);
    *where = what;
    ++header()->size;
  }
  /*
   * push_back
   */
  void
  push_back(const T& what)
  {
    RangeCheck(end() + 1);
    *end() = what;
    ++header()->size;
  }
  /*
   * ToString
   */
  std::string
  ToString() const
  {
    return header()->ToString() + "\n" + ContainerToString(*this);
  }

 private:

  /*
   * RangeCheck
   */
  bool 
  RangeCheck(const T* where)
  {
    if (where < begin() || ArrayEnd() <= where) {
      printf("Range Error: %p accessed: %p\n", this, where);
      return false;
    }
    return true;
  }
};

template<typename Key, typename Data>
struct Entry {
  Key key;
  Data data;

  std::string ToString(const Entry<Key,Data>& e) 
  {
    return "{key:" + std::to_string(e.key) + 
            ", data:" + std::to_string(e.data) + "}";
  }
};


/*
 * SpliceLastN
 */
template<typename H,typename T>
void
SpliceLastN(
    HeaderArray<H,T>* toArray,
    HeaderArray<H,T>* fromArray, 
    size_t n)
{
  //steal the last n
  T* whereFrom = fromArray->end() - n;
  std::move(whereFrom, fromArray->end() + 1, toArray.end());

  //adjust sizes
  fromArray->header()->size -= n;
  toArray->header()->size += n;
}

/*
 * NextUnique
 * Given range [first, last), and a value ival, let [first, ival] be the
 * range represented by [first], and the first occurence of ival in the
 * range [first, last).  Then the return value is the first occurence of a value in the
 * range (ival, last) that doesn't occur in [first, ival].
 */
template <
  typename ForwardIt,
  typename = std::enable_if_t< 
    std::is_base_of<
      std::forward_iterator_tag,
      typename std::iterator_traits<ForwardIt>::iterator_category
      >::value
    >
  >
ForwardIt
NextUnique(ForwardIt first, ForwardIt last, 
    const typename std::iterator_traits<ForwardIt>::value_type& ival)
{
  using value_type = typename std::iterator_traits<ForwardIt>::value_type;
  std::set<value_type> exclusion_set;
  bool found_ival = false;

  return std::find_if(
      first, last,
      [&exclusion_set, &found_ival, ival](const value_type& next) { 
        if (found_ival) return !exclusion_set.count(next);
        exclusion_set.insert(next);
        found_ival = next == ival;
        return false;
       }
  );
}


}; //namespace data_org_project_names



