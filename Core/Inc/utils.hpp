#pragma once

#include "stm32f1xx_hal.h"
#include <new>

namespace utils {

// A general state machine node based on HAL tick.
// E: Enum type of the state set.
template <typename E> class State {
public:
  explicit State(E s) { enter(s); }
  E get() const { return state; }
  void enter(const E &s) {
    state = s;
    entryTime = HAL_GetTick();
  }
  uint32_t timeElapsed() const { return HAL_GetTick() - entryTime; }

private:
  E state;
  uint32_t entryTime;
};

// A RAII Guard.
// Used to mask ISR during build and restore PRIMASK during destruct.
class IrqGuard {
public:
  IrqGuard() : primask(__get_PRIMASK()) { __disable_irq(); }
  ~IrqGuard() { release(); }
  void release() {
    if (released_)
      return;
    __set_PRIMASK(primask);
    released_ = 1;
  }

private:
  const uint32_t primask;
  bool released_ = 0;
};

// Undefined capacity instance registry (CRTP base class, heap allocation list).
// T: Derived classes. Automatically registered during construction and
// unregistered during destruction. Node allocation/release and linked list
// modification are all performed within the interrupt-free section.
template <typename T> class InstanceRegistry {
private:
  struct Node {
    T *self;
    Node *next;
  };
  inline static Node *head_ = nullptr;

public:
  template <typename Fn> static void forEach(Fn &&fn) {
    for (Node *n = head_; n; n = n->next)
      fn(n->self);
  }

  template <typename Fn> static T *find(Fn &&fn) {
    for (Node *n = head_; n; n = n->next)
      if (fn(n->self))
        return n->self;
    return nullptr;
  }

protected:
  InstanceRegistry() {
    IrqGuard g;
    Node *const n = new (std::nothrow) Node{static_cast<T *>(this), head_};
    if (!n)
      HAL_NVIC_SystemReset();
    head_ = n;
  }

  // Prevent copy construction or copy assignment
  InstanceRegistry(const InstanceRegistry &) = delete;
  InstanceRegistry &operator=(const InstanceRegistry &) = delete;

  ~InstanceRegistry() {
    IrqGuard g;
    for (Node **pp = &head_; *pp; pp = &(*pp)->next)
      if ((*pp)->self == static_cast<T *>(this)) {
        Node *const dead = *pp;
        *pp = dead->next;
        delete dead;
        break;
      }
  }
};

} // namespace utils
