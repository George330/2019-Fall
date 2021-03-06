/*
 * find_bugs.cpp -- program with few bugs related to persistent memory
 * 	programming. Can you find them?
 *
 * create the pool for this program using pmempool, for example:
 *	pmempool create obj --layout=find_bugs -s 1G find_bugs
 */

#include <libpmemobj++/experimental/array.hpp>
#include <libpmemobj++/experimental/string.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/mutex.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pext.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

static const std::string LAYOUT = "find_bugs";

struct data {
	data(): simple_variable(0), pmem_property(0), vec(10) {
		int_ptr = pmem::obj::make_persistent<int>(10);
	}

	int simple_variable;

	pmem::obj::p<int> pmem_property;

	pmem::obj::experimental::vector<int> vec;

	pmem::obj::persistent_ptr<int> int_ptr;
};

struct root {
	pmem::obj::persistent_ptr<data> ptr;
	pmem::obj::persistent_ptr<data> ptr2;
};

int
main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " file-name" << std::endl;
		return 1;
	}

	auto path = argv[1];

	auto pop = pmem::obj::pool<root>::open(path, LAYOUT);

	auto r = pop.root();
	if (r->ptr == nullptr) {
		pmem::obj::transaction::run(pop, [&] {
			r->ptr = pmem::obj::make_persistent<data>();
			r->ptr2 = pmem::obj::make_persistent<data>();
		});
	}

	/*******************************  BUG 1  ******************************/

	pmem::obj::transaction::run(pop, [&]{
		r->ptr->simple_variable = 10;
	});

	/**********************************************************************/
	/*******************************  BUG 2  ******************************/

	auto &ref = r->ptr->vec[1];
	auto it = r->ptr->vec.begin() + 2;

	pmem::obj::transaction::run(pop, [&]{
		ref = 1;
		*it = 2;
	});

	/**********************************************************************/
	/*******************************  BUG 3  ******************************/

	r->ptr->vec.push_back(10);

	pmem::obj::transaction::run(pop, [&]{
		*(r->ptr->int_ptr) = 11;
	});

	/**********************************************************************/
	/*******************************  BUG 4  ******************************/

	r->ptr->vec[0] = 0;

	/**********************************************************************/
	/*******************************  BUG 5  ******************************/

	r->ptr2->pmem_property = r->ptr->pmem_property;
	pop.persist(r->ptr2->pmem_property);

	r->ptr2 = r->ptr;
	pop.persist(r->ptr2);

	/**********************************************************************/

	pop.close();

	return 0;
}
