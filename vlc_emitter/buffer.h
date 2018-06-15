#define BUFFER_SIZE 30

class Buffer {
  private:
    char buf[BUFFER_SIZE + 1];
    int head, tail;
  public:
    Buffer();
    inline bool push(char ch);
    inline bool pop(char *ch);
};

Buffer::Buffer() {
  head = 0;
  tail = 0;
}

inline bool Buffer::push(char ch) {
  if ((tail + 1) % (BUFFER_SIZE + 1) == head) {
    return false; // buffer is full
  } else {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      buf[tail] = ch;
      tail = (tail + 1) % (BUFFER_SIZE + 1);
    }
    return true;
  }
}

inline bool Buffer::pop(char *ch) {
  if (head == tail) {
    *ch = 0;
    return false;
  } else {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      *ch = buf[head];
      head = (head + 1) % (BUFFER_SIZE + 1);
    }
    return true;
  }
}
