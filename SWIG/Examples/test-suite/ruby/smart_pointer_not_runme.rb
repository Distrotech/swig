require 'smart_pointer_not'

include Smart_pointer_not

f = Foo.new
b = Bar.new(f)
s = Spam.new(f)
g = Grok.new(f)

begin
  x = b.x
  puts "Error! b.x"
rescue
end

begin
  x = s.x
  puts "Error! s.x"    
rescue
end

begin
  x = g.x
  puts "Error! g.x"
rescue
end

begin
  x = b.getx()
  puts "Error! b.getx()"    
rescue
end

begin
  x = s.getx()
  puts "Error! s.getx()"        
rescue
end

begin
  x = g.getx()
  puts "Error! g.getx()"
rescue
end
