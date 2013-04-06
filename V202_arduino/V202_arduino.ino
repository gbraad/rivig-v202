#include <SPI.h>

// Inline nRF24L01.h because of Arduino's non-conforming behavior to C/C++
/* Register Map */
#define CONFIG      0x00
#define EN_AA       0x01
#define EN_RXADDR   0x02
#define SETUP_AW    0x03
#define SETUP_RETR  0x04
#define RF_CH       0x05
#define RF_SETUP    0x06
#define STATUS      0x07
#define OBSERVE_TX  0x08
#define CD          0x09
#define RX_ADDR_P0  0x0A
#define RX_ADDR_P1  0x0B
#define RX_ADDR_P2  0x0C
#define RX_ADDR_P3  0x0D
#define RX_ADDR_P4  0x0E
#define RX_ADDR_P5  0x0F
#define TX_ADDR     0x10
#define RX_PW_P0    0x11
#define RX_PW_P1    0x12
#define RX_PW_P2    0x13
#define RX_PW_P3    0x14
#define RX_PW_P4    0x15
#define RX_PW_P5    0x16
#define FIFO_STATUS 0x17
#define DYNPD	    0x1C
#define FEATURE	    0x1D

/* Bit Mnemonics */
#define MASK_RX_DR  6
#define MASK_TX_DS  5
#define MASK_MAX_RT 4
#define EN_CRC      3
#define CRCO        2
#define PWR_UP      1
#define PRIM_RX     0
#define ENAA_P5     5
#define ENAA_P4     4
#define ENAA_P3     3
#define ENAA_P2     2
#define ENAA_P1     1
#define ENAA_P0     0
#define ERX_P5      5
#define ERX_P4      4
#define ERX_P3      3
#define ERX_P2      2
#define ERX_P1      1
#define ERX_P0      0
#define AW          0
#define ARD         4
#define ARC         0
#define PLL_LOCK    4
#define RF_DR       3
#define RF_PWR      6
#define RX_DR       6
#define TX_DS       5
#define MAX_RT      4
#define RX_P_NO     1
#define TX_FULL     0
#define PLOS_CNT    4
#define ARC_CNT     0
#define TX_REUSE    6
#define FIFO_FULL   5
#define TX_EMPTY    4
#define RX_FULL     1
#define RX_EMPTY    0
#define DPL_P5	    5
#define DPL_P4	    4
#define DPL_P3	    3
#define DPL_P2	    2
#define DPL_P1	    1
#define DPL_P0	    0
#define EN_DPL	    2
#define EN_ACK_PAY  1
#define EN_DYN_ACK  0

/* Instruction Mnemonics */
#define R_REGISTER    0x00
#define W_REGISTER    0x20
#define REGISTER_MASK 0x1F
#define ACTIVATE      0x50
#define R_RX_PL_WID   0x60
#define R_RX_PAYLOAD  0x61
#define W_TX_PAYLOAD  0xA0
#define W_ACK_PAYLOAD 0xA8
#define FLUSH_TX      0xE1
#define FLUSH_RX      0xE2
#define REUSE_TX_PL   0xE3
#define NOP           0xFF

/* Non-P omissions */
#define LNA_HCURR   0

/* P model memory Map */
#define RPD         0x09

/* P model bit Mnemonics */
#define RF_DR_LOW   5
#define RF_DR_HIGH  3
#define RF_PWR_LOW  1
#define RF_PWR_HIGH 2

// End of inline

#define CE_PIN  9
#define CSN_PIN 10

class nRF24 {
  uint8_t ce_pin, csn_pin, payload_size;
  bool dynamic_payloads_enabled;
public:
  nRF24(uint8_t _cepin, uint8_t _cspin):
    ce_pin(_cepin), csn_pin(_cspin), payload_size(16), dynamic_payloads_enabled(false)
  {}
  void begin();
  void csn(int mode) {
    digitalWrite(csn_pin, mode);
  }
  void ce(int level)
  {
    digitalWrite(ce_pin, level);
  }
  uint8_t read_register(uint8_t reg, uint8_t* buf, uint8_t len);
  uint8_t read_register(uint8_t reg);
  uint8_t write_register(uint8_t reg, const uint8_t* buf, uint8_t len);
  uint8_t write_register(uint8_t reg, uint8_t value);
  uint8_t write_payload(const void* buf, uint8_t len);
  uint8_t read_payload(void* buf, uint8_t len);
  uint8_t flush_rx(void);
  uint8_t flush_tx(void);
  void    activate(uint8_t code);
};

