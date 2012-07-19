%module doxygen_basic_translate

%inline %{

/**
 * \brief
 * Brief description.
 * 
 * The comment text
 * \author Some author
 * \return Some number
 * \sa function2
 */
int function()
{
}

/**
 * A test of a very very very very very very very very very very very very very very very very
 * very very very very very long comment string.
 */
int function2()
{
}

/**
 * A test for overloaded functions
 * This is function \b one
 */
int function3(int a)
{
}

/**
 * A test for overloaded functions
 * This is function \b two
 */
int function3(int a, int b)
{
}

/**
 * A test of some mixed tag usage
 * \if CONDITION
 * This \a code fragment shows us something \.
 * \par Minuses:
 * \arg it's senseless
 * \arg it's stupid
 * \arg it's null
 *
 * \warning This may not work as expected
 *
 * \code
 * int main() { while(true); }
 * \endcode
 * \endif
 */
int function4()
{
}

/**
 * Test for default args
 */
int function5(int a=42)
{
}

%}
