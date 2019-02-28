// Copyright (C) 2019 Bolt Robotics <info@boltrobotics.com>
// License: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>

// SYSTEM INCLUDES
#include <avr/io.h>
#include <util/atomic.h>
#include <util/delay.h>

// PROJECT INCLUDES
#include "devices/avr/usart.hpp"  // class implemented

#if BTR_USART1_ENABLED > 0 || BTR_USART2_ENABLED > 0 || \
    BTR_USART3_ENABLED > 0 || BTR_USART4_ENABLED > 0

#if BTR_USART_USE_2X > 0
#define BAUD_CALC(BAUD) (((F_CPU) + 4UL * (BAUD)) /  (8UL * (BAUD)) - 1UL)
#else
#define BAUD_CALC(BAUD) (((F_CPU) + 8UL * (BAUD)) / (16UL * (BAUD)) - 1UL)
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
// Register bits {

#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__) || \
    defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)

// UCSRnA (status)
#define RXC       RXC0    // Bit 7. Receive complete
#define TXC       TXC0    // Bit 6. Transmit complete
#define UDRE      UDRE0   // Bit 5. Transmit buffer empty
#define FE        FE0     // Bit 4. Frame error
#define DOR       DOR0    // Bit 3. Data OverRun
#define UPE       UPE0    // Bit 2. Parity error
#define U2X       U2X0    // Bit 1. Double transmission speed
#define MPCM      MPCM0   // Bit 0. Multi-processor communication mode
// UCSRnB (control 1)
#define RXCIE     RXCIE0  // Bit 7. Receive complete interrupt enable
#define TXCIE     TXCIE0  // Bit 6. Transmit complete interrupt enable
#define UDRIE     UDRIE0  // Bit 5. Transmit buffer empty interrupt enable
#define RXEN      RXEN0   // Bit 4. Receive enable
#define TXEN      TXEN0   // Bit 3. Transmit enable
#define UCSZ2     UCSZ02  // Bit 2. Character size 2
// UCSRnC (control 2)
#define UCSZ1     UCSZ01  // Bit 2. Character size 1
#define UCSZ0     UCSZ00  // Bit 1. Character size 0
#endif // __AVR

// } Register bits

////////////////////////////////////////////////////////////////////////////////////////////////////
// RS485 { TODO Finish off

#if BTR_RTS_ENABLED > 0

#define RTS_PIN   PB0
#define RTS_DDR   DDRB
#define RTS_PORT  PORTB

#define RTS_INIT \
  do { \
    set_bit(RTS_DDR, RTS_PIN); \
    clear_bit(RTS_PORT, RTS_PIN); \
  } while (0);

#define RTS_HIGH \
  do { \
    set_bit(RTS_PORT, RTS_PIN); \
  } while (0);

#define RTS_LOW \
  do { \
    clear_bit(RTS_PORT, RTS_PIN); \
  } while (0);

#endif // BTR_RTS_ENABLED

// } RS485

////////////////////////////////////////////////////////////////////////////////////////////////////
// Static members {

#if BTR_USART1_ENABLED > 0
#if defined(UBRRH) && defined(UBRRL)
//static btr::Usart usart_1(1, &UBRRH, &UBRRL, &UCSRA, &UCSRB, &UCSRC, &UDR);
static btr::Usart usart_1(1, &UBRR0H, &UBRR0L, &UCSR0A, &UCSR0B, &UCSR0C, &UDR0);
#else
static btr::Usart usart_1(1, &UBRR0H, &UBRR0L, &UCSR0A, &UCSR0B, &UCSR0C, &UDR0);
#endif // UBRRH && UBRRL
#endif // BTR_USART1_ENABLED

#if BTR_USART2_ENABLED > 0
static btr::Usart usart_2(2, &UBRR1H, &UBRR1L, &UCSR1A, &UCSR1B, &UCSR1C, &UDR1);
#endif

#if BTR_USART3_ENABLED > 0
static btr::Usart usart_3(3, &UBRR2H, &UBRR2L, &UCSR2A, &UCSR2B, &UCSR2C, &UDR2);
#endif

#if BTR_USART4_ENABLED > 0
static btr::Usart usart_4(4, &UBRR3H, &UBRR3L, &UCSR3A, &UCSR3B, &UCSR3C, &UDR3);
#endif

// } Static members

////////////////////////////////////////////////////////////////////////////////////////////////////
// ISRs {
// See: http://www.nongnu.org/avr-libc/user-manual/group__avr__interrupts.html

#if defined (__AVR_ATmega168__) || defined(__AVR_ATmega328P__)
#if BTR_USART1_ENABLED > 0
ISR(USART_RX_vect)
{
  usart_1.onRecv();
}
ISR(USART_UDRE_vect)
{
  usart_1.onSend();
}
#endif

