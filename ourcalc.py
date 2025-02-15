if __name__!="__main__":
	from ourcalc_cell import get_cell_value
	from ourcalc_cell import get_cell_type

import builtins
import sys

#class range:
#	def __init__(self,a,b):
#		self._a = a;
#		self._b = b
#	def __str__(self):
#		return str(self._a) + ">>" + str(self._b)

class dummy:
	def __init__(self,s):
		self.x = int(s)
	def __str__(self):
		return str(self.x)

def foo(d):
	return dummy(str(d.x+1))

# __builtins__ should work, since it works in standalone python3,
# however, it doesnt in pybind11, so here goes a workaround
builtin_types = {cls.__name__: cls for cls in [str, int, float, complex, range, tuple, bool, list, dict, set, bytes]}
this_module = sys.modules[__name__]

def get_type_by_name(type_name):
	try:
		return getattr(__builtins__, type_name)
	except:
		print(type_name, "not in __builtins__")
		if type_name in builtin_types:
			return builtin_types[type_name]
		else:
			print(type_name, "not in builtin_types")
			try:
				return getattr(this_module, type_name)
			except:
				return globals()[type_name]

class column:
	def __init__(self,c):
		self.col = c
	def __getitem__(self,row):
		type_name = get_cell_type(self.col,row)
		print("let's try to find type:", type_name)
		t = get_type_by_name(type_name)
		print("Type found:", t)
		if t is not None:
			return t(get_cell_value(self.col,row))
		else:
			return None


def column_name_from_int(c):
	remainder = c%26
	c = int(c/26)
	result = "" + chr(ord('A')+remainder)
	while c!=0:
		c -= 1
		remainder = c%26
		c = int(c/26)
		result += chr(ord('A')+remainder)
	result = result[::-1]
	return result


def TODAY():
	from datetime import datetime
	return datetime.today().strftime('%Y-%m-%d')





# for testing purpose
if __name__=="__main__":
	for i in range(55):
		print(i, column_name_from_int(i))
	print(column_name_from_int(27*26-1))
	print(column_name_from_int(27*26  ))
	print(column_name_from_int(27*26+1))
