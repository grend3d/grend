ifeq ($(IS_EMSCRIPTEN),1)
DEMO_TARGETS += $(BUILD)/landscape-test.html
DEMO_CLEAN += clean-landscape-test-thing-html

else
DEMO_TARGETS += $(BUILD)/landscape-test
DEMO_CLEAN += clean-landscape-test-thing
endif

$(BUILD)/landscape-test: demos/landscape-test/main.o
	$(CXX) $< $(DEMO_CXXFLAGS) -o $@

$(BUILD)/landscape-test.html: demos/landscape-test/main.o
	$(CXX) $< $(DEMO_CXXFLAGS) $(EM_PRELOAD_FLAGS) -o $@

.PHONY: clean-landscape-test-thing
clean-landscape-test-thing:
	-rm -f demos/landscape-test/main.o
	-rm -f $(BUILD)/landscape-test

.PHONY: clean-landscape-test-thing-html
clean-landscape-test-thing-html:
	-rm -f demos/landscape-test/main.o
	-rm -f $(BUILD)/landscape-test.html
