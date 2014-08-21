/*
 * Copyright 2014 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FRUIT_UNSAFE_COMPONENT_TEMPLATES_H
#define FRUIT_UNSAFE_COMPONENT_TEMPLATES_H

#include "metaprogramming.h"
#include "demangle_type_name.h"
#include "type_info.h"
#include "fruit_assert.h"

// Redundant, but makes KDevelop happy.
#include "component_storage.h"

namespace fruit {
namespace impl {

template <typename AnnotatedSignature>
struct BindAssistedFactory;

// General case, value
template <typename C>
struct GetHelper {
  C operator()(ComponentStorage& component) {
    return *(component.getPtr<C>());
  }
};

template <typename C>
struct GetHelper<const C> {
  const C operator()(ComponentStorage& component) {
    return *(component.getPtr<C>());
  }
};

template <typename C>
struct GetHelper<std::shared_ptr<C>> {
  std::shared_ptr<C> operator()(ComponentStorage& component) {
    return std::shared_ptr<C>(std::shared_ptr<char>(), component.getPtr<C>());
  }
};

template <typename C>
struct GetHelper<C*> {
  C* operator()(ComponentStorage& component) {
    return component.getPtr<C>();
  }
};

template <typename C>
struct GetHelper<const C*> {
  const C* operator()(ComponentStorage& component) {
    return component.getPtr<C>();
  }
};

template <typename C>
struct GetHelper<C&> {
  C& operator()(ComponentStorage& component) {
    return *(component.getPtr<C>());
  }
};

template <typename C>
struct GetHelper<const C&> {
  const C& operator()(ComponentStorage& component) {
    return *(component.getPtr<C>());
  }
};

template <typename... Ps>
struct GetHelper<Provider<Ps...>> {
  Provider<Ps...> operator()(ComponentStorage& storage) {
    return Provider<Ps...>(&storage);
  }
};

// Non-assisted case.
template <int numAssistedBefore, typename Arg, typename ParamTuple>
struct GetAssistedArgHelper {
  auto operator()(ComponentStorage& m, ParamTuple) -> decltype(m.get<Arg>()) {
    return m.get<Arg>();
  }
};

// Assisted case.
template <int numAssistedBefore, typename Arg, typename ParamTuple>
struct GetAssistedArgHelper<numAssistedBefore, Assisted<Arg>, ParamTuple> {
  auto operator()(ComponentStorage&, ParamTuple paramTuple) -> decltype(std::get<numAssistedBefore>(paramTuple)) {
    return std::get<numAssistedBefore>(paramTuple);
  }
};

template <int index, typename AnnotatedArgs, typename ParamTuple>
struct GetAssistedArg : public GetAssistedArgHelper<NumAssistedBefore<index, AnnotatedArgs>::value, GetNthType<index, AnnotatedArgs>, ParamTuple> {};

template <typename AnnotatedSignature, typename InjectedFunctionType, typename Sequence>
class BindAssistedFactoryHelperForValue {};

template <typename AnnotatedSignature, typename C, typename... Params, int... indexes>
class BindAssistedFactoryHelperForValue<AnnotatedSignature, C(Params...), IntList<indexes...>> {
private:
  /* std::function<C(Params...)>, C(Args...) */
  using RequiredSignature = ConstructSignature<SignatureType<AnnotatedSignature>, RequiredArgsForAssistedFactory<AnnotatedSignature>>;
  
  ComponentStorage& storage;
  RequiredSignature* factory;
  
public:
  BindAssistedFactoryHelperForValue(ComponentStorage& storage, RequiredSignature* factory) 
    :storage(storage), factory(factory) {}

  C operator()(Params... params) {
      return factory(GetAssistedArg<indexes, SignatureArgs<AnnotatedSignature>, decltype(std::tie(params...))>()(storage, std::tie(params...))...);
  }
};

template <typename AnnotatedSignature, typename InjectedFunctionType, typename Sequence>
class BindAssistedFactoryHelperForPointer {};

template <typename AnnotatedSignature, typename C, typename... Params, int... indexes>
class BindAssistedFactoryHelperForPointer<AnnotatedSignature, std::unique_ptr<C>(Params...), IntList<indexes...>> {
private:
  /* std::function<std::unique_ptr<C>(Params...)>, std::unique_ptr<C>(Args...) */
  using RequiredSignature = ConstructSignature<std::unique_ptr<C>, RequiredArgsForAssistedFactory<AnnotatedSignature>>;
  
  ComponentStorage& storage;
  RequiredSignature* factory;
  
public:
  BindAssistedFactoryHelperForPointer(ComponentStorage& storage, RequiredSignature* factory) 
    :storage(storage), factory(factory) {}

  std::unique_ptr<C> operator()(Params... params) {
      return factory(GetAssistedArg<indexes, SignatureArgs<AnnotatedSignature>, decltype(std::tie(params...))>()(storage, std::tie(params...))...);
  }
};

