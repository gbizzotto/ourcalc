
def make_ourcell(val,col,row): #will work for any object you feed it, but only that object
    if isinstance(val,bool):
        class ourboolcell():
            def __init__(self, val):
                self.val = val
            def coords(self,col,row):
                setattr(self,"col",col)
                setattr(self,"row",row)
                return self
            def __bool__(self):
                return self.val
            def __str__(self):
                return str(val)
            def __repr__(self):
                return str(val)
            def __is__(self,other):
                return val is other 
            def __eq__(self,b):
                return val is b
            def __hash__(self):
                return val
        if type(val) is ourboolcell:
            result = deepcopy(val)
            result.col = col
            result.row = row
            return result
        else:
            return ourboolcell(val).coords(col,row)
    else:
        class ourcell(type(val)):
            def coords(self,col,row):
                setattr(self,"col",col)
                setattr(self,"row",row)
                return self
        if type(val) is ourcell:
            result = deepcopy(val)
            result.col = col
            result.row = row
            return result
        else:
            return ourcell(val).coords(col,row)

#        def type(self):
#        	return self.__class__.__bases__[0]


def is_ourcell(c):
	return c.__class__.__name__ == 'ourcell'

def TODAY():
	from datetime import datetime
	return datetime.today().strftime('%Y-%m-%d')
