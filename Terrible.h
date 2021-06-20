#pragma once

#include <vector>
#include <iostream>
#include <unordered_map>
#include <string_view>
#include <optional>
#include <concepts>
#include <array>

#include "LazyGlobal.h"

#define TERRIBLE_XSTR(s) STR(s)
#define TERRIBLE_STR(s) #s
#define TERRIBLE_M(type, member, ...) type member __VA_ARGS__; inline static int ___tr2##member = LazyGlobal<terrible::impl::SerializationRegistration>->registerMember<TR_NAME, type, &TR_NAME::member>(TERRIBLE_XSTR(TR_NAME), #member);
#define TERRIBLE_M2(type, member, ...) type member __VA_ARGS__; void __tr3##member() { using ThisType = std::remove_pointer_t<decltype(this)>; [[maybe_unused]] auto i = terrible::impl::RegisterStatic<ThisType, decltype(member), &ThisType::member>::val; };

#define TR_(...) __VA_ARGS__

namespace terrible
{
	namespace impl
	{
		struct MemberInfo
		{
			//std::string name;
			int32_t type;
			//StructInformation2* next = nullptr;

			void* (*get)(void*) = nullptr;
		};

		struct SerializationRegistration;

		struct StructInformation2
		{
			//std::string name;
			int32_t type = -1;
			std::vector<MemberInfo> members;

			bool (*write)(StructInformation2*, std::ostream&, void*);
			bool (*read)(StructInformation2*, std::istream&, void*);

			bool runWrite(std::ostream& out, void* val) {
				return this->write(this, out, val);
			};
			bool runRead(std::istream& in, void* val) {
				return this->read(this, in, val);
			};

			StructInformation2();
		};

		struct TypeIndexCounter
		{
			int32_t index;
			int32_t advance() {
				return this->index++;
			}
		};

		template<class T>
		struct TypeIndex
		{
			int32_t val = LazyGlobal<TypeIndexCounter>->advance();
		};

		struct SerializationRegistration
		{
			std::unordered_map<int32_t, StructInformation2> records;

			StructInformation2 currentStruct;

			void cycle(std::optional<int32_t> type = std::nullopt) {
				if (this->currentStruct.type != -1) {
					if (!type.has_value() || this->currentStruct.type != type.value()) {
						this->records[this->currentStruct.type] = this->currentStruct;
						this->currentStruct = {};
					}
				}
			}

			template<class Struct, class Member, auto memberPointer>
			int registerMember(
				std::string_view structName,
				std::string_view memberName
			) {
				this->cycle(LazyGlobal<TypeIndex<Struct>>->val);
				//this->currentStruct.name = structName;
				this->currentStruct.type = LazyGlobal<TypeIndex<Struct>>->val;
				MemberInfo member;
				member.type = LazyGlobal<TypeIndex<Member>>->val;
				member.get = [](void* obj) {
					return reinterpret_cast<void*>(&(reinterpret_cast<Struct*>(obj)->*memberPointer));
				};

				this->currentStruct.members.push_back(member);

				registerType<Member>();
				return 0;
			}

			std::unordered_map<int32_t, StructInformation2>& getRecords() {
				this->cycle();
				return this->records;
			}
		};
	}

	template<class T>
	bool write(std::ostream& out, T& val) {
		return LazyGlobal<terrible::impl::SerializationRegistration>->records[LazyGlobal<terrible::impl::TypeIndex<T>>->val].runWrite(out, &val);
	}

	template<class T>
	bool read(std::istream& in, T& val) {
		return LazyGlobal<terrible::impl::SerializationRegistration>->records[LazyGlobal<terrible::impl::TypeIndex<T>>->val].runRead(in, &val);
	}

	namespace impl
	{
		template<class Struct, class Member, auto memberPointer>
		struct RegisterStatic
		{
			inline static int val = LazyGlobal<SerializationRegistration>->registerMember<Struct, Member, memberPointer>("NestedMadness", "madness3");
		};

		template<class T>
		struct RegisterType;

		template<>
		struct RegisterType<std::string>
		{
			static void run() {
				using T = std::string;
				StructInformation2 info;
				info.write = [](StructInformation2* self, std::ostream& out, void* val) {
					T& str = *reinterpret_cast<T*>(val);
					out << str.size() << " ";
					out.write(str.data(), str.size());
					return out.good();
				};
				info.read = [](StructInformation2* self, std::istream& in, void* val) {
					size_t size;
					in >> size;
					in >> std::ws;
					T& str = *reinterpret_cast<T*>(val);
					str.resize(size);
					in.read(str.data(), str.size());
					return in.good();
				};
				info.type = LazyGlobal<TypeIndex<T>>->val;

				LazyGlobal<SerializationRegistration>->records[info.type] = info;
			}
		};

		template<class T>
		requires std::integral<T> || std::floating_point<T>
			struct RegisterType<T>
		{
			static void run() {
				StructInformation2 info;
				info.write = [](StructInformation2* self, std::ostream& out, void* val) {
					out << *reinterpret_cast<T*>(val) << " ";
					return out.good();
				};
				info.read = [](StructInformation2* self, std::istream& in, void* val) {
					in >> *reinterpret_cast<T*>(val);
					return in.good();
				};
				info.type = LazyGlobal<TypeIndex<T>>->val;

				LazyGlobal<SerializationRegistration>->records[info.type] = info;
			}
		};

		template<class E>
		struct RegisterType<std::vector<E>>
		{
			static void run() {
				using T = std::vector<E>;
				StructInformation2 info;
				info.write = [](StructInformation2* self, std::ostream& out, void* val_) {
					T& val = *reinterpret_cast<T*>(val_);

					out << val.size() << " ";
					for (auto& v : val) {
						terrible::write(out, v);
					}
					return out.good();
				};
				info.read = [](StructInformation2* self, std::istream& in, void* val_) {
					T& val = *reinterpret_cast<T*>(val_);
					size_t s;
					in >> s;

					val.resize(s);
					for (auto& v : val) {
						terrible::read(in, v);
					}
					return in.good();
				};
				info.type = LazyGlobal<TypeIndex<T>>->val;

				LazyGlobal<SerializationRegistration>->records[info.type] = info;
			}
		};

		template<class E, size_t N>
		struct RegisterType<std::array<E, N>>
		{
			static void run() {
				using T = std::array<E, N>;
				StructInformation2 info;
				info.write = [](StructInformation2* self, std::ostream& out, void* val_) {
					T& val = *reinterpret_cast<T*>(val_);

					out << val.size() << " ";
					for (auto& v : val) {
						terrible::write(out, v);
					}
					return out.good();
				};
				info.read = [](StructInformation2* self, std::istream& in, void* val_) {
					T& val = *reinterpret_cast<T*>(val_);
					size_t s;
					in >> s;

					if (s != N) {
						return false;
					}

					for (auto& v : val) {
						terrible::read(in, v);
					}
					return in.good();
				};
				info.type = LazyGlobal<TypeIndex<T>>->val;

				LazyGlobal<SerializationRegistration>->records[info.type] = info;
			}

		};
	}

	template<class T>
	void registerType() {
		impl::RegisterType<T>::run();
	}
}