template <typename AnnotatedSignature>
struct BindAssistedFactoryForValue : public BindAssistedFactoryHelperForValue<
      AnnotatedSignature,
      ConstructSignature<SignatureType<AnnotatedSignature>, InjectedFunctionArgsForAssistedFactory<AnnotatedSignature>>,
      GenerateIntSequence<
        list_size<
          RequiredArgsForAssistedFactory<AnnotatedSignature>
        >::value
      >> {
  BindAssistedFactoryForValue(ComponentStorage& storage, ConstructSignature<SignatureType<AnnotatedSignature>, RequiredArgsForAssistedFactory<AnnotatedSignature>>* factory) 
    : BindAssistedFactoryHelperForValue<
      AnnotatedSignature,
      ConstructSignature<SignatureType<AnnotatedSignature>, InjectedFunctionArgsForAssistedFactory<AnnotatedSignature>>,
      GenerateIntSequence<
        list_size<
          RequiredArgsForAssistedFactory<AnnotatedSignature>
        >::value
      >>(storage, factory) {}
};

template <typename AnnotatedSignature>
struct BindAssistedFactoryForPointer : public BindAssistedFactoryHelperForPointer<
      AnnotatedSignature,
      ConstructSignature<std::unique_ptr<SignatureType<AnnotatedSignature>>, InjectedFunctionArgsForAssistedFactory<AnnotatedSignature>>,
      GenerateIntSequence<
        list_size<
          RequiredArgsForAssistedFactory<AnnotatedSignature>
        >::value
      >> {
  BindAssistedFactoryForPointer(ComponentStorage& storage, ConstructSignature<std::unique_ptr<SignatureType<AnnotatedSignature>>, RequiredArgsForAssistedFactory<AnnotatedSignature>>* factory) 
    : BindAssistedFactoryHelperForPointer<
      AnnotatedSignature,
      ConstructSignature<std::unique_ptr<SignatureType<AnnotatedSignature>>, InjectedFunctionArgsForAssistedFactory<AnnotatedSignature>>,
      GenerateIntSequence<
        list_size<
          RequiredArgsForAssistedFactory<AnnotatedSignature>
        >::value
      >>(storage, factory) {}
};

template <typename MessageGenerator>
inline void ComponentStorage::check(bool b, MessageGenerator messageGenerator) {
  if (!b) {
    printError(messageGenerator());
    abort();
  }
}

template <typename C>
inline void ComponentStorage::createBindingData(void* (*create)(ComponentStorage&, void*),
                                             void* createArgument,
                                             void (*deleteOperation)(void*)) {
  createBindingData(getTypeInfo<C>(), create, createArgument, deleteOperation);
}

template <typename C>
inline void ComponentStorage::createBindingData(void* instance,
                                                void (*destroy)(void*)) {
  createBindingData(getTypeInfo<C>(), instance, destroy);
}

template <typename C>
inline void ComponentStorage::createBindingDataForMultibinding(void* (*create)(ComponentStorage&, void*),
                                                               void* createArgument,
                                                               void (*deleteOperation)(void*)) {
  createBindingDataForMultibinding(getTypeInfo<C>(), create, createArgument, deleteOperation, createSingletonSet<C>);
}

template <typename C>
inline void ComponentStorage::createBindingDataForMultibinding(void* instance,
                                                               void (*destroy)(void*)) {
  createBindingDataForMultibinding(getTypeInfo<C>(), instance, destroy, createSingletonSet<C>);
}

template <typename C>
inline std::shared_ptr<char> ComponentStorage::createSingletonSet(ComponentStorage& storage) {
  const TypeInfo* typeInfo = getTypeInfo<C>();
  auto itr = storage.typeRegistryForMultibindings.find(typeInfo);
  if (itr == storage.typeRegistryForMultibindings.end()) {
    // No multibindings.
    // We don't cache this result to remain thread-safe (as long as eager injection was performed).
    return nullptr;
  }
  if (itr->second.s.get() != nullptr) {
    // Result cached, return early.
    return itr->second.s;
  }
  
  BindingDataForMultibinding& bindingDataForMultibinding = storage.typeRegistryForMultibindings[typeInfo];
  storage.ensureConstructedMultibinding(getTypeInfo<C>(), bindingDataForMultibinding);
  
  std::set<C*> s;
  for (const BindingData& bindingData : itr->second.bindingDatas) {
    s.insert(reinterpret_cast<C*>(bindingData.storedSingleton));
  }
  
  std::shared_ptr<std::set<C*>> sPtr = std::make_shared<std::set<C*>>(std::move(s));
  std::shared_ptr<char> result(sPtr, reinterpret_cast<char*>(sPtr.get()));
  
  bindingDataForMultibinding.s = result;
  
  return result;
}

