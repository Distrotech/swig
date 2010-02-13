use strict;
use warnings;
use Test::More 'no_plan';
BEGIN { use_ok 'director_frob' }
require_ok 'director_frob';

my $foo = director_frob::Bravo->new();
isa_ok $foo, 'director_frob::Bravo';

is($foo->abs_method(), 'Bravo::abs_method()');
