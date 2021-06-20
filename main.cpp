#include "Terrible.h"

#include <cassert>
#include <sstream>

void absert(bool b) {
	assert(b);
	if (!b) {
		std::abort();
	}
}

// TR_NAME needs to match the declaration of the struct for TERRIBLE_M, not needed for TERRIBLE_M2
#define TR_NAME Struct2
struct Struct2
{
	TERRIBLE_M(float, float_member, = 1.0f);
	TERRIBLE_M(std::string, string, = "string");
};

struct Struct1
{
	TERRIBLE_M2(int, int_member1);
	TERRIBLE_M2(int, int_member2);
	TERRIBLE_M2(std::vector<Struct2>, vec_structs);
	TERRIBLE_M2(TR_(std::array<int, 5>), array, { 1,2,3,4,5 });

	std::string string_member = "don't serialize this";
};

int main() {
	Struct1 obj;

	obj.int_member1 = 123;
	obj.int_member2 = -123;
	obj.vec_structs.push_back({ 70.0f });
	obj.vec_structs.push_back({ 71.0f });
	obj.vec_structs.push_back({ 72.0f });
	obj.vec_structs.back().string = "different string";
	obj.array[2] = 0;
	obj.string_member = "pp";

	std::stringstream stream;

	auto writeSuccess = terrible::write(stream, obj);
	absert(writeSuccess);

	Struct1 obj2;

	auto readSuccess = terrible::read(stream, obj2);
	absert(readSuccess);

	absert(obj2.int_member1 == obj.int_member1);
	absert(obj2.int_member2 == obj.int_member2);
	absert(obj2.vec_structs[0].float_member == obj.vec_structs[0].float_member);
	absert(obj2.vec_structs[1].float_member == obj.vec_structs[1].float_member);
	absert(obj2.vec_structs[2].float_member == obj.vec_structs[2].float_member);
	absert(obj2.vec_structs[2].string == obj.vec_structs[2].string);
	absert(obj2.array[2] == obj.array[2]);

	absert(obj2.string_member != obj.string_member);

	std::cout << "great success!";

	return 0;
}