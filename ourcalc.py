
from copy import deepcopy
from datetime import *
import operator

class ourcell:
    def __init__(self,val):
        object.__setattr__(self, '_proxied', val)
    def coords(self,col,row):
        object.__setattr__(self, 'col', col)
        object.__setattr__(self, 'row', row)
        return self
    def __getattribute__(self, name):
        try:
            return object.__getattribute__(self, name)
        except AttributeError:
            p = object.__getattribute__(self, '_proxied')
            return getattr(p, name)
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
        p = object.__getattribute__(self, '_proxied')
        return p[key]            
    def __setitem__(self, key, value):
        p = object.__getattribute__(self, '_proxied')
        p[key] = value            
    def __delitem__(self, key):
        p = object.__getattribute__(self, '_proxied')
        del p[key]
    def __add__(self, o):
        return object.__getattribute__(self, '_proxied').__add__(o)
    def __radd__(self, o):
        return object.__getattribute__(self, '_proxied').__radd__(o)
    def __sub__(self, o):
        return object.__getattribute__(self, '_proxied').__sub__(o)
    def __rsub__(self, o):
        return object.__getattribute__(self, '_proxied').__rsub__(o)
    def __mul__(self, o):
        return object.__getattribute__(self, '_proxied').__mul__(o)
    def __rmul__(self, o):
        return object.__getattribute__(self, '_proxied').__rmul__(o)
    def __truediv__(self, o):
        return object.__getattribute__(self, '_proxied').__truediv__(o)
    def __rtruediv__(self, o):
        return object.__getattribute__(self, '_proxied').__rtruediv__(o)
    def __floordiv__(self, o):
        return object.__getattribute__(self, '_proxied').__floordiv__(o)
    def __rfloordiv__(self, o):
        return object.__getattribute__(self, '_proxied').__rfloordiv__(o)
    def __mod__(self, o):
        return object.__getattribute__(self, '_proxied').__mod__(o)
    def __rmod__(self, o):
        return object.__getattribute__(self, '_proxied').__rmod__(o)
    def __pow__(self, o):
        return object.__getattribute__(self, '_proxied').__pow__(o)
    def __rpow__(self, o):
        return object.__getattribute__(self, '_proxied').__rpow__(o)
    def __rshift__(self, o):
        return object.__getattribute__(self, '_proxied').__rshift__(o)
    def __rrshift__(self, o):
        return object.__getattribute__(self, '_proxied').__rrshift__(o)
    def __lshift__(self, o):
        return object.__getattribute__(self, '_proxied').__lshift__(o)
    def __rlshift__(self, o):
        return object.__getattribute__(self, '_proxied').__rlshift__(o)
    def __and__(self, o):
        return object.__getattribute__(self, '_proxied').__and__(o)
    def __rand__(self, o):
        return object.__getattribute__(self, '_proxied').__rand__(o)
    def __or__(self, o):
        return object.__getattribute__(self, '_proxied').__or__(o)
    def __ror__(self, o):
        return object.__getattribute__(self, '_proxied').__ror__(o)
    def __xor__(self, o):
        return object.__getattribute__(self, '_proxied').__xor__(o)
    def __rxor__(self, o):
        return object.__getattribute__(self, '_proxied').__rxor__(o)
    def __lt__(self, o):
        return object.__getattribute__(self, '_proxied').__lt__(o)
    def __rlt__(self, o):
        return object.__getattribute__(self, '_proxied').__rlt__(o)
    def __gt__(self, o):
        return object.__getattribute__(self, '_proxied').__gt__(o)
    def __rgt__(self, o):
        return object.__getattribute__(self, '_proxied').__rgt__(o)
    def __le__(self, o):
        return object.__getattribute__(self, '_proxied').__le__(o)
    def __rle__(self, o):
        return object.__getattribute__(self, '_proxied').__rle__(o)
    def __ge__(self, o):
        return object.__getattribute__(self, '_proxied').__ge__(o)
    def __rge__(self, o):
        return object.__getattribute__(self, '_proxied').__rge__(o)
    def __eq__(self, o):
        return object.__getattribute__(self, '_proxied').__eq__(o)
    def __req__(self, o):
        return object.__getattribute__(self, '_proxied').__req__(o)
    def __ne__(self, o):
        return object.__getattribute__(self, '_proxied').__ne__(o)
    def __rne__(self, o):
        return object.__getattribute__(self, '_proxied').__rne__(o)
    def __isub__(self, o):
        return object.__getattribute__(self, '_proxied').__isub__(o)
    def __risub__(self, o):
        return object.__getattribute__(self, '_proxied').__risub__(o)
    def __iadd__(self, o):
        return object.__getattribute__(self, '_proxied').__iadd__(o)
    def __riadd__(self, o):
        return object.__getattribute__(self, '_proxied').__riadd__(o)
    def __imul__(self, o):
        return object.__getattribute__(self, '_proxied').__imul__(o)
    def __rimul__(self, o):
        return object.__getattribute__(self, '_proxied').__rimul__(o)
    def __idiv__(self, o):
        return object.__getattribute__(self, '_proxied').__idiv__(o)
    def __ridiv__(self, o):
        return object.__getattribute__(self, '_proxied').__ridiv__(o)
    def __imod__(self, o):
        return object.__getattribute__(self, '_proxied').__imod__(o)
    def __rimod__(self, o):
        return object.__getattribute__(self, '_proxied').__rimod__(o)
    def __ipow__(self, o):
        return object.__getattribute__(self, '_proxied').__ipow__(o)
    def __ripow__(self, o):
        return object.__getattribute__(self, '_proxied').__ripow__(o)
    def __irshift__(self, o):
        return object.__getattribute__(self, '_proxied').__irshift__(o)
    def __rirshift__(self, o):
        return object.__getattribute__(self, '_proxied').__rirshift__(o)
    def __ilshift__(self, o):
        return object.__getattribute__(self, '_proxied').__ilshift__(o)
    def __rilshift__(self, o):
        return object.__getattribute__(self, '_proxied').__rilshift__(o)
    def __iand__(self, o):
        return object.__getattribute__(self, '_proxied').__iand__(o)
    def __riand__(self, o):
        return object.__getattribute__(self, '_proxied').__riand__(o)
    def __ior__(self, o):
        return object.__getattribute__(self, '_proxied').__ior__(o)
    def __rior__(self, o):
        return object.__getattribute__(self, '_proxied').__rior__(o)
    def __ixor__(self, o):
        return object.__getattribute__(self, '_proxied').__ixor__(o)
    def __rixor__(self, o):
        return object.__getattribute__(self, '_proxied').__rixor__(o)
    def __neg__(self):
        return object.__getattribute__(self, '_proxied').__neg__()
    def __rfloordiv__(self):
        return object.__getattribute__(self, '_proxied').__pos__()
    def __invert__(self):
        return object.__getattribute__(self, '_proxied').__invert__()

def make_ourcell(val,col,row): #will work for any object you feed it, but only that object
    if isinstance(val, ourcell):
    	return deepcopy(val).coords(col,row)
    else:
    	return ourcell(val).coords(col,row)

def is_ourcell(c):
    return c.__class__.__name__ == 'ourcell' or c.__class__.__name__ == 'ourboolcell'



def TODAY():
    return datetime.today()

