Look at main.cpp.

Expanded macros: 
```cpp
/*
struct Expanded_M2
{
    TERRIBLE_M2(TR_(std::array<int, 5>, array, { 1,2,3,4,5 });
};
*/

struct Expanded_M2
{
    // TERRIBLE_M2(TR_(std::array<int, 5>), array, { 1,2,3,4,5 });
    std::array<int, 5> array{ 1,2,3,4,5 };
    void __tr3array() {
        using ThisType = std::remove_pointer_t<decltype(this)>;
        terrible::impl::RegisterStatic<ThisType, decltype(array), &ThisType::array>::val;
    }
};
```

```cpp
/*
#define TR_NAME Expanded_M
struct Expanded_M
{
    TERRIBLE_M(float, float_member, = 1.0f);
};
*/

struct Expanded_M
{
    float float_member = 1.0f;
    inline static int __tr2float_member = LazyGlobal<terrible::impl::SerializationRegistration>->
        registerMember<Expanded_M, float, &Expanded_M::float_member>("Expanded_M", "float_member");
};
```
