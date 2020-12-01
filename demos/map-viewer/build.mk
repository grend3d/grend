ifeq ($(IS_EMSCRIPTEN),1)
DEMO_TARGETS += $(BUILD)/demo.html
DEMO_CLEAN += clean-demo-thing-html

else
DEMO_TARGETS += $(BUILD)/demo
DEMO_CLEAN += clean-demo-thing
endif

$(BUILD)/demo: demos/thing/main.o
	$(CXX) $< $(DEMO_CXXFLAGS) -o $@

$(BUILD)/demo.html: demos/thing/main.o
	$(CXX) $< $(DEMO_CXXFLAGS) $(EM_PRELOAD_FLAGS) -o $@

.PHONY: clean-demo-thing
clean-demo-thing:
	-rm -f demos/thing/main.o
	-rm -f $(BUILD)/demo

.PHONY: clean-demo-thing-html
clean-demo-thing-html:
	-rm -f demos/thing/main.o
	-rm -f $(BUILD)/demo.html
