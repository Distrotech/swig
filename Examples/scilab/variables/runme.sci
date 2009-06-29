// builder the *.so
exec builder.sce

//loader the *.so
exec loader.sce

// Try to set the values of some global variables

ivar_set  (42);
svar_set  (31000);
lvar_set  (65537);
uivar_set (123456);
usvar_set (61000);
ulvar_set (654321);
scvar_set (-13);
ucvar_set (251);
cvar_set  ("S");
fvar_set  (3.14159);
dvar_set  (2.1828);
strvar_set("Hello World");
Foo1_set([1,2,3;4,5,6]);
Foo2_set([1+2*%i,2+3*%i;3+4*%i,7+8*%i]);

// Now print out the values of the variables

printf("Variables (values printed from Scilab)\n");

printf("ivar      = %i\n", ivar_get());
printf("svar      = %i\n", svar_get());
printf("lvar      = %i\n", lvar_get());
printf("uivar     = %i\n", uivar_get());
printf("usvar     = %i\n", usvar_get());
printf("ulvar     = %i\n", ulvar_get());
printf("scvar     = %i\n", scvar_get());
printf("ucvar     = %i\n", ucvar_get());
printf("fvar      = %f\n", fvar_get());
printf("dvar      = %f\n", dvar_get());
printf("cvar      = %s\n", cvar_get());
printf("strvar    = %s\n", strvar_get());
Foo1_get();
Foo2_get();

printf("\nVariables (values printed from C)\n");

print_vars()




