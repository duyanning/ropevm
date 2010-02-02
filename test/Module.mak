#   The Jupiter JVM
#   Copyright (C) 2001-2002 Patrick Doyle and Carlos Cavanna
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

JAVA_FILES := \
	$(wildcard test/*.java) \
	$(wildcard test/kaffe/*.java) \
	$(wildcard test/threads/basic/*.java) \
	$(wildcard test/threads/producer_consumer/*.java) \
	$(wildcard test/threads/regress/*.java) \
	$(wildcard test/threads/runnable/*.java) \
	$(wildcard test/recursion/factorial/*.java) \
	$(wildcard test/recursion/fibonacci/*.java) \
	$(wildcard test/recursion/hanoi/*.java) \
	$(wildcard test/recursion/initialization/*.java) \
	$(wildcard test/recursion/power/*.java) \
	$(wildcard test/java/lang/*.java)
CLASS_FILES := $(patsubst %.java,%.class,$(JAVA_FILES))

GL_GENERATED_MISC += $(wildcard test/*.class) $(wildcard test/threads/basic/*.class) $(wildcard test/threads/producer_consumer/*.class) $(wildcard test/threads/regress/*.class) $(wildcard test/threads/runnable/*.class) $(wildcard test/recursion/factorial/*.class) $(wildcard test/recursion/fibonacci/*.class) $(wildcard test/recursion/hanoi/*.class) $(wildcard test/recursion/initialization/*.class) $(wildcard test/recursion/power/*.class) $(wildcard test/java/lang/*.class)
# TODO: Take the *.class out of the above.  It's not safe.  Instead,
# ask the Java compiler what .class files are generated from $(JAVA_FILES).

$(CLASS_FILES): %.class: %.java
	cd $(dir $^); $(JC) $(JC_FLAGS) $(notdir $^); cd -

# Regression Testing
# The system is that any xxx.out file will automatically cause
# xxx.java to be compiled.  The resulting xxx.class will be run
# through jupiter, and its output will be compared with xxx.out.
OUT_FILES := $(wildcard test/*.out) $(wildcard test/kaffe/*.out) $(wildcard test/threads/basic/*.out) $(wildcard test/threads/producer_consumer/*.out) $(wildcard test/threads/regress/*.out) $(wildcard test/threads/runnable/*.out) $(wildcard test/recursion/factorial/*.out) $(wildcard test/recursion/fibonacci/*.out) $(wildcard test/recursion/hanoi/*.out) $(wildcard test/recursion/initialization/*.out) $(wildcard test/recursion/power/*.out)
SPECIAL_OUT_FILES := $(wildcard test/java/lang/*.out)
# Output needs sorting:
K_SKIP := Bean BeanBug ClassDeadLock ExceptionTestClassLoader GCTest Preempt Reflect ThreadLocalTest 
# Not yet implemented:
K_SKIP += \
	Alias CatchDeath ClassGC DeadThread DosTimeVerify \
	ExceptionInInitializerTest ExceptionTestClassLoader2 ExecTest \
	FPUStack FileTest FindSystemClass GetInterfaces HashTest IllegalWait \
	KaffeVerifyBug LoaderTest MapTest MarkResetTest MethodBug Overflow \
	SoInterrupt SoTimeout SortTest InvTarExcTest ReaderReadVoidTest \
	StackDump TestNative TestSerializable TestSerializable2 TestUnlock \
	UDPTest URLTest UncaughtException \
	burford forNameTest sysdepCallMethod \
	tname ttest tthrd1 wc TestClassRef
# Should be implemented, but doesn't work for some reason:
K_SKIP += PropertiesTest TestCasts divtest
#K_SKIP += InternHog # TODO: Get interning to work
K_SKIP += ArrayForName # TODO: Implement Class.getComponentType properly
K_SKIP := $(patsubst %,test/kaffe/%.out,$(K_SKIP)) \
	$(wildcard test/kaffe/CLTest*.out) \
	$(wildcard test/kaffe/Double*.out) \
	$(wildcard test/kaffe/Process*.out) \
	$(wildcard test/kaffe/Reflect*.out) \
	$(wildcard test/kaffe/Thread*.out) \
	$(wildcard test/kaffe/Zip*.out) \
	$(wildcard test/kaffe/final*.out) \
	$(wildcard test/kaffe/*ReaderTest.out)
OUT_FILES := $(filter-out $(K_SKIP), $(OUT_FILES))
TEST_TARGETS := $(patsubst %.out,%.test,$(OUT_FILES))
SPECIAL_TEST_TARGETS := $(patsubst %.out,%.test,$(SPECIAL_OUT_FILES))

GL_REGRESSION_MISC += $(patsubst %.out,%.class,$(OUT_FILES) $(SPECIAL_OUT_FILES))

.PHONY: test/regress $(TEST_TARGETS)

test/qregress: JUPITER := qjupiter
test/qregress: qjupiter $(TEST_TARGETS) $(SPECIAL_TEST_TARGETS) ztest finish

test/regress: JUPITER := top/jupiter
test/regress: top/jupiter $(TEST_TARGETS) $(SPECIAL_TEST_TARGETS) ztest finish

finish:
	@echo
	@echo "Done execution of the regression tests"

test/dumpvars:
	@echo OUT_FILES:  $(OUT_FILES)
	@echo TEST_FILES: $(TEST_TARGETS)

$(TEST_TARGETS): %.test: %.class $(JUPITER)
	@cd $(dir $*); \
	if ./Run-jupiter $(JUPITER) $(notdir $*) | diff $(notdir $*).out - ; \
		then echo '[passed]  $*'; \
		else echo '*** FAILED:  $*'; \
	fi

$(SPECIAL_TEST_TARGETS): test/java/lang/%.test: test/java/lang/%.class
	@cd test; \
	if ./Run-jupiter $(JUPITER) java/lang/$* | diff java/lang/$*.out - ; \
		then echo '[passed]  java/lang/$*'; \
		else echo '*** FAILED:  java/lang/$*'; \
	fi

# Special case dependencies
test/ReflectionTest.test: test/EmptyBench.class

# To build all the classfiles:
classes: $(CLASS_FILES)

# Mathew's stuff
# Z_JAVA_FILES will be any file with a main method
Z_DIR := test/org/zaleski/thesis/joj/test
Z_JAVA_FILES := $(shell grep -l main test/org/zaleski/thesis/joj/test/*.java 2>/dev/null)
Z_CLASS_FILES := $(patsubst %.java,%.class,$(Z_JAVA_FILES))

Z_SKIP := CallJNI CallNativeStatic FloatTightLoop HelloWorld ICast TightLoop TestNewArray TestClassNewInstance
Z_SKIP := $(patsubst %,$(Z_DIR)/%.test,$(Z_SKIP))
Z_TESTS := $(filter-out $(Z_SKIP),$(patsubst %.java,%.test,$(Z_JAVA_FILES)))

.PHONY: $(Z_TESTS)

zclasses: $(Z_CLASS_FILES)
GL_GENERATED_MISC += $(wildcard $(Z_DIR)/*.class)

$(Z_CLASS_FILES): %.class: %.java
	$(JC) $(JC_FLAGS) $^

ztest: $(Z_TESTS)

$(Z_TESTS): test/%.test: test/%.class zclasses
	cd test; ../$(JUPITER) $*

