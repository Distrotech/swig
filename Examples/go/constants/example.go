/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 2.0.1
 * 
 * This file is not intended to be easily readable and contains a number of 
 * coding conventions designed to improve portability and efficiency. Do not make
 * changes to this file unless you know what you are doing--modify the SWIG 
 * interface file instead. 
 * ----------------------------------------------------------------------------- */

package example


type _swig_fnptr *byte
type _swig_memberptr *byte


func _swig_allocatememory(int) *byte
func _swig_internal_allocate(len int) *byte {
	return _swig_allocatememory(len)
}

func _swig_allocatestring(*byte, int) string
func _swig_internal_makegostring(p *byte, l int) string {
	return _swig_allocatestring(p, l)
}

func _swig_internal_gopanic(p *byte, l int) {
	panic(_swig_allocatestring(p, l))
}

const ICONST int = 42
const FCONST float64 = 2.1828
const CCONST byte = 'x'
func _swig_getCCONST2() byte
var CCONST2 byte = _swig_getCCONST2()
const SCONST string = "Hello World"
func _swig_getSCONST2() string
var SCONST2 string = _swig_getSCONST2()
func _swig_getEXPR() float64
var EXPR float64 = _swig_getEXPR()
const Iconst int = 37
const Fconst float64 = 3.14

