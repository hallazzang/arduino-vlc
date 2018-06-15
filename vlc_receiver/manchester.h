unsigned long manchesterEncode(char ch) {
  unsigned long ret = 0;

  for (int i = 0; i < 8; i++) {
    ret <<= 2;
    if (ch & 0x80) {
      ret |= 0x1; // manchester-encoded 1
    } else {
      ret |= 0x2; // manchester-encoded 0
    }
    ch <<= 1;
  }

  return ret;
}

unsigned long encodeWord(char ch) {
  unsigned long val = manchesterEncode(ch);

  return 0x80000 | (val << 2) | 0x1;
}

inline bool manchesterDecode(unsigned long val, char *ch) {
  *ch = 0;

  for (int i = 0; i < 8; i++) {
    unsigned long t = val & 0xc000;
    *ch <<= 1;
    if (t == 0x4000) {
      *ch |= 0x1;
    } else if (t != 0x8000) {
      return false;
    }
    val <<= 2;
  }

  return true;
}

inline bool checkWord(unsigned long val) {
  return (val & 0xc0000) == 0x80000 && (val & 0x3) == 0x1;
}

inline bool decodeWord(unsigned long val, char *ch) {
  if (!checkWord(val)) {
    return false;
  }

  return manchesterDecode(val >> 2, ch);
}
