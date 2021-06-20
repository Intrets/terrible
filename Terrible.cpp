#include "Terrible.h"

namespace terrible::impl
{
	std::once_flag once;

	void processStruct(StructInformation& structInfo) {
		for (auto& member : structInfo.members) {
			auto& s = LazyGlobal<SerializationRegistration>->records[member.type];
			member.typeStruct = &s;
		}
	}

	void fillPointers() {
		std::call_once(once, []() {
			for (auto& [k, v] : LazyGlobal<SerializationRegistration>->records) {
				processStruct(v);
			}
			});
	}


	StructInformation::StructInformation() {
		this->write = [](StructInformation* self, std::ostream& out, void* ptr) {
			for (auto& member : self->members) {
				if (!member.typeStruct->runWrite(out, member.get(ptr))) {
					return false;
				}
			}
			return out.good();
		};

		this->read = [](StructInformation* self, std::istream& in, void* ptr) {
			for (auto& member : self->members) {
				if (!member.typeStruct->runRead(in, member.get(ptr))) {
					return false;
				}
			}
			return in.good();
		};
	}
}