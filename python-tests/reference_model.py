def dot_i8(a, b):
  assert len(a) == len(b)
  return sum(int(x) * int(y) for x, y in zip(a, b))
