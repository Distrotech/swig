require 'example'

# This should invoke foo(double)
Example::foo(3.14159)

# This should invoke foo(double, char *)
Example::foo(3.14159, "Pi")

# This should invoke foo(int, int)
Example::foo(3, 4)

# This should invoke foo(char *)
Example::foo("This is a test")

# This should invoke foo(long)
Example::foo(42)

# This should invoke foo(const Bar&)
Example::foo(Example::Bar.new)

# This should invoke Bar::Bar(double)
Example::Bar.new(3.14159)

# This should invoke Bar::Bar(double, char *)
Example::Bar.new(3.14159, "Pi")

# This should invoke Bar::Bar(int, int)
Example::Bar.new(3, 4)

# This should invoke Bar::Bar(char *)
Example::Bar.new("This is a test")

# This should invoke Bar::Bar(long)
Example::Bar.new(42)

# This should invoke Bar::Bar() for the input argument,
# followed by Bar::Bar(const Bar&).
Example::Bar.new(Example::Bar.new)

# Construct a new Bar instance (invokes Bar::Bar())
bar = Example::Bar.new

# This should invoke Bar::foo(double)
bar.foo(3.14159)

# This should invoke Bar::foo(double, char *)
bar.foo(3.14159, "Pi")

# This should invoke Bar::foo(int, int)
bar.foo(3, 4)

# This should invoke Bar::foo(char *)
bar.foo("This is a test")

# This should invoke Bar::foo(long)
bar.foo(42)

# This should invoke Bar::Bar() to construct the input
# argument, followed by Bar::foo(Bar const&).
bar.foo(Example::Bar.new)
