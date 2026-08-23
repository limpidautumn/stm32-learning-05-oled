#pragma once

#include "utils.hpp"

#include "i2c.h"
#include <cstring>
#include <new>

namespace i2c {

void setup();
void loop();

class Bus : public utils::InstanceRegistry<Bus> {
public:
  using Hook = void (*)(bool ok, void *ctx);

  explicit Bus(I2C_HandleTypeDef &hi2c) : hi2c(&hi2c) {}

  void write(uint16_t devAddr, const uint8_t *data, uint16_t len,
             Hook onDone = nullptr, void *ctx = nullptr) {
    enqueue(Kind::Write, devAddr, 0, 0, data, len, nullptr, onDone, ctx);
  }

  void memWrite(uint16_t devAddr, uint16_t memAddr, uint16_t memAddrSize,
                const uint8_t *data, uint16_t len, Hook onDone = nullptr,
                void *ctx = nullptr) {
    enqueue(Kind::MemWrite, devAddr, memAddr, memAddrSize, data, len, nullptr,
            onDone, ctx);
  }

  void read(uint16_t devAddr, uint8_t *dst, uint16_t len, Hook onDone = nullptr,
            void *ctx = nullptr) {
    enqueue(Kind::Read, devAddr, 0, 0, nullptr, len, dst, onDone, ctx);
  }

  void memRead(uint16_t devAddr, uint16_t memAddr, uint16_t memAddrSize,
               uint8_t *dst, uint16_t len, Hook onDone = nullptr,
               void *ctx = nullptr) {
    enqueue(Kind::MemRead, devAddr, memAddr, memAddrSize, nullptr, len, dst,
            onDone, ctx);
  }

  static void loop() {
    forEach([](Bus *b) { b->pump(); });
  }

  bool idle() const { return hi2c->State == HAL_I2C_STATE_READY; }

  static void dispatch(I2C_HandleTypeDef *h, bool ok) {
    Bus *const b = find([&h](Bus *x) { return x->hi2c == h; });
    if (!b)
      return;
    Task *const t = b->active_;
    if (!t)
      return;
    b->active_ = nullptr;
    b->complete(t, ok);
  }

private:
  enum class Kind : uint8_t { Write, MemWrite, Read, MemRead };

  struct Task {
    Kind kind;
    uint16_t devAddr;
    uint16_t memAddr;
    uint16_t memAddrSize;
    uint16_t len;
    uint8_t *buf;
    uint8_t *dst;
    Hook hook;
    void *ctx;
    Task *next;
  };

  static constexpr uint8_t kMaxTasks = 255;

  I2C_HandleTypeDef *const hi2c;
  Task *head_ = nullptr;
  Task *tail_ = nullptr;
  Task *active_ = nullptr;
  uint8_t qCount_ = 0;

  void enqueue(Kind kind, uint16_t devAddr, uint16_t memAddr,
               uint16_t memAddrSize, const uint8_t *src, uint16_t len,
               uint8_t *dst, Hook onDone, void *ctx) {
    utils::IrqGuard g;

    if (qCount_ >= kMaxTasks) {
      return;
    }

    Task *t = new (std::nothrow) Task;
    uint8_t *buf = (len ? new (std::nothrow) uint8_t[len] : nullptr);

    if (!t || (len && !buf)) {
      g.release();
      HAL_NVIC_SystemReset();
    }

    t->kind = kind;
    t->devAddr = devAddr;
    t->memAddr = memAddr;
    t->memAddrSize = memAddrSize;
    t->len = len;
    t->buf = buf;
    t->dst = dst;
    t->hook = onDone;
    t->ctx = ctx;
    t->next = nullptr;
    if (kind == Kind::Write || kind == Kind::MemWrite)
      std::memcpy(buf, src, len);
    if (tail_)
      tail_->next = t;
    else
      head_ = t;
    tail_ = t;
    ++qCount_;
  }

  bool empty() const { return head_ == nullptr; }

  void pump() {
    if (!idle())
      return;

    Task *t = nullptr;

    {
      utils::IrqGuard g;
      t = head_;
      if (t) {
        head_ = t->next;
        if (!head_)
          tail_ = nullptr;
        --qCount_;
      }
    }

    if (!t)
      return;
    active_ = t;
    if (send(*t) != HAL_OK) {
      active_ = nullptr;
      complete(t, false);
    }
  }

  HAL_StatusTypeDef send(const Task &t) {
    switch (t.kind) {
    case Kind::Write:
      return HAL_I2C_Master_Transmit_DMA(hi2c, t.devAddr, t.buf, t.len);
    case Kind::MemWrite:
      return HAL_I2C_Mem_Write_DMA(hi2c, t.devAddr, t.memAddr, t.memAddrSize,
                                   t.buf, t.len);
    case Kind::Read:
      return HAL_I2C_Master_Receive_DMA(hi2c, t.devAddr, t.buf, t.len);
    case Kind::MemRead:
      return HAL_I2C_Mem_Read_DMA(hi2c, t.devAddr, t.memAddr, t.memAddrSize,
                                  t.buf, t.len);
    }
    return HAL_ERROR;
  }

  void complete(Task *t, bool ok) {
    if (ok && (t->kind == Kind::Read || t->kind == Kind::MemRead) && t->dst)
      std::memcpy(t->dst, t->buf, t->len);
    const Hook h = t->hook;
    void *const c = t->ctx;
    freeTask(t);
    if (h)
      h(ok, c);
  }

  void freeTask(Task *t) {
    utils::IrqGuard g;
    delete[] t->buf;
    delete t;
  }
};

extern Bus i2c1;

} // namespace i2c
