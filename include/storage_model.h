#pragma once

#include <map>
#include <iostream>
#include <fstream>
#include <bitset>
#include <limits.h>

/**
 * API definition for page-based memory model.
 *
 * New page can be created using create_page function. This function 
 * return address of the created page.
 *
 * User can get access to the page data by using load_page function. 
 * Data at the returned pointer are managed by this class and user must
 * not delete them.
 *
 * Content of the page can be saved by calling save_page. User is 
 * responsible by providing correct combination of address and page
 * parameters. After call of this function the page pointer must no
 * longer be used as it may be deleted during call of this function.
 *
 * If you just need to update the page in the storage and keep 
 * it in the memory for further use, then you can employ update_page
 * function.
 * 
 * For both aforementioned functions (save_page and update_page),
 * the page must have been obtained by calling create_page function.
 * 
 * The release_page function can be used to release a page from
 * the primary memory without changing it in the secondary memory. This
 * function does NOT delete the page from the storage.
 *
 */
class storage_model {

public:

	virtual size_t get_page_size() const = 0;

	/**
	 * @return Address of new page.
	 */
	virtual size_t create_page() = 0;

	virtual char* load_page(size_t address) = 0;

	virtual void save_page(size_t address, char* page) = 0;

	virtual void update_page(size_t address, char* page) = 0;

	virtual void release_page(size_t address) = 0;

};

/**
 * In-memory implementation of the memory model.
 */
class unsafe_inmemory_storage : public storage_model {

public:

	unsafe_inmemory_storage(size_t page_size) {
		this->page_size = page_size;
	}

	~unsafe_inmemory_storage() {
		clear();
	}

	size_t get_page_size() const {
		return page_size;
	}

	size_t create_page() {
		size_t address = pages.size();
		char* page = new char[page_size];
		pages.insert(std::make_pair(address, page));
		return address;
	}

	char* load_page(size_t address) {
		return pages.at(address);
	}

	void save_page(size_t address, char* page) {
		// No operation here.
	}

	void update_page(size_t address, char* page) {
		// No operation here.
	}

	void release_page(size_t address) {
		// No operation here.
	}

public:

	void print_page(size_t address) {
		char* page = load_page(address);
		std::cout << "Page " << address << " :" << std::endl;
		for (size_t i = 0; i < page_size; ++i) {
			std::cout << std::bitset<CHAR_BIT>(page[i]) << " ";
		}
		std::cout << std::endl;
	}

	void save_to_file(const std::string& path) const {
		std::fstream stream(path, std::ios::out | std::ios::binary);
		stream << pages.size() << "\n";
		for (auto iter = pages.begin(); iter != pages.end(); ++iter) {
			write_page(stream, iter->first, iter->second);
		}
		stream.close();
	}

	void load_from_file(const std::string& path) {
		clear();
		std::fstream stream(path, std::ios::in | std::ios::binary);
		size_t page_count;
		stream >> page_count;
		for (size_t index = 0; index < page_count; ++index) {
			read_page(stream);
		}
		stream.close();
	}

	void clear() {
		for (auto iter = pages.begin(); iter != pages.end(); ++iter) {
			delete[] iter->second;
		}
		pages.clear();
	}

private:

	void write_page(std::fstream& stream, size_t index, char* data) const {
		stream << index << " ";
		stream.write(data, page_size);
		stream << "\n";
	}

	void read_page(std::fstream& stream) {
		size_t address;
		stream >> address;
		stream.get();
		char* page = new char[page_size];
		stream.read(page, page_size);
		pages.insert(std::make_pair(address, page));		
	}

private:

	size_t page_size;

	std::map<size_t, char*> pages;

};
