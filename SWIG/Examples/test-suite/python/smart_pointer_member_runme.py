from smart_pointer_member import *

f = Foo()
f.y = 1

if f.y != 1:
  raise RuntimeError

b = Bar(f)
b.y = 2

if f.y != 2:
  raise RuntimeError

if b.x != f.x:
  raise RuntimeError

if b.z != f.z:
  raise RuntimeError
