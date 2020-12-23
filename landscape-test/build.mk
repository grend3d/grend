ifeq ($(IS_EMSCRIPTEN),1)
DEMO_TARGETS += $(BUILD)/landscape-test.html
DEMO_CLEAN += clean-landscape-test-thing-html

else
DEMO_TARGETS += $(BUILD)/landscape-test
DEMO_CLEAN += clean-landscape-test-thing
endif

LANDSCAPE_DEMO_SRC     = $(wildcard demos/landscape-test/*.cpp)
LANDSCAPE_DEMO_OBJ     = $(LANDSCAPE_DEMO_SRC:.cpp=.o)
LANDSCAPE_DEMO_INCLUDE = -I./demos/landscape-test/

$(BUILD)/landscape-test: $(LANDSCAPE_DEMO_OBJ)
	$(CXX) $(LANDSCAPE_DEMO_OBJ) $(DEMO_CXXFLAGS) $(LANDSCAPE_DEMO_INCLUDE) -o $@

$(BUILD)/landscape-test.html: $(LANDSCAPE_DEMO_OBJ)
	$(CXX) $< $(DEMO_CXXFLAGS) $(EM_PRELOAD_FLAGS) -o $@

.PHONY: clean-landscape-test-thing
clean-landscape-test-thing:
	-rm -f $(LANDSCAPE_DEMO_OBJ)
	-rm -f $(BUILD)/landscape-test

.PHONY: clean-landscape-test-thing-html
clean-landscape-test-thing-html:
	-rm -f $(LANDSCAPE_DEMO_OBJ)
	-rm -f $(BUILD)/landscape-test.html
