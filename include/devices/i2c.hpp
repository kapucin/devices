// Copyright (C) 2017 Sergey Kapustin <kapucin@gmail.com>

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/** @file */

#ifndef _btr_I2C_hpp_
#define _btr_I2C_hpp_

// SYSTEM INCLUDES

// PROJECT INCLUDES
#include "devices/defines.hpp"
#include "utility/value_codec.hpp"

namespace btr
{

/**
 * The class implements I2C protocol handling for AVR/STM32 platforms.
 */
class I2C
{
public:

// LIFECYCLE

  /**
   * Construct new object with a given device ID.
   *
   * @param dev_id - device ID. On AVR it can be any numeric value. On STM32, it's one of
   *  I2C1 or I2C2.
   */
  I2C(uint32_t dev_id);

  /**
   * No-op destructor.
   */
  ~I2C() = default;

// OPERATIONS

  /**
   * Provide a device identified by port id.
   * The device is statically allocated (if configured @see defines.hpp) and is closed initially.
   *
   * @param dev_id - I2C device id, 0 or 1 for STM32F103, 0 for AVR.
   * @param open - open the device if it is not already
   * @return device instance
   */
  static I2C* instance(uint32_t dev_id, bool open);

  /**
   * Check if device is open.
   *
   * @return true if device is initialized, false otherwise
   */
  bool isOpen();

  /**
   * Initialize the device.
   */
  void open();

  /**
   * Shut down the device.
   */
  void close();

  /**
   * Scan for i2c devices and provide the number of available devices.
   *
   * @return upper 16 bits is status code as described in defines.hpp, lower 16 bits contain
   *  the number of detected devices
   */
  uint32_t scan();

  /**
   * Write address, register and variable-byte value.
   *
   * @param addr - the address of a device to send the data to
   * @param reg - the address of a register on the device
   * @param value - the value
   * @return status code as described in defines.hpp
   */
  template<typename T>
  uint32_t write(uint8_t addr, uint8_t reg, T value);

  /**
   * Write address, register and multi-byte value.
   *
   * @param addr - the address of a device to send the data to
   * @param reg - the address of a register on the device
   * @param buff - the buffer with data
   * @param count - the number of bytes in buff
   * @return status code as described in defines.hpp
   */
  uint32_t write(uint8_t addr, uint8_t reg, const uint8_t* buff, uint8_t count);

  /**
   * Read value from a given register.
   *
   * @param addr - slave address
   * @param reg - register to read from
   * @param val - value to store received data to
   * @return status code as described in defines.hpp
   */
  template<typename T>
  uint32_t read(uint8_t addr, uint8_t reg, T* val);

  /**
   * Read multi-byte value from a specific register.
   *
   * @param addr - slave address
   * @param reg - register to read from
   * @param buff - buffer to store the data in
   * @param count - number of bytes to read
   * @return status code as described in defines.hpp
   */
  uint32_t read(uint8_t addr, uint8_t reg, uint8_t* buff, uint8_t count);

  /**
   * Read multi-byte value.
   *
   * @param addr - slave address
   * @param buff - the buffer to store the data to
   * @param count - the number of bytes in buff
   * @param stop_comm - if true, stop i2c communication once the data is read. If false, do NOT
   *  stop the communication. The reasons is that the read operation started elsewhere (other
   *  read() function, in which case that function will call stop instead.
   * @return status code as described in defines.hpp
   */
  uint32_t read(uint8_t addr, uint8_t* buff, uint8_t count, bool stop_comm = true);

private:

// OPERATIONS

  /**
   * Activate or deactivate internal pull-up resistors.
   *
   * @param activate - deactivate pull-ups if false, activate if true
   */
  void setPullups(bool activate);

  /**
   * Set TWI line speed.
   *
   * @param fast - when true, the line is set to 400kHz. Otherwise, it's set to 100kHz
   */
  void setSpeed(bool fast);

  /**
   * Start I2C transaction and send an address.
   *
   * @param addr - slave address
   * @param rw - 1 for read, 0 for write
   * @return status code as described in defines.hpp
   */
  uint32_t start(uint8_t addr, uint8_t rw);

  /**
   * Generate I2C stop condition.
   * @return status code as described in defines.hpp
   */
  uint32_t stop();

  /**
   * Close, then open this I2C object.
   */
  void reset();

  /**
   * Send a single byte.
   *
   * @param val - the byte
   * @return status code as described in defines.hpp
   */
  uint32_t sendByte(uint8_t val);

  /**
   * Receive status ACK or NACK.
   *
   * @param ack - expect ACK, otherwise expect NACK
   * @return status code as described in defines.hpp
   */
  uint32_t receiveByte(bool ack);

  /**
   * Receive one byte, the status should be ACK or NACK.
   *
   * @param ack - expect status ACK, otherwise NACK
   * @param val - buffer to store received value
   * @return status code as described in defines.hpp
   */
  uint32_t receiveByte(bool ack, uint8_t* val);

  /**
   * Whait while I2C is busy with another transation.
   * @return status code as described in defines.hpp
   */
  uint32_t waitBusy();

// ATTRIBUTES

  /** This device's port identifier (I2C1, I2C2 in STM32); it is not I2C address. */
  uint32_t dev_id_;
  /** Temporary buffer to read/write a byte to. */
  uint8_t buff_[sizeof(uint64_t)];
  /** Flag indicating if the device is open. */
  bool open_;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// INLINE OPERATIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////// PUBLIC /////////////////////////////////////////////

//============================================= OPERATIONS =========================================

template<typename T>
inline uint32_t I2C::read(uint8_t addr, uint8_t reg, T* val)
{
  int rc = I2C::read(addr, reg, buff_, sizeof(T));

  if (is_ok(rc)) {
    ValueCodec::decodeFixedInt(buff_, val, sizeof(T), true);
  }
  return rc;
}

template<typename T>
inline uint32_t I2C::write(uint8_t addr, uint8_t reg, T value)
{
  if (sizeof(T) > 1 && ValueCodec::isLittleEndian()) {
    ValueCodec::swap(&value);
  }

  const uint8_t* buff = reinterpret_cast<uint8_t*>(&value);
  return write(addr, reg, buff, sizeof(T));
}

} // namespace btr

#endif // _btr_I2C_hpp_