class V202_TX {
  nRF24& radio;
  bool packet_sent;
  uint8_t rf_ch_num;
public:
  V202_TX(nRF24& radio_) :
    radio(radio_)
  {}
  void begin();
  void command(uint8_t throttle, int8_t yaw, int8_t pitch, int8_t roll, uint8_t flags);
};

void V202_TX::begin()
{
  radio.begin();
  radio.write_register(CONFIG, _BV(EN_CRC) | _BV(CRCO));
  radio.write_register(EN_AA, 0x00);
  radio.write_register(EN_RXADDR, 0x3F);
  radio.write_register(SETUP_AW, 0x03);
  radio.write_register(SETUP_RETR, 0xFF);
  radio.write_register(RF_CH, 0x08);
  radio.write_register(RF_SETUP, 0x05);
  radio.write_register(STATUS, 0x70);
  radio.write_register(OBSERVE_TX, 0x00);
  radio.write_register(CD, 0x00);
  radio.write_register(RX_ADDR_P2, 0xC3);
  radio.write_register(RX_ADDR_P3, 0xC4);
  radio.write_register(RX_ADDR_P4, 0xC5);
  radio.write_register(RX_ADDR_P5, 0xC6);
  radio.write_register(RX_PW_P0, 0x10);
  radio.write_register(RX_PW_P1, 0x10);
  radio.write_register(RX_PW_P2, 0x10);
  radio.write_register(RX_PW_P3, 0x10);
  radio.write_register(RX_PW_P4, 0x10);
  radio.write_register(RX_PW_P5, 0x10);
  radio.write_register(FIFO_STATUS, 0x00);
  const uint8_t* rx_tx_addr = reinterpret_cast<const uint8_t *>("\x66\x88\x68\x68\x68");
  const uint8_t* rx_p1_addr = reinterpret_cast<const uint8_t *>("\x88\x66\x86\x86\x86");
  radio.write_register(RX_ADDR_P0, rx_tx_addr, 5);
  radio.write_register(RX_ADDR_P1, rx_p1_addr, 5);
  radio.write_register(TX_ADDR, rx_tx_addr, 5);
  // Check for Beken BK2421 chip
  radio.activate(0x53); // magic for BK2421 bank switch
  Serial.write("Try to switch banks "); Serial.print(radio.read_register(STATUS)); Serial.write("\n");
  if (radio.read_register(STATUS) & 0x80) {
    Serial.write("BK2421!\n");
    long nul = 0;
    radio.write_register(0x00, (const uint8_t *) "\x40\x4B\x01\xE2", 4);
    radio.write_register(0x01, (const uint8_t *) "\xC0\x4B\x00\x00", 4);
    radio.write_register(0x02, (const uint8_t *) "\xD0\xFC\x8C\x02", 4);
    radio.write_register(0x03, (const uint8_t *) "\xF9\x00\x39\x21", 4);
    radio.write_register(0x04, (const uint8_t *) "\xC1\x96\x9A\x1B", 4);
    radio.write_register(0x05, (const uint8_t *) "\x24\x06\x7F\xA6", 4);
    radio.write_register(0x06, (const uint8_t *) &nul, 4);
    radio.write_register(0x07, (const uint8_t *) &nul, 4);
    radio.write_register(0x08, (const uint8_t *) &nul, 4);
    radio.write_register(0x09, (const uint8_t *) &nul, 4);
    radio.write_register(0x0A, (const uint8_t *) &nul, 4);
    radio.write_register(0x0B, (const uint8_t *) &nul, 4);
    radio.write_register(0x0C, (const uint8_t *) "\x00\x12\x73\x00", 4);
    radio.write_register(0x0D, (const uint8_t *) "\x46\xB4\x80\x00", 4);
    radio.write_register(0x0E, (const uint8_t *) "\x41\x10\x04\x82\x20\x08\x08\xF2\x7D\xEF\xFF", 11);
    radio.write_register(0x04, (const uint8_t *) "\xC7\x96\x9A\x1B", 4);
    radio.write_register(0x04, (const uint8_t *) "\xC1\x96\x9A\x1B", 4);
  }
  radio.activate(0x53); // switch bank back
  
  delay(50);
  radio.flush_tx();
  radio.write_register(CONFIG, _BV(EN_CRC) | _BV(CRCO) | _BV(PWR_UP));
  delayMicroseconds(150);
  packet_sent = true;
  rf_ch_num = 0;
  uint8_t buf[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9B,
                      0x9C, 0x88, 0x48, 0x8F, 0xD3, 0x00, 0xDA, 0x8F };
  radio.flush_tx();
  radio.write_payload(buf, 16);
  radio.ce(HIGH);
  delayMicroseconds(15);
//  radio.ce(LOW);
}

