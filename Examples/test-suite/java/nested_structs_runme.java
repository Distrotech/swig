
import nested_structs.*;

public class nested_structs_runme {

  static {
    try {
	System.loadLibrary("nested_structs");
    } catch (UnsatisfiedLinkError e) {
      System.err.println("Native code library failed to load. See the chapter on Dynamic Linking Problems in the SWIG Java documentation for help.\n" + e);
      System.exit(1);
    }
  }

  public static void main(String argv[]) {
    Outer outer = new Outer();
    nested_structs.setValues(outer, 10);

    Outer_inner1 inner1 = outer.getInner1();
    Outer_inner2 inner2 = outer.getInner2();
    Outer_inner3 inner3 = outer.getInner3();
    Outer_inner4 inner4 = outer.getInner4();
    if (inner1.getVal() != 10) throw new RuntimeException("failed inner1");
    if (inner2.getVal() != 20) throw new RuntimeException("failed inner2");
    if (inner3.getVal() != 20) throw new RuntimeException("failed inner3");
    if (inner4.getVal() != 40) throw new RuntimeException("failed inner4");

    Outer_inside1 inside1 = outer.getInside1();
    Outer_inside2 inside2 = outer.getInside2();
    Outer_inside3 inside3 = outer.getInside3();
    Outer_inside4 inside4 = outer.getInside4();
    if (inside1.getVal() != 100) throw new RuntimeException("failed inside1");
    if (inside2.getVal() != 200) throw new RuntimeException("failed inside2");
    if (inside3.getVal() != 200) throw new RuntimeException("failed inside3");
    if (inside4.getVal() != 400) throw new RuntimeException("failed inside4");
  }
}
