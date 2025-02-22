
from copy import deepcopy
from datetime import *
import operator
import datetime
import dateparser

class ourcell:
    """As per https://stackoverflow.com/a/68932800/231306"""
    def __init__(self,val):
        object.__setattr__(self, '_proxied', val)
    def coords(self,col,row): # should we hide this from cell formulas?
        object.__setattr__(self, 'col', col)
        object.__setattr__(self, 'row', row)
        return self
    def __getattribute__(self, name):
        try:
            return object.__getattribute__(self, name)
        except AttributeError:
            return object.__getattribute__(self, '_proxied').getattr(p, name)
    def __setattr__(self, name, value):
        p = object.__getattribute__(self, '_proxied')
        if hasattr(p, name):
            setattr(p, name, value)
        else:
            setattr(self, name, value)
    def __bool__(self):
        return object.__getattribute__(self, '_proxied').__bool__()
    def __str__(self):
        return object.__getattribute__(self, '_proxied').__str__()
    def __repr__(self):
        return object.__getattribute__(self, '_proxied').__repr__()
    def __eq__(self,b):
        return object.__getattribute__(self, '_proxied').__eq__(b)
    def __hash__(self):
        return object.__getattribute__(self, '_proxied').__hash__()
    def __getitem__(self, key):
        return object.__getattribute__(self, '_proxied').__getitem__(key)
    def __setitem__(self, key, value): # Should we return an altered copy of self?
        object.__getattribute__(self, '_proxied').__setitem__(key, value)
    def __delitem__(self, key):        # Should we return an altered copy of self?
        object.__getattribute__(self, '_proxied').__delitem__(key)
    def __add__(self, o):
        return object.__getattribute__(self, '_proxied') + o
    def __radd__(self, o):
        return o + object.__getattribute__(self, '_proxied')
    def __sub__(self, o):
        return object.__getattribute__(self, '_proxied') - o
    def __rsub__(self, o):
        return o - object.__getattribute__(self, '_proxied')
    def __mul__(self, o):
        return object.__getattribute__(self, '_proxied') * o
    def __rmul__(self, o):
        return o * object.__getattribute__(self, '_proxied')
    def __truediv__(self, o):
        return object.__getattribute__(self, '_proxied') / o
    def __rtruediv__(self, o):
        return o / object.__getattribute__(self, '_proxied')
    def __floordiv__(self, o):
        return object.__getattribute__(self, '_proxied') // o
    def __rfloordiv__(self, o):
        return o // object.__getattribute__(self, '_proxied')
    def __mod__(self, o):
        return object.__getattribute__(self, '_proxied') % o
    def __rmod__(self, o):
        return o % object.__getattribute__(self, '_proxied')
    def __pow__(self, o):
        return object.__getattribute__(self, '_proxied') ** o
    def __rpow__(self, o):
        return o ** object.__getattribute__(self, '_proxied')
    def __rshift__(self, o):
        return object.__getattribute__(self, '_proxied') >> o
    def __rrshift__(self, o):
        return o >> object.__getattribute__(self, '_proxied')
    def __lshift__(self, o):
        return object.__getattribute__(self, '_proxied') << o
    def __rlshift__(self, o):
        return o << object.__getattribute__(self, '_proxied')
    def __and__(self, o):
        return object.__getattribute__(self, '_proxied') & o
    def __rand__(self, o):
        return o & object.__getattribute__(self, '_proxied')
    def __or__(self, o):
        return object.__getattribute__(self, '_proxied') | o
    def __ror__(self, o):
        return o | object.__getattribute__(self, '_proxied')
    def __xor__(self, o):
        return object.__getattribute__(self, '_proxied') ^ o
    def __rxor__(self, o):
        return o ^ object.__getattribute__(self, '_proxied')
    def __lt__(self, o):
        return object.__getattribute__(self, '_proxied') < o
    def __rlt__(self, o):
        return o < object.__getattribute__(self, '_proxied')
    def __gt__(self, o):
        return object.__getattribute__(self, '_proxied') > o
    def __rgt__(self, o):
        return o > object.__getattribute__(self, '_proxied')
    def __le__(self, o):
        return object.__getattribute__(self, '_proxied') <= o
    def __rle__(self, o):
        return o <= object.__getattribute__(self, '_proxied')
    def __ge__(self, o):
        return object.__getattribute__(self, '_proxied') >= o
    def __rge__(self, o):
        return o >= object.__getattribute__(self, '_proxied')
    def __eq__(self, o):
        return object.__getattribute__(self, '_proxied') == o
    def __req__(self, o):
        return o == object.__getattribute__(self, '_proxied')
    def __ne__(self, o):
        return object.__getattribute__(self, '_proxied') != o
    def __rne__(self, o):
        return o != object.__getattribute__(self, '_proxied')
    def __isub__(self, o):
        x = object.__getattribute__(self, '_proxied')
        x -= o
    def __risub__(self, o):
        o -= object.__getattribute__(self, '_proxied')
    def __iadd__(self, o):
        x = object.__getattribute__(self, '_proxied')
        x += o
    def __riadd__(self, o):
        o += object.__getattribute__(self, '_proxied')
    def __imul__(self, o):
        x = object.__getattribute__(self, '_proxied')
        x *= o
    def __rimul__(self, o):
        o *= object.__getattribute__(self, '_proxied')
    def __idiv__(self, o):
        x = object.__getattribute__(self, '_proxied')
        x /= o
    def __ridiv__(self, o):
        o /= object.__getattribute__(self, '_proxied')
    def __imod__(self, o):
        x = object.__getattribute__(self, '_proxied')
        x %= o
    def __rimod__(self, o):
        o %= object.__getattribute__(self, '_proxied')
    def __ipow__(self, o):
        x = object.__getattribute__(self, '_proxied')
        x **= o
    def __ripow__(self, o):
        o **= object.__getattribute__(self, '_proxied')
    def __irshift__(self, o):
        x = object.__getattribute__(self, '_proxied')
        x >>= o
    def __rirshift__(self, o):
        o >>= object.__getattribute__(self, '_proxied')
    def __ilshift__(self, o):
        x = object.__getattribute__(self, '_proxied')
        x <<= o
    def __rilshift__(self, o):
        o <<= object.__getattribute__(self, '_proxied')
    def __iand__(self, o):
        x = object.__getattribute__(self, '_proxied')
        x &= o
    def __riand__(self, o):
        o &= object.__getattribute__(self, '_proxied')
    def __ior__(self, o):
        x = object.__getattribute__(self, '_proxied')
        x |= o
    def __rior__(self, o):
        o |= object.__getattribute__(self, '_proxied')
    def __ixor__(self, o):
        x = object.__getattribute__(self, '_proxied')
        x ^= o
    def __rixor__(self, o):
        o ^= object.__getattribute__(self, '_proxied')
    def __neg__(self):
        return - object.__getattribute__(self, '_proxied')
    def __pos__(self):
        return + object.__getattribute__(self, '_proxied')
    def __invert__(self):
        return ~ object.__getattribute__(self, '_proxied')

def make_ourcell(val,col,row): #will work for any object you feed it, but only that object
    if isinstance(val, ourcell):
        return deepcopy(val).coords(col,row)
    else:
        return ourcell(val).coords(col,row)

def is_ourcell(c):
    return isinstance(c, ourcell)


def try_parse_text(raw_text):
    print("parsing", raw_text)
    try:
        return int(raw_text)
    except:
        pass
    print("not an int")
    try:
        return float(raw_text)
    except:
        pass
    print("not a float")
    try:
        x = dateparser.parse(raw_text)
        print(x)
        if x is None:
            raise
        if x.time() == datetime.time(0, 0, 0, 0):
            print("only date")
            return x.date()
        return x
    except:
        pass
    print("not a datetime")

    return str(raw_text)


def TODAY():
    return datetime.today().date()
def NOW():
    return datetime.today()

