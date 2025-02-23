
from copy import deepcopy
from datetime import *
import operator
import datetime
import dateparser

class ourcell:
    """As per https://stackoverflow.com/a/68932800/231306"""
    def __init__(self,val):
        self.set_val(val)
    def coords(self,col,row): # should we hide this from cell formulas?
        object.__setattr__(self, 'col', col)
        object.__setattr__(self, 'row', row)
        return self
    def fixed(self,col_fixed,row_fixed): # should we hide this from cell formulas?
        object.__setattr__(self, 'col_fixed', col_fixed)
        object.__setattr__(self, 'row_fixed', row_fixed)
        return self
    def set_val(self, val):
        object.__setattr__(self, '_proxied', val)
    def get_val(self):
        return object.__getattribute__(self, '_proxied')
    def get_final_val(self):
        v = self.get_val();
        if is_ourcell(v):
            v = v.get_final_val()
        return v
    def is_reference(self):
        return is_ourcell(self.get_val());
    def __getattribute__(self, name):
        try:
            return object.__getattribute__(self, name)
        except AttributeError:
            return self.get_final_val().getattr(p, name)
    def __setattr__(self, name, value):
        p = self.get_final_val()
        if hasattr(p, name):
            setattr(p, name, value)
        else:
            setattr(self, name, value)
    def __bool__(self):
        return self.get_final_val().__bool__()
    def __str__(self):
        return self.get_final_val().__str__()
    def __repr__(self):
        return self.get_final_val().__repr__()
    def __eq__(self,b):
        return self.get_final_val().__eq__(b)
    def __hash__(self):
        return self.get_final_val().__hash__()
    def __getitem__(self, key):
        return self.get_final_val().__getitem__(key)
    def __setitem__(self, key, value): # Should we return an altered copy of self?
        self.get_final_val().__setitem__(key, value)
    def __delitem__(self, key):        # Should we return an altered copy of self?
        self.get_final_val().__delitem__(key)
    def __add__(self, o):
        return self.get_final_val() + o
    def __radd__(self, o):
        return o + self.get_final_val()
    def __sub__(self, o):
        return self.get_final_val() - o
    def __rsub__(self, o):
        return o - self.get_final_val()
    def __mul__(self, o):
        return self.get_final_val() * o
    def __rmul__(self, o):
        return o * self.get_final_val()
    def __truediv__(self, o):
        return self.get_final_val() / o
    def __rtruediv__(self, o):
        return o / self.get_final_val()
    def __floordiv__(self, o):
        return self.get_final_val() // o
    def __rfloordiv__(self, o):
        return o // self.get_final_val()
    def __mod__(self, o):
        return self.get_final_val() % o
    def __rmod__(self, o):
        return o % self.get_final_val()
    def __pow__(self, o):
        return self.get_final_val() ** o
    def __rpow__(self, o):
        return o ** self.get_final_val()
    def __rshift__(self, o):
        return self.get_final_val() >> o
    def __rrshift__(self, o):
        return o >> self.get_final_val()
    def __lshift__(self, o):
        return self.get_final_val() << o
    def __rlshift__(self, o):
        return o << self.get_final_val()
    def __and__(self, o):
        return self.get_final_val() & o
    def __rand__(self, o):
        return o & self.get_final_val()
    def __or__(self, o):
        return self.get_final_val() | o
    def __ror__(self, o):
        return o | self.get_final_val()
    def __xor__(self, o):
        return self.get_final_val() ^ o
    def __rxor__(self, o):
        return o ^ self.get_final_val()
    def __lt__(self, o):
        return self.get_final_val() < o
    def __rlt__(self, o):
        return o < self.get_final_val()
    def __gt__(self, o):
        return self.get_final_val() > o
    def __rgt__(self, o):
        return o > self.get_final_val()
    def __le__(self, o):
        return self.get_final_val() <= o
    def __rle__(self, o):
        return o <= self.get_final_val()
    def __ge__(self, o):
        return self.get_final_val() >= o
    def __rge__(self, o):
        return o >= self.get_final_val()
    def __eq__(self, o):
        return self.get_final_val() == o
    def __req__(self, o):
        return o == self.get_final_val()
    def __ne__(self, o):
        return self.get_final_val() != o
    def __rne__(self, o):
        return o != self.get_final_val()
    def __isub__(self, o):
        x = self.get_final_val()
        x -= o
    def __risub__(self, o):
        o -= self.get_final_val()
    def __iadd__(self, o):
        x = self.get_final_val()
        x += o
    def __riadd__(self, o):
        o += self.get_final_val()
    def __imul__(self, o):
        x = self.get_final_val()
        x *= o
    def __rimul__(self, o):
        o *= self.get_final_val()
    def __idiv__(self, o):
        x = self.get_final_val()
        x /= o
    def __ridiv__(self, o):
        o /= self.get_final_val()
    def __imod__(self, o):
        x = self.get_final_val()
        x %= o
    def __rimod__(self, o):
        o %= self.get_final_val()
    def __ipow__(self, o):
        x = self.get_final_val()
        x **= o
    def __ripow__(self, o):
        o **= self.get_final_val()
    def __irshift__(self, o):
        x = self.get_final_val()
        x >>= o
    def __rirshift__(self, o):
        o >>= self.get_final_val()
    def __ilshift__(self, o):
        x = self.get_final_val()
        x <<= o
    def __rilshift__(self, o):
        o <<= self.get_final_val()
    def __iand__(self, o):
        x = self.get_final_val()
        x &= o
    def __riand__(self, o):
        o &= self.get_final_val()
    def __ior__(self, o):
        x = self.get_final_val()
        x |= o
    def __rior__(self, o):
        o |= self.get_final_val()
    def __ixor__(self, o):
        x = self.get_final_val()
        x ^= o
    def __rixor__(self, o):
        o ^= self.get_final_val()
    def __neg__(self):
        return - self.get_final_val()
    def __pos__(self):
        return + self.get_final_val()
    def __invert__(self):
        return ~ self.get_final_val()

def make_ourcell(val,col,row,col_fixed,row_fixed): #will work for any object you feed it, but only that object
    if isinstance(val, ourcell) and not col_fixed and not row_fixed:
        return deepcopy(val).coords(col,row).fixed(col_fixed,row_fixed)
    else:
        return ourcell(val).coords(col,row).fixed(col_fixed,row_fixed)

def is_ourcell(c):
    return isinstance(c, ourcell)


def try_parse_text(raw_text):
    for t in [bool, int, float, complex, list, tuple, set, dict]:
        try:
            return t(raw_text)
        except:
            continue
    try:
        x = dateparser.parse(raw_text)
        if x is None:
            raise
        if x.time() == datetime.time(0, 0, 0, 0):
            return x.date()
        return x
    except:
        pass
    return str(raw_text)


def TODAY():
    return datetime.today().date()
def NOW():
    return datetime.today()

