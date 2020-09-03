DEMO_TARGETS += $(BUILD)/demo
DEMO_CLEAN += clean-demo-thing

$(BUILD)/demo: demos/thing/main.o
	$(CXX) $< $(DEMO_CXXFLAGS)  -o $@

.PHONY: clean-demo-thing
clean-demo-thing:
	-rm -f demos/thing/main.o
	-rm -f $(BUILD)/demo
