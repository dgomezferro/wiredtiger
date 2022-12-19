from enum import Enum

class PrepareState(Enum):
    PREPARE_INIT = 0
    PREPARE_INPROGRESS = 1
    PREPARE_LOCKED = 2
    PREPARE_RESOLVED = 3

class UpdateType(Enum):
    UPDATE_MODIFY = 0
    UPDATE_RESERVE = 1
    UPDATE_STANDARD = 2
    UPDATE_TOMBSTONE = 3

class Timestamp():
    def __init__(self, start, stop):
        self.start = start
        self.stop = stop

    def __eq__(self, other):
        return self.start == other.start and self.stop == other.stop

    def __lt__(self, rhs):
        return ((self.start, self.stop) < (rhs.start, rhs.stop))

    def __le__(self, rhs):
        return self.__lt__(rhs) or self.__eq__(rhs)

    def __gt__(self, rhs):
        return ((self.start, self.stop) > (rhs.start, rhs.stop))

    def __ge__(self, rhs):
        return self.__gt__(rhs) or self.__eq__(rhs)

    def __repr__(self):
        return f"({self.start}, {self.stop})"

class Tree():
    def __init__(self, filename):
        self.logged = None
        self.file = filename

    def __eq__(self, other):
        return self.file == other.file

    def __hash__(self):
        return hash(self.file)

class Page():
    def __init__(self, addr):
        self.addr = addr
        self.modified = None

    def __eq__(self, other):
        return self.addr == other.addr

    def __hash__(self):
        return hash(self.addr)
