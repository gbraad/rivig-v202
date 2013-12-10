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

ring_buffer = [0xff] * 16
ring_pos = 0
cycle_begin = -1
in_cycle = False
cycles = {}
last_id = None
first_cycle = False

def process_command(packet_id, command, status, data_out, data_in):
  global bank
  global ring_buffer
  global ring_pos
  global cycle_begin
  global in_cycle
  global cycles
  global last_id
  global first_cycle
  if packet_id == -1: return
  mnemonic = ""
  if (command & 0xe0) == 0:
      # Read register
      register = command & 0x1f
      rname = register_name(bank, register)
#      mnemonic = "R_REGISTER(%s) " % rname
#      mnemonic += " ".join(map(lambda x: "%02X" % x, data_in))
  elif (command & 0xe0) == 0x20:
      # Write register
      register = command & 0x1f
      rname = register_name(bank, register)
      if rname == 'EN_AA' and len(data_out) == 1 and data_out[0] == 0:
          mnemonic = "Radio start"
          bank = 0
          ring_buffer = [0xff] * 16
          ring_pos = 0
          in_cycle = False
          first_cycle = True
      elif rname == 'RF_CH' and len(data_out) == 1:
          ch = data_out[0]
          mnemonic = "CH %02X" % ch
          if ring_buffer[ring_pos] == ch:
              if not in_cycle:
                  print "Cycle detected"
                  cycle_begin = ring_pos
                  cycle = ring_buffer[ring_pos:] + ring_buffer[0:ring_pos]
                  cycles.setdefault(frozenset(cycle), []).append((cycle, first_cycle, last_id))
                  first_cycle = False
              elif cycle_begin == ring_pos:
                  print "Cycle begin"
              in_cycle = True
          else:
              if in_cycle:
                  print "Cycle broken"
                  ring_buffer = [0xff] * 16
                  ring_pos = 0
                  in_cycle = False
              ring_buffer[ring_pos] = ch
          ring_pos = (ring_pos + 1) % 16
#      mnemonic = "W_REGISTER(%s) " % rname
#      mnemonic += " ".join(map(lambda x: "%02X" % x, data_out))
  elif command == 0x50:
#      mnemonic = "ACTIVATE(%02X)" % data_out[0]
      if data_out[0] == 0x53:
          # Beken BK2421 register bank switch
          bank = 1 - bank
#          mnemonic += " bank switch to %d" % bank
  elif command == 0x61:
      mnemonic = "R_RX_PAYLOAD " + " ".join(map(lambda x: "%02X" % x, data_in[7:10]))
      last_id = data_in[7:10]
  elif command == 0xa0:
      mnemonic = "W_TX_PAYLOAD " + " ".join(map(lambda x: "%02X" % x, data_out))
  elif command == 0xe1:
#      mnemonic = "FLUSH_TX"
       pass
  elif command == 0xe2:
#      mnemonic = "FLUSH_RX"
       pass
  elif command == 0xe3:
      mnemonic = "REUSE_TX_PL"

  if mnemonic:
      print packet_id, mnemonic

def process_file(f):
    global last_id
    packet_id = -1
    # SPI exchange parameters
    command = -1
    status = -1
    data_out = []
    data_in = []
    last_id = None
    

    count = 0

    for line in f:
        line = line.strip()
        if len(line) == 0 or line[0] == '#': continue
        parts = line.split(',')
        if len(parts) != 4 or parts[1] == 'Packet ID' or parts[1] == '': continue
        if packet_id != int(parts[1]):
            process_command(packet_id, command, status, data_out, data_in)
            command = int(parts[2], 16)
            status = int(parts[3], 16)
            packet_id = int(parts[1])
            data_out = []
            data_in = []
        else:
            data_out.append(int(parts[2], 16))
            data_in.append(int(parts[3], 16))
    process_command(packet_id, command, status, data_out, data_in)

def main(argv):
    global cycles
    for fn in argv:
        f = file(fn)
        process_file(f)
    print "%d distinct cycles found" % len(cycles)
    real_cycles = []
    for index, (key, val) in enumerate(cycles.iteritems()):
#        common = []
#        for index1, key1  in enumerate(cycles.iterkeys()):
#            common.append(len(key & key1))
        best_cycle = val[0][0]
        tr_ids = set()
        for cycle, first, tr_id in val:
            if first and tr_id:
                if not tr_ids:
                    best_cycle = cycle
                    real_cycles.append(cycle)
                tr_ids.add("".join(map(lambda x : "%02X" % x, tr_id)))
        print "%2d, %s - %s |  %s" % (len(val), 'R' if first else ' ',
                                           ", ".join(map(lambda x: "%02X" % x, best_cycle)),
                                           ", ".join(tr_ids))
#                                           ", ".join(map(lambda x: "%2d" % x, common)))
    for cycle in real_cycles:
        print "{", ", ".join(map(lambda x: "0x%02x" % x, cycle)), "},"

if __name__ == '__main__':
    import sys
    main(sys.argv[1:])