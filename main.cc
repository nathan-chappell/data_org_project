
#include <iostream>
#include <string>
#include "storage_model.hpp"

// Example class stored in the storage page.
struct node_record {

	int pointer;

	int separator;

};

void populate_database(storage_model& storage) {
	size_t address = storage.create_block();
	char* block = storage.load_block(address);

	size_t records_count = storage.get_page_size() / sizeof(node_record);

	node_record* records = reinterpret_cast<node_record*>(block);
	
	records[0].pointer = 1;
	records[0].separator = 2;
	
	records[records_count - 1].pointer = 1;
	records[records_count - 1].separator = 2;

	std::cout << "  database info. record size: " << sizeof(node_record) << " records per page: " << records_count << std::endl;
}

void print_some_data(storage_model& storage) {
	// We know there is address zero.
	node_record* records = reinterpret_cast<node_record*>(storage.load_block(0));
	std::cout << "  pointer: " << records[0].pointer << " separator: " << records[0].separator << std::endl;
}

int main()
{
	unsafe_inmemory_storage storage(32);
	
	std::cout << "Initialize database and save to a file" << std::endl;
	populate_database(storage);
	storage.save_to_file("database.dat");
	
	std::cout << "Database content" << std::endl;
	print_some_data(storage);

	std::cout << "Clear and load from file" << std::endl;
	storage.clear();
	storage.load_from_file("database.dat");

	std::cout << "Database content" << std::endl;
	print_some_data(storage);

	return 0;
}