#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)

#if BTR_USART1_ENABLED > 0
ISR(USART0_RX_vect)
{
  usart_1.onRecv();
}
ISR(USART0_UDRE_vect)
{
  usart_1.onSend();
}
#endif
#if BTR_USART2_ENABLED > 0
ISR(USART1_RX_vect)
{
  usart_2.onRecv();
}
ISR(USART1_UDRE_vect)
{
  usart_2.onSend();
}
#endif
#if BTR_USART3_ENABLED > 0
ISR(USART2_RX_vect)
{
  usart_3.onRecv();
}
ISR(USART2_UDRE_vect)
{
  usart_3.onSend();
}
#endif
#if BTR_USART4_ENABLED > 0
ISR(USART3_RX_vect)
{
  usart_4.onRecv();
}
ISR(USART3_UDRE_vect)
{
  usart_4.onSend();
}
#endif // BTR_USARTx

#endif // __AVR_ATmegaX

// } ISRs

namespace btr
{

/////////////////////////////////////////////// PUBLIC /////////////////////////////////////////////

//============================================= LIFECYCLE ==========================================

Usart::Usart(
    uint8_t id,
    volatile uint8_t* ubrr_h,
    volatile uint8_t* ubrr_l,
    volatile uint8_t* ucsr_a,
    volatile uint8_t* ucsr_b,
    volatile uint8_t* ucsr_c,
    volatile uint8_t* udr
    )
  :
    id_(id),
    ubrr_h_(ubrr_h),
    ubrr_l_(ubrr_l),
    ucsr_a_(ucsr_a),
    ucsr_b_(ucsr_b),
    ucsr_c_(ucsr_c),
    udr_(udr),
    rx_error_(0),
    rx_head_(0),
    rx_tail_(0),
    tx_head_(0),
    tx_tail_(0),
    rx_buff_(),
    tx_buff_()
{
  clear_bit(*ucsr_b_, TXEN);
  clear_bit(*ucsr_b_, RXEN);
  clear_bit(*ucsr_b_, RXCIE);
  clear_bit(*ucsr_b_, UDRIE);
} 

//============================================= OPERATIONS =========================================

// static
Usart* Usart::instance(uint32_t usart_id)
{
  switch (usart_id) {
#if BTR_USART1_ENABLED > 0
    case 1:
      return &usart_1;
#endif
#if BTR_USART2_ENABLED > 0
    case 2:
      return &usart_2;
#endif
#if BTR_USART3_ENABLED > 0
    case 3:
      return &usart_3;
#endif
#if BTR_USART4_ENABLED > 0
    case 4:
      return &usart_4;
#endif
    default:
      return nullptr;
  }
}

bool Usart::isOpen()
{
  return (bit_is_set(*ucsr_b_, TXEN) || bit_is_set(*ucsr_b_, RXEN));
}

int Usart::open()
{
  if (true == isOpen()) {
    return 0;
  }

  uint16_t baud;
  uint8_t config;

  switch (id_) {
    case 1:
      baud = BAUD_CALC(BTR_USART1_BAUD);
      config = BTR_USART_CONFIG(BTR_USART1_PARITY, BTR_USART1_STOP_BITS, BTR_USART1_DATA_BITS);
      break;
    case 2:
      baud = BAUD_CALC(BTR_USART2_BAUD);
      config = BTR_USART_CONFIG(BTR_USART2_PARITY, BTR_USART2_STOP_BITS, BTR_USART2_DATA_BITS);
      break;
    case 3:
      baud = BAUD_CALC(BTR_USART3_BAUD);
      config = BTR_USART_CONFIG(BTR_USART3_PARITY, BTR_USART3_STOP_BITS, BTR_USART3_DATA_BITS);
      break;
    case 4:
      baud = BAUD_CALC(BTR_USART4_BAUD);
      config = BTR_USART_CONFIG(BTR_USART4_PARITY, BTR_USART4_STOP_BITS, BTR_USART4_DATA_BITS);
      break;
    default:
      return -1;
  }

#if BTR_USART_USE_2X > 0
  *ucsr_a_ = (1 << U2X);
#endif

  *ubrr_h_ = baud >> 8;
  *ubrr_l_ = baud;
  *ucsr_c_ = config;

  set_bit(*ucsr_b_, TXEN);
  set_bit(*ucsr_b_, RXEN);
  set_bit(*ucsr_b_, RXCIE);
  clear_bit(*ucsr_b_, UDRIE);

  return 0;
}

void Usart::close()
{
  flush(DirectionType::OUT);
  clear_bit(*ucsr_b_, TXEN);
  clear_bit(*ucsr_b_, RXEN);
  clear_bit(*ucsr_b_, RXCIE);
  clear_bit(*ucsr_b_, UDRIE);
  rx_head_ = rx_tail_;
}

void Usart::onRecv()
{
  // Can be called from ISR.

  rx_error_ = (*ucsr_a_ & ((1 << FE) | (1 << DOR) | (1 << UPE)));
  uint16_t head_next = (rx_head_ + 1) % BTR_USART_RX_BUFF_SIZE;

  if (head_next != rx_tail_) {
    rx_buff_[rx_head_] = *udr_;
    rx_head_ = head_next;
  } else {
    rx_error_ |= (BTR_USART_OVERFLOW_ERR >> 8);
  }
  LED_TOGGLE();
}

void Usart::onSend()
{
  // Can be called from ISR.

  uint8_t ch = tx_buff_[tx_tail_];
  tx_tail_ = (tx_tail_ + 1) % BTR_USART_TX_BUFF_SIZE;
  *udr_ = ch;

  if (tx_head_ == tx_tail_) {
    // Disable transmit buffer empty interrupt since there is no more data to send.
    clear_bit(*ucsr_b_, UDRIE);
  }
  //LED_TOGGLE();
}

int Usart::available()
{
  uint16_t bytes = BTR_USART_RX_BUFF_SIZE + rx_head_ - rx_tail_;
  return (bytes % BTR_USART_RX_BUFF_SIZE);
}

int Usart::flush(DirectionType queue_selector)
{
  (void) queue_selector;

  while (bit_is_set(*ucsr_b_, UDRIE) || bit_is_clear(*ucsr_a_, TXC)) {
    if (bit_is_clear(SREG, SREG_I) && bit_is_set(*ucsr_b_, UDRIE)) {
      if (bit_is_set(*ucsr_a_, UDRE)) {
        // Call manually since global interrupts are disabled.
        onSend();
      }
    }
  }
  return 0;
}

int Usart::send(char ch, bool drain, uint32_t timeout)
{
  uint32_t delays = 0;
  uint16_t head_next = (tx_head_ + 1) % BTR_USART_TX_BUFF_SIZE;

  // No room in tx buffer, wait while data is being drained from tx_buff_.
  //
  while (head_next == tx_tail_) {
    if (bit_is_clear(SREG, SREG_I)) {
      if (bit_is_set(*ucsr_a_, UDRE)) {
        onSend();
        continue;
      }
    }

    if (timeout > 0) {
      _delay_ms(BTR_USART_TX_DELAY);
      delays += BTR_USART_TX_DELAY;

      if (delays >= timeout) {
        return -1;
      }
    }
  }

  tx_buff_[head_next] = ch;

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    tx_head_ = head_next;
    set_bit(*ucsr_b_, UDRIE);
  }

