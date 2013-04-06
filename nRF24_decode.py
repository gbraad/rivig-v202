reg_map = {
0x00 : "CONFIG",
0x01 : "EN_AA",
0x02 : "EN_RXADDR",
0x03 : "SETUP_AW",
0x04 : "SETUP_RETR",
0x05 : "RF_CH",
0x06 : "RF_SETUP",
0x07 : "STATUS",
0x08 : "OBSERVE_TX",
0x09 : "CD",
0x0A : "RX_ADDR_P0",
0x0B : "RX_ADDR_P1",
0x0C : "RX_ADDR_P2",
0x0D : "RX_ADDR_P3",
0x0E : "RX_ADDR_P4",
0x0F : "RX_ADDR_P5",
0x10 : "TX_ADDR",
0x11 : "RX_PW_P0",
0x12 : "RX_PW_P1",
0x13 : "RX_PW_P2",
0x14 : "RX_PW_P3",
0x15 : "RX_PW_P4",
0x16 : "RX_PW_P5",
0x17 : "FIFO_STATUS",
0x1C : "DYNPD",
0x1D : "FEATURE"
}


bank = 0

def register_name(bank, reg):
    def_name = "%02X" % reg
    if bank == 1:
        return def_name
    return reg_map.get(reg, def_name)
        

#define R_REGISTER    0x00
#define W_REGISTER    0x20
#define REGISTER_MASK 0x1F
#define ACTIVATE      0x50
#define R_RX_PL_WID   0x60
#define R_RX_PAYLOAD  0x61
#define W_TX_PAYLOAD  0xA0
#define W_ACK_PAYLOAD 0xA8 + PIPE (0..5)
#define FLUSH_TX      0xE1
#define FLUSH_RX      0xE2
#define REUSE_TX_PL   0xE3
#define W_TX_PAYLOAD_NOACK  0xD0
#define NOP           0xFF

def dump_command(packet_id, command, status, data_out, data_in):
  global bank
  if packet_id == -1: return
  mnemonic = "UNKNOWN"
  if (command & 0xe0) == 0:
      mnemonic = "R_REGISTER(%s) " % register_name(bank, command & 0x1f)
      mnemonic += " ".join(map(lambda x: "%02X" % x, data_in))
  elif (command & 0xe0) == 0x20:
      mnemonic = "W_REGISTER(%s) " % register_name(bank, command & 0x1f)
      mnemonic += " ".join(map(lambda x: "%02X" % x, data_out))
  elif command == 0x50:
      mnemonic = "ACTIVATE(%02X)" % data_out[0]
      if data_out[0] == 0x53:
          # Beken BK2421 register bank switch
          bank = 1 - bank
          mnemonic += " bank switch to %d" % bank
  elif command == 0x61:
      mnemonic = "R_RX_PAYLOAD " + " ".join(map(lambda x: "%02X" % x, data_in))
  elif command == 0xa0:
      mnemonic = "W_TX_PAYLOAD " + " ".join(map(lambda x: "%02X" % x, data_out))
  elif command == 0xe1:
      mnemonic = "FLUSH_TX"
  elif command == 0xe2:
      mnemonic = "FLUSH_RX"
  elif command == 0xe3:
      mnemonic = "REUSE_TX_PL"

  print packet_id, mnemonic

def process_file(f):
    packet_id = -1
    # SPI exchange parameters
    command = -1
    status = -1
    data_out = []
    data_in = []

    count = 0

    for line in f:
        line = line.strip()
        if len(line) == 0 or line[0] == '#': continue
        parts = line.split(',')
        if len(parts) != 4 or parts[1] == 'Packet ID' or parts[1] == '': continue
        if packet_id != int(parts[1]):
            dump_command(packet_id, command, status, data_out, data_in)
            command = int(parts[2], 16)
            status = int(parts[3], 16)
            packet_id = int(parts[1])
            data_out = []
            data_in = []
        else:
            data_out.append(int(parts[2], 16))
            data_in.append(int(parts[3], 16))
    dump_command(packet_id, command, status, data_out, data_in)

def main(argv):
    for fn in argv:
        f = file(fn)
        process_file(f)

if __name__ == '__main__':
    import sys
    main(sys.argv[1:])