template <typename C>
inline C* ComponentStorage::getPtr() {
  void* p = getPtr(getTypeInfo<C>());
  return reinterpret_cast<C*>(p);
}

// I, C must not be pointers.
template <typename I, typename C>
inline void ComponentStorage::bind() {
  FruitStaticAssert(!std::is_pointer<I>::value, "I should not be a pointer");
  FruitStaticAssert(!std::is_pointer<C>::value, "C should not be a pointer");
  auto create = [](ComponentStorage& m, void*) {
    C* cPtr = m.getPtr<C>();
    // This step is needed when the cast C->I changes the pointer
    // (e.g. for multiple inheritance).
    I* iPtr = static_cast<I*>(cPtr);
    return reinterpret_cast<void*>(iPtr);
  };
  createBindingData<I>(create, nullptr, nopDeleter);
}

template <typename C>
inline void ComponentStorage::bindInstance(C& instance) {
  createBindingData<C>(&instance, nopDeleter);
}

template <typename C, typename... Args>
inline C* ComponentStorage::constructSingleton(Args&&... args) {
  size_t misalignment = (singletonStorageNumUsedBytes % alignof(C));
  if (misalignment != 0) {
    singletonStorageNumUsedBytes += alignof(C) - misalignment;
  }
  C* c = reinterpret_cast<C*>(singletonStorageBegin + singletonStorageNumUsedBytes);
  new (c) C(std::forward<Args>(args)...);
  singletonStorageNumUsedBytes += sizeof(C);
  return c;
}

template <typename C, typename... Args>
inline void ComponentStorage::registerProvider(C* (*provider)(Args...)) {
  FruitStaticAssert(!std::is_pointer<C>::value, "C should not be a pointer");
  check(provider != nullptr, "attempting to register nullptr as provider");
  using provider_type = decltype(provider);
  auto create = [](ComponentStorage& m, void* arg) {
    provider_type provider = reinterpret_cast<provider_type>(arg);
    C* cPtr = provider(m.get<Args>()...);
    return reinterpret_cast<void*>(cPtr);
  };
  auto destroy = [](void* p) {
    C* cPtr = reinterpret_cast<C*>(p);
    delete cPtr;
  };
  createBindingData<C>(create, reinterpret_cast<void*>(provider), destroy);
}

template <typename C, bool is_movable, typename... Args>
struct RegisterValueProviderHelper {};

template <typename C, typename... Args>
inline void ComponentStorage::registerProvider(C (*provider)(Args...)) {
  FruitStaticAssert(!std::is_pointer<C>::value, "C should not be a pointer");
  // TODO: Move this check into ComponentImpl.
  static_assert(std::is_move_constructible<C>::value, "C should be movable");
  check(provider != nullptr, "attempting to register nullptr as provider");
  using provider_type = decltype(provider);
  auto create = [](ComponentStorage& m, void* arg) {
    provider_type provider = reinterpret_cast<provider_type>(arg);
    C* cPtr = m.constructSingleton<C, Args...>(provider(m.get<Args>()...));
    return reinterpret_cast<void*>(cPtr);
  };
  auto destroy = [](void* p) {
    C* cPtr = reinterpret_cast<C*>(p);
    cPtr->C::~C();
  };
  createBindingData<C>(create, reinterpret_cast<void*>(provider), destroy);
}

template <typename C, typename... Args>
inline void ComponentStorage::registerConstructor() {
  FruitStaticAssert(!std::is_pointer<C>::value, "C should not be a pointer");
  auto create = [](ComponentStorage& m, void* arg) {
    (void)arg;
    C* cPtr = m.constructSingleton<C, Args...>(m.get<Args>()...);
    return reinterpret_cast<void*>(cPtr);
  };
  auto destroy = [](void* p) {
    C* cPtr = reinterpret_cast<C*>(p);
    cPtr->C::~C();
  };
  createBindingData<C>(create, nullptr, destroy);
}

// I, C must not be pointers.
template <typename I, typename C>
inline void ComponentStorage::addMultibinding() {
  FruitStaticAssert(!std::is_pointer<I>::value, "I should not be a pointer");
  FruitStaticAssert(!std::is_pointer<C>::value, "C should not be a pointer");
  auto create = [](ComponentStorage& m, void*) {
    C* cPtr = m.getPtr<C>();
    // This step is needed when the cast C->I changes the pointer
    // (e.g. for multiple inheritance).
    I* iPtr = static_cast<I*>(cPtr);
    return reinterpret_cast<void*>(iPtr);
  };
  createBindingDataForMultibinding<I>(create, nullptr, nopDeleter);
}

template <typename C>
inline void ComponentStorage::addInstanceMultibinding(C& instance) {
  createBindingDataForMultibinding<C>(&instance, nopDeleter);
}

