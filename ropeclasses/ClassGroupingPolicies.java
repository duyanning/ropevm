// note: default is implemented by ropevm rather than the compiler.

@java.lang.annotation.Retention(java.lang.annotation.RetentionPolicy.RUNTIME)
@java.lang.annotation.Target({ java.lang.annotation.ElementType.TYPE })
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

