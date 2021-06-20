#include "Terrible.h"

#include <cassert>
#include <sstream>

#define TR_NAME InnerStruct
struct InnerStruct
{
	TERRIBLE_M2(float, float_member, = 1.0f);
	TERRIBLE_M2(std::string, this_is_ridiculous, = "do not use this");

	bool operator==(InnerStruct const& other) const = default;
};

#define TR_NAME OuterStruct
struct OuterStruct
{
	TERRIBLE_M2(int, int_member1);
	TERRIBLE_M2(int, int_member2);
	TERRIBLE_M2(std::vector<InnerStruct>, vec_inner_member);
	TERRIBLE_M2(TR_(std::array<int, 5>), array_member, { 1,2,3,4,5 });

	bool operator==(OuterStruct const& other) const = default;
};

int main() {
	terrible::registerType<std::array<int, 5>>();
	terrible::registerType<int>();
	[[maybe_unused]]
	auto wat = LazyGlobal<terrible::impl::SerializationRegistration>->getRecords();

	OuterStruct obj;

	obj.int_member1 = 123;
	obj.int_member2 = -123;
	obj.vec_inner_member.push_back({ 70.0f });
	obj.vec_inner_member.push_back({ 71.0f });
	obj.vec_inner_member.push_back({ 72.0f });
	obj.vec_inner_member.back().this_is_ridiculous = "no really";
	obj.array_member[2] = 0;

	std::stringstream stream;

	auto writeSuccess = terrible::write(stream, obj);

	OuterStruct obj2;

	assert(obj != obj2);

	auto readSuccess = terrible::read(stream, obj2);
	auto success = obj == obj2;

	assert(obj == obj2);

	std::cout << stream.str();

	rand();

	return 0;
}