#include "Terrible.h"

namespace terrible::impl
{
	StructInformation2::StructInformation2() {
		this->write = [](StructInformation2* self, std::ostream& out, void* ptr) {
			//out << self->name << "\n";

			for (auto& member : self->members) {
				auto& selfNext = LazyGlobal<SerializationRegistration>->records[member.type];
				if (!selfNext.runWrite(out, member.get(ptr))) {
					return false;
				}
			}
			return out.good();
		};

		this->read = [](StructInformation2* self, std::istream& in, void* ptr) {
			//std::string name;
			//in >> name;

			for (auto& member : self->members) {
				auto& selfNext = LazyGlobal<SerializationRegistration>->records[member.type];
				if (!selfNext.runRead(in, member.get(ptr))) {
					return false;
				}
			}
			return in.good();
		};
	}
}