#pragma once

#include "i2c.h"
#include "stm32f1xx_hal_i2c.h"

#include <cstring>
#include <ios>
#include <new>

namespace i2c {

void init();
void loop();

class Bus {
public:
  using Hook = void (*)(bool ok, void *ctx);

  explicit Bus(I2C_HandleTypeDef &hi2c) : hi2c(&hi2c) {
    if (count < kMaxInstances)
      slots[count++] = this;
    else
      HAL_NVIC_SystemReset();
  }

  Bus(const Bus &) = delete;
  Bus &operator=(const Bus &) = delete;

  ~Bus() {
    for (uint8_t i = 0; i < count; ++i)
      if (slots[i] == this) {
        slots[i] = slots[--count];
        break;
      }
  }

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
    for (uint8_t i = 0; i < count; ++i)
      slots[i]->pump();
  }

  bool idle() const { return hi2c->State == HAL_I2C_STATE_READY; }

  static void dispatch(I2C_HandleTypeDef *h, bool ok) {
    for (uint8_t i = 0; i < count; ++i)
      if (slots[i]->hi2c == h) {
        Task *const t = slots[i]->active_;
        if (!t)
          return;
        slots[i]->active_ = nullptr;

        slots[i]->complete(t, ok);
        return;
      }
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
  static constexpr uint8_t kMaxInstances = 4;

  I2C_HandleTypeDef *const hi2c;
  Task *head_ = nullptr;
  Task *tail_ = nullptr;
  Task *active_ = nullptr;
  uint8_t qCount_ = 0;

  inline static Bus *slots[kMaxInstances] = {};
  inline static uint8_t count = 0;

  void enqueue(Kind kind, uint16_t devAddr, uint16_t memAddr,
               uint16_t memAddrSize, const uint8_t *src, uint16_t len,
               uint8_t *dst, Hook onDone, void *ctx) {

    const uint32_t primask = __get_PRIMASK();
    __disable_irq();

    if (qCount_ >= kMaxTasks) {
      __set_PRIMASK(primask);
      return;
    }

    Task *t = new (std::nothrow) Task;
    uint8_t *buf = (len ? new (std::nothrow) uint8_t[len] : nullptr);

    if (!t || (len && !buf)) {
      __set_PRIMASK(primask);
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

    __set_PRIMASK(primask);
  }

  bool empty() const { return head_ == nullptr; }

  void pump() {
    if (!idle())
      return;

    Task *t = nullptr;

    const uint32_t primask = __get_PRIMASK();
    __disable_irq();

    t = head_;
    if (t) {
      head_ = t->next;
      if (!head_)
        tail_ = nullptr;
      --qCount_;
    }

    __set_PRIMASK(primask);

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
    const uint32_t primask = __get_PRIMASK();
    __disable_irq();

    delete[] t->buf;
    delete t;

    __set_PRIMASK(primask);
  }
};

extern Bus i2c1;

} // namespace i2c
