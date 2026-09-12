#include <array>
#include <tuple>
#include <stack>
#include <stdexcept>
#include <unordered_map>
#include <Rocket/UI/Reconciler.hpp>

namespace Rocket {

auto constexpr _CONTEXT_CAPACITY = 5;

struct Reconciler::_Component {
    std::int64_t type = 0;
    std::int64_t index = 0;
    std::string id;
    std::string key;
    std::string name;
    std::int64_t revision = 0;
    std::int64_t nextChildIndex = 0;
    std::int64_t nextStateIndex = 0;
    std::unordered_map<std::int64_t, std::any> state;
    std::unordered_map<std::int64_t, std::int64_t> keySeq;
    _Component* parent = nullptr;
    _Component* firstChild = nullptr;
    _Component* lastChild = nullptr;
    _Component* nextSibling = nullptr;
    _Component* prevSibling = nullptr;
    std::int64_t contextCount = 0;
    std::array<
        std::tuple<std::type_index, void*>,
        _CONTEXT_CAPACITY
    > contexts = {
        std::tuple{ std::type_index(typeid(void)), nullptr },
        std::tuple{ std::type_index(typeid(void)), nullptr },
        std::tuple{ std::type_index(typeid(void)), nullptr },
        std::tuple{ std::type_index(typeid(void)), nullptr },
        std::tuple{ std::type_index(typeid(void)), nullptr }
    };
};

static auto _currentReconciler = std::stack<Reconciler*>();

static Reconciler& _GetCurrentReconciler() {
    PROFILE

    if (_currentReconciler.empty()) {
        throw std::runtime_error("No reconciler is currently updating");
    }

    return *_currentReconciler.top();
}

_ComponentScope::_ComponentScope(std::int64_t type, std::string const& name, std::string const& key) {
    PROFILE

    _GetCurrentReconciler()._openComponent(type, name, key);
}

_ComponentScope::~_ComponentScope() {
    PROFILE

    _GetCurrentReconciler()._closeComponent();
}

Reconciler::~Reconciler() {
    PROFILE

    _rootComponent->revision += 1;
    _unmountComponent(_rootComponent);

    delete _rootComponent;
}

Reconciler::Reconciler()
    : onAfterUpdate()
    , _updateMode(OnUpdate)
    , _updateFn(nullptr)
    , _rootComponent(new _Component())
    , _currentComponent(nullptr)
    , _isUpdating(false)
    , _needsUpdate(false) {}

ReconcilerUpdateMode Reconciler::getUpdateMode() const {
    PROFILE

    return _updateMode;
}

void Reconciler::setUpdateMode(ReconcilerUpdateMode updateMode) {
    PROFILE

    _updateMode = updateMode;
}

void Reconciler::setUpdateFn(std::function<void()> const& updateFn) {
    PROFILE

    _updateFn = updateFn;
}

void Reconciler::needsUpdate() {
    PROFILE

    _needsUpdate = true;
}

void Reconciler::update() {
    PROFILE

    if (_isUpdating == true) {
        _needsUpdate = true;
        return;
    }

    _isUpdating = true;

    for (;;) {
        _needsUpdate = false;

        _beginUpdate();

        try {
            if (_updateFn) {
                Context(*this, _updateFn);
            }
        } catch (...) {
            _isUpdating = false;
            _currentComponent = nullptr;
            _currentReconciler.pop();
            throw;
        }

        _endUpdate();

        if (_needsUpdate == false) {
            break;
        }
    }

    _isUpdating = false;

    onAfterUpdate.publish();
}

std::any& Reconciler::_useState() {
    PROFILE

    if (_currentComponent == nullptr) {
        throw std::runtime_error("Current component not found");
    }

    auto [ it, _ ] = _currentComponent->state.insert({ _currentComponent->nextStateIndex++, {} });

    return it->second;
}

void* Reconciler::_useContext(std::type_index const& typeIndex) {
    PROFILE

    if (_currentComponent == nullptr) {
        throw std::runtime_error("Current component not found");
    }

    for (auto component = _currentComponent; component != nullptr; component = component->parent) {
        for (auto i = 0llu; i < component->contextCount; i++) {
            auto const& [ type, pointer ] = component->contexts[i];
            if (type == typeIndex) {
                return pointer;
            }
        }
    }

    return nullptr;
}

std::int64_t const& Reconciler::_useComponentIndex() {
    PROFILE

    if (_currentComponent == nullptr) {
        throw std::runtime_error("Current component not found");
    }

    return _currentComponent->index;
}

std::string const& Reconciler::_useComponentName() {
    PROFILE

    if (_currentComponent == nullptr) {
        throw std::runtime_error("Current component not found");
    }

    return _currentComponent->name;
}

std::string const& Reconciler::_useComponentKey() {
    PROFILE

    if (_currentComponent == nullptr) {
        throw std::runtime_error("Current component not found");
    }

    return _currentComponent->key;
}

std::string const& Reconciler::_useComponentID() {
    PROFILE

    if (_currentComponent == nullptr) {
        throw std::runtime_error("Current component not found");
    }

    return _currentComponent->id;
}

std::int64_t Reconciler::_getComponentType(std::string const& name) {
    PROFILE

    return GetComponentType(name);
}

void Reconciler::_setContext(std::type_index const& typeIndex, void* pointer) {
    PROFILE

    if (_currentComponent == nullptr) {
        throw std::runtime_error("Current component not found");
    }

    auto& component = *_currentComponent;

    for (auto i = 0llu; i < component.contextCount; i++) {
        auto& [ type, existing ] = component.contexts[i];
        if (type == typeIndex) {
            existing = pointer;
            return;
        }
    }

    if (component.contextCount == _CONTEXT_CAPACITY) {
        throw std::runtime_error(
            std::format("Component `{}` exceeds the limit of {} contexts", component.name, _CONTEXT_CAPACITY)
        );
    }

    component.contexts[(std::size_t)component.contextCount++] = { typeIndex, pointer };
}

void Reconciler::_openComponent(std::int64_t type, std::string const& name, std::string const& key) {
    PROFILE

    if (_currentComponent == nullptr) {
        throw std::runtime_error("Reconciler is not updating");
    }

    auto id = (
        (key.empty() || _checkKeyDuplicate(type, key))
            ? (name + std::to_string(_currentComponent->keySeq[type]++))
            : key
    );

    auto component = static_cast<_Component*>(nullptr);

    for (auto child = _currentComponent->firstChild; child != nullptr; child = child->nextSibling) {
        if (child->id == id) {
            component = child;
            break;
        }
    }

    if (component == nullptr) {
        component = new _Component();
        component->id = id;
        component->key = key;
        component->type = type;
        component->name = name;
        _addComponentChild(_currentComponent, component);
    }

    component->index = _currentComponent->nextChildIndex++;
    component->revision = _currentComponent->revision;
    component->nextChildIndex = 0;
    component->nextStateIndex = 0;
    component->contextCount = 0;

    for (auto& entry : component->keySeq) {
        entry.second = 0;
    }

    _currentComponent = component;
}

void Reconciler::_closeComponent() {
    PROFILE

    if ((_currentComponent == nullptr) || (_currentComponent == _rootComponent)) {
        throw std::runtime_error("Current component not found");
    }

    _currentComponent = _currentComponent->parent;
}

void Reconciler::_addComponentChild(_Component* parent, _Component* child) {
    PROFILE

    if (child->parent != nullptr) {
        _removeComponentChild(child->parent, child);
    }

    child->parent = parent;
    child->prevSibling = parent->lastChild;
    child->nextSibling = nullptr;

    if (parent->lastChild != nullptr) {
        parent->lastChild->nextSibling = child;
    } else {
        parent->firstChild = child;
    }

    parent->lastChild = child;
}

void Reconciler::_removeComponentChild(_Component* parent, _Component* child) {
    PROFILE

    if (child->prevSibling != nullptr) {
        child->prevSibling->nextSibling = child->nextSibling;
    } else {
        parent->firstChild = child->nextSibling;
    }

    if (child->nextSibling != nullptr) {
        child->nextSibling->prevSibling = child->prevSibling;
    } else {
        parent->lastChild = child->prevSibling;
    }

    child->parent = nullptr;
    child->prevSibling = nullptr;
    child->nextSibling = nullptr;
}

void Reconciler::_removeComponentFromParent(_Component* component) {
    PROFILE

    if (component->parent != nullptr) {
        _removeComponentChild(component->parent, component);
    }
}

void Reconciler::_unmountComponent(_Component* component) {
    PROFILE

    for (auto child = component->firstChild; child != nullptr;) {
        auto next = child->nextSibling;
        _unmountComponent(child);
        child = next;
    }

    if (component->revision < _rootComponent->revision) {
        component->state.clear();

        _removeComponentFromParent(component);
        delete component;
    }
}

bool Reconciler::_checkKeyDuplicate(std::int64_t type, std::string const& key) {
    PROFILE

    if (_currentComponent == nullptr) {
        throw std::runtime_error("Current component not found");
    }

    for (auto child = _currentComponent->firstChild; child != nullptr; child = child->nextSibling) {
        if (
            (child->revision == _currentComponent->revision) &&
            (child->type == type) &&
            (child->key == key)
        ) {
            return true;
        }
    }

    return false;
}

void Reconciler::_beginUpdate() {
    PROFILE

    _currentReconciler.push(this);

    _rootComponent->revision += 1;
    _rootComponent->nextChildIndex = 0;
    _rootComponent->nextStateIndex = 0;
    _rootComponent->keySeq = {};

    _currentComponent = _rootComponent;
}

void Reconciler::_endUpdate() {
    PROFILE

    if (_currentComponent != _rootComponent) {
        throw std::runtime_error("Unbalanced OpenComponent/CloseComponent");
    }

    _currentComponent = nullptr;
    _unmountComponent(_rootComponent);

    _currentReconciler.pop();
}

void SetContextAny(std::type_index const& typeIndex, void* pointer) {
    PROFILE

    _GetCurrentReconciler()._setContext(typeIndex, pointer);
}

void ContextAny(std::type_index const& typeIndex, void* pointer, std::function<void()> const& children) {
    PROFILE

    auto name = std::string(typeIndex.name());
    auto type = GetComponentType(name);
    auto scope = _ComponentScope(type, name, {});

    SetContextAny(typeIndex, pointer);

    if (children) {
        children();
    }
}

std::any& UseStateAny() {
    PROFILE

    return _GetCurrentReconciler()._useState();
}

void* UseContextAny(std::type_index const& typeIndex) {
    PROFILE

    return _GetCurrentReconciler()._useContext(typeIndex);
}

std::int64_t GetComponentType(std::string const& name) {
    PROFILE

    static auto _typeMap = std::unordered_map<std::string, std::int64_t>();
    static auto _typeSeq = 1ll;

    auto& typeId = _typeMap[name];

    if (typeId == 0) {
        typeId = _typeSeq;
        _typeSeq += 1;
    }

    return typeId;
}

std::string const& UseComponentName() {
    PROFILE

    return _GetCurrentReconciler()._useComponentName();
}

std::string const& UseComponentKey() {
    PROFILE

    return _GetCurrentReconciler()._useComponentKey();
}

std::string const& UseComponentID() {
    PROFILE

    return _GetCurrentReconciler()._useComponentID();
}

std::int64_t const& UseComponentIndex() {
    PROFILE

    return _GetCurrentReconciler()._useComponentIndex();
}

} /* namespace Rocket */
