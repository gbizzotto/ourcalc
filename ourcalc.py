if __name__!="__main__":
	from ourcalc_cell import get_cell_value

class range:
	def __init__(self,a,b):
		self._a = a;
		self._b = b
	def __str__(self):
		return str(self._a) + ">>" + str(self._b)

class cell:
	def __init__(self,c,r):
		self._c = c
		self._r = r
	def __str__(self):
		return get_cell_value(self._c,self._r)
	def __rshift__(self, other):
		return range(self,other)


class column:
	def __init__(self,c):
		self.col = c
	def __getitem__(self,row):
		return cell(self.col,row)


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

if __name__=="__main__":
	i = 0
	while i<=55:
		print(i, column_name_from_int(i))
		i = i+1
	print(column_name_from_int(27*26-1))
	print(column_name_from_int(27*26  ))
	print(column_name_from_int(27*26+1))