uint8_t rf_channels[16] = { 0x25, 0x2A, 0x1A, 0x3C, 0x37, 0x2B, 0x2E, 0x1D,
                            0x1B, 0x2D, 0x24, 0x3B, 0x13, 0x29, 0x23, 0x22 };

//uint8_t rf_channels[16] = { 0x14, 0x2D, 0x35, 0x3C, 0x37, 0x2B, 0x2E, 0x1D,
//                            0x1B, 0x2D, 0x24, 0x3B, 0x13, 0x29, 0x23, 0x22 };

void V202_TX::command(uint8_t throttle, int8_t yaw, int8_t pitch, int8_t roll, uint8_t flags)
{
  uint8_t buf[16];
  buf[0] = throttle;
  buf[1] = (uint8_t) yaw;
  buf[2] = (uint8_t) pitch;
  buf[3] = (uint8_t) roll;
  if (flags == 0xc0) {
    buf[4] = 0x00;
    buf[5] = 0x00;
    buf[6] = 0x00;
  } else {
    // Trims, middle is 0x40
    buf[4] = 0x40; // yaw
    buf[5] = 0x40; // pitch
    buf[6] = 0x40; // roll
  }
  // TX id
//  buf[7] = 0xcd;
  buf[7] = 0xcd;
  buf[8] = 0x31;
  buf[9] = 0x71;
  // empty
  buf[10] = 0x00;
  buf[11] = 0x00;
  buf[12] = 0x00;
  buf[13] = 0x00;
  //
  buf[14] = flags;
  uint8_t sum = 0;
  uint8_t i;
  for (i = 0; i < 15;  ++i) sum += buf[i];
  buf[15] = sum;
  if (packet_sent) {
    bool report_done = false;
    if  (!(radio.read_register(STATUS) & _BV(TX_DS))) { Serial.write("Waiting for radio\n"); report_done = true; }
    while (!(radio.read_register(STATUS) & _BV(TX_DS))) ;
    radio.write_register(STATUS, _BV(TX_DS));
    if (report_done) Serial.write("Done\n");
  }
  packet_sent = true;
  uint8_t rf_ch = rf_channels[rf_ch_num >> 1];
  rf_ch_num++; if (rf_ch_num >= 32) rf_ch_num = 0;
//  Serial.print(rf_ch); Serial.write("\n");
  radio.write_register(RF_CH, rf_ch);
  radio.flush_tx();
  radio.write_payload(buf, 16);
  //radio.ce(HIGH);
  delayMicroseconds(15);
  //radio.ce(LOW);
}

nRF24 radio(CE_PIN, CSN_PIN);
V202_TX tx(radio);

uint8_t throttle, flags;
int8_t yaw, pitch, roll;

int a0, a1, a2, a3;

bool readInput()
{
  bool changed = false;
  long a;
  a = analogRead(A0);
  a = (a-93)*255/(723-93+1);
  if (a != a0) { changed = true; a0 = a; }
  a = analogRead(A1);
  a = (a-116)*255/(755-116);
  if (a != a1) { changed = true; a1 = a; }
  a = analogRead(A2);
  a = (a-752)*255/(108-752);
  if (a != a2) { changed = true; a2 = a; }
  a = analogRead(A3);
  a = (a-712)*255/(85-712);
  if (a != a3) { changed = true; a3 = a; }
  return changed;
}

void setup() 
{
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  readInput();
  Serial.begin(115200);
  tx.begin();
  throttle = 0; yaw = 0; pitch = 0; roll = 0; flags = 0;
  
  Serial.write("Reading status\n");
  uint8_t res = radio.read_register(STATUS);
  Serial.write("Result: ");
  Serial.print(res);
  Serial.write("\n");
} 

int counter = 0;
int direction = 1;
bool bind = true;
bool calibrated = false;
void loop() 
{
  bool changed = readInput();
  if (changed) {
    Serial.write("sticks: ");
    Serial.print(a0); Serial.write(" ");
    Serial.print(a1); Serial.write(" ");
    Serial.print(a2); Serial.write(" ");
    Serial.print(a3); Serial.write("\n");
  }
  if (bind) {
    throttle = 0;
    flags = 0xc0;
    counter += direction;
    if (direction > 0) {
      if (counter > 256) direction = -1;
    } else {
      if (counter < 0) {
        direction = 1;
        counter = 0;
        bind = false;
        flags = 0;
        Serial.write("Bound\n");
      }
    }
  } else {
    throttle = a0;
    yaw = a1 < 0x80 ? 0x7f - a1 : a1;
    roll = a2 < 0x80 ? 0x7f - a2 : a2 ;
    pitch = a3 < 0x80 ? 0x7f - a3 : a3;
    counter += direction;
    if (direction > 0) {
      if (counter > 255) {
        direction = -1;
        counter = 255;
        flags = 0x10;
      }
    } else {
      if (counter < 0) {
        direction = 1;
        counter = 0;
        flags = 0x00;
      }
    }
  }
  tx.command(throttle, yaw, pitch, roll, flags);
  delay(4);
}