  if (true == drain) {
    flush(DirectionType::OUT);
  }
  return 0;
}

int Usart::send(const char* buff, bool drain)
{
  int rc = 0;

  while (*buff) {
    if (0 != (rc = send(*buff++, false))) {
      break;
    }
  }

  if (0 == rc && true == drain) {
    flush(DirectionType::OUT);
  }
  return rc;
}

int Usart::send(const char* buff, uint32_t bytes, bool drain)
{
  int rc = 0;

  while (bytes-- > 0) {
    if (0 != (rc = send(*buff, false))) {
      break;
    }
    ++buff;
  }

  if (0 == rc && true == drain) {
    flush(DirectionType::OUT);
  }
  return rc;
}

uint16_t Usart::recv()
{
  if (rx_head_ != rx_tail_) {
    uint8_t ch = rx_buff_[rx_tail_];  
    rx_tail_ = (rx_tail_ + 1) % BTR_USART_RX_BUFF_SIZE;

    uint16_t rc = (rx_error_ << 8);
    rx_error_ = 0;
    return (rc + ch);
  }
  return BTR_USART_NO_DATA;
}

uint16_t Usart::recv(char* buff, uint16_t bytes, uint32_t timeout)
{
  uint32_t delays = 0;
  uint16_t rc = 0;

  while (bytes > 0) {
    uint16_t ch = recv();

    if (BTR_USART_NO_DATA & ch) {
      if (timeout > 0) {
        _delay_ms(BTR_USART_RX_DELAY);
        delays += BTR_USART_RX_DELAY;

        if (delays >= timeout) {
          rc |= BTR_USART_TIMEDOUT_ERR;
          return rc;
        }
      }
      continue;
    }
    rc |= (ch & 0xFF00);
    *buff++ = ch;
    --bytes;
  }
  return rc;
}

/////////////////////////////////////////////// PROTECTED //////////////////////////////////////////

//============================================= OPERATIONS =========================================

/////////////////////////////////////////////// PRIVATE ////////////////////////////////////////////

//============================================= OPERATIONS =========================================

} // namespace btr

#endif // BTR_USARTn_ENABLED