template <typename C, typename... Args>
inline void ComponentStorage::registerMultibindingProvider(C* (*provider)(Args...)) {
  FruitStaticAssert(!std::is_pointer<C>::value, "C should not be a pointer");
  check(provider != nullptr, "attempting to register nullptr as provider");
  using provider_type = decltype(provider);
  auto create = [](ComponentStorage& m, void* arg) {
    provider_type provider = reinterpret_cast<provider_type>(arg);
    C* cPtr = provider(m.get<std::forward<Args>>()...);
    return reinterpret_cast<void*>(cPtr);
  };
  auto destroy = [](void* p) {
    C* cPtr = reinterpret_cast<C*>(p);
    delete cPtr;
  };
  createBindingDataForMultibinding<C>(create, reinterpret_cast<void*>(provider), destroy);
}

template <typename C, typename... Args>
inline void ComponentStorage::registerMultibindingProvider(C (*provider)(Args...)) {
  FruitStaticAssert(!std::is_pointer<C>::value, "C should not be a pointer");
  // TODO: Move this check into ComponentImpl.
  static_assert(std::is_move_constructible<C>::value, "C should be movable");
  check(provider != nullptr, "attempting to register nullptr as provider");
  using provider_type = decltype(provider);
  auto create = [](ComponentStorage& m, void* arg) {
    provider_type provider = reinterpret_cast<provider_type>(arg);
    C* cPtr = m.constructSingleton<C, Args...>(provider(m.get<Args>()...));
    return reinterpret_cast<void*>(cPtr);
  };
  auto destroy = [](void* p) {
    C* cPtr = reinterpret_cast<C*>(p);
    cPtr->C::~C();
  };
  createBindingDataForMultibinding<C>(create, reinterpret_cast<void*>(provider), destroy);
}

template <typename AnnotatedSignature, typename... Argz>
inline void ComponentStorage::registerFactory(SignatureType<AnnotatedSignature>(*factory)(Argz...)) {
  check(factory != nullptr, "attempting to register nullptr as factory");
  using Signature = ConstructSignature<SignatureType<AnnotatedSignature>, RequiredArgsForAssistedFactory<AnnotatedSignature>>;
  using InjectedFunctionType = ConstructSignature<SignatureType<AnnotatedSignature>, InjectedFunctionArgsForAssistedFactory<AnnotatedSignature>>;
  auto create = [](ComponentStorage& m, void* arg) {
    Signature* factory = reinterpret_cast<Signature*>(arg);
    std::function<InjectedFunctionType>* fPtr = 
        new std::function<InjectedFunctionType>(BindAssistedFactoryForValue<AnnotatedSignature>(m, factory));
    return reinterpret_cast<void*>(fPtr);
  };
  createBindingData<std::function<InjectedFunctionType>>(create, reinterpret_cast<void*>(factory), standardDeleter<std::function<InjectedFunctionType>>);
}

template <typename AnnotatedSignature, typename... Argz>
inline void ComponentStorage::registerFactory(std::unique_ptr<SignatureType<AnnotatedSignature>>(*factory)(Argz...)) {
  check(factory != nullptr, "attempting to register nullptr as factory");
  using Signature = ConstructSignature<std::unique_ptr<SignatureType<AnnotatedSignature>>, RequiredArgsForAssistedFactory<AnnotatedSignature>>;
  using InjectedFunctionType = ConstructSignature<std::unique_ptr<SignatureType<AnnotatedSignature>>, InjectedFunctionArgsForAssistedFactory<AnnotatedSignature>>;
  auto create = [](ComponentStorage& m, void* arg) {
    Signature* factory = reinterpret_cast<Signature*>(arg);
    std::function<InjectedFunctionType>* fPtr = 
        new std::function<InjectedFunctionType>(BindAssistedFactoryForPointer<AnnotatedSignature>(m, factory));
    return reinterpret_cast<void*>(fPtr);
  };
  createBindingData<std::function<InjectedFunctionType>>(create, reinterpret_cast<void*>(factory), standardDeleter<std::function<InjectedFunctionType>>);
}

template <typename C>
std::set<C*> ComponentStorage::getMultibindings() {
  void* p = getMultibindings(getTypeInfo<C>());
  if (p == nullptr) {
    return {};
  } else {
    return *reinterpret_cast<std::set<C*>*>(p);
  }
}

template <typename C>
inline ComponentStorage::BindingData& ComponentStorage::getBindingData() {
  TypeInfo* typeInfo = getTypeInfo<C>();
  auto itr = typeRegistry.find(typeInfo);
  FruitCheck(itr != typeRegistry.end(), [=](){return "attempting to getBindingData() on a non-registered type: " + typeInfo->name();});
  return itr->second;
}

} // namespace fruit
} // namespace impl


#endif // FRUIT_UNSAFE_COMPONENT_TEMPLATES_H