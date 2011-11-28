import java.lang.annotation.*;
// note: default is implemented by ropevm rather than the compiler.

@Retention(RetentionPolicy.RUNTIME)
@Target({ ElementType.TYPE })
public @interface ClassGroupingPolicies {
	GroupingPolicy self() default GroupingPolicy.UNSPECIFIED;
	GroupingPolicy others() default GroupingPolicy.UNSPECIFIED;
}

/*
example:

@ClassGroupingPolicies(self=GroupingPolicy.NEW_GROUP, others=GroupingPolicy.CURRENT_GROUP)
class Car {
}

*/