// nRF24 --------------------------------------------------------------

/****************************************************************************/

void nRF24::begin() {
  // Initialize SPI bus
  SPI.begin();

  // Initialize pins
  pinMode(ce_pin,OUTPUT);
  pinMode(csn_pin,OUTPUT);

  digitalWrite(ce_pin, LOW);
  digitalWrite(csn_pin, HIGH);

  delay(5);

  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV8);
}

uint8_t nRF24::read_register(uint8_t reg, uint8_t* buf, uint8_t len)
{
  uint8_t status;

  csn(LOW);
  status = SPI.transfer( R_REGISTER | ( REGISTER_MASK & reg ) );
  while ( len-- )
    *buf++ = SPI.transfer(0xff);

  csn(HIGH);

  return status;
}

/****************************************************************************/

uint8_t nRF24::read_register(uint8_t reg)
{
  csn(LOW);
  SPI.transfer( R_REGISTER | ( REGISTER_MASK & reg ) );
  uint8_t result = SPI.transfer(0xff);

  csn(HIGH);
  return result;
}

/****************************************************************************/

uint8_t nRF24::write_register(uint8_t reg, const uint8_t* buf, uint8_t len)
{
  uint8_t status;

  csn(LOW);
  status = SPI.transfer( W_REGISTER | ( REGISTER_MASK & reg ) );
  while ( len-- )
    SPI.transfer(*buf++);

  csn(HIGH);

  return status;
}

/****************************************************************************/

uint8_t nRF24::write_register(uint8_t reg, uint8_t value)
{
  uint8_t status;

//  IF_SERIAL_DEBUG(printf_P(PSTR("write_register(%02x,%02x)\r\n"),reg,value));

  csn(LOW);
  status = SPI.transfer( W_REGISTER | ( REGISTER_MASK & reg ) );
  SPI.transfer(value);
  csn(HIGH);

  return status;
}

/****************************************************************************/

uint8_t nRF24::write_payload(const void* buf, uint8_t len)
{
  uint8_t status;

  const uint8_t* current = reinterpret_cast<const uint8_t*>(buf);

  uint8_t data_len = min(len,payload_size);
  uint8_t blank_len = dynamic_payloads_enabled ? 0 : payload_size - data_len;
  
  //printf("[Writing %u bytes %u blanks]",data_len,blank_len);
  
  csn(LOW);
  status = SPI.transfer( W_TX_PAYLOAD );
  while ( data_len-- )
    SPI.transfer(*current++);
  while ( blank_len-- )
    SPI.transfer(0);
  csn(HIGH);

  return status;
}

/****************************************************************************/

uint8_t nRF24::read_payload(void* buf, uint8_t len)
{
  uint8_t status;
  uint8_t* current = reinterpret_cast<uint8_t*>(buf);

  uint8_t data_len = min(len,payload_size);
  uint8_t blank_len = dynamic_payloads_enabled ? 0 : payload_size - data_len;
  
  //printf("[Reading %u bytes %u blanks]",data_len,blank_len);
  
  csn(LOW);
  status = SPI.transfer( R_RX_PAYLOAD );
  while ( data_len-- )
    *current++ = SPI.transfer(0xff);
  while ( blank_len-- )
    SPI.transfer(0xff);
  csn(HIGH);

  return status;
}

/****************************************************************************/

uint8_t nRF24::flush_rx(void)
{
  uint8_t status;

  csn(LOW);
  status = SPI.transfer( FLUSH_RX );
  csn(HIGH);

  return status;
}

/****************************************************************************/

uint8_t nRF24::flush_tx(void)
{
  uint8_t status;

  csn(LOW);
  status = SPI.transfer( FLUSH_TX );
  csn(HIGH);

  return status;
}

/****************************************************************************/

void nRF24::activate(uint8_t code)
{
  csn(LOW);
  SPI.transfer(ACTIVATE);
  SPI.transfer(code);
  csn(HIGH);
}

