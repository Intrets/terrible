#pragma once

#include <vector>
#include <iostream>
#include <unordered_map>
#include <string_view>
#include <optional>
#include <concepts>
#include <array>
#include <mutex>

#include "LazyGlobal.h"

#define TERRIBLE_XSTR(s) TERRIBLE_STR(s)
#define TERRIBLE_STR(s) #s
#define TERRIBLE_M(type, member, ...) \
	type member __VA_ARGS__;\
	inline static int __tr2##member = LazyGlobal<terrible::impl::SerializationRegistration>->registerMember<TR_NAME, type, &TR_NAME::member>(TERRIBLE_XSTR(TR_NAME), #member);

#define TERRIBLE_M2(type, member, ...) \
	type member __VA_ARGS__;\
	void __tr3##member() {\
		using ThisType = std::remove_pointer_t<decltype(this)>;\
		terrible::impl::RegisterStatic<ThisType, decltype(member), &ThisType::member>::val;\
	};

#define TR_(...) __VA_ARGS__

namespace terrible
{
	namespace impl
	{
		struct StructInformation;

		static bool writeString(std::ostream& out, std::string& str) {
			out << str.size() << " ";
			out.write(str.data(), str.size());
			return out.good();
		};

		static bool readString(std::istream& in, std::string& str) {
			size_t size;
			in >> size;
			in.get();
			str.resize(size);
			in.read(str.data(), str.size());
			return in.good();
		};

		struct MemberInfo
		{
			std::string memberName;
			int32_t type;
			StructInformation* typeStruct = nullptr;

			void* (*get)(void*) = nullptr;
		};

		struct SerializationRegistration;

		struct StructInformation
		{
			std::optional<std::string> structName;
			int32_t type = -1;
			std::vector<MemberInfo> members;

			bool (*write)(StructInformation*, std::ostream&, void*);
			bool (*read)(StructInformation*, std::istream&, void*);

			bool runWrite(std::ostream& out, void* val) {
				out << "\n";
				if (this->structName.has_value()) {
					writeString(out, this->structName.value());
				}
				else {
					std::string empty = "";
					writeString(out, empty);
				}

				return this->write(this, out, val);
			};
			bool runRead(std::istream& in, void* val) {
				std::string drain;
				readString(in, drain);

				return this->read(this, in, val);
			};

			StructInformation();
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
			std::unordered_map<int32_t, StructInformation> records;

			StructInformation currentStruct;

			template<class Struct, class Member, auto memberPointer>
			int registerMember(
				std::optional<std::string_view> structName,
				std::string_view memberName
			) {
				auto type = LazyGlobal<TypeIndex<Struct>>->val;
				this->records.try_emplace(type);

				MemberInfo member;
				member.memberName = memberName;
				member.type = LazyGlobal<TypeIndex<Member>>->val;
				member.get = [](void* obj) {
					return reinterpret_cast<void*>(&(reinterpret_cast<Struct*>(obj)->*memberPointer));
				};

				this->records[type].members.push_back(member);
				this->records[type].structName = structName;

				registerType<Member>();
				return 0;
			}

			std::unordered_map<int32_t, StructInformation>& getRecords() {
				return this->records;
			}

			template<class T>
			bool contains() {
				return this->records.contains(LazyGlobal<TypeIndex<T>>->val);
			}
		};

		template<class T>
		bool write(std::ostream& out, T& val) {
			return LazyGlobal<terrible::impl::SerializationRegistration>->records[LazyGlobal<terrible::impl::TypeIndex<T>>->val].runWrite(out, &val);
		}

		template<class T>
		bool read(std::istream& in, T& val) {
			return LazyGlobal<terrible::impl::SerializationRegistration>->records[LazyGlobal<terrible::impl::TypeIndex<T>>->val].runRead(in, &val);
		}

		void fillPointers();
	}


	template<class T>
	bool write(std::ostream& out, T& val) {
		terrible::impl::fillPointers();
		return impl::write(out, val);
	}

	template<class T>
	bool read(std::istream& in, T& val) {
		terrible::impl::fillPointers();
		return impl::read(in, val);
	}

	template<class T>
	void registerType();

	namespace impl
	{
		template<class Struct, class Member, auto memberPointer>
		struct RegisterStatic
		{
			inline static int val = LazyGlobal<SerializationRegistration>->registerMember<Struct, Member, memberPointer>(std::nullopt, "##NoMemberName");
		};

