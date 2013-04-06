def process_file(f):
    phase = 0
    packet = -1
    count = 0
    for line in f:
        line = line.strip()
        if len(line) == 0 or line[0] == '#': continue
        parts = line.split(',')
        if len(parts) != 4: continue
        if phase == 0:
            if parts[2] == '0x25':
                packet = int(parts[1])
                phase = 1
        else:
            if packet == int(parts[1]):
                print ("%02X" % int(parts[2], 16)),
                count += 1
                if count == 32:
                    print
                    count = 0
            phase = 0

def main(argv):
    for fn in argv:
        f = file(fn)
        process_file(f)

if __name__ == '__main__':
    import sys
    main(sys.argv[1:])