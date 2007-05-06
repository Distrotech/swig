#!/usr/bin/env ruby
#
# This is a test of STL containers using proc
# objects to change the sorting function used in them.  Same as a
# std::binary_predicate in C++.
#
# Author::    
# Copyright:: 
# License::   Ruby
#

require 'swig_assert'
require 'stl_new'


def _set(container)
  swig_assert_each_line(<<EOF, binding)
    cont = #{container}.new
    [9,1,8,2,7,3,6,4,5].each { |x| cont.insert(x) }
    i0 = cont.begin()
    cont.to_a == [1,2,3,4,5,6,7,8,9]
    cont = #{container}.new( proc { |a,b| b < a } )
    [9,1,8,2,7,3,6,4,5].each { |x| cont.insert(x) }
    cont.to_a == [9, 8, 7, 6, 5, 4, 3, 2, 1]
    cont = #{container}.new( proc { |a,b| b > a } )
    [9,1,8,2,7,3,6,4,5].each { |x| cont.insert(x) }
    cont.to_a == [1, 2, 3, 4, 5, 6, 7, 8, 9]
    cont = #{container}.new(proc { |a,b| b < a } )
    cont.insert(1)
    cont.to_a == [1]
    i0 = cont.begin()
    cont.erase(i0) # don't use i0 anymore, it is invalid now
    cont.to_a == []
EOF
end
    
def _map(container)
  swig_assert_each_line(<<EOF, binding)
    cont = #{container}.new
    cont['z'] = 9
    cont['y'] = 1
    cont['x'] = 8
    cont['w'] = 2
    cont.to_a == [['w',2],['x',8],['y',1],['z',9]]

    cont = #{container}.new(proc { |a,b| b < a } )
    cont['z'] = 9
    cont['y'] = 1
    cont['x'] = 8
    cont['w'] = 2
    cont.to_a == [['z',9],['y',1],['x',8],['w',2]]
EOF
end

def test
  yield method(:_set), Stl_new::Set
  yield method(:_map), Stl_new::Map
end


test do |proc, container|
  proc.call(container)
end