		template<class T>
		struct RegisterType;

		template<>
		struct RegisterType<std::string>
		{
			static void run() {
				using T = std::string;

				StructInformation info;
				info.structName = "String";
				info.type = LazyGlobal<TypeIndex<T>>->val;

				info.write = [](StructInformation* self, std::ostream& out, void* val) {
					T& str = *reinterpret_cast<T*>(val);
					out << str.size() << " ";
					out.write(str.data(), str.size());
					return out.good();
				};

				info.read = [](StructInformation* self, std::istream& in, void* val) {
					size_t size;
					in >> size;
					in.get();
					T& str = *reinterpret_cast<T*>(val);
					str.resize(size);
					in.read(str.data(), str.size());
					return in.good();
				};

				LazyGlobal<SerializationRegistration>->records[info.type] = info;
			}
		};

		template<class T>
		requires std::integral<T> || std::floating_point<T>
			struct RegisterType<T>
		{
			static void run() {
				StructInformation info;
				if constexpr (std::integral<T>) {
					info.structName = "integer";
				}
				else {
					info.structName = "float";
				}
				info.type = LazyGlobal<TypeIndex<T>>->val;

				info.write = [](StructInformation* self, std::ostream& out, void* val) {
					out << *reinterpret_cast<T*>(val) << " ";
					return out.good();
				};
				info.read = [](StructInformation* self, std::istream& in, void* val) {
					in >> *reinterpret_cast<T*>(val);
					return in.good();
				};

				LazyGlobal<SerializationRegistration>->records[info.type] = info;
			}
		};

		template<class E>
		struct RegisterType<std::vector<E>>
		{
			static void run() {
				using T = std::vector<E>;

				StructInformation info;
				info.structName = "vector";
				info.type = LazyGlobal<TypeIndex<T>>->val;

				MemberInfo valueMember;
				valueMember.type = LazyGlobal<TypeIndex<E>>->val;
				info.members.push_back(valueMember);

				info.write = [](StructInformation* self, std::ostream& out, void* val_) {
					T& val = *reinterpret_cast<T*>(val_);
					auto typeStruct = self->members.front().typeStruct;

					out << val.size() << " ";
					for (auto& v : val) {
						typeStruct->runWrite(out, &v);
					}
					return out.good();
				};

				info.read = [](StructInformation* self, std::istream& in, void* val_) {
					T& val = *reinterpret_cast<T*>(val_);
					auto typeStruct = self->members.front().typeStruct;

					size_t s;
					in >> s;

					val.resize(s);
					for (auto& v : val) {
						typeStruct->runRead(in, &v);
					}
					return in.good();
				};

				LazyGlobal<SerializationRegistration>->records[info.type] = info;

				terrible::registerType<E>();
			}
		};

		template<class E, size_t N>
		struct RegisterType<std::array<E, N>>
		{
			static void run() {
				using T = std::array<E, N>;

				StructInformation info;
				info.type = LazyGlobal<TypeIndex<T>>->val;

				MemberInfo valueMember;
				valueMember.type = LazyGlobal<TypeIndex<E>>->val;
				info.members.push_back(valueMember);

				info.write = [](StructInformation* self, std::ostream& out, void* val_) {
					T& val = *reinterpret_cast<T*>(val_);
					auto typeStruct = self->members.front().typeStruct;

					out << val.size() << " ";
					for (auto& v : val) {
						typeStruct->runWrite(out, &v);
					}
					return out.good();
				};

				info.read = [](StructInformation* self, std::istream& in, void* val_) {
					T& val = *reinterpret_cast<T*>(val_);
					auto typeStruct = self->members.front().typeStruct;

					size_t s;
					in >> s;

					if (s != N) {
						return false;
					}

					for (auto& v : val) {
						typeStruct->runRead(in, &v);
					}
					return in.good();
				};

				LazyGlobal<SerializationRegistration>->records[info.type] = info;
			}
		};
	}

	template<class T>
	requires requires () { impl::RegisterType<T>::run(); }
	void registerType() {
		if (!LazyGlobal<impl::SerializationRegistration>->contains<T>()) {
			impl::RegisterType<T>::run();
		}
	}

	template<class T>
	void registerType() {
	}
}

